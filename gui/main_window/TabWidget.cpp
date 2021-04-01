#include "stdafx.h"

#include "TabWidget.h"
#include "TabBar.h"

#include "../utils/utils.h"
#include "../utils/features.h"
#include "../utils/animations/SlideController.h"
#include "styles/ThemeParameters.h"

namespace
{
    constexpr std::chrono::milliseconds kFadeDuration = std::chrono::milliseconds(85);
}

namespace Ui
{
    TabWidget::TabWidget(QWidget* _parent)
        : QWidget(_parent)
    {
        using Utils::SlideController;

        setContentsMargins(0, 0, 0, 0);

        Utils::setDefaultBackground(this);

        auto layout = Utils::emptyVLayout(this);
        tabbar_ = new TabBar();

        pages_ = new QStackedWidget();
        pages_->setStyleSheet(ql1s("background: transparent; border: none; color: %1").arg(Styling::getParameters().getColorHex(Styling::StyleVariable::BASE_PRIMARY)));

        SlideController* slideController = new SlideController(pages_);
        slideController->setWidget(pages_);
        slideController->setDuration(kFadeDuration);
        slideController->setFading(SlideController::Fading::FadeInOut);
        slideController->setEffects(SlideController::SlideEffect::NoEffect);
        slideController->setSlideDirection(SlideController::SlideDirection::NoSliding);
        slideController->setInverse(true);

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
        if (tabbar_->getCurrentIndex() != _index)
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

    void TabWidget::insertAdditionalWidget(QWidget * _w)
    {
        if (auto l = qobject_cast<QBoxLayout*>(layout()))
            l->insertWidget(l->count() - 1, _w);
    }

    void TabWidget::onTabClicked(int _index)
    {
        Q_EMIT tabBarClicked(_index, QPrivateSignal());
        if (tabbar_->getCurrentIndex() != _index)
            setCurrentIndex(_index);
    }
}
