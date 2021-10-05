#pragma once

namespace Logic
{
    class ThreadSubContainer : public QObject
    {
    public:
        static ThreadSubContainer& instance();
        ThreadSubContainer(ThreadSubContainer const&) = delete;
        ThreadSubContainer& operator=(ThreadSubContainer const&) = delete;
        ~ThreadSubContainer() = default;

        void subscribe(const QString& _threadId);
        void unsubscribe(const QString& _threadId);
        void clear();

    private:
        ThreadSubContainer() = default;

    private:
        std::unordered_set<QString> subscriptions_;
    };
}