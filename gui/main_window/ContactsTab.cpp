#include "stdafx.h"

#include "ContactsTab.h"

#include "contact_list/ContactListWidget.h"
#include "contact_list/ContactListUtils.h"
#include "contact_list/ContactListTopPanel.h"
#include "contact_list/SearchWidget.h"
#include "contact_list/AddContactDialogs.h"
#include "MainPage.h"
#include "../utils/utils.h"

namespace Ui
{
    ContactsTab::ContactsTab(QWidget* _parent)
        : QWidget(_parent)
        , state_(LeftPanelState::min)
        , header_(new ContactListTopPanel())
        , contactListWidget_(new ContactListWidget(this, Logic::MembersWidgetRegim::CONTACT_LIST_POPUP, nullptr, nullptr))
        , addButton_((build::is_biz() || build::is_dit()) ? nullptr : new HeaderTitleBarButton(this))
        , spacer_(new QWidget(this))
    {
        if (addButton_)
        {
            addButton_->setDefaultImage(qsl(":/header/add_user"), QColor(), QSize(24, 24));
            addButton_->setCustomToolTip(QT_TRANSLATE_NOOP("tab header", "Add contact"));
            header_->addButtonToRight(addButton_);

            connect(addButton_, &CustomButton::clicked,
                this, []() {
                Utils::showAddContactsDialog({ Utils::AddContactDialogs::Initiator::From::ScrContactTab });
            });
        }

        connect(contactListWidget_, &ContactListWidget::searchEnd, header_->getSearchWidget(), &SearchWidget::searchCompleted);
        contactListWidget_->connectSearchWidget(header_->getSearchWidget());

        auto l = Utils::emptyVLayout(this);
        spacer_->setFixedHeight(Utils::scale_value(8));
        l->addWidget(header_);
        l->addWidget(spacer_);
        l->addWidget(contactListWidget_);
    }

    ContactsTab::~ContactsTab()
    {
    }

    void ContactsTab::setState(const LeftPanelState _state)
    {
        if (state_ != _state)
        {
            state_ = _state;
            const bool isCompact = state_ == LeftPanelState::picture_only;
            header_->setState(state_);

            header_->setVisible(!isCompact);
            spacer_->setVisible(!isCompact);

            contactListWidget_->setPictureOnlyForDelegate(isCompact);
        }
    }

    void ContactsTab::setClWidth(int _width)
    {
        contactListWidget_->setWidthForDelegate(_width);
    }

    ContactListWidget* ContactsTab::getContactListWidget() const
    {
        return contactListWidget_;
    }
}
