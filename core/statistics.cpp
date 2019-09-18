#include "stdafx.h"
#include "statistics.h"

#include "core.h"
#include "tools/binary_stream.h"
#include "tools/strings.h"
#include "tools/system.h"
#include "tools/tlv.h"
#include "../external/curl/include/curl.h"
#include "http_request.h"
#include "tools/hmac_sha_base64.h"
#include "async_task.h"
#include "utils.h"
#include "../corelib/enumerations.h"
#include "../common.shared/keys.h"
#include "../common.shared/common.h"

using namespace core::stats;
using namespace core;

enum statistics_info_types
{
    //0,2 and 3 reserved
    event_name = 1,
    event_props = 4,
    event_prop_name = 5,
    event_prop_value = 6,
    last_sent_time = 7,
    event_time = 8,
    event_id = 9,
};


namespace
{
const event_prop_key_type StartAccumTimeProp("start_accum_time");
const event_prop_key_type AccumulateFinishedProp("accumulate_finished");
const static auto RamUsageThresholdLowerBoundMb = 200;
const static auto RamUsageThresholdUpperBoundMb = 1500;
const static auto RamUsageThresholdStepMb = 100;
constexpr auto ThresholdSteps = (RamUsageThresholdUpperBoundMb - RamUsageThresholdLowerBoundMb) / RamUsageThresholdStepMb;

using AccumulationTime = std::chrono::milliseconds;
static std::unordered_map<stats_event_names, AccumulationTime> Stat_Accumulated_Events = {
  {
        stats_event_names::send_used_traffic_size_event,
#if defined(DEBUG)
        AccumulationTime(std::chrono::minutes(3))
#else
        AccumulationTime(std::chrono::hours(24))
#endif
   }
};

event_props_type::iterator get_property(event_props_type& _props, const event_prop_key_type& _key);
bool has_property(event_props_type& _props, const event_prop_key_type& _key);
bool is_accumulated_event(stats_event_names _name);
void prepare_props_for_traffic(Out event_props_type &_props, const statistics::disk_stats &_disk_stats);
}

long long statistics::stats_event::session_event_id_ = 0;
std::shared_ptr<statistics::stop_objects> statistics::stop_objects_;

statistics::statistics(std::wstring _file_name)
    : file_name_(std::move(_file_name))
    , changed_(false)
    , stats_thread_(std::make_unique<async_executer>("stats"))
    , last_sent_time_(std::chrono::system_clock::now())
{
    stop_objects_ = std::make_shared<stop_objects>();
}

void statistics::init()
{
    load();
    start_save();
    delayed_start_send();
    start_disk_operations();
    start_ram_usage_monitoring();
}

statistics::~statistics()
{
    stop_objects_->is_stop_ = true;
    if (save_timer_ > 0 && g_core)
        g_core->stop_timer(save_timer_);

    if (send_timer_ > 0 && g_core)
        g_core->stop_timer(send_timer_);

    if (disk_stats_timer_ > 0 && g_core)
        g_core->stop_timer(disk_stats_timer_);

    save_if_needed();

    stats_thread_.reset();
}

void statistics::start_save()
{
    auto wr_this = weak_from_this();

    save_timer_ =  g_core->add_timer([wr_this]
    {
        auto ptr_this = wr_this.lock();
        if (!ptr_this)
            return;

        ptr_this->save_if_needed();
    }, save_to_file_interval);
}

void statistics::delayed_start_send()
{
    auto wr_this = weak_from_this();
    start_send_timer_ = g_core->add_timer([wr_this]
    {
        auto ptr_this = wr_this.lock();
        if (!ptr_this)
            return;

        ptr_this->start_send();
        g_core->stop_timer(ptr_this->start_send_timer_);

    }, delay_send_on_start);
}

void statistics::start_disk_operations()
{
    auto wr_this = weak_from_this();
    disk_stats_timer_ = g_core->add_timer([wr_this]{
        auto ptr_this = wr_this.lock();
        if (!ptr_this)
            return;

        ptr_this->query_disk_size_async();
    }, fetch_disk_size_interval);

    query_disk_size_async();
}

