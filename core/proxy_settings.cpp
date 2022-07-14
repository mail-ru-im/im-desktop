#include "stdafx.h"

#include "curl.h"
#include "core.h"
#include "tools/scope.h"
#include "tools/strings.h"
#include "../corelib/collection_helper.h"
#include "../common.shared/string_utils.h"
#include "../common.shared/uri_matcher/uri_matcher.h"

#include "core_settings.h"

#include "proxy_settings.h"

namespace
{
    enum pos_by_proxe_type
    {
        user_settings = 0,
        auto_settings,
#ifdef _WIN32
        system_settings
#endif
    };
}

namespace core
{
    enum proxy_settings_values
    {
        proxy_settings_proxy_type = 1,
        proxy_settings_proxy_server = 2,
        proxy_settings_proxy_port = 3,
        proxy_settings_proxy_login = 4,
        proxy_settings_proxy_password = 5,
        proxy_settings_proxy_need_auth = 6,
        proxy_settings_proxy_auth_type = 7,
    };

    const int32_t proxy_settings::default_proxy_port = -1;

    proxy_settings_manager::proxy_settings_manager(core_settings& _settings_storage)
        : settings_storage_(_settings_storage)
        , current_settings_(user_settings)
    {
        settings_[user_settings] = settings_storage_.get_user_proxy_settings(); // at first we apply the user settings
        settings_[auto_settings] = proxy_settings::auto_proxy();                // then we apply default (auto) settings
#ifdef _WIN32
        settings_[system_settings] = load_from_registry();                      // and only on Windows we apply system settings at the end
        const auto& s = settings_[system_settings];
        g_core->write_string_to_network_log(su::concat("System proxy: ", s.use_proxy_ ? s.to_string() : std::string { "disabled" }, "\r\n"));
#endif

        if (settings_[user_settings].proxy_type_ == static_cast<int32_t>(core::proxy_type::auto_proxy))
            current_settings_ = auto_settings;
    }

    proxy_settings proxy_settings_manager::get_current_settings() const
    {
        std::lock_guard<std::mutex> lock(mutex_);
        return settings_[current_settings_];
    }

    void proxy_settings_manager::apply(const proxy_settings& _settings)
    {
        {
            std::lock_guard<std::mutex> lock(mutex_);
            if (_settings.proxy_type_ == static_cast<int32_t>(core::proxy_type::auto_proxy))
            {
                current_settings_ = auto_settings;
            }
            else
            {
                settings_[user_settings] = _settings;
                current_settings_ = user_settings;
            }
        }

        settings_storage_.set_user_proxy_settings(_settings);
    }

