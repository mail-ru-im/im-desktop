#pragma once


namespace Ui
{
    class Replacable
    {
    public:
        Replacable(const QString& _text = QString());
        QString& Text();
        int64_t& Cursor();
    private:
        QString text_;
        int64_t cursor_;
    };

    class HistoryUndoStack
    {
    public:
        HistoryUndoStack();
        HistoryUndoStack(size_t _size);

        void push(const QString& _state);
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
        void clear(const ClearType _clearType = ClearType::Last);
    private:
        std::vector<Replacable> vect_;
        int64_t currentPosition_;
        bool currentRedoAvailable_;
        const size_t maxSize_;
    };
}