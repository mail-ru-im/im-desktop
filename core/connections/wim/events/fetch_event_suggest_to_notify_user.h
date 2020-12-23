#pragma once

#include "fetch_event.h"

namespace core::wim
{

class fetch_event_suggest_to_notify_user : public fetch_event
{
public:

    struct notify_user_info
    {
        std::string sn_;
        std::string sms_notify_context_;

        void serialize(coll_helper& _coll) const;
        bool unserialize(const rapidjson::Value& _node);
    };

    fetch_event_suggest_to_notify_user() = default;

    virtual int32_t parse(const rapidjson::Value& _node_event_data) override;
    virtual void on_im(std::shared_ptr<core::wim::im> _im, std::shared_ptr<auto_callback> _on_complete) override;

    const notify_user_info& get_info() const;

private:
    notify_user_info info_;
};

}
