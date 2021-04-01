#include "stdafx.h"

#include "zstd_helper.h"
#include "core.h"

#include "tools/features.h"
#include "tools/strings.h"
#include "tools/system.h"

#include "connections/im_container.h"

#include "zstd_wrap/zstd_wrap.h"
#include "zstd_wrap/zstd_dicts/internal_request_dict.h"
#include "zstd_wrap/zstd_dicts/internal_response_dict.h"

#include "../common.shared/string_utils.h"

namespace
{
    constexpr auto get_request_dict_prefix() noexcept
    {
        return std::string_view("im-zstd-dict-desktop-request");
    }

    constexpr auto get_response_dict_prefix() noexcept
    {
        return std::string_view("im-zstd-dict-desktop-response");
    }

    constexpr auto get_zstd_dict_extension() noexcept
    {
        return std::wstring_view(L".zdict");
    }

    constexpr size_t get_dict_timestamp_size() noexcept
    {
        return 6;
    }

    constexpr auto get_delay_update_timeout() noexcept
    {
        return std::chrono::seconds(10);
    }

    constexpr auto get_dict_update_interval() noexcept
    {
        if constexpr (build::is_debug())
            return std::chrono::minutes(1);
        else
            return std::chrono::hours(24);
    }

    auto get_url_basename(std::string_view _path)
    {
        if (auto pos = _path.find_last_of('/'); pos != std::string_view::npos && _path.size() > ++pos)
            return _path.substr(pos);

        return _path;
    }

    void write_to_log(std::string_view _msg)
    {
        if (core::g_core)
            core::g_core->write_string_to_network_log(su::concat("zstd_helper: ", _msg, "\r\n"));
    }
}

using namespace core;

zstd_helper::dict_info::dict_info(dict_location _location, std::string_view _path, const char* _data, size_t _size)
    : location_(_location)
    , path_(_path)
    , data_(_data)
    , size_(_size)
{
    if (!path_.empty())
    {
        auto name = boost::filesystem::path(path_);
        if (location_ == dict_location::external)
            name = name.filename();

        name_ = name.string();

        // dictionary name format: <dict_prefix>-DDMMYYN.zdict
        // DDMMYY - timestamp (day, month and year)
        // N - extra number (may be absent)
        auto pos_begin = name_.find_last_of('-');
        auto pos_end = name_.find_last_of('.');
        if (pos_begin != std::string::npos && pos_end != std::string::npos && pos_begin < pos_end)
        {
            ++pos_begin;
            if (auto info = std::string_view(name_).substr(pos_begin, pos_end - pos_begin); info.size() >= get_dict_timestamp_size())
            {
                // change DDMMYYN -> YYMMDDN for string comparation
                auto ts = std::string(info);
                std::swap(ts[0], ts[4]);
                std::swap(ts[1], ts[5]);
                timestamp_ = std::move(ts);
            }
        }
    }
}

zstd_helper::zstd_helper(std::wstring_view _dirname)
    : dicts_dirname_(_dirname)
    , request_dicts_(std::make_shared<dicts_list>())
    , response_dicts_(std::make_shared<dicts_list>())
    , timer_id_(0)
    , is_reinited_(false)
{
    tools::system::create_directory_if_not_exists(dicts_dirname_);
    init_dicts();
}

zstd_helper::~zstd_helper()
{
    stop_auto_updater();
}

void zstd_helper::start_auto_updater(std::weak_ptr<im_container> _im_cont)
{
    if (timer_id_ > 0 || !g_core)
        return;

    im_cont_ = _im_cont;

    timer_id_ = g_core->add_timer({ [wr_this = weak_from_this()] ()
    {
        auto ptr_this = wr_this.lock();
        if (!ptr_this)
            return;

        ptr_this->stop_auto_updater();
        ptr_this->update_dicts();

        ptr_this->timer_id_ = g_core->add_timer({ [wr_this]()
        {
            auto ptr_this = wr_this.lock();
            if (!ptr_this)
                return;

            ptr_this->update_dicts();

        } }, get_dict_update_interval());

    } }, get_delay_update_timeout());

    write_to_log("start auto-update dictionaries\r\n" + get_dicts_list());
}

void zstd_helper::stop_auto_updater()
{
    if (timer_id_ > 0 && g_core)
    {
        g_core->stop_timer(timer_id_);
        timer_id_ = 0;
    }
}

bool zstd_helper::add_dict(dict_info _info)
{
    if (_info.name_.find(get_request_dict_prefix()) != std::string::npos)
        return add_dict_common(dict_type::request, std::move(_info));
    else if (_info.name_.find(get_response_dict_prefix()) != std::string::npos)
        return add_dict_common(dict_type::response, std::move(_info));

    return false;
}

