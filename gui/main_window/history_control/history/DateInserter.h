#pragma once

#include "../../../types/message.h"

namespace Logic
{
    class MessageKey;
}

namespace Ui
{
  namespace ComplexMessage
  {
      class ComplexMessageItem;
  }
  class HistoryControlPageItem;
};

namespace Data
{
    class MessageBuddy;
};

namespace hist
{
    class DateInserter : public QObject
    {
        Q_OBJECT

    public:
        explicit DateInserter(const QString& _contact, QObject* _parent = nullptr);

        static bool needDate(const Logic::MessageKey& _prev, const Logic::MessageKey& _key);
        static bool needDate(const QDate& _prev, const QDate& _key);

        bool isChat() const;

        Logic::MessageKey makeDateKey(const Logic::MessageKey& _key) const;
        Data::MessageBuddy makeDateMessage(const Logic::MessageKey& _key) const;
        std::unique_ptr<Ui::HistoryControlPageItem> makeDateItem(const Logic::MessageKey& _key, int _width, QWidget* _parent) const;
        std::unique_ptr<Ui::HistoryControlPageItem> makeDateItem(const Data::MessageBuddy& _message, int _width, QWidget* _parent) const;

    private:
        const QString contact_;
    };
}