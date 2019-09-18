#pragma once

#include "persons.h"

namespace core::wim
{
    class persons_packet
    {
    public:
        virtual ~persons_packet() {}

        virtual const std::shared_ptr<core::archive::persons_map>& get_persons() const = 0;
    };
}
