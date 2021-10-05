#pragma once

#include "fetch_event.h"
#include "archive/draft_storage.h"
#include "archive/thread_parent_topic.h"

namespace core::wim
{

class fetch_event_draft : public fetch_event
{
public:
    fetch_event_draft() = default;

    virtual int32_t parse(const rapidjson::Value& _node_event_data) override;
    virtual void on_im(std::shared_ptr<core::wim::im> _im, std::shared_ptr<auto_callback> _on_complete) override;

    const archive::draft& get_draft() const noexcept { return draft_; }
    const std::string& get_contact() const noexcept { return contact_; }
    const std::optional<archive::thread_parent_topic>& get_parent_topic() const noexcept { return parent_topic_; }
    const core::archive::persons_map& get_persons() const noexcept { return persons_; }

private:
    archive::draft draft_;
    std::string contact_;
    std::optional<archive::thread_parent_topic> parent_topic_;
    core::archive::persons_map persons_;
};

}
