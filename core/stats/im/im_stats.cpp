#include "stdafx.h"
#include "im_stats.h"

#include "core.h"
#include "connections/wim/auth_parameters.h"
#include "tools/binary_stream.h"
#include "tools/strings.h"
#include "tools/system.h"
#include "tools/tlv.h"
#include "http_request.h"
#include "async_task.h"
#include "utils.h"
#include "../common.shared/keys.h"
#include "../common.shared/common.h"
#include "../common.shared/version_info.h"
#include "../corelib/enumerations.h"
#include "../external/curl/include/curl.h"

#include "../../../libomicron/include/omicron/omicron.h"

using namespace core::stats;
using namespace core;

enum im_stats_info_types
{
    //0,2 and 3 reserved
    event_name = 1,
    event_props = 4,
    event_prop_name = 5,
    event_prop_value = 6,
    last_sent_time = 7,
    event_time = 8,
};

std::shared_ptr<im_stats::stop_objects> im_stats::stop_objects_;

auto omicron_get_events_send_interval()
{
    return std::chrono::seconds(omicronlib::_o("im_stats_send_interval", static_cast<int64_t>(feature::default_im_stats_send_interval())));
}

auto omicron_get_events_max_store_interval()
{
    return std::chrono::minutes(omicronlib::_o("im_stats_max_store_interval", static_cast<int64_t>(feature::default_im_stats_max_store_interval())));
}

std::string get_client_stat_url()
{
    std::stringstream ss_url;
    ss_url << urls::get_url(urls::url_type::client_stat_url) << std::string_view("/clientStat");

    return ss_url.str();
}

im_stats::im_stats(std::wstring _file_name)
    : file_name_(std::move(_file_name))
    , changed_(false)
    , is_sending_(false)
    , save_timer_id_(0)
    , send_timer_id_(0)
    , start_send_timer_id_(0)
    , stats_thread_(std::make_unique<async_executer>("im_stats"))
    , last_sent_time_(std::chrono::system_clock::now())
    , start_send_time_(std::chrono::system_clock::now())
    , events_send_interval_(omicron_get_events_send_interval())
    , events_max_store_interval_(omicron_get_events_max_store_interval())
{
    stop_objects_ = std::make_shared<stop_objects>();
}

void im_stats::init()
{
    load();
    clear(0); // remove expired events if needed
    start_save();
    delayed_start_send();
}

im_stats::~im_stats()
{
    stop_objects_->is_stop_ = true;

    if (save_timer_id_ > 0 && g_core)
    {
        g_core->stop_timer(save_timer_id_);
        save_timer_id_ = 0;
    }

    if (send_timer_id_ > 0 && g_core)
    {
        g_core->stop_timer(send_timer_id_);
        send_timer_id_ = 0;
    }

    save_if_needed();

    stats_thread_.reset();
}

void im_stats::start_save()
{
    if (save_timer_id_ > 0)
        return;

    auto wr_this = weak_from_this();
    save_timer_id_ = g_core->add_timer([wr_this]()
    {
        auto ptr_this = wr_this.lock();
        if (!ptr_this)
            return;

        ptr_this->save_if_needed();

    }, save_events_to_file_interval);
}

void im_stats::delayed_start_send()
{
    if (start_send_timer_id_ > 0)
        return;

    auto wr_this = weak_from_this();
    start_send_timer_id_ = g_core->add_timer([wr_this]()
    {
        auto ptr_this = wr_this.lock();
        if (!ptr_this)
            return;

        g_core->stop_timer(ptr_this->start_send_timer_id_);
        ptr_this->start_send_timer_id_ = 0;
        ptr_this->start_send();

    }, delay_send_events_on_start);
}

void im_stats::start_send(bool _check_start_now)
{
    if (send_timer_id_ > 0)
        return;

    if (_check_start_now && (std::chrono::system_clock::now() - last_sent_time_) >= events_send_interval_)
        events_send_interval_ = std::chrono::seconds(1);

    auto wr_this = weak_from_this();
    send_timer_id_ = g_core->add_timer([wr_this]()
    {
        auto ptr_this = wr_this.lock();
        if (!ptr_this)
            return;

        ptr_this->events_max_store_interval_ = omicron_get_events_max_store_interval();

        ptr_this->send_async();

        auto new_send_interval = omicron_get_events_send_interval();
        if (new_send_interval.count() > 0 && ptr_this->events_send_interval_ != new_send_interval && g_core)
        {
            g_core->stop_timer(ptr_this->send_timer_id_);
            ptr_this->send_timer_id_ = 0;
            ptr_this->events_send_interval_ = new_send_interval;
            ptr_this->start_send(false);
        }

    }, events_send_interval_);
}

