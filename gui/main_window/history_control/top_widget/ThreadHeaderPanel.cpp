#include "stdafx.h"

#include "ThreadHeaderPanel.h"
#include "utils/utils.h"
#include "utils/InterConnector.h"
#include "styles/ThemeParameters.h"
#include "styles/StyleSheetGenerator.h"
#include "styles/StyleSheetContainer.h"
#include "main_window/contact_list/TopPanel.h"
#include "app_config.h"
#include "main_window/contact_list/ContactListModel.h"
#include "main_window/containers/FriendlyContainer.h"

namespace
{
    constexpr auto getTopIconsSize() noexcept
    {
        return QSize(24, 24);
    }

    std::unique_ptr<Styling::ArrayStyleSheetGenerator> getHeaderQss()
    {
        QString qss = ql1s(
            "background-color: %2; border-bottom: %1 solid %3;"
        ).arg(QString::number(Utils::scale_value(1)) % u"px");

        return std::make_unique<Styling::ArrayStyleSheetGenerator>(
            qss,
            std::vector<Styling::ThemeColorKey> { Styling::ThemeColorKey { Styling::StyleVariable::BASE_GLOBALWHITE },
                                                  Styling::ThemeColorKey { Styling::StyleVariable::BASE_BRIGHT } });
    }
}

namespace Ui
{
    ThreadHeaderPanel::ThreadHeaderPanel(const QString& _aimId, QWidget* _parent)
        : QWidget(_parent)
        , aimId_(_aimId)
        , titleBar_ { new HeaderTitleBar(this) }
    {
        Styling::setStyleSheet(titleBar_, getHeaderQss(), Styling::StyleSheetPolicy::UseSetStyleSheet);

        auto title = QT_TRANSLATE_NOOP("sidebar", "Replies");
        if (Q_UNLIKELY(Ui::GetAppConfig().IsShowMsgIdsEnabled()))
            title += u" " % aimId_;
        titleBar_->setTitle(title);
        Testing::setAccessibleName(titleBar_, qsl("AS ThreadHeaderPanel ") % aimId_);

        const auto parentId = Logic::getContactListModel()->getThreadParent(_aimId);
        titleBar_->setSubTitle(!parentId || parentId->isEmpty() ? QT_TRANSLATE_NOOP("sidebar", "Task") : Logic::GetFriendlyContainer()->getFriendly(*parentId));
        connect(Logic::GetFriendlyContainer(), &Logic::FriendlyContainer::friendlyChanged, this, &ThreadHeaderPanel::updateParentChatName);

        const auto headerIconSize = getTopIconsSize();
        titleBar_->setButtonSize(Utils::scale_value(headerIconSize));

        {
            auto infoButton = new HeaderTitleBarButton(this);
            Testing::setAccessibleName(infoButton, qsl("AS Sidebar Thread infoButton"));
            infoButton->setDefaultImage(qsl(":/about_icon_small"), Styling::ThemeColorKey{ Styling::StyleVariable::BASE_SECONDARY }, headerIconSize);
            infoButton->setCustomToolTip(QT_TRANSLATE_NOOP("tooltips", "Information"));
            titleBar_->addButtonToRight(infoButton);
            connect(infoButton, &HeaderTitleBarButton::clicked, this, &ThreadHeaderPanel::onInfoClicked);
        }

        {
            searchButton_ = new HeaderTitleBarButton(this);
            searchButton_->setDefaultImage(qsl(":/search_icon"), Styling::ThemeColorKey{ Styling::StyleVariable::BASE_SECONDARY }, headerIconSize);
            searchButton_->setCustomToolTip(QT_TRANSLATE_NOOP("tooltips", "Search for messages"));
            Testing::setAccessibleName(searchButton_, qsl("AS Sidebar Thread searchButton"));
            titleBar_->addButtonToRight(searchButton_);
            connect(searchButton_, &HeaderTitleBarButton::clicked, this, &ThreadHeaderPanel::onSearchClicked);
            connect(&Utils::InterConnector::instance(), &Utils::InterConnector::startSearchInThread, this, &ThreadHeaderPanel::activateSearchButton);
            connect(&Utils::InterConnector::instance(), &Utils::InterConnector::threadSearchClosed, this, &ThreadHeaderPanel::deactivateSearchButton);
            connect(&Utils::InterConnector::instance(), &Utils::InterConnector::threadSearchButtonDeactivate, this, &ThreadHeaderPanel::deactivateSearchButton);
        }

        {
            closeButton_ = new HeaderTitleBarButton(this);
            Testing::setAccessibleName(closeButton_, qsl("AS Sidebar Thread closeButton"));
            titleBar_->addButtonToLeft(closeButton_);
            titleBar_->setTitleLeftOffset(closeButton_->width());
            connect(closeButton_, &HeaderTitleBarButton::clicked, this, &ThreadHeaderPanel::onCloseClicked);
        }

        auto layout = Utils::emptyHLayout(this);
        layout->addWidget(titleBar_);

        updateCloseIcon(FrameCountMode::_3);
    }

    void ThreadHeaderPanel::updateCloseIcon(FrameCountMode _mode)
    {
        mode_ = _mode;
        const auto headerIconSize = getTopIconsSize();
        QString iconPath;
        QString tooltipText;
        if (_mode == FrameCountMode::_1 || searchButton_->isActive())
        {
            iconPath = qsl(":/controls/back_icon");
            tooltipText = QT_TRANSLATE_NOOP("sidebar", "Back");
        }
        else
        {
            iconPath = qsl(":/controls/close_icon");
            tooltipText = QT_TRANSLATE_NOOP("sidebar", "Close");
        }

        closeButton_->setDefaultImage(iconPath, Styling::ColorParameter{}, headerIconSize);
        closeButton_->setCustomToolTip(tooltipText);
        Styling::Buttons::setButtonDefaultColors(closeButton_);
    }

    bool ThreadHeaderPanel::isSearchIsActive() const
    {
        return searchButton_ && searchButton_->isActive();
    }

    void ThreadHeaderPanel::onInfoClicked()
    {
        Utils::InterConnector::instance().showSidebar(aimId_);
    }

    void ThreadHeaderPanel::onCloseClicked()
    {
        Q_EMIT Utils::InterConnector::instance().threadCloseClicked(searchButton_->isActive());
    }

    void ThreadHeaderPanel::onSearchClicked()
    {
        if (searchButton_->isActive())
            Q_EMIT Utils::InterConnector::instance().threadSearchClosed();
        else
            Q_EMIT Utils::InterConnector::instance().startSearchInThread(aimId_);
    }

    void ThreadHeaderPanel::activateSearchButton(const QString& _searchedDialog)
    {
        if (aimId_ == _searchedDialog)
            searchButton_->setActive(true);
    }

    void ThreadHeaderPanel::deactivateSearchButton()
    {
        searchButton_->setActive(false);
        updateCloseIcon(mode_);
    }

    void ThreadHeaderPanel::updateParentChatName(const QString& _aimId, const QString& _friendly)
    {
        if (const auto parentId = Logic::getContactListModel()->getThreadParent(_aimId); parentId && *parentId == _aimId)
            titleBar_->setSubTitle(_friendly);
    }
}