#ifndef __SEMAPHORE_H__
#define __SEMAPHORE_H__

#pragma once


namespace core
{
    namespace tools
    {
        class semaphore
        {
            std::mutex mtx_;
            std::condition_variable cv_;

            unsigned long count_;

        public:

            void notify();
            void wait();

            semaphore(unsigned long count = 0);
            virtual ~semaphore();
        };
    }
}

#endif //__SEMAPHORE_H__
