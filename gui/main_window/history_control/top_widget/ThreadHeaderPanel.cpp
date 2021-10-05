#include "stdafx.h"

#include "ThreadHeaderPanel.h"
#include "utils/utils.h"
#include "utils/InterConnector.h"
#include "styles/ThemeParameters.h"
#include "main_window/contact_list/TopPanel.h"
#include "app_config.h"

namespace
{
    constexpr auto getTopIconsSize() noexcept
    {
        return QSize(24, 24);
    }

    constexpr bool threadInfoEnabled = false;
}

namespace Ui
{
    ThreadHeaderPanel::ThreadHeaderPanel(const QString& _aimId, QWidget* _parent)
        : QWidget(_parent)
        , aimId_(_aimId)
    {
        auto titleBar = new HeaderTitleBar(this);
        titleBar->setStyleSheet(ql1s("background-color: %1; border-bottom: %2 solid %3;").arg(Styling::getParameters().getColorHex(Styling::StyleVariable::BASE_GLOBALWHITE),
            QString::number(Utils::scale_value(1)) % u"px",
            Styling::getParameters().getColorHex(Styling::StyleVariable::BASE_BRIGHT)));

        auto title = QT_TRANSLATE_NOOP("sidebar", "Replies");
        if (Q_UNLIKELY(Ui::GetAppConfig().IsShowMsgIdsEnabled()))
            title += u" " % aimId_;
        titleBar->setTitle(title);

        const auto headerIconSize = getTopIconsSize();
        titleBar->setButtonSize(Utils::scale_value(headerIconSize));

        if (threadInfoEnabled)
        {
            auto infoButton = new HeaderTitleBarButton(this);
            Testing::setAccessibleName(infoButton, qsl("AS Sidebar Thread infoButton"));
            infoButton->setDefaultImage(qsl(":/about_icon_small"), Styling::getParameters().getColor(Styling::StyleVariable::BASE_SECONDARY), headerIconSize);
            infoButton->setCustomToolTip(QT_TRANSLATE_NOOP("tooltips", "Information"));
            titleBar->addButtonToRight(infoButton);
            connect(infoButton, &HeaderTitleBarButton::clicked, this, &ThreadHeaderPanel::onInfoClicked);
        }

        {
            closeButton_ = new HeaderTitleBarButton(this);
            Testing::setAccessibleName(closeButton_, qsl("AS Sidebar Thread closeButton"));
            titleBar->addButtonToLeft(closeButton_);
            connect(closeButton_, &HeaderTitleBarButton::clicked, this, &ThreadHeaderPanel::onCloseClicked);
        }

        auto layout = Utils::emptyHLayout(this);
        layout->addWidget(titleBar);

        updateCloseIcon(FrameCountMode::_3);
    }

    void ThreadHeaderPanel::updateCloseIcon(FrameCountMode _mode)
    {
        const auto headerIconSize = getTopIconsSize();
        QString iconPath;
        QString tooltipText;
        if (_mode == FrameCountMode::_1)
        {
            iconPath = qsl(":/controls/back_icon");
            tooltipText = QT_TRANSLATE_NOOP("sidebar", "Back");
        }
        else
        {
            iconPath = qsl(":/controls/close_icon");
            tooltipText = QT_TRANSLATE_NOOP("sidebar", "Close");
        }

        closeButton_->setDefaultImage(iconPath, QColor(), headerIconSize);
        closeButton_->setCustomToolTip(tooltipText);
        Styling::Buttons::setButtonDefaultColors(closeButton_);
    }

    void ThreadHeaderPanel::onInfoClicked()
    {
        Utils::InterConnector::instance().showSidebar(aimId_);
    }

    void ThreadHeaderPanel::onCloseClicked()
    {
        Utils::InterConnector::instance().setSidebarVisible(false);
    }
}