// Copyright (c) 2012 The Chromium Authors. All rights reserved.

#pragma once

#include <CoreFoundation/CoreFoundation.h>

#include "scoped_typeref.h"

namespace core
{

    // scoped_cftype_ref<> is patterned after std::unique_ptr<>, but maintains
    // ownership of a CoreFoundation object: any object that can be represented
    // as a CFTypeRef.  Style deviations here are solely for compatibility with
    // std::unique_ptr<>'s interface, with which everyone is already familiar.
    //
    // By default, scoped_cftype_ref<> takes ownership of an object (in the
    // constructor or in reset()) by taking over the caller's existing ownership
    // claim.  The caller must own the object it gives to scoped_cftype_ref<>, and
    // relinquishes an ownership claim to that object.  scoped_cftype_ref<> does not
    // call CFRetain(). This behavior is parameterized by the |ownership_policy|
    // enum. If the value |RETAIN| is passed (in the constructor or in reset()),
    // then scoped_cftype_ref<> will call CFRetain() on the object, and the initial
    // ownership is not changed.

    namespace internal
    {

        template<typename CFT>
        struct scoped_cftype_ref_traits
        {
            static CFT invalid_value()
            {
                return nullptr;
            }
            static CFT retain(CFT object)
            {
                CFRetain(object);
                return object;
            }
            static void release(CFT object)
            {
                CFRelease(object);
            }
        };

        template<typename CFT>
        using scoped_cftype_ref = scoped_type_ref<CFT, internal::scoped_cftype_ref_traits<CFT>>;

    }  // namespace internal

}  // namespace core

