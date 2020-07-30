#ifndef __EVENT_H__
#define __EVENT_H__

#pragma once


namespace core
{
    namespace tools
    {
        class autoreset_event
        {
            std::mutex mtx_;
            std::condition_variable cv_;

            bool signalled_ = false;

        public:

            void notify();
            void wait();
            bool wait_for(std::chrono::milliseconds _timeout);
        };
    }
}

#endif //__EVENT_H__
