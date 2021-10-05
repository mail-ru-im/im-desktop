#include "stdafx.h"
#include "thread_types.h"

namespace core::threads::parent_topic
{
    type string_to_type(std::string_view _str)
    {
        if (_str == "message")
            return type::message;
        else if (_str == "task")
            return type::task;

        im_assert(!"not implemented yet");
        return type::invalid;
    }

    std::string_view type_to_string(type _type)
    {
        if (_type == type::message)
            return "message";
        if (_type == type::task)
            return "task";

        im_assert(!"not implemented yet");
        return {};
    }
}
