#pragma once

#include "../../../types/message.h"

namespace Logic
{
    class MessageKey;
}

namespace hist
{
    class DateInserter : public QObject
    {
        Q_OBJECT
    public:

        explicit DateInserter(const QString& _contact, QObject* _parent = nullptr);

        static bool needDate(const Logic::MessageKey& prev, const Logic::MessageKey& key);
        static bool needDate(const QDate& prev, const QDate& key);

        bool isChat() const;

        Logic::MessageKey makeDateKey(const Logic::MessageKey& key) const;
        std::unique_ptr<QWidget> makeDateItem(const Logic::MessageKey& key, int width, QWidget* parent) const;

    private:
        const QString contact_;
    };
}