void statistics::start_ram_usage_monitoring()
{
    auto wr_this = weak_from_this();
    ram_stats_timer_ = g_core->add_timer([wr_this]{
        auto ptr_this = wr_this.lock();
        if (!ptr_this)
            return;

        ptr_this->check_ram_usage();
    }, fetch_ram_usage_interval);
}

void statistics::check_ram_usage()
{
    auto ram_usage_b = core::utils::get_current_process_ram_usage();
    decltype(ram_usage_b) ram_usage_mb = ram_usage_b / MEGABYTE;

    auto reached_threshold = RamUsageThresholdUpperBoundMb;
    auto steps = (ram_usage_mb - RamUsageThresholdLowerBoundMb) / RamUsageThresholdStepMb;
    auto last_steps = (last_used_ram_mb_ - RamUsageThresholdLowerBoundMb) / RamUsageThresholdStepMb;

    last_used_ram_mb_ = ram_usage_mb;

    if (ram_usage_mb < RamUsageThresholdLowerBoundMb || steps == last_steps)
        return;

    std::string addendum;
    if (steps < ThresholdSteps)
        reached_threshold = RamUsageThresholdLowerBoundMb + steps * RamUsageThresholdStepMb;
    else
        addendum = " +";

    core::stats::event_props_type props;
    props.emplace_back("mem_size", std::to_string(ram_usage_mb));
    props.emplace_back("mem_size_threshold", std::to_string(reached_threshold) + MEGABYTE_STR + addendum);
    insert_event(core::stats::stats_event_names::thresholdreached_event, props);

    last_used_ram_mb_ = ram_usage_mb;
}

void statistics::start_send()
{
    auto current_time = std::chrono::system_clock::now();
    if (current_time - last_sent_time_ >= send_interval)
        send_async();

    send_timer_ = g_core->add_timer([wr_this = weak_from_this()]
    {
        auto ptr_this = wr_this.lock();
        if (!ptr_this)
            return;

        ptr_this->send_async();
    }, send_interval);
}

bool statistics::load()
{
    core::tools::binary_stream bstream;
    if (!bstream.load_from_file(file_name_))
        return false;

    return unserialize(bstream);
}

void statistics::serialize(tools::binary_stream& _bs) const
{
    tools::tlvpack pack;
    int32_t counter = 0;

    // push stats info
    {
        tools::tlvpack value_tlv;
        value_tlv.push_child(tools::tlv(statistics_info_types::last_sent_time, (int64_t)std::chrono::system_clock::to_time_t(last_sent_time_)));

        tools::binary_stream bs_value;
        value_tlv.serialize(bs_value);
        pack.push_child(tools::tlv(++counter, bs_value));
    }

    auto serialize_events = [&pack, &counter](const decltype(events_)& events)
    {
        for (auto stat_event = events.begin(); stat_event != events.end(); ++stat_event)
        {
            tools::tlvpack value_tlv;
            // TODO : push id, time, ..
            value_tlv.push_child(tools::tlv(statistics_info_types::event_name, stat_event->get_name()));
            value_tlv.push_child(tools::tlv(statistics_info_types::event_time, (int64_t)std::chrono::system_clock::to_time_t(stat_event->get_time())));
            value_tlv.push_child(tools::tlv(statistics_info_types::event_id, (int64_t)stat_event->get_id()));

            tools::tlvpack props_pack;
            int32_t prop_counter = 0;
            auto props = stat_event->get_props();

            for (auto prop : props)
            {
                tools::tlvpack value_tlv_prop;
                value_tlv_prop.push_child(tools::tlv(statistics_info_types::event_prop_name, prop.first));
                value_tlv_prop.push_child(tools::tlv(statistics_info_types::event_prop_value, prop.second));

                tools::binary_stream bs_value;
                value_tlv_prop.serialize(bs_value);
                props_pack.push_child(tools::tlv(++prop_counter, bs_value));
            }

            value_tlv.push_child(tools::tlv(statistics_info_types::event_props, props_pack));

            tools::binary_stream bs_value;
            value_tlv.serialize(bs_value);
            pack.push_child(tools::tlv(++counter, bs_value));
        }
    };

    serialize_events(events_);
    serialize_events(accumulated_events_);

    pack.serialize(_bs);
}