bool im_stats::load()
{
    core::tools::binary_stream bstream;
    if (!bstream.load_from_file(file_name_))
        return false;

    return unserialize(bstream);
}

void im_stats::serialize(tools::binary_stream& _bs) const
{
    tools::tlvpack pack;
    uint32_t counter = 0;

    // push stats info
    {
        tools::tlvpack value_tlv;
        value_tlv.push_child(tools::tlv(im_stats_info_types::last_sent_time,
                                        static_cast<int64_t>(std::chrono::system_clock::to_time_t(last_sent_time_))));
        tools::binary_stream bs_value;
        value_tlv.serialize(bs_value);
        pack.push_child(tools::tlv(++counter, bs_value));
    }

    for (const auto& event_stat : events_)
    {
        tools::tlvpack value_tlv;
        value_tlv.push_child(tools::tlv(im_stats_info_types::event_name,
                                        event_stat.get_name()));
        value_tlv.push_child(tools::tlv(im_stats_info_types::event_time,
                                        static_cast<int64_t>(std::chrono::system_clock::to_time_t(event_stat.get_time()))));

        tools::tlvpack props_pack;
        uint32_t prop_counter = 0;
        auto props = event_stat.get_props();

        for (auto prop : props)
        {
            tools::tlvpack value_tlv_prop;
            value_tlv_prop.push_child(tools::tlv(im_stats_info_types::event_prop_name, prop.first));
            value_tlv_prop.push_child(tools::tlv(im_stats_info_types::event_prop_value, prop.second));

            tools::binary_stream bs_value;
            value_tlv_prop.serialize(bs_value);
            props_pack.push_child(tools::tlv(++prop_counter, bs_value));
        }

        value_tlv.push_child(tools::tlv(im_stats_info_types::event_props, props_pack));

        tools::binary_stream bs_value;
        value_tlv.serialize(bs_value);
        pack.push_child(tools::tlv(++counter, bs_value));
    }

    pack.serialize(_bs);
}

bool im_stats::unserialize_props(tools::tlvpack& prop_pack, event_props_type* props)
{
    assert(!!props);

    for (auto tlv_prop_val = prop_pack.get_first(); tlv_prop_val; tlv_prop_val = prop_pack.get_next())
    {
        tools::binary_stream val_data = tlv_prop_val->get_value<tools::binary_stream>();

        tools::tlvpack pack_val;
        if (!pack_val.unserialize(val_data))
        {
            assert(false);
            return false;
        }

        auto tlv_event_name = pack_val.get_item(im_stats_info_types::event_prop_name);
        auto tlv_event_value = pack_val.get_item(im_stats_info_types::event_prop_value);

        if (!tlv_event_name || !tlv_event_value)
        {
            assert(false);
            return false;
        }

        props->emplace_back(tlv_event_name->get_value<std::string>(), tlv_event_value->get_value<std::string>());
    }
    return true;
}

bool im_stats::unserialize(tools::binary_stream& _bs)
{
    if (!_bs.available())
    {
        assert(false);
        return false;
    }

    tools::tlvpack pack;
    if (!pack.unserialize(_bs))
        return false;

    uint32_t counter = 0;
    for (auto tlv_val = pack.get_first(); tlv_val; tlv_val = pack.get_next())
    {
        tools::binary_stream val_data = tlv_val->get_value<tools::binary_stream>();

        tools::tlvpack pack_val;
        if (!pack_val.unserialize(val_data))
            return false;

        if (counter++ == 0)
        {
            auto tlv_last_sent_time = pack_val.get_item(im_stats_info_types::last_sent_time);

            if (!tlv_last_sent_time)
            {
                assert(false);
                return false;
            }

            time_t last_time = tlv_last_sent_time->get_value<int64_t>();
            last_sent_time_ = std::chrono::system_clock::from_time_t(last_time);
        }
        else
        {
            auto curr_event_name = pack_val.get_item(im_stats_info_types::event_name);
            if (!curr_event_name)
            {
                assert(false);
                return false;
            }
            auto name = curr_event_name->get_value<im_stat_event_names>();

            auto tlv_event_time = pack_val.get_item(im_stats_info_types::event_time);
            if (!tlv_event_time)
            {
                assert(false);
                return false;
            }
            auto read_event_time = std::chrono::system_clock::from_time_t(tlv_event_time->get_value<int64_t>());

            event_props_type props;
            const auto tlv_prop_pack = pack_val.get_item(im_stats_info_types::event_props);
            assert(tlv_prop_pack);
            if (tlv_prop_pack)
            {
                auto prop_pack = tlv_prop_pack->get_value<tools::tlvpack>();
                if (!unserialize_props(prop_pack, &props))
                {
                    assert(false);
                    return false;
                }
            }

            insert_event(name, std::move(props), read_event_time);
        }
    }

    return true;
}

