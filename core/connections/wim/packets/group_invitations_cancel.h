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
        class group_invitations_cancel : public robusto_packet
        {
        private:
            std::string contact_;
            std::string chat_;

            int32_t init_request(const std::shared_ptr<core::http_request_simple>& _request) override;

        public:
            group_invitations_cancel(wim_packet_params _params, std::string _contact, std::string _chat);
            const std::string& get_contact() const noexcept { return contact_; }
            const std::string& get_chat() const noexcept { return chat_; }
            std::string_view get_method() const override;
            int minimal_supported_api_version() const override;
        };

    }

}