#pragma once

#include "fetch_event.h"
#include "../call_log_cache.h"
#include "../persons.h"

namespace core
{
    namespace wim
    {
        class fetch_event_recent_call_log : public fetch_event
        {
        public:

            fetch_event_recent_call_log();
            virtual ~fetch_event_recent_call_log();

            virtual int32_t parse(const rapidjson::Value& _node_event_data) override;
            virtual void on_im(std::shared_ptr<core::wim::im> _im, std::shared_ptr<auto_callback> _on_complete) override;

            const core::archive::call_log_cache& get_log() const;
            const core::archive::persons_map& get_persons() const;

        private:
            core::archive::call_log_cache log_;
            core::archive::persons_map persons_;
        };

    }
}