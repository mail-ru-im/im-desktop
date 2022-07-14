#pragma once

#include "types/message.h"
#include "controls/ClickWidget.h"

namespace Data
{
    class ChatInfo;
};

namespace Ui
{
    class ThreadFeedItemHeader : public ClickableWidget
    {
        Q_OBJECT

    public:
        ThreadFeedItemHeader(const QString& _chatId, QWidget* _parent);
        ~ThreadFeedItemHeader();

    protected:
        void paintEvent(QPaintEvent* _event);
        void resizeEvent(QResizeEvent* _event);

    private:
        TextRendering::TextUnitPtr chatName_;
        TextRendering::TextUnitPtr countMembers_;
        QPainterPath path_;
        QRect avatarRect_;
        QString chatId_;
        int32_t chatMembersCount_;
        bool pressed_ = false;

    private:
        void updateContent(int _width);

    private Q_SLOTS:
        void updateFromChatInfo(const std::shared_ptr<Data::ChatInfo>& _info);
    };
}
