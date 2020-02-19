#include "stdafx.h"
#include "LiveChatProfile.h"

#include "LiveChatMembersControl.h"
#include "../contact_list/ContactListModel.h"
#include "../contact_list/ChatMembersModel.h"
#include "../../core_dispatcher.h"

#include "../../controls/ContactAvatarWidget.h"
#include "../../controls/LabelEx.h"
#include "../../controls/GeneralDialog.h"
#include "../../controls/PictureWidget.h"
#include "../../controls/TextEditEx.h"
#include "../../controls/DialogButton.h"

#include "../../fonts.h"
#include "../../utils/InterConnector.h"
#include "../../utils/gui_coll_helper.h"
#include "../../utils/utils.h"
#include "../../utils/features.h"
#include "styles/ThemeParameters.h"
#include "main_window/MainWindow.h"

namespace
{
    void switchToContact(const QString& _contact)
    {
        const auto selectedContact = Logic::getContactListModel()->selectedContact();
        if (selectedContact != _contact)
            emit Utils::InterConnector::instance().addPageToDialogHistory(selectedContact);

        Logic::getContactListModel()->setCurrent(_contact, -1, true);
    }
}

namespace Ui
{
    constexpr int membersLimit = 5;
    constexpr int maxNameTextSize = 40;
    constexpr int maxAboutTextSize = 200;
    constexpr int horMargins = 48;
    constexpr int textMargin = 4;

    LiveChats::LiveChats(QObject* _parent)
        : QObject(_parent)
        , connected_(false)
        , activeDialog_(nullptr)
        , seq_(-1)
    {
        connect(&Utils::InterConnector::instance(), &Utils::InterConnector::needJoinLiveChatByStamp, this, &LiveChats::needJoinLiveChatStamp);
        connect(&Utils::InterConnector::instance(), &Utils::InterConnector::needJoinLiveChatByAimId, this, &LiveChats::needJoinLiveChatAimId);
    }

    LiveChats::~LiveChats()
    {

    }

    void LiveChats::needJoinLiveChatStamp(const QString& _stamp)
    {
        onNeedJoin(_stamp, QString());
    }

    void LiveChats::needJoinLiveChatAimId(const QString& _aimId)
    {
        onNeedJoin(QString(), _aimId);
    }

    void LiveChats::onNeedJoin(const QString& _stamp, const QString& _aimId)
    {
        if (_stamp.isEmpty() && _aimId.isEmpty())
            return;

        if (!connected_)
        {
            connect(Ui::GetDispatcher(), &Ui::core_dispatcher::chatInfo, this, &LiveChats::onChatInfo, Qt::UniqueConnection);
            connect(Ui::GetDispatcher(), &Ui::core_dispatcher::chatInfoFailed, this, &LiveChats::onChatInfoFailed, Qt::UniqueConnection);
            connect(Logic::getContactListModel(), &Logic::ContactListModel::liveChatJoined, this, &LiveChats::liveChatJoined);
        }

        connected_ = true;

        Ui::gui_coll_helper collection(Ui::GetDispatcher()->create_collection(), true);

        if (!_stamp.isEmpty())
            collection.set_value_as_qstring("stamp", _stamp);
        else if (!_aimId.isEmpty())
            collection.set_value_as_qstring("aimid", _aimId);

        collection.set_value_as_int("limit", Logic::ChatMembersModel::get_limit(membersLimit));

        seq_ = Ui::GetDispatcher()->post_message_to_core("chats/info/get", collection.get());

        if (activeDialog_)
        {
            activeDialog_->reject();
            return;
        }
    }

    void LiveChats::liveChatJoined(const QString& _aimId)
    {
        if (joinedLiveChat_ == _aimId)
        {
            switchToContact(_aimId);
            joinedLiveChat_.clear();
        }
    }

