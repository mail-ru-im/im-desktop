#include "stdafx.h"

#include "ThreadSubContainer.h"

#include "utils/features.h"
#include "core_dispatcher.h"

namespace Logic
{
    ThreadSubContainer& ThreadSubContainer::instance()
    {
        static ThreadSubContainer instance;
        return instance;
    }

    void ThreadSubContainer::subscribe(const QString& _threadId)
    {
        if (_threadId.isEmpty() || !Features::isThreadsEnabled())
            return;

        if (subscriptions_.insert(_threadId).second)
            Ui::GetDispatcher()->subscribeThread({ _threadId });
    }

    void ThreadSubContainer::unsubscribe(const QString& _threadId)
    {
        if (_threadId.isEmpty())
            return;

        if (subscriptions_.erase(_threadId))
            Ui::GetDispatcher()->unsubscribeThread({ _threadId });
    }

    void ThreadSubContainer::clear()
    {
        subscriptions_.clear();
    }
}
