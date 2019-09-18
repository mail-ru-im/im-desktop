#include "stdafx.h"

#include "TabWidget.h"
#include "TabBar.h"

#include "../utils/utils.h"
#include "../controls/LineLayoutSeparator.h"

#include "styles/ThemeParameters.h"

namespace Ui
{
    TabWidget::TabWidget(QWidget* _parent)
        : QWidget(_parent)
    {
        setContentsMargins(0, 0, 0, 0);

        Utils::setDefaultBackground(this);

        auto layout = Utils::emptyVLayout(this);
        tabbar_ = new TabBar(this);

        pages_ = new QStackedWidget(this);
        pages_->setStyleSheet(qsl("background: transparent; border: none; color: %1").arg(Styling::getParameters().getColorHex(Styling::StyleVariable::BASE_PRIMARY)));

        Testing::setAccessibleName(pages_, qsl("AS tab pages_"));
        layout->addWidget(pages_);

        Testing::setAccessibleName(tabbar_, qsl("AS tab tabbar_"));
        layout->addWidget(tabbar_);

        QObject::connect(tabbar_, &TabBar::clicked, this, &TabWidget::onTabClicked);
    }

    TabWidget::~TabWidget()
    {
    }

    void TabWidget::hideTabbar()
    {
        tabbar_->hide();
    }

    void TabWidget::showTabbar()
    {
        tabbar_->show();
    }

    int TabWidget::addTab(QWidget* _widget, const QString& _name, const QString& _icon, const QString& _iconActive)
    {
        int index = pages_->indexOf(_widget);
        if (index == -1)
        {
            index = pages_->addWidget(_widget);
            auto tab = new TabItem(_icon, _iconActive, this);
            Testing::setAccessibleName(tab, ql1s("AS tab ") + _widget->accessibleName());
            tab->setName(_name);
            tabbar_->addItem(tab);
            if (index == 0)
            {
                pages_->setCurrentIndex(index);
                tabbar_->setCurrentIndex(index);
            }
        }
        else
        {
            qWarning("TabWidget: widget is already in tabWidget");
        }
        return index;
    }

    void TabWidget::setCurrentIndex(int _index)
    {
        if (tabbar_->getCurrentIndex() != _index)
        {
            setUpdatesEnabled(false);
            tabbar_->setCurrentIndex(_index);
            pages_->setCurrentIndex(_index);
            setUpdatesEnabled(true);
            emit currentChanged(_index, QPrivateSignal());
        }
    }

    int TabWidget::currentIndex() const noexcept
    {
        return tabbar_->getCurrentIndex();
    }

    TabBar* TabWidget::tabBar() const
    {
        return tabbar_;
    }

    void TabWidget::setBadgeText(int _index, const QString& _text)
    {
        tabbar_->setBadgeText(_index, _text);
    }

    void TabWidget::setBadgeIcon(int _index, const QString& _icon)
    {
        tabbar_->setBadgeIcon(_index, _icon);
    }

    void TabWidget::onTabClicked(int _index)
    {
        emit tabBarClicked(_index, QPrivateSignal());
        if (tabbar_->getCurrentIndex() != _index)
            setCurrentIndex(_index);
    }
}
