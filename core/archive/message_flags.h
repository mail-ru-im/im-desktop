#ifndef __MESSAGE_FLAGS_H_
#define __MESSAGE_FLAGS_H_

#pragma once

namespace core
{
    namespace archive
    {
        union message_flags
        {
            struct
            {
                uint32_t	unused0_    : 1;
                uint32_t	unread_     : 1;
                uint32_t	outgoing_   : 1;
                uint32_t	invisible_  : 1;
                uint32_t    patch_      : 1;
                uint32_t    deleted_    : 1;
                uint32_t    modified_   : 1;
                uint32_t    updated_    : 1;
                uint32_t    clear_      : 1;
            } flags_;

            uint32_t	value_;

            message_flags()
                :	value_(0)
            {
            }
        };
    }
}

#endif //__MESSAGE_FLAGS_H_