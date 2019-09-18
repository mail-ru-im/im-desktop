#include "stdafx.h"

#include "../../../core_dispatcher.h"

#include "../../../controls/LabelEx.h"
#include "../../../controls/DialogButton.h"
#include "../../../fonts.h"
#include "../../../main_window/ContactDialog.h"
#include "../../../main_window/GroupChatOperations.h"
#include "../../../utils/utils.h"
#include "../../../utils/gui_coll_helper.h"
#include "../../contact_list/ContactList.h"
#include "../../contact_list/ContactListModel.h"
#include "../../contact_list/SelectionContactsForGroupChat.h"
#include "../../MainPage.h"
#include "../../history_control/MessagesScrollArea.h"

#include "SelectionPanel.h"

namespace
{
    constexpr int horizontalSpace = 12;
    constexpr int verticalSpace = 12;
    constexpr int buttonSpace = 12;
}

namespace Ui
{
    SelectionPanel::SelectionPanel(QWidget* _parent, MessagesScrollArea* _messages)
        : QFrame(_parent)
        , messages_(_messages)
    {
        assert(_messages);

        auto forward = new DialogButton(this, QT_TRANSLATE_NOOP("chat_page", "FORWARD"), DialogButtonRole::CONFIRM);
        forward->setCursor(Qt::PointingHandCursor);

        auto copy = new DialogButton(this, QT_TRANSLATE_NOOP("chat_page", "COPY"), DialogButtonRole::CONFIRM);
        copy->setCursor(Qt::PointingHandCursor);

        auto cancel = new LabelEx(this);
        cancel->setText(QT_TRANSLATE_NOOP("chat_page", "CANCEL"));
        cancel->setCursor(Qt::PointingHandCursor);
        cancel->adjustSize();

        connect(forward, &QPushButton::clicked, this, &SelectionPanel::onForwardClicked);
        connect(copy, &QPushButton::clicked, this, &SelectionPanel::onCopyClicked);
        connect(cancel, &LabelEx::clicked, this, &SelectionPanel::closePanel);

        auto layout = Utils::emptyHLayout();
        layout->setContentsMargins(
            Utils::scale_value(horizontalSpace), Utils::scale_value(verticalSpace),
            Utils::scale_value(horizontalSpace), Utils::scale_value(verticalSpace));

        Testing::setAccessibleName(forward, qsl("AS selection forward"));
        layout->addWidget(forward);
        layout->addSpacing(Utils::scale_value(buttonSpace));
        Testing::setAccessibleName(copy, qsl("AS selection copy"));
        layout->addWidget(copy);
        layout->addStretch();
        Testing::setAccessibleName(cancel, qsl("AS selection cancel"));
        layout->addWidget(cancel);

        setLayout(layout);
    }

    void SelectionPanel::closePanel()
    {
        messages_->clearSelection();
    }

    void SelectionPanel::onForwardClicked()
    {
        const auto quotes = messages_->getQuotes();
        if (quotes.empty())
            return;

        forwardMessage(quotes, false);

        closePanel();
    }

    void SelectionPanel::onCopyClicked()
    {
        const auto text = messages_->getSelectedText();

        const auto clipboard = QApplication::clipboard();
        clipboard->setText(text);

        closePanel();

        MainPage::instance()->getContactDialog()->setFocusOnInputWidget();
    }
}
