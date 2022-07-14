#pragma once

#include "fetch_event.h"

namespace core::wim
{
    class fetch_event_tasks_count : public fetch_event
    {
    public:
        int32_t parse(const rapidjson::Value& _node_event_data) override;
        void on_im(std::shared_ptr<core::wim::im> _im, std::shared_ptr<auto_callback> _on_complete) override;

        int get_unread_tasks_count() const noexcept { return tasks_; }

    private:
        int tasks_ = 0;
    };
}