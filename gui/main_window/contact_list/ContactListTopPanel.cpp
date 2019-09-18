#include "stdafx.h"
#include "ContactListTopPanel.h"
#include "../MainPage.h"
#include "SearchWidget.h"

namespace Ui
{
    ContactListTopPanel::ContactListTopPanel(QWidget* _parent)
        : TopPanelHeader(_parent)
        , titleBar_(new HeaderTitleBar(this))
        , searchWidget_(new SearchWidget(this))
        , state_(LeftPanelState::min)
    {
        auto layout = Utils::emptyVLayout(this);

        setFixedHeight(titleBar_->height() + searchWidget_->height());

        layout->setSpacing(0);

        titleBar_->setTitle(QT_TRANSLATE_NOOP("head", "Contacts"));

        Testing::setAccessibleName(titleBar_, qsl("AS cltp titleBar_"));
        layout->addWidget(titleBar_);

        searchWidget_->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
        Testing::setAccessibleName(searchWidget_, qsl("AS cltp searchWidget_"));
        layout->addWidget(searchWidget_);
    }

    ContactListTopPanel::~ContactListTopPanel()
    {
    }

    void ContactListTopPanel::setState(const LeftPanelState _state)
    {
        if (state_ == _state)
            return;

        TopPanelHeader::setState(_state);
    }

    SearchWidget* ContactListTopPanel::getSearchWidget() const noexcept
    {
        return searchWidget_;
    }

    void ContactListTopPanel::addButtonToLeft(HeaderTitleBarButton* _button)
    {
        titleBar_->addButtonToLeft(_button);
    }

    void ContactListTopPanel::addButtonToRight(HeaderTitleBarButton* _button)
    {
        titleBar_->addButtonToRight(_button);
    }
}