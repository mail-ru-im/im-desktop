#pragma once

namespace core
{
    class coll_helper;

    namespace smartreply
    {
        enum class type;

        struct marker
        {
            int64_t msgid_;
            smartreply::type type_;

            marker();
            bool unserialize(const coll_helper& _coll);
        };

        using marker_opt = std::optional<smartreply::marker>;
    }
}