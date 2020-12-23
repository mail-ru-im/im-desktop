#pragma once

#include "../robusto_packet.h"

namespace core::wim
{
    class group_inviteblacklist_del : public robusto_packet
    {
    public:
        group_inviteblacklist_del(wim_packet_params _params, std::vector<std::string> _contacts_to_del, bool _del_all);
        const std::vector<std::string>& get_contacts() const noexcept { return contacts_; }
        bool auto_resend_on_fail() const override { return true; }
        virtual std::string_view get_method() const override;

    private:
        int32_t init_request(const std::shared_ptr<core::http_request_simple>& _request) override;

        std::vector<std::string> contacts_;
        bool all_ = false;
    };
}