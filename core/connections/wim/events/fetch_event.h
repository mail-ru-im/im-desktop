#pragma once

#include "../../../namespaces.h"

namespace core
{
    class auto_callback;
}

CORE_WIM_NS_BEGIN

class im;


class fetch_event
{
public:
    virtual ~fetch_event() {}
    virtual int32_t parse(const rapidjson::Value& node_event_data) { return  0; }

    virtual void on_im(std::shared_ptr<core::wim::im> _im, std::shared_ptr<auto_callback> _on_complete) {}

};

CORE_WIM_NS_END
