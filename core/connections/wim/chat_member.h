#pragma once

#include "lastseen.h"

namespace core
{
    namespace wim
    {
        struct chat_member_short_info
        {
            std::string aimid_;
            std::string role_;
            lastseen lastseen_;
            std::optional<int64_t> can_remove_till_;
        };

        struct chat_member_info : public chat_member_short_info
        {
            std::string nick_;
            std::string friendly_;

            bool friend_ = false;
            bool no_avatar_ = false;
        };
    }
}
