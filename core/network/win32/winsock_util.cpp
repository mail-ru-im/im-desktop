// Copyright (c) 2011 The Chromium Authors. All rights reserved.

#include "stdafx.h"
#include "winsock_util.h"

namespace
{

    // Prevent the compiler from optimizing away the arguments so they appear
    // nicely on the stack in crash dumps.
#pragma warning(push)
#pragma warning (disable: 4748)
#pragma optimize( "", off )

    // Pass the important values as function arguments so that they are available
    // in crash dumps.
    void check_event_wait(WSAEVENT hEvent, DWORD wait_rv, DWORD expected)
    {
        if (wait_rv != expected)
        {
            DWORD err = ERROR_SUCCESS;
            if (wait_rv == WAIT_FAILED)
                err = GetLastError();
            assert(false);  // Crash.
        }
    }

#pragma optimize( "", on )
#pragma warning(pop)

    class winsock_init_singleton
    {
    public:
        winsock_init_singleton()
        {
            WORD winsock_ver = MAKEWORD(2, 2);
            WSAData wsa_data;
            bool did_init = (WSAStartup(winsock_ver, &wsa_data) == 0);
            if (did_init)
            {
                assert(wsa_data.wVersion == winsock_ver);

                // The first time WSAGetLastError is called, the delay load helper will
                // resolve the address with GetProcAddress and fixup the import.  If a
                // third party application hooks system functions without correctly
                // restoring the error code, it is possible that the error code will be
                // overwritten during delay load resolution.  The result of the first
                // call may be incorrect, so make sure the function is bound and future
                // results will be correct.
                WSAGetLastError();
            }
        }
    };

}  // namespace

namespace core
{
    namespace internal
    {

        void assert_event_not_signaled(WSAEVENT _hEvent) 
        {
            DWORD wait_rv = WaitForSingleObject(_hEvent, 0);
            check_event_wait(_hEvent, wait_rv, WAIT_TIMEOUT);
        }

        bool reset_event_if_signaled(WSAEVENT _hEvent) {
            DWORD wait_rv = WaitForSingleObject(_hEvent, 0);
            if (wait_rv == WAIT_TIMEOUT)
                return false;  // The event object is not signaled.
            check_event_wait(_hEvent, wait_rv, WAIT_OBJECT_0);
            WSAResetEvent(_hEvent);
            return true;
        }

        void ensure_winsock_init()
        {
            static winsock_init_singleton s;
        }

    } // namespace internal 
} // namespace core
