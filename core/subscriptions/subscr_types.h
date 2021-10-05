#pragma once

namespace core::wim::subscriptions
{
    enum class type
    {
        invalid,

        antivirus,
        status,
        call_room_info,
        thread,
        task,
    };

    std::string_view type_2_string(subscriptions::type _type) noexcept;
    subscriptions::type string_2_type(std::string_view _type_string) noexcept;

    const std::vector<type>& types_for_start_session_subscribe();
}
