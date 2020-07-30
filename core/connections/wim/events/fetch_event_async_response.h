#pragma once

#include "fetch_event.h"
#include "../bot_payload.h"

namespace core
{
    namespace wim
    {
        class fetch_event_async_response : public fetch_event
        {
        private:
            std::string req_id_;
            bot_payload payload_;

        public:

            fetch_event_async_response();
            virtual ~fetch_event_async_response();

            virtual int32_t parse(const rapidjson::Value& _node_event_data) override;
            virtual void on_im(std::shared_ptr<core::wim::im> _im, std::shared_ptr<auto_callback> _on_complete) override;

            const std::string& get_bot_req_id() const noexcept;
            const bot_payload& get_payload() const noexcept;
        };
    }
}