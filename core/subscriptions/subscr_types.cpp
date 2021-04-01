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
        default:
            break;
        }

        assert(!"unsupported subscription event type");
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

        assert(!"unsupported subscription event type");
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
