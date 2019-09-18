#pragma once

#include "../wim_packet.h"
#include "../voip_call_quality_popup_conf.h"

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
        class get_voip_calls_quality_popup_conf : public wim_packet
        {


            virtual int32_t init_request(std::shared_ptr<core::http_request_simple> _request) override;
            virtual int32_t parse_response(std::shared_ptr<core::tools::binary_stream> _response) override;

            void clear();

            // request params
            std::string app_name_;
            std::string platform_;
            std::string locale_;
            std::string country_;
            std::string version_;

            // parsed response
            voip_call_quality_popup_conf conf_;

        public:
            get_voip_calls_quality_popup_conf(
                wim_packet_params _params,
                const std::string& _app_name,
                const std::string& _platform,
                const std::string& _locale,
                const std::string& _country,
                const std::string& _version
                );

            virtual ~get_voip_calls_quality_popup_conf();

            const voip_call_quality_popup_conf& get_popup_configuration() const;
        };

    }

}
