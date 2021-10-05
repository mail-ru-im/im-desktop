#pragma once

#include "fetch_event.h"
#include "archive/thread_update.h"

namespace core::wim
{
    class fetch_event_thread_update: public fetch_event
    {
    public:
        int32_t parse(const rapidjson::Value& _node_event_data) override;
        void on_im(std::shared_ptr<core::wim::im> _im, std::shared_ptr<auto_callback> _on_complete) override;

        const core::archive::thread_update& get_update() const noexcept { return update_; }

    private:
        core::archive::thread_update update_;
    };
}