bool unserialize_props(tools::tlvpack& prop_pack, event_props_type* props)
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

        auto tlv_event_name = pack_val.get_item(statistics_info_types::event_prop_name);
        auto tlv_event_value = pack_val.get_item(statistics_info_types::event_prop_value);
//        auto tlv_event_


        if (!tlv_event_name || !tlv_event_value)
        {
            assert(false);
            return false;
        }

        props->emplace_back(std::make_pair(tlv_event_name->get_value<std::string>(), tlv_event_value->get_value<std::string>()));
    }
    return true;
}

bool statistics::unserialize(tools::binary_stream& _bs)
{
    if (!_bs.available())
    {
        assert(false);
        return false;
    }

    tools::tlvpack pack;
    if (!pack.unserialize(_bs))
        return false;

    int32_t counter = 0;
    for (auto tlv_val = pack.get_first(); tlv_val; tlv_val = pack.get_next())
    {
        tools::binary_stream val_data = tlv_val->get_value<tools::binary_stream>();

        tools::tlvpack pack_val;
        if (!pack_val.unserialize(val_data))
            return false;

        if (counter++ == 0)
        {
            auto tlv_last_sent_time = pack_val.get_item(statistics_info_types::last_sent_time);

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
            auto curr_event_name = pack_val.get_item(statistics_info_types::event_name);

            if (!curr_event_name)
            {
                assert(false);
                return false;
            }
            stats_event_names name = curr_event_name->get_value<stats_event_names>();

            auto tlv_event_time = pack_val.get_item(statistics_info_types::event_time);
            auto tlv_event_id = pack_val.get_item(statistics_info_types::event_id);
            if (!tlv_event_time || !tlv_event_id)
            {
                assert(false);
                return false;
            }

            event_props_type props;
            const auto tlv_prop_pack = pack_val.get_item(statistics_info_types::event_props);
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

            auto read_event_time = std::chrono::system_clock::from_time_t(tlv_event_time->get_value<int64_t>());
            auto read_event_id = tlv_event_id->get_value<int64_t>();
            insert_event(name, props, read_event_time, read_event_id);
        }
    }

    return true;
}

void statistics::save_if_needed()
{
    if (g_core && changed_)
    {
        changed_ = false;

        auto bs_data = std::make_shared<tools::binary_stream>();
        serialize(*bs_data);
        std::wstring file_name = file_name_;

        g_core->run_async([bs_data, file_name]
        {
            bs_data->save_2_file(file_name);
            return 0;
        });
    }
}

void statistics::clear()
{
    last_sent_time_ = std::chrono::system_clock::now();

    if (!events_.empty())
    {
        auto last_service_event_ptr = events_.end();
        while (last_service_event_ptr != events_.begin())
        {
            --last_service_event_ptr;
            if (last_service_event_ptr->get_name() == stats_event_names::service_session_start)
                break;
        }

        auto last_service_event = *last_service_event_ptr;
        events_.clear();
        events_.push_back(last_service_event);
    }

    changed_ = true;

    // reset_session_event_id();
    // TODO : mb need save map with counts here?
}

void statistics::send_async()
{
    if (events_.empty())
        return;

    std::vector<std::string> post_data_vector = get_post_data();
    auto wr_this = weak_from_this();

    if (post_data_vector.empty())
        return;

    auto user_proxy = g_core->get_proxy_settings();

    stats_thread_->run_async_function([post_data_vector = std::move(post_data_vector), user_proxy, file_name = file_name_]
    {
        for (auto& post_data : post_data_vector)
            statistics::send(user_proxy, post_data, file_name);
        return 0;

    })->on_result_ = [wr_this](int32_t _error)
    {
        auto ptr_this = wr_this.lock();
        if (!ptr_this)
            return;
        ptr_this->clear();
        ptr_this->save_if_needed();
    };
}

void statistics::set_disk_stats(const statistics::disk_stats &_stats)
{
    disk_stats_ = _stats;
}

