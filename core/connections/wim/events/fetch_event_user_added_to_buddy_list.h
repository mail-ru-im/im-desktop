#pragma once

#include "../../../namespaces.h"

#include "fetch_event.h"

CORE_WIM_NS_BEGIN

class im;

class fetch_event_user_added_to_buddy_list : public fetch_event
{
public:
    virtual int32_t parse(const rapidjson::Value& _node_event_data) override;

    virtual void on_im(std::shared_ptr<core::wim::im> _im, std::shared_ptr<auto_callback> _on_complete) override;

    const std::string& get_display_aimid() const;

    const std::string& get_requester_aimid() const;

private:
    std::string requester_uid_;

    std::string display_aimid_;

};

CORE_WIM_NS_END