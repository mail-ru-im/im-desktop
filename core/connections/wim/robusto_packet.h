#ifndef __ROBUSTO_PACKET_H_
#define __ROBUSTO_PACKET_H_

#pragma once

#include "wim_packet.h"

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
        class robusto_packet : public core::wim::wim_packet
        {
        protected:

            robusto_packet_params robusto_params_;

            int32_t parse_response(const std::shared_ptr<core::tools::binary_stream>& _response) override;
            int32_t on_response_error_code() override;
            int32_t execute_request(const std::shared_ptr<core::http_request_simple>& _request) override;
            void execute_request_async(const std::shared_ptr<core::http_request_simple>& request, handler_t _handler) override;

            virtual int32_t parse_results(const rapidjson::Value& _node_results);
            std::string get_req_id() const;

            void setup_common_and_sign(
                rapidjson::Value& _node,
                rapidjson_allocator& _a,
                const std::shared_ptr<core::http_request_simple>& _request,
                std::string_view _method);

        public:

            robusto_packet(wim_packet_params _params);
            virtual ~robusto_packet();

            void set_robusto_params(const robusto_packet_params& _params);
            virtual bool is_post() const override { return true; }
        };

    }
}



#endif //__ROBUSTO_PACKET_H_