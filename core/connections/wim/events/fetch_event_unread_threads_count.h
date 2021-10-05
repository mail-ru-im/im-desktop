#pragma once

#include "fetch_event.h"

namespace core::wim
{
    class fetch_event_unread_threads_count : public fetch_event
    {
    public:
        int32_t parse(const rapidjson::Value& _node_event_data) override;
        void on_im(std::shared_ptr<core::wim::im> _im, std::shared_ptr<auto_callback> _on_complete) override;

        int get_unread_threads_count() const noexcept { return unread_threads_; }
        int get_unread_mention_me_count() const noexcept { return unread_mentions_me_; }

    private:
        int unread_threads_ = 0;
        int unread_mentions_me_ = 0;
    };
}