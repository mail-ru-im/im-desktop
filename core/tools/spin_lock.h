#ifndef __SPIN_LOCK_H__
#define __SPIN_LOCK_H__

#pragma once


namespace core
{
    namespace tools
    {
        class spin_lock
        {
            std::atomic_flag locked = ATOMIC_FLAG_INIT;
        public:
            void lock()
            {
                while (locked.test_and_set(std::memory_order_acquire)) { ; }
            }
            void unlock()
            {
                locked.clear(std::memory_order_release);
            }
        };
    }
}

#endif //__SPIN_LOCK_H__
