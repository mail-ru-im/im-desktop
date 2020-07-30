#pragma once

#include "fetch_event.h"
#include "archive/reactions.h"

namespace core::wim
{

class fetch_event_reactions : public fetch_event
{
public:
    fetch_event_reactions() = default;

    virtual int32_t parse(const rapidjson::Value& _node_event_data) override;
    virtual void on_im(std::shared_ptr<core::wim::im> _im, std::shared_ptr<auto_callback> _on_complete) override;

    const archive::reactions_data& get_reactions() const;

private:
    archive::reactions_data reactions_;
};

}
