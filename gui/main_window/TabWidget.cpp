#include "stdafx.h"

#include "TabWidget.h"
#include "TabBar.h"

#include "../utils/utils.h"
#include "../utils/features.h"
#include "../utils/animations/SlideController.h"
#include "styles/ThemeParameters.h"
#include "Common.h"

namespace Ui
{
    TabWidget::TabWidget(QWidget* _parent)
        : QWidget(_parent)
    {
        setContentsMargins(0, 0, 0, 0);

        auto layout = Utils::emptyVLayout(this);
        tabbar_ = new TabBar(this);

        pages_ = new QStackedWidget(this);
        pages_->setStyleSheet(ql1s("background-color: transparent; border: none;"));

        Testing::setAccessibleName(pages_, qsl("AS Tab page"));
        layout->addWidget(pages_);

        Testing::setAccessibleName(tabbar_, qsl("AS Tab bar"));
        layout->addWidget(tabbar_);

        QObject::connect(tabbar_, &TabBar::clicked, this, &TabWidget::onTabClicked);
    }

    TabWidget::~TabWidget() = default;

    void TabWidget::hideTabbar()
    {
        tabbar_->hide();
    }

    void TabWidget::showTabbar()
    {
        tabbar_->show();
    }

    std::pair<int, QWidget*> TabWidget::addTab(QWidget* _widget, const QString& _name, const QString& _icon, const QString& _iconActive)
    {
        QWidget* icon = nullptr;
        if (!pages_)
            return std::make_pair(-1, nullptr);

        int index = pages_->indexOf(_widget);
        if (index == -1)
        {
            index = pages_->addWidget(_widget);
            auto tab = new TabItem(_icon, _iconActive, this);
            Testing::setAccessibleName(tab, u"AS Tab " % _widget->accessibleName());
            tab->setName(_name);
            tabbar_->addItem(tab);
            icon = tab;
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

        return std::make_pair(index, icon);
    }

    void TabWidget::setCurrentIndex(int _index)
    {
        if (tabbar_ && tabbar_->getCurrentIndex() != _index)
        {
            setUpdatesEnabled(false);
            tabbar_->setCurrentIndex(_index);
            setUpdatesEnabled(true);

            pages_->setCurrentIndex(_index);

            Q_EMIT currentChanged(_index, QPrivateSignal());
        }
    }

    int TabWidget::currentIndex() const noexcept
    {
        if (!tabbar_)
            return -1;
        return tabbar_->getCurrentIndex();
    }

    TabBar* TabWidget::tabBar() const
    {
        return tabbar_;
    }

    void TabWidget::setBadgeText(int _index, const QString& _text)
    {
        if (tabbar_)
            tabbar_->setBadgeText(_index, _text);
    }

    void TabWidget::setBadgeIcon(int _index, const QString& _icon)
    {
        if (tabbar_)
            tabbar_->setBadgeIcon(_index, _icon);
    }

    void TabWidget::updateItemInfo(int _index, const QString& _badgeText, const QString& _badgeIcon)
    {
        setBadgeText(_index, _badgeText);
        setBadgeIcon(_index, _badgeIcon);
    }

    void TabWidget::insertAdditionalWidget(QWidget * _w)
    {
        if (auto l = qobject_cast<QBoxLayout*>(layout()))
            l->insertWidget(l->count() - 1, _w);
    }

    void TabWidget::onTabClicked(int _index)
    {
        if (!tabbar_)
            return;

        Q_EMIT tabBarClicked(_index, QPrivateSignal());
        setCurrentIndex(_index);
    }
}