void zstd_helper::reinit_dicts(std::string_view _fault_dict_name)
{
    bool expected = false;
    if (!is_reinited_.compare_exchange_strong(expected, true))
        return;

    if (!_fault_dict_name.empty())
        write_to_log(su::concat("failure ", _fault_dict_name, "\r\n", get_dicts_list()));

    g_core->execute_core_context({ [wr_this = weak_from_this()] ()
    {
        auto ptr_this = wr_this.lock();
        if (!ptr_this)
            return;

        ptr_this->init_dicts();

        ptr_this->is_reinited_ = false;

        write_to_log("reinitialization of dictionaries\r\n" + ptr_this->get_dicts_list());
    } });
}

bool zstd_helper::is_dict_exist(std::string_view _dict_name) const
{
    return get_dict_info(_dict_name).has_value();
}

std::string zstd_helper::get_last_request_dict() const
{
    if (const auto dicts = get_request_dicts(); !dicts->empty())
        return dicts->back().name_;

    return std::string();
}

std::string zstd_helper::get_last_response_dict() const
{
    if (const auto dicts = get_response_dicts(); !dicts->empty())
        return dicts->back().name_;

    return std::string();
}

int zstd_helper::compress(const char* _data_in, size_t _data_in_size, char* _data_out, size_t _data_out_size, size_t* _data_size_written, std::string_view _dict_name, int _compress_level) const
{
    if (auto dict = get_dict_info(_dict_name); dict.has_value())
    {
        const auto compression_level = (_compress_level < 0) ? features::get_zstd_compression_level() : _compress_level;
        if (dict->location_ == dict_location::internal)
            return ZSTDW_Encode_buf(_data_in, _data_in_size, _data_out, _data_out_size, _data_size_written,
                                    dict->path_.c_str(), dict->data_, dict->size_, compression_level, false);
        else
            return ZSTDW_Encode(_data_in, _data_in_size, _data_out, _data_out_size, _data_size_written,
                                dict->path_.c_str(), compression_level, false);
    }

    return -1;
}

int zstd_helper::decompress(const char* _data_in, size_t _data_in_size, char* _data_out, size_t _data_out_size, size_t* _data_size_written, std::string_view _dict_name) const
{
    if (_dict_name.empty())
        return ZSTDW_Decode(_data_in, _data_in_size, _data_out, _data_out_size, _data_size_written, nullptr);

    if (auto dict = get_dict_info(_dict_name); dict.has_value())
    {
        if (dict->location_ == dict_location::internal)
            return ZSTDW_Decode_buf(_data_in, _data_in_size, _data_out, _data_out_size, _data_size_written,
                                    dict->path_.c_str(), dict->data_, dict->size_);
        else
            return ZSTDW_Decode(_data_in, _data_in_size, _data_out, _data_out_size, _data_size_written,
                                dict->path_.c_str());
    }

    return -1;
}

void zstd_helper::init_dicts()
{
    auto init_request_dicts = std::make_shared<dicts_list>();
    auto init_response_dicts = std::make_shared<dicts_list>();

    // add internal zstd dictionaries
    const auto& internal_request_dict = zstd_internal::get_request_dict();
    init_request_dicts->emplace_back(dict_info(dict_location::internal, zstd_internal::get_request_dict_name(),
                                     reinterpret_cast<const char*>(internal_request_dict.data()),
                                     internal_request_dict.size()));
    const auto& internal_response_dict = zstd_internal::get_response_dict();
    init_response_dicts->emplace_back(dict_info(dict_location::internal, zstd_internal::get_response_dict_name(),
                                      reinterpret_cast<const char*>(internal_response_dict.data()),
                                      internal_response_dict.size()));

    // find and add external dictionaries
    boost::system::error_code ec;
    try
    {
        auto is_dict_present = [](const dicts_list_sptr& _list, const std::string& _name)
        {
            const auto it = std::find_if(_list->begin(), _list->end(), [_name](const auto& _dict) { return _dict.name_ == _name; });
            return it != _list->end();
        };

        auto it = boost::filesystem::recursive_directory_iterator(dicts_dirname_, ec);
        auto end = boost::filesystem::recursive_directory_iterator();
        for (; it != end && !ec; it.increment(ec))
        {
            if (it->status().type() != boost::filesystem::file_type::regular_file)
                continue;

            const auto& added_dict = it->path();
            if (added_dict.extension().wstring() == get_zstd_dict_extension())
            {
                auto dict = dict_info(dict_location::external, tools::from_utf16(added_dict.wstring()));

                if (dict.name_.find(get_request_dict_prefix()) != std::string::npos)
                {
                    if (!is_dict_present(init_request_dicts, dict.name_))
                        init_request_dicts->emplace_back(std::move(dict));
                }
                else if (dict.name_.find(get_response_dict_prefix()) != std::string::npos)
                {
                    if (!is_dict_present(init_response_dicts, dict.name_))
                        init_response_dicts->emplace_back(std::move(dict));
                }
            }
        }
    }
    catch (const std::exception&)
    {
    }

    for (auto& d : { init_request_dicts, init_response_dicts })
        std::sort(d->begin(), d->end(), [](const auto& _lhs, const auto& _rhs) { return _lhs.timestamp_ < _rhs.timestamp_; });

    {
        std::scoped_lock lock(request_dicts_lock_);
        request_dicts_.swap(init_request_dicts);
    }
    {
        std::scoped_lock lock(response_dicts_lock_);
        response_dicts_.swap(init_response_dicts);
    }

}

