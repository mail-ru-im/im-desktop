#pragma once
#include "../robusto_packet.h"

namespace core::wim
{
    class send_notify_sms : public robusto_packet
    {
    public:
        send_notify_sms(wim_packet_params _params, std::string_view _sms_notify_context);
        virtual std::string_view get_method() const override;
        virtual bool support_self_resending() const override { return true; }

    private:
        int32_t init_request(const std::shared_ptr<core::http_request_simple>& _request) override;

        std::string sms_notify_context_;
    };

}