    void LiveChats::onChatInfo(qint64 _seq, const std::shared_ptr<Data::ChatInfo>& _info, const int _requestMembersLimit)
    {
        if (_seq != seq_)
            return;

        joinedLiveChat_.clear();

        const auto canSwitchNotInCL = _info->Public_ && !_info->ApprovedJoin_;
        if ((canSwitchNotInCL && !Features::forceShowChatPopup()) || Logic::getContactListModel()->contains(_info->AimId_))
        {
            switchToContact(_info->AimId_);
        }
        else
        {
            auto liveChatProfileWidget = new LiveChatProfileWidget(nullptr, _info->Name_);
            liveChatProfileWidget->setFixedWidth(Utils::scale_value(360));
            liveChatProfileWidget->viewChat(_info);

            GeneralDialog containerDialog(liveChatProfileWidget, Utils::InterConnector::instance().getMainWindow());
            activeDialog_ = &containerDialog;

            bool pending = _info->YouPending_;
            auto pendingText = QT_TRANSLATE_NOOP("popup_window", "Waiting");
            containerDialog.addButtonsPair(
                pending ? QT_TRANSLATE_NOOP("popup_window", "Close") : QT_TRANSLATE_NOOP("popup_window", "Cancel"),
                pending ? pendingText : QT_TRANSLATE_NOOP("popup_window", "Join"),
                !pending);

            QMetaObject::Connection joinedCon;
            if (_info->ApprovedJoin_)
            {
                auto button = containerDialog.takeAcceptButton();
                auto clickCon = std::make_shared<QMetaObject::Connection>();
                *clickCon = connect(button, &DialogButton::clicked, button, [button, pendingText, stamp = _info->Stamp_]()
                {
                    Logic::getContactListModel()->joinLiveChat(stamp, true);
                    button->setText(pendingText);
                    button->setEnabled(false);
                });
                joinedCon = connect(Logic::getContactListModel(), &Logic::ContactListModel::liveChatJoined, this, [id = _info->AimId_, button, this, clickCon](const QString& aimId)
                {
                    if (id == aimId)
                    {
                        button->setText(QT_TRANSLATE_NOOP("popup_window", "Open"));
                        button->setEnabled(true);

                        disconnect(*clickCon);

                        connect(button, &QPushButton::clicked, this, [id, this]()
                        {
                            Logic::getContactListModel()->setCurrent(id, -1, true);
                            activeDialog_->reject();
                        });
                    }
                });
            }

            if (containerDialog.showInCenter())
            {
                Logic::getContactListModel()->joinLiveChat(_info->Stamp_, true);
                Ui::GetDispatcher()->post_stats_to_core(core::stats::stats_event_names::chats_join_frompopup);
                if (!_info->ApprovedJoin_)
                    joinedLiveChat_ = _info->AimId_;
            }

            if (joinedCon)
                disconnect(joinedCon);

            GetDispatcher()->post_stats_to_core(core::stats::stats_event_names::chats_open_popup);
            activeDialog_ = nullptr;
        }
    }

    void LiveChats::onChatInfoFailed(qint64 _seq, core::group_chat_info_errors _error, const QString& _aimId)
    {
        if (_seq != seq_)
            return;

        joinedLiveChat_.clear();

        QString errorText;

        switch (_error)
        {
        case core::group_chat_info_errors::network_error:
            errorText = QT_TRANSLATE_NOOP("groupchats", "Chat information is unavailable now, please try again later");
            break;
        default:
            errorText = QT_TRANSLATE_NOOP("groupchats", "Chat does not exist or it is hidden by privacy settings");
            break;
        }

        auto errorWidget = new LiveChatErrorWidget(nullptr, errorText);
        errorWidget->setFixedWidth(Utils::scale_value(360));
        errorWidget->show();

        GeneralDialog containerDialog(errorWidget, Utils::InterConnector::instance().getMainWindow());
        activeDialog_ = &containerDialog;

        containerDialog.addCancelButton(QT_TRANSLATE_NOOP("popup_window", "Close"), true);
        containerDialog.showInCenter();

        GetDispatcher()->post_stats_to_core(core::stats::stats_event_names::chats_open_popup);
        activeDialog_ = nullptr;
    }

    LiveChatProfileWidget::LiveChatProfileWidget(QWidget* _parent, const QString& _stamp)
        :   QWidget(_parent),
            stamp_(_stamp),
            rootLayout_(nullptr),
            avatar_(nullptr),
            members_(nullptr),
            membersCount_(-1),
            initialiNameHeight_(0)
    {
        setStyleSheet(Utils::LoadStyle(qsl(":/qss/livechats")));

        rootLayout_ = Utils::emptyVLayout();
        rootLayout_->setContentsMargins(Utils::scale_value(horMargins), 0, Utils::scale_value(horMargins), 0);
        rootLayout_->setAlignment(Qt::AlignTop | Qt::AlignHCenter);
        rootLayout_->addSpacing(Utils::scale_value(horMargins));

        requestProfile();

        setLayout(rootLayout_);
    }

