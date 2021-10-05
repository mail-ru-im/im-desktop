#pragma once

#include "fetch_event.h"

namespace core::wim
{
    class fetch_event_trust_status : public fetch_event
    {
    public:
        fetch_event_trust_status();

        int32_t parse(const rapidjson::Value& _node_event_data) override;
        void on_im(std::shared_ptr<core::wim::im> _im, std::shared_ptr<auto_callback> _on_complete) override;

        bool is_trusted() const noexcept { return trusted_; }

    private:
        bool trusted_ = false;
    };
}