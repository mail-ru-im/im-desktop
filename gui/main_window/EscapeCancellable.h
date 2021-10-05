#pragma once

namespace Ui
{
    class EscapeCancelItem;
    class IEscapeCancellable;
    using EscCancelPtr = std::shared_ptr<EscapeCancelItem>;

    // the lower number is the higher priority
    enum class EscPriority
    {
        usual = 100,
        high = 99,
    };

    class EscapeCancelItem
    {
    public:
        EscapeCancelItem(QObject* _context, std::function<void()> _callback = {}, EscPriority _priority = EscPriority::usual);
        EscapeCancelItem() = default;

        void addChild(IEscapeCancellable* _cancellable);
        void addChild(QObject* _context, std::function<void()> _callback, EscPriority _priority = EscPriority::usual)
        {
            addChild(std::make_shared<EscapeCancelItem>(_context, std::move(_callback), _priority));
        }
        void addChild(EscCancelPtr _cancellable);
        void removeChild(QObject* _context);
        bool hasChildren() const noexcept { return !children_.empty(); }

        bool canCancel() const;
        bool cancel();

        void dumpTree(int _level);

        void setCancelCallback(std::function<void()> _callback)
        {
            cancelCallback_ = std::move(_callback);
        }

        void setCancelPriority(EscPriority _priority)
        {
            priority_ = int(_priority);
        }

    private:
        std::vector<EscCancelPtr> children_;

        QPointer<QObject> context_;
        std::function<void()> cancelCallback_;
        int priority_ = int(EscPriority::usual);
    };

    class IEscapeCancellable
    {
    public:
        EscCancelPtr escCancel_;

        explicit IEscapeCancellable(QObject* _context = nullptr)
            : escCancel_(std::make_shared<EscapeCancelItem>(_context))
        {}

        void setCancelCallback(std::function<void()> _callback)
        {
            escCancel_->setCancelCallback(std::move(_callback));
        }
    };
}