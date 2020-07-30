#pragma once

#include "../robusto_packet.h"

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
        class join_chat : public robusto_packet
        {
        private:
            std::string stamp_;

            int32_t init_request(const std::shared_ptr<core::http_request_simple>& _request) override;
            int32_t on_response_error_code() override;

        public:
            join_chat(wim_packet_params _params, const std::string& _stamp);

            virtual ~join_chat();
        };

    }

}
