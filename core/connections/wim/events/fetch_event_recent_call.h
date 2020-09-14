#pragma once

#include "fetch_event.h"
#include "../call_log_cache.h"

namespace core
{
    namespace wim
    {
        class fetch_event_recent_call : public fetch_event
        {
        public:

            explicit fetch_event_recent_call();
            virtual ~fetch_event_recent_call();

            virtual int32_t parse(const rapidjson::Value& _node_event_data) override;
            virtual void on_im(std::shared_ptr<core::wim::im> _im, std::shared_ptr<auto_callback> _on_complete) override;

            const core::archive::call_info& get_call() const;
            const core::archive::persons_map& get_persons() const;

        private:
            core::archive::call_info call_;
            core::archive::persons_map persons_;
        };

    }
}