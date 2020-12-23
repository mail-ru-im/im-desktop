// Copyright (c) 2011 The Chromium Authors. All rights reserved.

#pragma once

#include <stddef.h>
#include <winsock2.h>

namespace core 
{
    namespace internal 
    {

        // Assert that the (manual-reset) event object is not signaled.
        void assert_event_not_signaled(WSAEVENT _hEvent);

        // If the (manual-reset) event object is signaled, resets it and returns true.
        // Otherwise, does nothing and returns false.  Called after a Winsock function
        // succeeds synchronously
        //
        // Our testing shows that except in rare cases (when running inside QEMU),
        // the event object is already signaled at this point, so we call this method
        // to avoid a context switch in common cases.  This is just a performance
        // optimization.  The code still works if this function simply returns false.
        bool reset_event_if_signaled(WSAEVENT _hEvent);

        void ensure_winsock_init();

    } // namespace internal
}  // namespace core
