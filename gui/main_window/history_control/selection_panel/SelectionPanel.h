#pragma once

namespace Ui
{
    class MessagesScrollArea;

    class SelectionPanel : public QFrame
    {
        Q_OBJECT

    private Q_SLOTS:
        void onForwardClicked();
        void onCopyClicked();

    public:
        SelectionPanel(QWidget* _parent, MessagesScrollArea* _messages);

    private:
        void closePanel();

    private:
        MessagesScrollArea* messages_;
    };
}
