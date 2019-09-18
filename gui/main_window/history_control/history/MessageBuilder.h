#pragma once

#include "../../../types/message.h"
#include "../../mediatype.h"

namespace Ui
{
    class MessageItem;
    class ServiceMessageItem;
    class HistoryControlPageItem;
    enum class MessagesBuddiesOpt;
}

namespace hist::MessageBuilder
{
    [[nodiscard]] std::unique_ptr<Ui::HistoryControlPageItem> makePageItem(const Data::MessageBuddy& _msg, int _itemWidth, QWidget* _parent);

    [[nodiscard]] Ui::MediaWithText formatRecentsText(const Data::MessageBuddy &buddy);

    [[nodiscard]] std::unique_ptr<Ui::ServiceMessageItem> createNew(const QString& _aimId, int _itemWidth, QWidget* _parent);
}