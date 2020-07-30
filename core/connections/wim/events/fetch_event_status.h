#pragma once

#include "fetch_event.h"
#include "../status.h"

namespace core
{
    namespace wim
    {
        class fetch_event_status : public fetch_event
        {
        private:
            status status_;
            std::string aimid_;

        public:
            virtual ~fetch_event_status() = default;

            virtual int32_t parse(const rapidjson::Value& _node_event_data) override;
            virtual void on_im(std::shared_ptr<core::wim::im> _im, std::shared_ptr<auto_callback> _on_complete) override;

            const status& get_status() const noexcept { return status_; }
            const std::string& get_aimid() const noexcept { return aimid_; }
        };
    }
}
