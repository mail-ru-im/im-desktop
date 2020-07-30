#pragma once

#include "fetch_event.h"

namespace core::wim
{

class fetch_event_call_room_info : public fetch_event
{
public:

    struct call_room_info
    {
        bool failed_ = false;
        std::string room_id_;
        int64_t members_count_;

        void serialize(icollection* _collection) const;
        bool unserialize(const rapidjson::Value& _node);
    };

    fetch_event_call_room_info() = default;

    virtual int32_t parse(const rapidjson::Value& _node_event_data) override;
    virtual void on_im(std::shared_ptr<core::wim::im> _im, std::shared_ptr<auto_callback> _on_complete) override;

    const call_room_info& get_info() const;

private:
    call_room_info info_;
};

}