    bool proxy_settings_manager::try_to_apply_alternative_settings()
    {
        std::lock_guard<std::mutex> lock(mutex_);

        if (current_settings_ == user_settings) // we won't change the settings if they are user
            return false;

        if (current_settings_ < settings_.size() - 1) // try the next settings if they available
        {
            ++current_settings_;
            return true;
        }

        return false;
    }

#ifdef _WIN32
    proxy_settings proxy_settings_manager::load_from_registry()
    {
        proxy_settings settings;
        settings.is_system_ = true;

        CRegKey key;
        if (key.Open(HKEY_CURRENT_USER, L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Internet Settings") != ERROR_SUCCESS)
            return settings;

        DWORD proxy_used = 0;
        if ((key.QueryDWORDValue(L"ProxyEnable", proxy_used) != ERROR_SUCCESS) || (proxy_used == 0))
            return settings;

        {
            wchar_t buffer[MAX_PATH];
            unsigned long len = MAX_PATH;
            if (key.QueryStringValue(L"ProxyServer", buffer, &len) == ERROR_SUCCESS)
            {
                std::wstring_view server{ buffer };
                if (!settings.parse_servers_list(core::tools::from_utf16(server)))
                    return settings;
            }
            else
            {
                return settings;
            }
        }
        {
            wchar_t buffer[MAX_PATH];
            unsigned long len = MAX_PATH;
            if (key.QueryStringValue(L"ProxyOverride", buffer, &len) == ERROR_SUCCESS)
                settings.ignore_list_.parse_hosts(core::tools::from_utf16(buffer));
        }
        {
            wchar_t buffer[MAX_PATH];
            unsigned long len = MAX_PATH;
            if (key.QueryStringValue(L"ProxyUser", buffer, &len) == ERROR_SUCCESS)
            {
                settings.login_ = core::tools::from_utf16(buffer);
                if (settings.login_.empty())
                    return settings;
                std::string user_info = settings.login_;
                settings.need_auth_ = true;

                if (key.QueryStringValue(L"ProxyPass", buffer, &len) == ERROR_SUCCESS)
                    settings.password_ = core::tools::from_utf16(buffer);
            }
        }

        return settings;
    }
#endif //_WIN32

    bool proxy_settings::unserialize(tools::binary_stream& _stream)
    {
        core::tools::tlvpack tlv_pack;
        if (!tlv_pack.unserialize(_stream))
            return false;

        auto root_tlv = tlv_pack.get_item(0);
        if (!root_tlv)
            return false;

        core::tools::tlvpack tlv_pack_childs;
        if (!tlv_pack_childs.unserialize(root_tlv->get_value<core::tools::binary_stream>()))
            return false;

        auto tlv_proxy_server = tlv_pack_childs.get_item(proxy_settings_values::proxy_settings_proxy_server);
        auto tlv_proxy_port = tlv_pack_childs.get_item(proxy_settings_values::proxy_settings_proxy_port);
        auto tlv_login = tlv_pack_childs.get_item(proxy_settings_values::proxy_settings_proxy_login);
        auto tlv_password = tlv_pack_childs.get_item(proxy_settings_values::proxy_settings_proxy_password);
        auto tlv_proxy_type = tlv_pack_childs.get_item(proxy_settings_values::proxy_settings_proxy_type);
        auto tlv_need_auth = tlv_pack_childs.get_item(proxy_settings_values::proxy_settings_proxy_need_auth);
        auto tlv_auth_type = tlv_pack_childs.get_item(proxy_settings_values::proxy_settings_proxy_auth_type);

        if (!tlv_proxy_server
            || !tlv_proxy_port
            || !tlv_login
            || !tlv_password
            || !tlv_proxy_type
            || !tlv_need_auth)
            return false;

        proxy_server_ = tlv_proxy_server->get_value<std::string>("");
        proxy_port_ = tlv_proxy_port->get_value<int32_t>(proxy_settings::default_proxy_port);
        login_ = tlv_login->get_value<std::string>("");
        password_ = tlv_password->get_value<std::string>("");
        proxy_type_ = tlv_proxy_type->get_value<int32_t>(static_cast<int32_t>(core::proxy_type::auto_proxy));

        need_auth_ = tlv_need_auth->get_value<bool>(false);
        use_proxy_ = proxy_type_ != (int32_t)core::proxy_type::auto_proxy;

        if (tlv_auth_type)
            auth_type_ = static_cast<core::proxy_auth>(tlv_auth_type->get_value<int32_t>(static_cast<int32_t>(core::proxy_auth::basic)));

        return true;
    }

    void proxy_settings::serialize(tools::binary_stream& _stream) const
    {
        core::tools::tlvpack pack;
        core::tools::binary_stream temp_stream;

        pack.push_child(tools::tlv(proxy_settings_values::proxy_settings_proxy_type, proxy_type_));
        pack.push_child(tools::tlv(proxy_settings_values::proxy_settings_proxy_server, proxy_server_));
        pack.push_child(tools::tlv(proxy_settings_values::proxy_settings_proxy_port, proxy_port_));
        pack.push_child(tools::tlv(proxy_settings_values::proxy_settings_proxy_login, login_));
        pack.push_child(tools::tlv(proxy_settings_values::proxy_settings_proxy_password, password_));
        pack.push_child(tools::tlv(proxy_settings_values::proxy_settings_proxy_need_auth, need_auth_));
        pack.push_child(tools::tlv(proxy_settings_values::proxy_settings_proxy_auth_type, static_cast<int32_t>(auth_type_)));

        pack.serialize(temp_stream);

        core::tools::tlvpack rootpack;
        rootpack.push_child(core::tools::tlv(0, temp_stream));

        rootpack.serialize(_stream);
    }

    void proxy_settings::serialize(core::coll_helper _collection) const
    {
        _collection.set_value_as_int("settings_proxy_type", proxy_type_);
        _collection.set_value_as_string("settings_proxy_server", proxy_server_);
        _collection.set_value_as_int("settings_proxy_port", proxy_port_);
        _collection.set_value_as_string("settings_proxy_username", login_);
        _collection.set_value_as_string("settings_proxy_password", password_);
        _collection.set_value_as_bool("settings_proxy_need_auth", need_auth_);
        _collection.set_value_as_enum<core::proxy_auth>("settings_proxy_auth_type", auth_type_);
        _collection.set_value_as_bool("is_system", is_system_);
    }

    bool proxy_settings::need_use_proxy(std::string_view _url, std::string_view _ip_address)
    {
        return use_proxy_ && !ignore_list_.need_ignore(_url, _ip_address);
    }

    bool proxy_settings::parse_servers_list(std::string_view _hosts)
    {
        for (auto index = _hosts.find(L';'); index != _hosts.npos; index = _hosts.find(L';')) // https=127.0.0.1;http=127.0.0.1
        {
            if (parse_server(_hosts.substr(0, index)))
                return true;
            _hosts.remove_prefix(index + 1);
        }
        return parse_server(_hosts);
    }

    std::string proxy_settings::to_string() const
    {
        if (!use_proxy_)
            return {};
        std::string result;

        if (need_auth_)
        {
            result += login_;
            if (!password_.empty())
                result += su::concat(':', password_);
            result += '@';
        }

        result += proxy_server_;
        if (proxy_port_ != default_proxy_port)
            result += su::concat(':', std::to_string(proxy_port_));
        result += "; ignore list: ";
        result += ignore_list_.to_string();

        return result;
    }

    bool proxy_settings::parse_server(std::string_view _server)
    {
        if (const auto space_index = _server.find_first_not_of(' '); (space_index != _server.npos) && (space_index != 0))
            _server.remove_prefix(space_index);
        if (const auto space_index = _server.find(' '); space_index != _server.npos)
            _server.remove_suffix(_server.size() - space_index);
        if (_server.empty())
            return false;

        using namespace std::literals;
        if (const auto pos = _server.find(L'='); pos != _server.npos)
        {
            if (auto proxy_type = _server.substr(0, pos); proxy_type != "http"sv && proxy_type != "https"sv) // support only http[s] proxy
                return false;
            _server.remove_prefix(pos + 1); // remove proxy type
        }

        if (const auto pos = _server.find("://"sv); pos != _server.npos)
            _server.remove_prefix(pos + 3); // remove scheme

        if (const auto pos = _server.find(L':'); pos != _server.npos)
        {
            std::string_view port{ _server.substr(pos + 1) };
            _server.remove_suffix(_server.size() - pos);

            const char* ptr = port.data();
            char* end_ptr;
            const long result = std::strtol(ptr, &end_ptr, 10);
            if (ptr != end_ptr)
                proxy_port_ = result;
        }

        proxy_server_ = _server;
        use_proxy_ = true;
        proxy_type_ = CURLPROXY_HTTP;

        return true;
    }
}

namespace core::proxy_detail
{
    void ignore_list::parse_hosts(std::string_view _hosts)
    {
        for (auto index = _hosts.find(L';'); index != _hosts.npos; index = _hosts.find(L';'))
        {
            add_host(_hosts.substr(0, index));
            _hosts.remove_prefix(index + 1);
        }
        add_host(_hosts);
    }

    bool proxy_detail::ignore_list::need_ignore(std::string_view _url, std::string_view _ip_address)
    {
        basic_uri_view uri_view{ _url };
        const auto host = uri_view.host();
        if (std::find(host_list_.cbegin(), host_list_.cend(), host) != host_list_.cend())
            return true;
        if (std::find(ip_list_.cbegin(), ip_list_.cend(), _ip_address) != ip_list_.cend())
            return true;

        auto ip_predicate = [_ip_address](std::string_view _ip_mask)
        {
            return su::starts_with(_ip_address, _ip_mask);
        };
        if (std::find_if(ip_mask_list_.cbegin(), ip_mask_list_.cend(), ip_predicate) != ip_mask_list_.cend())
            return true;

        auto host_predicate = [host](std::string_view _host_mask)
        {
            return su::ends_with(host, _host_mask);
        };
        if (std::find_if(host_mask_list_.cbegin(), host_mask_list_.cend(), host_predicate) != host_mask_list_.cend())
            return true;

        return false;
    }

    std::string ignore_list::to_string() const
    {
        std::string result { "[ " };
        for (const auto& host : host_list_)
            result += su::concat(host, ' ');
        for (const auto& host : host_mask_list_)
            result += su::concat('*', host, ' ');
        for (const auto& ip : ip_list_)
            result += su::concat(ip, ' ');
        for (const auto& ip: ip_mask_list_)
            result += su::concat(ip, ' ');
        result += "]";
        return result;
    }

    void ignore_list::add_host(std::string_view _host)
    {
        if (const auto space_index = _host.find_first_not_of(' '); (space_index != _host.npos) && (space_index != 0))
            _host.remove_prefix(space_index);
        if (const auto space_index = _host.find(' '); space_index != _host.npos)
            _host.remove_suffix(_host.size() - space_index);
        if (_host.empty())
            return;

        using namespace std::string_view_literals;
        using uri_matcher = basic_uri_matcher<std::string_view::value_type>;
        if (su::starts_with(_host, L"*."sv))
            host_mask_list_.emplace_back(_host.substr(1));
        else if (uri_matcher::has_domain(_host.cbegin(), _host.cend()))
            host_list_.emplace_back(_host);
        else if (uri_matcher::has_ipaddr(_host.cbegin(), _host.cend()))
            ip_list_.emplace_back(_host);
        else if (uri_matcher::has_ipaddr_mask(_host.cbegin(), _host.cend()))
            ip_mask_list_.emplace_back(_host);
        else
            im_assert(!"Address did not added");
    }
}
