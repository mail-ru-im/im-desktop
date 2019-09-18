#ifndef __COMMON_CRASH_SENDER_H_
#define __COMMON_CRASH_SENDER_H_

#ifdef _WIN32

namespace common_crash_sender
{
    void start_sending_process();
    void post_dump_to_hockey_app_and_delete();
}

#endif // _WIN32

#endif // __COMMON_CRASH_SENDER_H_
