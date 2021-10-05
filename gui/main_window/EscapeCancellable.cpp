#include "stdafx.h"

#include "EscapeCancellable.h"

namespace Ui
{
    EscapeCancelItem::EscapeCancelItem(QObject* _context, std::function<void()> _callback, EscPriority _priority)
        : context_(_context)
        , cancelCallback_(std::move(_callback))
        , priority_(int(_priority))
    {
    }

    void EscapeCancelItem::addChild(IEscapeCancellable* _cancellable)
    {
        if (!_cancellable)
            return;

        addChild(_cancellable->escCancel_);
    }

    void EscapeCancelItem::addChild(EscCancelPtr _cancellable)
    {
        im_assert(_cancellable);
        im_assert(_cancellable->context_);

        if (!_cancellable || !_cancellable->context_)
            return;

        removeChild(_cancellable->context_);

        auto it = std::find_if_not(children_.begin(), children_.end(), [&_cancellable](const auto& x) { return x->priority_ < _cancellable->priority_; });
        children_.insert(it, std::move(_cancellable));
    }

    void EscapeCancelItem::removeChild(QObject* _context)
    {
        if (!_context)
            return;

        children_.erase(
            std::remove_if(children_.begin(), children_.end(), [_context](const auto& x)
            {
                return x->context_ == _context || !x->context_; // remove asked and invalid
            }), children_.end());
    }

    bool EscapeCancelItem::canCancel() const
    {
        return (context_ && cancelCallback_) || (hasChildren() && children_.front()->canCancel());
    }

    bool EscapeCancelItem::cancel()
    {
        if (hasChildren() && children_.front()->canCancel())
            return children_.front()->cancel();

        if (context_ && cancelCallback_)
            QMetaObject::invokeMethod(context_, cancelCallback_);

        return true;
    }

    void EscapeCancelItem::dumpTree(int _level)
    {
        qDebug().noquote()
            << QString().fill(u' ', _level) << "+"
            << (context_ ? context_->metaObject()->className() : "NO CONTEXT")
            << (cancelCallback_ ? "cb" : "") << priority_
            << (canCancel() ? "<-" : "");
        for (const auto& c : children_)
            c->dumpTree(_level + 1);
    }
}