#include "stdafx.h"
#include "LiveChatsHome.h"

#include "LiveChatProfile.h"
#include "../contact_list/ContactListModel.h"
#include "../../core_dispatcher.h"

#include "../../utils/utils.h"
#include "../../utils/InterConnector.h"
#include "styles/ThemeParameters.h"
#include "../../controls/DialogButton.h"

namespace
{
    int spacing = 24;
    int profile_height = 500;
    int profile_width = 400;
}

namespace Ui
{
    LiveChatHomeWidget::LiveChatHomeWidget(QWidget* _parent, const Data::ChatInfo& _info)
        :   QWidget(_parent)
        ,   info_(_info)
        ,   joinButton_(new DialogButton(_parent, QString(), DialogButtonRole::CONFIRM))
    {
        auto layout = Utils::emptyVLayout(this);
        profile_ = new LiveChatProfileWidget(_parent, _info.Stamp_);
        profile_->setFixedSize(Utils::scale_value(profile_width), Utils::scale_value(profile_height));
        profile_->viewChat(std::make_shared<Data::ChatInfo>(_info));
        layout->addSpacerItem(new QSpacerItem(0, Utils::scale_value(spacing), QSizePolicy::Preferred, QSizePolicy::Fixed));
        layout->addWidget(profile_);
        layout->addSpacerItem(new QSpacerItem(0, Utils::scale_value(spacing), QSizePolicy::Preferred, QSizePolicy::Fixed));
        initButtonText();
        auto hLayout = new QHBoxLayout();
        hLayout->addSpacerItem(new QSpacerItem(0, 0, QSizePolicy::Expanding, QSizePolicy::Preferred));
        hLayout->addWidget(joinButton_);
        hLayout->addSpacerItem(new QSpacerItem(0, 0, QSizePolicy::Expanding, QSizePolicy::Preferred));
        layout->addLayout(hLayout);
        layout->addSpacerItem(new QSpacerItem(0, Utils::scale_value(spacing), QSizePolicy::Preferred, QSizePolicy::Fixed));
        setStyleSheet(qsl("background-color: %1; border-radius: 8px;").arg(Styling::getParameters().getColorHex(Styling::StyleVariable::BASE_GLOBALWHITE)));
        setFixedWidth(Utils::scale_value(profile_width));
        setFixedHeight(profile_->height() + Utils::scale_value(spacing) * 3 + joinButton_->height());
        Utils::addShadowToWidget(this);

        connect(joinButton_, &QPushButton::clicked, this, &LiveChatHomeWidget::joinButtonClicked);
        connect(Logic::getContactListModel(), &Logic::ContactListModel::liveChatJoined, this, &LiveChatHomeWidget::chatJoined);
        connect(Logic::getContactListModel(), &Logic::ContactListModel::liveChatRemoved, this, &LiveChatHomeWidget::chatRemoved);
    }

    LiveChatHomeWidget::~LiveChatHomeWidget()
    {
        delete profile_;
    }

    void LiveChatHomeWidget::chatJoined(const QString& _aimId)
    {
        if (info_.AimId_ == _aimId)
            initButtonText();
    }

    void LiveChatHomeWidget::chatRemoved(const QString& _aimId)
    {
        if (info_.AimId_ == _aimId)
            initButtonText();
    }

    void LiveChatHomeWidget::joinButtonClicked()
    {
        if (Logic::getContactListModel()->contains(info_.AimId_) && info_.YouMember_)
        {
            Logic::getContactListModel()->setCurrent(info_.AimId_, -1, true);
        }
        else
        {
            Logic::getContactListModel()->joinLiveChat(info_.Stamp_, true);
            info_.YouMember_ = true;
            if (info_.ApprovedJoin_)
            {
                joinButton_->setText(QT_TRANSLATE_NOOP("popup_window", "WAITING"));
                joinButton_->setEnabled(false);
            }
        }
    }

    void LiveChatHomeWidget::initButtonText()
    {
        if (Logic::getContactListModel()->contains(info_.AimId_))
        {
            joinButton_->setText(QT_TRANSLATE_NOOP("popup_window", "OPEN"));
            joinButton_->setEnabled(true);
        }
        else
        {
            if (info_.YouPending_)
            {
                joinButton_->setText(QT_TRANSLATE_NOOP("popup_window", "WAITING"));
            }
            else
            {
                joinButton_->setText(QT_TRANSLATE_NOOP("popup_window", "JOIN"));
            }

            joinButton_->setEnabled(!info_.YouPending_);
        }

        joinButton_->adjustSize();
    }

    void LiveChatHomeWidget::paintEvent(QPaintEvent* _e)
    {
        QStyleOption opt;
        opt.init(this);
        QPainter p(this);
        style()->drawPrimitive(QStyle::PE_Widget, &opt, &p, this);

        return QWidget::paintEvent(_e);
    }

    LiveChatHome::LiveChatHome(QWidget* _parent)
        :   QWidget(_parent)
        ,   layout_(Utils::emptyVLayout(this))
    {
        layout_->setAlignment(Qt::AlignCenter);
        setStyleSheet(qsl("background: transparent;"));

        connect(&Utils::InterConnector::instance(), &Utils::InterConnector::showLiveChat, this, &LiveChatHome::onLiveChatSelected);
    }


    LiveChatHome::~LiveChatHome()
    {
    }

    void LiveChatHome::liveChatSelected(const Data::ChatInfo& _info)
    {
        auto item = layout_->takeAt(0);
        if (item)
        {
            item->widget()->hide();
            item->widget()->deleteLater();
        }

        emit Utils::InterConnector::instance().closeAnySemitransparentWindow(Utils::CloseWindowInfo());

        layout_->addWidget(new LiveChatHomeWidget(this, _info));
    }

    void LiveChatHome::onLiveChatSelected(const std::shared_ptr<Data::ChatInfo>& _info)
    {
        liveChatSelected(*_info);
    }

    void LiveChatHome::paintEvent(QPaintEvent* _e)
    {
        QStyleOption opt;
        opt.init(this);
        QPainter p(this);
        style()->drawPrimitive(QStyle::PE_Widget, &opt, &p, this);

        return QWidget::paintEvent(_e);
    }
}
