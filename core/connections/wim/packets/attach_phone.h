#pragma once
#include "../../login_info.h"
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
        class attach_phone : public robusto_packet
        {
            phone_info phone_info_;

            int32_t init_request(const std::shared_ptr<core::http_request_simple>& _request) override;

        public:
            attach_phone(wim_packet_params _params, const phone_info& _phone_info);
            ~attach_phone();

            int32_t get_response_error_code();
            bool is_valid() const override { return true; }
            bool is_post() const override { return true; }

            std::string_view get_method() const override;
        };

    }

}

