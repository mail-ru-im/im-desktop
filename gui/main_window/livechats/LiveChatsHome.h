#pragma once

#include "../../types/chat.h"

namespace Ui
{
    class LiveChatProfileWidget;

    class LiveChatHomeWidget : public QWidget
    {
        Q_OBJECT
    public:
        LiveChatHomeWidget(QWidget* _parent, const Data::ChatInfo& _info);
        virtual ~LiveChatHomeWidget();

    protected:
        void paintEvent(QPaintEvent* _e) override;

    private Q_SLOTS:
        void joinButtonClicked();
        void chatJoined(const QString&);
        void chatRemoved(const QString&);

    private:
        void initButtonText();

    private:
        Data::ChatInfo info_;
        LiveChatProfileWidget* profile_;
        QPushButton* joinButton_;
    };

    class LiveChatHome : public QWidget
    {
        Q_OBJECT
    public:

        LiveChatHome(QWidget* _parent);
        virtual ~LiveChatHome();

    protected:
        void paintEvent(QPaintEvent* _e) override;

    private Q_SLOTS:
        void liveChatSelected(const Data::ChatInfo&);
        void onLiveChatSelected(const std::shared_ptr<Data::ChatInfo>&);

    private:
        QVBoxLayout* layout_;
    };
}