void im_stats::save_if_needed()
{
    if (changed_)
    {
        changed_ = false;

        auto bs_data = std::make_shared<tools::binary_stream>();
        serialize(*bs_data);
        std::wstring file_name = file_name_;

        g_core->run_async([bs_data, file_name]()
        {
            bs_data->save_2_file(file_name);
            return 0;
        });
    }
}

void im_stats::clear(int32_t _send_result)
{
    if (events_.empty())
        return;

    auto end = events_.end();
    auto it = end;

    if (events_max_store_interval_.count() > 0)
    {
        if (is_sending_)
        {
            if (_send_result == 0)
                last_sent_time_ = start_send_time_;
            else
                mark_all_events_sent(false);
        }

        auto clear_time = std::chrono::system_clock::now();
        it = std::remove_if(events_.begin(), events_.end(),
                            [clear_time, interval = events_max_store_interval_](const auto& event)
        {
            return event.is_sent() || (clear_time - event.get_time()) > interval;
        });
    }
    else
    {
        if (is_sending_)
        {
            last_sent_time_ = start_send_time_;

            it = std::remove_if(events_.begin(), events_.end(), [](const auto& event)
            {
                return event.is_sent();
            });
        }
    }

    if (it != end)
    {
        events_.erase(it, end);
        changed_ = true;
    }
}

void im_stats::send_async()
{
    if (events_.empty() || is_sending_)
        return;

    is_sending_ = true;
    start_send_time_ = std::chrono::system_clock::now();

    std::string post_data = get_post_data();
    if (post_data.empty())
    {
        is_sending_ = false;
        return;
    }

    mark_all_events_sent(true);

    auto wr_this = weak_from_this();
    auto user_proxy = g_core->get_proxy_settings();
    stats_thread_->run_async_function([post_data = std::move(post_data), user_proxy, file_name = file_name_]()
    {
        return im_stats::send(user_proxy, post_data, file_name);

    })->on_result_ = [wr_this](int32_t _send_result)
    {
        auto ptr_this = wr_this.lock();
        if (!ptr_this)
            return;

        ptr_this->clear(_send_result);
        ptr_this->save_if_needed();
        ptr_this->is_sending_ = false;
    };
}

void im_stats::serialize_events(rapidjson::Value& _node, rapidjson_allocator& _a) const
{
    using event_counter_type = std::pair<im_stats_event, uint32_t>;
    std::vector<event_counter_type> check_events;

    check_events.reserve(events_.size());

    for (auto event : events_)
    {
        auto pos = std::find_if(check_events.begin(), check_events.end(), [&event](const auto& item)
        {
            return item.first.is_equal(event);
        });

        if (pos != check_events.end())
            pos->second++;
        else
            check_events.emplace_back(std::move(event), 1);
    }

    for (const auto& [event, counter] : check_events)
    {
        rapidjson::Value node_event(rapidjson::Type::kObjectType);
        event.serialize(node_event, _a);

        if (counter > 1)
            node_event.AddMember("count", counter, _a);

        _node.PushBack(std::move(node_event), _a);
    }
}

std::string im_stats::get_post_data() const
{
    rapidjson::Document doc(rapidjson::Type::kObjectType);
    auto& a = doc.GetAllocator();

    rapidjson::Value node_general(rapidjson::Type::kObjectType);

    auto app_version = core::tools::version_info().get_major_version() + '.' + core::tools::version_info().get_minor_version();
    node_general.AddMember("app_ver", app_version, a);
    node_general.AddMember("app_build", core::tools::version_info().get_build_version(), a);
    node_general.AddMember("os_version", core::tools::system::get_os_version_string(), a);
    node_general.AddMember("device_id", g_core->get_uniq_device_id(), a);
    node_general.AddMember("uin", g_core->get_root_login(), a);
    node_general.AddMember("developer_id", core::wim::auth_parameters().dev_id_, a);
    int64_t start_time = std::chrono::system_clock::to_time_t(last_sent_time_);
    node_general.AddMember("start_time", start_time, a);
    int64_t end_time = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
    node_general.AddMember("end_time", end_time, a);
    doc.AddMember("general", std::move(node_general), a);

    rapidjson::Value node_events(rapidjson::Type::kArrayType);
    node_events.Reserve(events_.size(), a);
    serialize_events(node_events, a);
    doc.AddMember("events", std::move(node_events), a);

    rapidjson::StringBuffer buffer;
    rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
    doc.Accept(writer);

    return rapidjson_get_string(buffer);
}

