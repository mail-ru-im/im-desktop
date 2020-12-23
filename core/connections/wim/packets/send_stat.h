#pragma once

#include "../wim_packet.h"

namespace core::tools
{
    class http_request_simple;
}

namespace core::wim
{
    class send_stat: public wim_packet
    {
        virtual int32_t init_request(const std::shared_ptr<core::http_request_simple>& _request) override;
        virtual int32_t parse_response_data(const rapidjson::Value& _data) override;

        const std::vector<std::string> audio_devices_;
        const std::vector<std::string> video_devices_;


    public:

        send_stat(wim_packet_params _params, const std::vector<std::string>& _audio_devices, const std::vector<std::string>& _video_devices);
        virtual ~send_stat();

        virtual std::string_view get_method() const override;
    };
}
