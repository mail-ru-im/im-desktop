#pragma once

#include "../wim_packet.h"

namespace core
{
    namespace tools
    {
        class http_request_simple;
    }
}


namespace core
{
    namespace wim
    {
        class update_profile : public wim_packet
        {
            int32_t init_request(const std::shared_ptr<core::http_request_simple>& _request) override;
            int32_t parse_response_data(const rapidjson::Value& _data) override;

            std::vector<std::pair<std::string, std::string>> fields_;

        public:

            update_profile(
                wim_packet_params _params,
                const std::vector<std::pair<std::string, std::string>>& _fields);

            virtual ~update_profile();

            std::string_view get_method() const override;
            int minimal_supported_api_version() const override;
        };

    }

}