int32_t im_stats::send(const proxy_settings& _user_proxy,
                       const std::string& _post_data,
                       const std::wstring& _file_name)
{
#ifdef DEBUG
    {
        auto path = _file_name + L".txt";
        boost::system::error_code e;
        boost::filesystem::ofstream fOut;

        fOut.open(path.c_str(), std::ios::out);
        fOut << _post_data;
        fOut.close();
    }
#endif // DEBUG

    const std::weak_ptr<stop_objects> wr_stop(stop_objects_);

    const auto stop_handler = [wr_stop]()
    {
        auto ptr_stop = wr_stop.lock();
        if (!ptr_stop)
            return true;

        return ptr_stop->is_stop_.load();
    };

    core::http_request_simple post_request(_user_proxy, utils::get_user_agent(), stop_handler);
    post_request.set_connect_timeout(std::chrono::seconds(5));
    post_request.set_timeout(std::chrono::seconds(5));
    post_request.set_keep_alive();
    post_request.set_custom_header_param("Content-Encoding: gzip");
    post_request.set_custom_header_param("Content-Type: application/json");
    post_request.set_url(get_client_stat_url());
    post_request.set_normalized_url("clientStat");

    tools::binary_stream data;
    data.write(_post_data);

    tools::binary_stream compressed_data;
    if (tools::compress(data, compressed_data))
    {
        post_request.set_post_data(compressed_data.get_data(), compressed_data.all_size(), false);

        if (post_request.post())
        {
            auto http_code = post_request.get_response_code();
            if (http_code == 200)
                return 0;
        }
    }

    return -1;
}

void im_stats::insert_event(im_stat_event_names _event_name,
                            event_props_type&& _props,
                            std::chrono::system_clock::time_point _event_time)
{
    if (_event_name > im_stat_event_names::min && _event_name < im_stat_event_names::max)
    {
        events_.emplace_back(_event_name, std::move(_props), _event_time);
        changed_ = true;
    }
}

void im_stats::insert_event(im_stat_event_names _event_name,
                            event_props_type&& _props)
{
    insert_event(_event_name, std::move(_props), std::chrono::system_clock::now());
}

void im_stats::mark_all_events_sent(bool _is_sent)
{
    for (auto& event : events_)
        event.mark_event_sent(_is_sent);
}

im_stats::im_stats_event::im_stats_event(im_stat_event_names _event_name,
                                         event_props_type&& _event_props,
                                         std::chrono::system_clock::time_point _event_time)
    : name_(_event_name)
    , props_(std::move(_event_props))
    , time_(_event_time)
    , event_sent_(false)
{
    if (props_.size() > 1)
    {
        auto pred = [](const auto& x1, const auto& x2)
        {
            if (x1.first != x2.first)
                return x1.first < x2.first;
            return x1.second < x2.second;
        };
        std::sort(props_.begin(), props_.end(), pred);
    }
}

void im_stats::im_stats_event::serialize(rapidjson::Value& _node, rapidjson_allocator& _a) const
{
    std::string event_name(im_stat_event_to_string(name_));
    if (!event_name.empty())
    {
        _node.AddMember("name", event_name, _a);

        rapidjson::Value node_prop(rapidjson::Type::kObjectType);
        for (const auto& [key, value] : props_)
            node_prop.AddMember(rapidjson::Value(key, _a), rapidjson::Value(value, _a), _a);

        _node.AddMember("params", std::move(node_prop), _a);
    }
}

im_stat_event_names im_stats::im_stats_event::get_name() const
{
    return name_;
}

const event_props_type& im_stats::im_stats_event::get_props() const
{
    return props_;
}

std::chrono::system_clock::time_point im_stats::im_stats_event::get_time() const
{
    return time_;
}

void im_stats::im_stats_event::mark_event_sent(bool _is_sent)
{
    event_sent_ = _is_sent;
}

bool im_stats::im_stats_event::is_sent() const
{
    return event_sent_;
}

bool core::stats::im_stats::im_stats_event::is_equal(const im_stats_event& _event) const
{
    if (this != &_event)
    {
        if (name_ != _event.name_)
            return false;

        auto size = props_.size();
        if (size != _event.props_.size())
            return false;

        if (size > 0)
        {
            decltype(size) i = 0;
            do
            {
                if (props_[i] != _event.props_[i])
                    return false;
            } while (++i < size);
        }
    }

    return true;
}
