#pragma once

#include "types/friendly.h"

namespace Logic
{
    class FriendlyContainerProxy
    {
    public:

        enum Flags
        {
            ReplaceFavorites = 1 << 0,
        };

        FriendlyContainerProxy(int32_t _flags) : flags_(_flags) {}

        QString getFriendly(const QString& _aimid) const;
        Data::Friendly getFriendly2(const QString& _aimid) const;

    private:
        int32_t flags_;
    };

    FriendlyContainerProxy getFriendlyContainerProxy(int32_t _flags);

}
