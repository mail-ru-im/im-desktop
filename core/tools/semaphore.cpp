#include "stdafx.h"
#include "semaphore.h"

using namespace core;
using namespace tools;

semaphore::semaphore(unsigned long count)
    : count_(count)
{
}


semaphore::~semaphore()
{
}

void semaphore::notify()
{
    std::lock_guard<std::mutex> lock(mtx_);
    ++count_;
    cv_.notify_one();
}

void semaphore::wait()
{
    std::unique_lock<std::mutex> lock(mtx_);

    cv_.wait(lock, [&]{ return count_ > 0; });
    --count_;
}