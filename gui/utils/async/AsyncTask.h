#pragma once

namespace Async
{
    inline namespace impl
    {
        template<typename T>
        struct hasBoolOp
        {
            template<typename U, bool(U::*)() const> struct SFINAE {};
            template<typename U> static char TestMethod(SFINAE<U, &U::operator bool>*);
            template<typename U> static int TestMethod(...);
            static const bool value = sizeof(TestMethod<T>(0)) == sizeof(char);
        };

        template <typename Callable>
        class Runnable : public QRunnable
        {
        public:
            Runnable(Callable&& callable) : _callable(std::forward<Callable>(callable))
            {
            }

            void run() override
            {
                if constexpr (hasBoolOp<decltype(_callable)>::value)
                {
                    if (_callable)
                        _callable();
                }
                else
                {
                    _callable();
                }
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