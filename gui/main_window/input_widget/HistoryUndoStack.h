#pragma once
#include "../../types/message.h"


namespace Ui
{
    class Replacable
    {
    public:
        Replacable(const Data::FString& _text = {});
        Data::FString& Text();
        int64_t& Cursor();
    private:
        Data::FString text_;
        int64_t cursor_;
    };

    class HistoryUndoStack
    {
    public:
        HistoryUndoStack();
        HistoryUndoStack(size_t _size);

        void push(const Data::FString& _state);
        void pop();
        Replacable& top();
        void redo();
        bool canRedo() const;
        bool canUndo() const;

        enum class ClearType
        {
            Full,
            Last
        };
        void clear(ClearType _clearType = ClearType::Last);
        bool isEmpty() const;
    private:
        std::vector<Replacable> vect_;
        size_t currentPosition_;
        const size_t maxSize_;
    };
}
