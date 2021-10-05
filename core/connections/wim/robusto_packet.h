#ifndef __ROBUSTO_PACKET_H_
#define __ROBUSTO_PACKET_H_

#pragma once

#include "wim_packet.h"
#include "../urls_cache.h"

namespace core
{
    namespace tools
    {
        class binary_stream;
        class http_request_simple;
    }
}


namespace core
{
    namespace wim
    {
        enum robusto_protocol_error
        {
            ok = 20000,
            reset_page = 20002,

            message_reactions_disabled = 40300,
        };

        class robusto_packet : public core::wim::wim_packet
        {
        protected:

            robusto_packet_params robusto_params_;

            int32_t parse_response(const std::shared_ptr<core::tools::binary_stream>& _response) override;
            int32_t on_response_error_code() override;
            int32_t execute_request(const std::shared_ptr<core::http_request_simple>& _request) override;
            void execute_request_async(const std::shared_ptr<core::http_request_simple>& request, handler_t _handler) override;

            virtual bool is_status_code_ok() const { return get_status_code() == robusto_protocol_error::ok; }

            virtual int32_t parse_results(const rapidjson::Value& _node_results);
            std::string get_req_id() const;

            enum class use_aimsid
            {
                yes,
                no
            };
            void setup_common_and_sign(
                rapidjson::Value& _node,
                rapidjson_allocator& _a,
                const std::shared_ptr<core::http_request_simple>& _request,
                std::string_view _method,
                use_aimsid _use_aimsid = use_aimsid::yes,
                urls::url_type _url_type = urls::url_type::rapi_host);

        public:

            robusto_packet(wim_packet_params _params);
            virtual ~robusto_packet();

            void set_robusto_params(const robusto_packet_params& _params);
            virtual bool is_post() const override { return true; }
        };

    }
}



#endif //__ROBUSTO_PACKET_H_