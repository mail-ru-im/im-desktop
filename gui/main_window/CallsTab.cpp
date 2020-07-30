#include "stdafx.h"

#include "CallsTab.h"

#include "../common.shared/config/config.h"

#include "contact_list/ContactListWidget.h"
#include "contact_list/ContactListUtils.h"
#include "contact_list/ContactListTopPanel.h"
#include "contact_list/SearchWidget.h"
#include "contact_list/AddContactDialogs.h"
#include "MainPage.h"
#include "../utils/utils.h"

namespace Ui
{
    CallsTab::CallsTab(QWidget* _parent)
        : QWidget(_parent)
        , state_(LeftPanelState::min)
        , header_(new ContactListTopPanel())
        , contactListWidget_(new ContactListWidget(this, Logic::MembersWidgetRegim::CALLS_LIST, nullptr, nullptr))
        , callButton_(new HeaderTitleBarButton(this))
        , spacer_(new QWidget(this))
    {
        if (callButton_)
        {
            callButton_->setDefaultImage(qsl(":/header/call"), QColor(), QSize(24, 24));
            callButton_->setCustomToolTip(QT_TRANSLATE_NOOP("tab header", "Group call"));
            header_->addButtonToRight(callButton_);
            Testing::setAccessibleName(callButton_, qsl("AS CallsTab callButton"));

            connect(callButton_, &CustomButton::clicked,
                this, []() {
                Q_EMIT Utils::InterConnector::instance().createGroupCall();
            });
        }

        header_->setTitle(QT_TRANSLATE_NOOP("head", "Calls"));

        QObject::connect(contactListWidget_, &ContactListWidget::searchEnd, header_->getSearchWidget(), &SearchWidget::searchCompleted);
        contactListWidget_->connectSearchWidget(header_->getSearchWidget());
        QObject::connect(&Utils::InterConnector::instance(), &Utils::InterConnector::setContactSearchFocus, this, [this]() {
            header_->getSearchWidget()->setFocus();
            Q_EMIT Utils::InterConnector::instance().hideContactListPlaceholder();
        });


        auto l = Utils::emptyVLayout(this);
        spacer_->setFixedHeight(Utils::scale_value(8));
        l->addWidget(header_);
        l->addWidget(spacer_);
        l->addWidget(contactListWidget_);
    }

    CallsTab::~CallsTab()
    {
    }

    void CallsTab::setState(const LeftPanelState _state)
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

    void CallsTab::setClWidth(int _width)
    {
        contactListWidget_->setWidthForDelegate(_width);
    }

    ContactListWidget* CallsTab::getContactListWidget() const
    {
        return contactListWidget_;
    }

    SearchWidget* CallsTab::getSearchWidget() const
    {
        return header_->getSearchWidget();
    }

    void CallsTab::setPictureOnlyView(bool _isPictureOnly)
    {
        contactListWidget_->setPictureOnlyForDelegate(_isPictureOnly);
    }
}