    LiveChatProfileWidget::~LiveChatProfileWidget()
    {
    }

    void LiveChatProfileWidget::paintEvent(QPaintEvent* _e)
    {
        QStyleOption opt;
        opt.init(this);
        QPainter p(this);
        style()->drawPrimitive(QStyle::PE_Widget, &opt, &p, this);

        return QWidget::paintEvent(_e);
    }

    void LiveChatProfileWidget::requestProfile()
    {

    }

    void LiveChatProfileWidget::viewChat(std::shared_ptr<Data::ChatInfo> _info)
    {
        int currentTextWidth = width() - Utils::scale_value(horMargins) * 2 - Utils::scale_value(textMargin) * 2;

        int needHeight = Utils::scale_value(horMargins);

        membersCount_ = _info->MembersCount_;

        QHBoxLayout* avatar_layout = new QHBoxLayout();

        avatar_ = new ContactAvatarWidget(this, _info->AimId_, _info->Name_, Utils::scale_value(180), true);
        avatar_layout->addWidget(avatar_);

        needHeight += avatar_->height();

        rootLayout_->addLayout(avatar_layout);

        QString nameText = ((_info->Name_.length() > maxNameTextSize) ? (_info->Name_.midRef(0, maxNameTextSize) % getEllipsis()) : _info->Name_);

        name_ = new TextEditEx(this, Fonts::appFontScaled(24), Styling::getParameters().getColor(Styling::StyleVariable::TEXT_SOLID), false, true);
        connect(name_, &TextEditEx::setSize, this, &LiveChatProfileWidget::nameResized, Qt::QueuedConnection);
        name_->setPlainText(nameText, false, QTextCharFormat::AlignNormal);
        name_->setAlignment(Qt::AlignHCenter);
        name_->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        name_->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        name_->setFrameStyle(QFrame::NoFrame);
        name_->setContentsMargins(0, 0, 0, 0);
        name_->setContextMenuPolicy(Qt::NoContextMenu);
        name_->adjustHeight(currentTextWidth);
        Utils::ApplyStyle(name_, Styling::getParameters().getTextEditCommonQss(false));
        initialiNameHeight_ = name_->height();
        needHeight += initialiNameHeight_;
        rootLayout_->addWidget(name_);

        QString aboutText = ((_info->About_.length() > maxAboutTextSize) ? (_info->About_.midRef(0, maxAboutTextSize) % getEllipsis()) : _info->About_);

        TextEditEx* about = new TextEditEx(this, Fonts::appFontScaled(16), Styling::getParameters().getColor(Styling::StyleVariable::BASE_PRIMARY), false, false);
        about->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        about->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        about->setFrameStyle(QFrame::NoFrame);
        about->setContextMenuPolicy(Qt::NoContextMenu);
        Utils::ApplyStyle(about, Styling::getParameters().getTextEditCommonQss(false));

        int height = 0;
        for (int i = 0; i < maxAboutTextSize; ++i)
        {
            about->setPlainText(aboutText, false);
            about->setAlignment(Qt::AlignHCenter);
            height = about->adjustHeight(currentTextWidth);

            if (height < Utils::scale_value(70))
                break;

            int newLength = int(((float) aboutText.length()) * 0.9);
            if (newLength <= 0)
            {
                assert(false);
                break;
            }

            aboutText = aboutText.midRef(0, newLength) % getEllipsis();
        }

        needHeight += height;
        rootLayout_->addWidget(about);

        QString location_string;

        if (!_info->Location_.isEmpty())
        {
            location_string = _info->Location_;
        }

        if (_info->FriendsCount)
        {
            if (!location_string.isEmpty())
                location_string += ql1s(" - ");

            location_string += QString::number(_info->FriendsCount) % ql1c(' ') %
                Utils::GetTranslator()->getNumberString(
                _info->FriendsCount,
                QT_TRANSLATE_NOOP3("groupchats", "friend", "1"),
                QT_TRANSLATE_NOOP3("groupchats", "friends", "2"),
                QT_TRANSLATE_NOOP3("groupchats", "friends", "5"),
                QT_TRANSLATE_NOOP3("groupchats", "friends", "21")
                );
        }

        if (!location_string.isEmpty())
        {
            TextEditEx* location = new TextEditEx(this, Fonts::appFontScaled(16), Styling::getParameters().getColor(Styling::StyleVariable::TEXT_SOLID), false, false);
            location->setPlainText(location_string, false);
            location->setAlignment(Qt::AlignHCenter);
            location->setFrameStyle(QFrame::NoFrame);
            height = location->adjustHeight(currentTextWidth);
            needHeight += height;
            rootLayout_->addWidget(location);
        }

        rootLayout_->addSpacing(Utils::scale_value(16));
        needHeight += Utils::scale_value(16);

        QHBoxLayout* avatarsLayout = new QHBoxLayout();
        members_ = new LiveChatMembersControl(this, _info);
        avatarsLayout->addWidget(members_);
        members_->adjustWidth();
        avatarsLayout->setAlignment(Qt::AlignHCenter);

        avatarsLayout->addSpacing(Utils::scale_value(6));

        int realAvatarsCount = members_->getRealCount();

        LabelEx* count = new LabelEx(this);
        count->setText(ql1c('+') + QString::number(membersCount_ - realAvatarsCount));
        count->setColor(Styling::getParameters().getColor(Styling::StyleVariable::BASE_PRIMARY));
        count->setFont(Fonts::appFontScaled(12));
        avatarsLayout->addWidget(count);

        if (realAvatarsCount >= membersCount_)
            count->hide();

        needHeight += members_->height();

        rootLayout_->addLayout(avatarsLayout);

        setFixedHeight(needHeight);
    }

