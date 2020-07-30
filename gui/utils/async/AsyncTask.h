#pragma once

namespace Async
{
    inline namespace impl
    {
        template <typename Callable>
        class Runnable : public QRunnable {
        public:
            Runnable(Callable&& callable) : _callable(std::forward<Callable>(callable))
            {
            }

            void run() override
            {
                if (_callable)
                    _callable();
            }

        private:
            Callable _callable;
        };
    }

    template <typename Callable>
    void runInMain(Callable&& callable)
    {
        if constexpr (build::is_release())
        {
            QMetaObject::invokeMethod(qApp, std::forward<Callable>(callable));
        }
        else
        {
            QMetaObject::invokeMethod(qApp, [f = std::forward<Callable>(callable)]() mutable
            {
                Utils::ensureMainThread();
                f();
            });
        }
    }

    template <typename Callable>
    void runAsync(Callable&& callable)
    {
        QThreadPool::globalInstance()->start(std::make_unique<Runnable<Callable>>(std::forward<Callable>(callable)).release());
    }
}