#include "stdafx.h"
#include "utils.h"
#include "../../../http_request.h"
#include "../../../common.shared/version_info.h"
#include "../../../common.shared/config/config.h"

#include "send_stat.h"

namespace
{

std::string make_devices_str(const std::vector<std::string>& devices)
{
    std::string ret;
    for (auto it = devices.begin(); it != devices.end(); ++it)
    {
        if (it != devices.begin())
            ret += ';';
        ret += *it;
    }
    if (ret.empty())
        ret += '0';

    return ret;
}

}

namespace core::wim
{

send_stat::send_stat(wim_packet_params _params, const std::vector<std::string> &_audio_devices, const std::vector<std::string> &_video_devices)
    : wim_packet(std::move(_params))
    , audio_devices_(_audio_devices)
    , video_devices_(_video_devices)
{
}

send_stat::~send_stat()
{
}

int32_t send_stat::init_request(const std::shared_ptr<core::http_request_simple>& _request)
{
    time_t server_time = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now()) - params_.time_offset_;
    time_t time_offset = boost::posix_time::to_time_t(boost::posix_time::second_clock::local_time()) - server_time;

    int time_zone = std::round(time_offset / 3600.);

    std::stringstream ss_url;

    const auto host = su::concat("https://", config::get().url(config::urls::stat_base), "/stat/stat");

    // params should be sorted alphabetically
    ss_url << host <<
              "?app=" << escape_symbols(utils::get_client_string()) <<
              "&deviceId=" << escape_symbols(g_core->get_uniq_device_id()) <<
              "&devId=" << escape_symbols(params_.dev_id_) <<
              "&gaid=null" <<
              "&idfa=null" <<
              "&language=" << escape_symbols(params_.locale_.substr(0, 2)) <<
              "&osVersion=" << escape_symbols(g_core->get_core_gui_settings().os_version_) <<
              "&platform=" << escape_symbols(utils::get_protocol_platform_string()) <<
              "&timezone=" << escape_symbols((time_zone > 0 ? "+" : "")) << time_zone <<
              "&uin=" << escape_symbols(params_.aimid_) <<
              "&version=" << escape_symbols(tools::version_info().get_version()) <<
              "&voip_camera=" << escape_symbols(make_devices_str(video_devices_)) <<
              "&voip_microphone=" << escape_symbols(make_devices_str(audio_devices_));

    _request->set_url(ss_url.str());
    _request->set_normalized_url("dauStatistics");
    _request->set_keep_alive();

    return 0;
}

int32_t send_stat::parse_response_data(const rapidjson::Value& _data)
{
    return 0;
}

}