void statistics::query_disk_size_async()
{
    auto wr_this = weak_from_this();

    stats_thread_->run_async_function([wr_this]() -> int32_t
    {
        auto ptr_this = wr_this.lock();
        if (!ptr_this)
            return -1;

        auto folder_size = core::utils::get_folder_size(utils::get_product_data_path());
        ptr_this->set_disk_stats({ folder_size });

        return 0;
    });
}

std::string statistics::events_to_json(events_ci begin, events_ci end, time_t _start_time) const
{
    std::stringstream data_stream;
    std::map<stats_event_names, int32_t> events_and_count;

    bool is_first = true;

    for (auto stat_event  = begin; stat_event != end; ++stat_event)
    {
        const auto event_string = stat_event->to_string(_start_time);
        if (event_string.empty())
            continue;

        if (!is_first)
            data_stream << ",";

        is_first = false;

        data_stream << stat_event->to_string(_start_time);

        ++events_and_count[stat_event->get_name()];
    }

    data_stream << "],\"bm\":false,\"bn\":{";

    is_first = true;

    for (auto stat_event  = events_and_count.begin(); stat_event != events_and_count.end(); ++stat_event)
    {
        const auto event_name = stat_event_to_string(stat_event->first);
        if (event_name.empty())
            continue;

        if (!is_first)
            data_stream << ",";

        is_first = false;

        data_stream << "\"" << event_name << "\":" << stat_event->second;
    }
    return data_stream.str();
}

std::vector<std::string> statistics::get_post_data() const
{
    std::vector<std::string> result;

    events_ci begin = events_.begin();

    while (begin != events_.end())
    {
        events_ci end = std::next(begin);
        while (end != events_.end()
            && end->get_name() != stats_event_names::service_session_start)
            ++end;

        assert(begin->get_name() == stats_event_names::service_session_start);

        const long long time_now = std::chrono::system_clock::to_time_t(begin->get_time()) * 1000; // milliseconds

        std::string user_key = "";
        auto props = begin->get_props();

        if (props.size() > 0 && props.begin()->first == "hashed_user_key")
        {
            user_key = props.begin()->second;
        }
        else
        {
            assert(false);
        }

        ++begin;

        if (begin == end)
            continue;

        auto time = time_now;
        auto time1 = time + 4;
        auto time3 = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now()) * 1000;
        auto delta = time3 - time;

        auto version = core::utils::get_user_agent();
        auto flurryKey = build::get_product_variant(flurry_key, agent_flurry_key, biz_flurry_key, dit_flurry_key);
        std::stringstream data_stream;

        data_stream << "{\"a\":{\"af\":" << time3
            <<",\"aa\":1,\"ab\":10,\"ac\":9,\"ae\":\""<< version
            << "\",\"ad\":\"" << flurryKey
            << "\",\"ag\":" << time
            << ",\"ah\":" << time1
            << ",\"ak\":1,"
            << "\"cg\":\"" << user_key
            << "\"},\"b\":[{\"bd\":\"\",\"be\":\"\",\"bk\":-1,\"bl\":0,\"bj\":\"ru\",\"bo\":[";

        data_stream << events_to_json(begin, end, time);

        data_stream << "}"
            <<",\"bv\":[],\"bt\":false,\"bu\":{},\"by\":[],\"cd\":0,"
            << "\"ba\":" << time1
            << ",\"bb\":" << delta
            << ",\"bc\":-1,\"ch\":\"Etc/GMT-3\"}]}";
        result.push_back(data_stream.str());

        if (end != events_.end())
        {
            begin = end;
        }
        else
            break;
    }

    return result;
}

bool statistics::send(const proxy_settings& _user_proxy, const std::string& post_data, const std::wstring& _file_name)
{
    const std::weak_ptr<stop_objects> wr_stop(stop_objects_);

    const auto stop_handler =
        [wr_stop]
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

#ifdef DEBUG
    {
        auto path = _file_name + L".txt";
        boost::system::error_code e;
        boost::filesystem::ofstream fOut;

        fOut.open(path.c_str(), std::ios::out);
        fOut << post_data;
        fOut.close();
    }
#endif // DEBUG

    auto result_url = flurry_url
        + "?d=" + core::tools::base64::encode64(post_data)
        + "&c=" + core::tools::adler32(post_data);
    post_request.set_url(result_url);
    post_request.set_send_im_stats(false);
    return post_request.get();
}

