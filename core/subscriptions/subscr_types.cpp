#include "stdafx.h"

#include "subscr_types.h"

namespace core::wim::subscriptions
{
    std::string_view type_2_string(subscriptions::type _type) noexcept
    {
        switch (_type)
        {
        case subscriptions::type::antivirus:
            return "antivirus";
        case subscriptions::type::status:
            return "status";
        case subscriptions::type::call_room_info:
            return "callRoomInfo";
        case subscriptions::type::thread:
            return "threadUpdate";
        case subscriptions::type::task:
            return "task";
        case subscriptions::type::tasks_counter:
            return "unreadTasksCount";
        case subscriptions::type::mails_counter:
            return "unreadEmailsCount";
        default:
            break;
        }

        im_assert(!"unsupported subscription event type");
        return std::string_view();
    }

    subscriptions::type string_2_type(std::string_view _type_string) noexcept
    {
        if (_type_string == "antivirus")
            return subscriptions::type::antivirus;
        else if (_type_string == "status")
            return subscriptions::type::status;
        else if (_type_string == "callRoomInfo")
            return subscriptions::type::call_room_info;
        else if (_type_string == "threadUpdate")
            return subscriptions::type::thread;
        else if (_type_string == "task")
            return subscriptions::type::task;
        else if (_type_string == "unreadTasksCount")
            return subscriptions::type::tasks_counter;
        else if (_type_string == "unreadEmailsCount")
            return subscriptions::type::mails_counter;

        im_assert(!"unsupported subscription event type");
        return subscriptions::type::invalid;
    }

    const std::vector<type>& types_for_start_session_subscribe()
    {
        static const std::vector<subscriptions::type> types =
        {
            subscriptions::type::status,
        };
        return types;
    }
}