    void LiveChatProfileWidget::nameResized(int, int)
    {
        auto needHeight = height();
        needHeight -= initialiNameHeight_;
        needHeight += name_->height();
        setFixedHeight(needHeight);
    }


    void LiveChatProfileWidget::setStamp(const QString& _stamp)
    {
        stamp_ = _stamp;
    }



    LiveChatErrorWidget::LiveChatErrorWidget(QWidget* _parent, const QString& _errorText)
        :   QWidget(_parent),
            errorText_(_errorText)
    {

    }

    LiveChatErrorWidget::~LiveChatErrorWidget()
    {
    }

    void LiveChatErrorWidget::show()
    {
        int height = 0;
        int currentWidth = width() - Utils::scale_value(horMargins) * 2;

        setStyleSheet(Utils::LoadStyle(qsl(":/qss/livechats")));

        QVBoxLayout* rootLayout = Utils::emptyVLayout();
        rootLayout->setContentsMargins(Utils::scale_value(horMargins), 0, Utils::scale_value(horMargins), 0);
        rootLayout->setAlignment(Qt::AlignTop | Qt::AlignHCenter);

        rootLayout->addSpacing(Utils::scale_value(horMargins));
        height += Utils::scale_value(horMargins);

        QHBoxLayout* pictureLayout = Utils::emptyHLayout();
        PictureWidget* picture = new PictureWidget(this, qsl(":/placeholders/empty_livechat_100"));
        picture->setFixedSize(Utils::scale_value(136), Utils::scale_value(216));
        pictureLayout->addWidget(picture);

        height += picture->height();

        rootLayout->addLayout(pictureLayout);

        rootLayout->addSpacing(Utils::scale_value(26));
        height += Utils::scale_value(26);

        TextEditEx* errorText = new TextEditEx(this, Fonts::appFontScaled(16), Styling::getParameters().getColor(Styling::StyleVariable::BASE_PRIMARY), false, false);
        errorText->setText(errorText_);
        errorText->setStyleSheet(qsl("* { background: transparent; border: none; }"));
        errorText->setAlignment(Qt::AlignHCenter);
        height += errorText->adjustHeight(currentWidth);
        rootLayout->addWidget(errorText);

        setLayout(rootLayout);

        setFixedHeight(height);
    }

    void LiveChatErrorWidget::paintEvent(QPaintEvent* _e)
    {
        QStyleOption opt;
        opt.init(this);
        QPainter p(this);
        style()->drawPrimitive(QStyle::PE_Widget, &opt, &p, this);

        return QWidget::paintEvent(_e);
    }
}
