#pragma once

#include "fetch_event.h"
#include "../../../tasks/task.h"

namespace core
{
    namespace wim
    {
        class fetch_event_task : public fetch_event
        {
            core::tasks::task_data task_;

        public:
            virtual ~fetch_event_task() = default;

            virtual int32_t parse(const rapidjson::Value& _node_event_data) override;
            virtual void on_im(std::shared_ptr<core::wim::im> _im, std::shared_ptr<auto_callback> _on_complete) override;

            const core::tasks::task_data& get_task() const noexcept { return task_; }
        };
    }
}
