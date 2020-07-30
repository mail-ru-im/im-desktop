#pragma once
#include "../main_window/contact_list/SelectionContactsForGroupChat.h"
#include "../main_window/contact_list/ChatMembersModel.h"
#include "../controls/FlatMenu.h"

namespace Ui
{
    class SelectionStatusWidget : public SelectContactsWidget
    {
        Q_OBJECT

    Q_SIGNALS:
        void resendStatus();
    public:

        SelectionStatusWidget(
            const QString& _labelText,
            QWidget* _parent);
    private:
        FlatMenu* menu_;
        QString curStatus_;
        QTimer* updateTimer_;

        void updateModel();
        void showMoreMenu(const QString& _status);
    };
}
