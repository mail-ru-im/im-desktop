#include "stdafx.h"

#include "smartreply_types.h"

using namespace core;

std::string_view smartreply::type_2_string(const smartreply::type _type) noexcept
{
    switch (_type)
    {
    case smartreply::type::sticker:
        return "sticker-smartreply";

    case smartreply::type::sticker_by_text:
        return "sticker-by-text";

    case smartreply::type::text:
        return "text-smartreply";

    default:
        break;
    }

    assert(!"invalid smartreply type or unimplemented");
    return std::string_view();
}

smartreply::type smartreply::string_2_type(const std::string_view _str) noexcept
{
    if (_str == smartreply::type_2_string(smartreply::type::sticker))
        return smartreply::type::sticker;
    if (_str == smartreply::type_2_string(smartreply::type::sticker_by_text))
        return smartreply::type::sticker_by_text;
    else if (_str == smartreply::type_2_string(smartreply::type::text))
        return smartreply::type::text;

    assert(!"invalid smartreply type string or unimplemented");
    return smartreply::type::invalid;
}

std::string_view core::smartreply::array_node_name_for(const smartreply::type _type) noexcept
{
    switch (_type)
    {
    case smartreply::type::sticker:
    case smartreply::type::sticker_by_text:
        return "stickers";

    case smartreply::type::text:
        return "text";

    default:
        break;
    }

    assert(!"invalid smartreply type or unimplemented");
    return std::string_view();
}