void statistics::insert_event(stats_event_names _event_name, const event_props_type& _props,
                              std::chrono::system_clock::time_point _event_time, int32_t _event_id)
{
    event_props_type props = _props;

    if (is_accumulated_event(_event_name))
    {
        auto it = get_property(props, AccumulateFinishedProp);
        if (it == props.cend())
        {
            accumulated_events_.emplace_back(_event_name, _event_time, _event_id, props);
            return;
        }
    }

    events_.emplace_back(_event_name, _event_time, _event_id, props);
    changed_ = true;
}

void statistics::insert_event(stats_event_names _event_name, const event_props_type& _props)
{
    if (_event_name == stats_event_names::start_session)
    {
        stats_event::reset_session_event_id();
        insert_event(core::stats::stats_event_names::service_session_start, _props);
    }
    insert_event(_event_name, _props, std::chrono::system_clock::now(), -1);
}

void statistics::insert_event(stats_event_names _event_name)
{
    event_props_type props;
    insert_event(_event_name, props);
}

void statistics::insert_accumulated_event(stats_event_names _event_name, core::stats::event_props_type _props)
{
    insert_event(_event_name, _props);
}

statistics::stats_event::stats_event(stats_event_names _name,
                                     std::chrono::system_clock::time_point _event_time, int32_t _event_id, const event_props_type& _props)
    : name_(_name)
    , props_(_props)
    , event_time_(_event_time)
{
    if (_event_id == -1)
        event_id_ = session_event_id_++; // started from 1
    else
        event_id_ = _event_id;
}

std::string statistics::stats_event::to_string(time_t _start_time) const
{
    std::stringstream result;

    const auto event_name = stat_event_to_string(name_);
    if (!event_name.empty())
    {
        // TODO : use actual params here
        auto br = 0;

        std::stringstream params_in_json;
        for (auto prop = props_.begin(); prop != props_.end(); ++prop)
        {
            if (prop != props_.begin())
                params_in_json << ",";

            std::string val = prop->second;
            boost::replace_all(val, "\"", "\\\"");

            params_in_json << "\"" << prop->first << "\":\"" << val << "\"";
        }

        result << "{\"ce\":" << event_id_
            << ",\"bp\":\"" << event_name
            << "\",\"bq\":" << std::chrono::system_clock::to_time_t(event_time_) * 1000 - _start_time// milliseconds
            << ",\"bs\":{" << params_in_json.str() << "},"
            << "\"br\":" << br << "}";
    }

    return result.str();
}

stats_event_names statistics::stats_event::get_name() const
{
    return name_;
}

event_props_type statistics::stats_event::get_props() const
{
    return props_;
}

void statistics::stats_event::reset_session_event_id()
{
    session_event_id_ = 0;
}

std::chrono::system_clock::time_point statistics::stats_event::get_time() const
{
    return event_time_;
}

int32_t statistics::stats_event::get_id() const
{
    return event_id_;
}

bool statistics::stats_event::finished_accumulating()
{
    if (!is_accumulated_event(name_))
        return true;

    auto it = get_property(props_, StartAccumTimeProp);
    if (it == props_.end())
        return true;

    auto ms = std::chrono::milliseconds(std::stoll(it->second));
    auto dur = std::chrono::system_clock::now() - std::chrono::time_point<std::chrono::system_clock>(ms);

    return dur > Stat_Accumulated_Events[name_];
}

template<typename ValueType>
void statistics::stats_event::increment_prop(event_prop_key_type _key, ValueType _value)
{
    static_assert(std::is_integral<ValueType>::value);
    static_assert(std::is_same<decltype(props_[0].first), decltype(event_prop_key_type())>::value, "must be the same");

    auto it = std::find_if(props_.begin(), props_.end(),
                        [_key](decltype(props_.front()) _prop)
    {
        return _prop.first == _key;
    });

    if (it == props_.end())
    {
        props_.emplace_back(_key, std::to_string(_value));
        return;
    }

    auto cur_val = static_cast<ValueType>(std::stoll(it->second));
    cur_val += _value;

    it->second = std::to_string(cur_val);
}
template void statistics::stats_event::increment_prop<int32_t>(event_prop_key_type _key, int32_t _value);