void zstd_helper::update_dicts()
{
    auto im = im_cont_.lock();
    if (!im)
        return;

    for (const auto& url : { features::get_zstd_dict_request(), features::get_zstd_dict_response() })
    {
        auto dict_name = get_url_basename(url);
        if (!is_dict_exist(dict_name))
        {
            std::string dest = su::concat(tools::from_utf16(dicts_dirname_), '/', dict_name);
            im->download_file(low_priority(), url, dest, "zdictsDownload", true, [wr_this = weak_from_this(), dict_name = std::string(dict_name), dest](bool _is_downloaded)
            {
                auto ptr_this = wr_this.lock();
                if (!ptr_this)
                    return;

                std::string ss;
                if (_is_downloaded)
                {
                    ptr_this->add_dict({ dict_location::external, dest });
                    ss = su::concat("downloaded ", dict_name, "\r\n", ptr_this->get_dicts_list());
                }
                else
                {
                    ss = su::concat("failed to download ", dict_name);
                }
                write_to_log(ss);
            });
        }
    }
}

std::optional<zstd_helper::dict_info> zstd_helper::get_dict_info(std::string_view _dict_name) const
{
    dicts_list_sptr dicts = nullptr;
    if (_dict_name.find(get_request_dict_prefix()) != std::string::npos)
        dicts = get_request_dicts();
    else if (_dict_name.find(get_response_dict_prefix()) != std::string::npos)
        dicts = get_response_dicts();

    if (dicts && !dicts->empty())
    {
        const auto it = std::find_if(dicts->begin(), dicts->end(), [_dict_name](const auto& _dict) { return _dict.name_ == _dict_name; });
        if (it != dicts->end())
            return *it;
    }

    return std::nullopt;
}

bool zstd_helper::add_dict_common(dict_type _type, dict_info _info)
{
    dicts_list_sptr new_dicts;
    if (_type == dict_type::request)
        new_dicts = std::make_shared<dicts_list>(*get_request_dicts());
    else
        new_dicts = std::make_shared<dicts_list>(*get_response_dicts());

    auto it = std::find_if(new_dicts->begin(), new_dicts->end(), [name = _info.name_](const auto& _dict) { return _dict.name_ == name; });
    if (it == new_dicts->end())
    {
        new_dicts->emplace_back(std::move(_info));
        std::sort(new_dicts->begin(), new_dicts->end(), [](const auto& _lhs, const auto& _rhs) { return _lhs.timestamp_ < _rhs.timestamp_; });

        if (_type == dict_type::request)
        {
            std::scoped_lock lock(request_dicts_lock_);
            request_dicts_.swap(new_dicts);
        }
        else
        {
            std::scoped_lock lock(response_dicts_lock_);
            response_dicts_.swap(new_dicts);
        }

        return true;
    }

    return false;
}

std::string zstd_helper::get_dicts_list() const
{
    std::string ss;
    ss += "dicts: ";
    for (const auto& dicts: { get_request_dicts(), get_response_dicts() })
    {
        for (const auto& dict : *dicts)
        {
            if (dict.location_ == dict_location::internal)
                ss += '*';
            ss += dict.name_;
            ss += "; ";
        }
    }

    return ss;
}

zstd_helper::dicts_list_sptr zstd_helper::get_request_dicts() const
{
    std::scoped_lock lock(request_dicts_lock_);
    return request_dicts_;
}

zstd_helper::dicts_list_sptr zstd_helper::get_response_dicts() const
{
    std::scoped_lock lock(response_dicts_lock_);
    return response_dicts_;
}
