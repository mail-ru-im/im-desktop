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

        layout->setSpacing(0);

        titleBar_->setTitle(QT_TRANSLATE_NOOP("head", "Contacts"));

        Testing::setAccessibleName(titleBar_, qsl("AS General titleBar"));
        layout->addWidget(titleBar_);

        searchWidget_->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
        Testing::setAccessibleName(searchWidget_, qsl("AS General searchWidget"));
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
        setFixedHeight(titleBar_->height() + searchWidget_->height());
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

    void ContactListTopPanel::setTitle(const QString& _title)
    {
        titleBar_->setTitle(_title);
    }
}