template<typename ValueType>
void statistics::stats_event::set_prop(event_prop_key_type _key, ValueType _value)
{
    auto it = std::find_if(props_.begin(), props_.end(),
                        [_key](decltype(props_.front()) _prop)
    {
        return _prop.first == _key;
    });

    if (it == props_.end())
    {
        props_.emplace_back(_key, std::to_string(_value));
        return;
    }

    it->second = std::to_string(_value);
}
template void statistics::stats_event::set_prop<int32_t>(event_prop_key_type _key, int32_t _value);


template<typename ValueType>
void statistics::increment_event_prop(stats_event_names _event_name,
                                      event_prop_key_type _prop_key,
                                      ValueType _value)
{
    static_assert(std::is_integral<ValueType>::value);
    assert(is_accumulated_event(_event_name));

    auto cnt = 0;

    std::vector<int> to_remove;
    for (size_t i = 0; i < accumulated_events_.size(); ++i)
    {
        auto &it = accumulated_events_[i];
        if (it.get_name() != _event_name)
            continue;

        it.increment_prop(_prop_key, _value);
        cnt++;

        if (it.finished_accumulating())
        {
            auto name = it.get_name();
            auto props = it.get_props();
            props.push_back(std::make_pair(AccumulateFinishedProp, ""));

            if (name == stats_event_names::send_used_traffic_size_event)
            {
                if (!disk_stats_.is_initialized()) // postpone
                    continue;

                prepare_props_for_traffic(props, disk_stats_);
            }

            // set last send time for new empty event
            insert_event(name, props);
            to_remove.push_back(i);
        }
    }

    for (auto it = to_remove.rbegin(); it != to_remove.rend(); it++)
    {
        accumulated_events_.erase(accumulated_events_.begin() + *it);
    }

    if (!cnt)
    {
        event_props_type props =
        {
            { StartAccumTimeProp, std::to_string(std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count()) },
            { _prop_key, std::to_string(_value) }
        };

        insert_event(_event_name, props);
    }

    changed_ = true;
}

template void statistics::increment_event_prop<int32_t>(stats_event_names _event_name,
                                                        event_prop_key_type _prop_key,
                                                        int32_t _value);
template void statistics::increment_event_prop<long>(stats_event_names _event_name,
                                                     event_prop_key_type _prop_key,
                                                     long _value);

namespace
{

event_props_type::iterator get_property(event_props_type& _props, const event_prop_key_type& _key)
{
    event_props_type::iterator it = std::find_if(_props.begin(), _props.end(), [_key](decltype(_props[0]) prop) {
        return prop.first == _key;
    });

    return it;
}

bool has_property(event_props_type& _props, const event_prop_key_type& _key)
{
    return get_property(_props, _key) != _props.cend();
}

void prepare_props_for_traffic(event_props_type& _props, const statistics::disk_stats& _disk_stats)
{
    long long downloaded = 0, uploaded = 0;

    assert(has_property(_props, "download"));
    assert(has_property(_props, "upload"));

    auto d_p = get_property(_props, "download");
    auto u_p = get_property(_props, "upload");

    if (d_p != _props.end())
        downloaded = std::stoll(d_p->second);

    if (u_p != _props.end())
        uploaded = std::stoll(u_p->second);

    if (_disk_stats.is_initialized())
    {
        _props.push_back({"used_space", std::to_string(static_cast<size_t>(_disk_stats.user_folder_size_ / MEGABYTE))});
        _props.push_back({"used_space_interval", core::stats::disk_space_interval(_disk_stats.user_folder_size_)});
    }

    // interval
    _props.push_back({"download_interval", core::stats::traffic_size_interval(downloaded)});
    _props.push_back({"upload_interval", core::stats::traffic_size_interval(uploaded)});
}

bool is_accumulated_event(stats_event_names _name)
{
    return Stat_Accumulated_Events.find(_name) != Stat_Accumulated_Events.end();
}

}
