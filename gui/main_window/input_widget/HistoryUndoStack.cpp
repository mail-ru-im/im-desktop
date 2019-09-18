#include "stdafx.h"

#include "HistoryUndoStack.h"
#include "utils/features.h"

namespace Ui
{
    Replacable::Replacable(const QString& _text)
    {
        text_ = _text;
        cursor_ = text_.isEmpty() ? -1 : text_.length();
    }

    QString& Replacable::Text()
    {
        return text_;
    }

    int64_t& Replacable::Cursor()
    {
        return cursor_;
    }

    HistoryUndoStack::HistoryUndoStack()
        : currentPosition_(0)
        , currentRedoAvailable_(false)
        , maxSize_(Features::maximumUndoStackSize())
    {
        vect_.reserve(maxSize_);
    }

    HistoryUndoStack::HistoryUndoStack(size_t _size)
        : currentPosition_(0)
        , currentRedoAvailable_(false)
        , maxSize_(std::min(_size, Features::maximumUndoStackSize()))
    {
        vect_.reserve(maxSize_);
    }

    void HistoryUndoStack::push(const QString& _state)
    {
        if (currentPosition_ == 0 || (currentPosition_ < maxSize_ && vect_.size() < maxSize_))
        {
            if (!vect_.empty())
            {
                clear();
                vect_.push_back(_state);
            }
            else
            {
                if (currentPosition_ == vect_.size() || currentPosition_ == 0)
                    vect_.push_back(_state);
                else
                    vect_[currentPosition_] = _state;

            }
            ++currentPosition_;
        }
        else
        {
            std::rotate(vect_.begin(), vect_.begin() + 1, vect_.end());
            clear();
            vect_[currentPosition_ - 1] = _state;
        }
    }

    void HistoryUndoStack::pop()
    {
        if (currentPosition_ > 0)
            --currentPosition_;
    }

    void HistoryUndoStack::redo()
    {
        if (currentPosition_ < maxSize_)
            ++currentPosition_;
    }

    Replacable& HistoryUndoStack::top()
    {
        if (vect_.size() > 0 && currentPosition_ > 0 && currentPosition_ <= vect_.size())
            return vect_[currentPosition_ - 1];

        static Replacable emptyRep;
        return emptyRep;
    }

    bool HistoryUndoStack::canRedo() const
    {
        return currentPosition_ < vect_.size();
    }

    bool HistoryUndoStack::canUndo() const
    {
        return currentPosition_ > 0;
    }

    void HistoryUndoStack::clear(const ClearType _clearType)
    {
        if (_clearType == ClearType::Full || currentPosition_ == 0)
        {
            vect_.clear();
            currentPosition_ = 0;
            return;
        }

        if (currentPosition_ < vect_.size())
            vect_.erase(vect_.begin() + currentPosition_, vect_.end());
    }
}