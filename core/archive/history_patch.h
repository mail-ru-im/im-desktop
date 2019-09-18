#pragma once

#include "../namespaces.h"

CORE_ARCHIVE_NS_BEGIN

class history_patch;

using history_patch_uptr = std::unique_ptr<history_patch>;

class history_patch
{
public:
    enum class type
    {
        min,

        deleted,
        modified,
        pinned,
        unpinned,
        updated,
        clear,

        max
    };

    static history_patch_uptr make(const type _type, const int64_t _archive_id);
    static std::string type_to_string(const type _type);
    static bool is_valid_type(const type _type);

    int64_t get_message_id() const;

    type get_type() const;

private:
    history_patch(const type _type, const int64_t _archive_id);

    const type type_;

    const int64_t archive_id_;

    std::string new_text_;
};

static_assert(
    std::is_move_constructible<history_patch>::value,
    "std::is_move_constructible<history_patch>::value"
    );

CORE_ARCHIVE_NS_END