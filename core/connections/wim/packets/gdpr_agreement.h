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
        class gdpr_agreement : public robusto_packet
        {
        private:
            int32_t init_request(const std::shared_ptr<core::http_request_simple>& _request) override;
            int32_t get_response_error_code();

        public:
            enum class agreement_state
            {
                rejected = 0,
                accepted = 1
            };
            gdpr_agreement(wim_packet_params _params, agreement_state _state);
            virtual ~gdpr_agreement() = default;

            bool is_post() const override { return true; }
            std::string_view get_method() const override;
        private:
            agreement_state state_;
        };
    }
}
