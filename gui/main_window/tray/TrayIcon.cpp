#include "stdafx.h"
#include "TrayIcon.h"

#include "RecentMessagesAlert.h"
#include "../MainWindow.h"
#include "../MainPage.h"
#include "../contact_list/ContactListModel.h"
#include "../friendly/FriendlyContainer.h"
#include "../contact_list/RecentItemDelegate.h"
#include "../contact_list/RecentsModel.h"
#include "../contact_list/UnknownsModel.h"
#include "../sounds/SoundsManager.h"
#include "../../core_dispatcher.h"
#include "../../gui_settings.h"
#include "../../my_info.h"
#include "../../controls/ContextMenu.h"
#include "../../utils/gui_coll_helper.h"
#include "../../utils/InterConnector.h"
#include "../../utils/utils.h"
#include "../../utils/log/log.h"
#include "../history_control/history/MessageBuilder.h"
#include "utils/gui_metrics.h"
#include "main_window/LocalPIN.h"
#include "styles/ThemeParameters.h"

#if defined(_WIN32)
    //#   include "toast_notifications/win32/ToastManager.h"
    typedef HRESULT (__stdcall *QueryUserNotificationState)(QUERY_USER_NOTIFICATION_STATE *pquns);
    typedef BOOL (__stdcall *QuerySystemParametersInfo)(__in UINT uiAction, __in UINT uiParam, __inout_opt PVOID pvParam, __in UINT fWinIni);
#elif defined(__APPLE__)
    #include "notification_center/macos/NotificationCenterManager.h"
    #include "../../utils/macos/mac_support.h"
#endif

#ifdef _WIN32
    #include <Shobjidl.h>
    extern HICON qt_pixmapToWinHICON(const QPixmap &p);
    const QString pinPath = qsl("\\Microsoft\\Internet Explorer\\Quick Launch\\User Pinned\\TaskBar\\ICQ.lnk");
#endif // _WIN32

namespace
{
    constexpr std::chrono::milliseconds init_mail_timeout = std::chrono::seconds(5);

#ifdef _WIN32
    HICON createHIconFromQIcon(const QIcon &icon, int xSize, int ySize) {
        if (!icon.isNull()) {
            const QPixmap pm = icon.pixmap(icon.actualSize(QSize(xSize, ySize)));
            if (!pm.isNull()) {
                return qt_pixmapToWinHICON(pm);
            }
        }
        return nullptr;
    }
#endif //_WIN32
}

namespace Ui
{
    TrayIcon::TrayIcon(MainWindow* parent)
        : QObject(parent)
        , systemTrayIcon_(new QSystemTrayIcon(this))
        , emailSystemTrayIcon_(nullptr)
        , Menu_(nullptr)
        , MessageAlert_(new RecentMessagesAlert(AlertType::alertTypeMessage))
        , MentionAlert_(new RecentMessagesAlert(AlertType::alertTypeMentionMe))
        , MailAlert_(new  RecentMessagesAlert(AlertType::alertTypeEmail))
        , MainWindow_(parent)
        , first_start_(true)
        , Base_(build::GetProductVariant(qsl(":/logo/ico_icq"), qsl(":/logo/ico_agent"), qsl(":/logo/ico_biz"), qsl(":/logo/ico_dit")))
        , Unreads_(build::GetProductVariant(qsl(":/logo/ico_icq_unread"), qsl(":/logo/ico_agent_unread"), qsl(":/logo/ico_biz_unread"), qsl(":/logo/ico_dit_unread")))
#ifdef _WIN32
        , TrayBase_(build::GetProductVariant(qsl(":/logo/ico_icq_tray"), qsl(":/logo/ico_agent_tray"), qsl(":/logo/ico_biz_tray"), qsl(":/logo/ico_dit_tray")))
        , TrayUnreads_(build::GetProductVariant(qsl(":/logo/ico_icq_unread_tray"), qsl(":/logo/ico_agent_unread_tray"), qsl(":/logo/ico_biz_unread_tray"), qsl(":/logo/ico_dit_unread_tray")))
#else
        , TrayBase_(build::GetProductVariant(qsl(":/logo/ico_icq"), qsl(":/logo/ico_agent"), qsl(":/logo/ico_biz"), qsl(":/logo/ico_dit")))
        , TrayUnreads_(build::GetProductVariant(qsl(":/logo/ico_icq_unread"), qsl(":/logo/ico_agent_unread"), qsl(":/logo/ico_biz_unread"), qsl(":/logo/ico_dit_unread")))
#endif //_WIN32
        , InitMailStatusTimer_(nullptr)
        , UnreadsCount_(0)
        , MailCount_(0)
#ifdef _WIN32
        , ptbl(nullptr)
        , overlayIcon_(nullptr)
#endif //_WIN32
    {
#ifdef _WIN32
        if (QSysInfo().windowsVersion() >= QSysInfo::WV_WINDOWS7)
        {
            HRESULT hr = CoCreateInstance(CLSID_TaskbarList, NULL, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&ptbl));
            if (FAILED(hr))
                ptbl = nullptr;
        }
#endif //_WIN32
        init();

        InitMailStatusTimer_ = new QTimer(this);
        InitMailStatusTimer_->setInterval(init_mail_timeout.count());
        InitMailStatusTimer_->setSingleShot(true);

        connect(MessageAlert_, &RecentMessagesAlert::messageClicked, this, &TrayIcon::messageClicked, Qt::QueuedConnection);
        connect(MailAlert_, &RecentMessagesAlert::messageClicked, this, &TrayIcon::messageClicked, Qt::QueuedConnection);
        connect(MentionAlert_, &RecentMessagesAlert::messageClicked, this, &TrayIcon::messageClicked, Qt::QueuedConnection);

        connect(MessageAlert_, &RecentMessagesAlert::changed, this, &TrayIcon::updateAlertsPosition, Qt::QueuedConnection);
        connect(MailAlert_, &RecentMessagesAlert::changed, this, &TrayIcon::updateAlertsPosition, Qt::QueuedConnection);
        connect(MentionAlert_, &RecentMessagesAlert::changed, this, &TrayIcon::updateAlertsPosition, Qt::QueuedConnection);

        connect(Ui::GetDispatcher(), &core_dispatcher::im_created, this, &TrayIcon::loggedIn, Qt::QueuedConnection);
        connect(Ui::GetDispatcher(), &core_dispatcher::myInfo, this, &TrayIcon::forceUpdateIcon);
        connect(Ui::GetDispatcher(), &core_dispatcher::needLogin, this, &TrayIcon::loggedOut, Qt::QueuedConnection);
        connect(Ui::GetDispatcher(), &core_dispatcher::activeDialogHide, this, &TrayIcon::clearNotifications, Qt::QueuedConnection);
        connect(Ui::GetDispatcher(), &core_dispatcher::mailStatus, this, &TrayIcon::mailStatus, Qt::QueuedConnection);
        connect(Ui::GetDispatcher(), &core_dispatcher::newMail, this, &TrayIcon::newMail, Qt::QueuedConnection);
        connect(Ui::GetDispatcher(), &core_dispatcher::mentionMe, this, &TrayIcon::mentionMe, Qt::QueuedConnection);

        connect(&Utils::InterConnector::instance(), &Utils::InterConnector::historyControlPageFocusIn, this, &TrayIcon::clearNotifications, Qt::QueuedConnection);
        connect(&Utils::InterConnector::instance(), &Utils::InterConnector::mailBoxOpened, this, &TrayIcon::mailBoxOpened, Qt::QueuedConnection);
        connect(&Utils::InterConnector::instance(), &Utils::InterConnector::logout, this, &TrayIcon::resetState, Qt::QueuedConnection);

        if constexpr (platform::is_apple())
        {
            connect(&Utils::InterConnector::instance(), &Utils::InterConnector::mailBoxOpened, this, [this]()
            {
                clearNotifications(qsl("mail"));
            });

            setVisible(get_gui_settings()->get_value(settings_show_in_menubar, false));
        }
    }

    TrayIcon::~TrayIcon()
    {
        cleanupOverlayIcon();
        clearAllNotifications();
        disconnect(get_gui_settings());
    }

    void TrayIcon::cleanupOverlayIcon()
    {
#ifdef _WIN32
        if (overlayIcon_)
        {
            DestroyIcon(overlayIcon_);
            overlayIcon_ = nullptr;
        }
#endif
    }

    void TrayIcon::openMailBox(const QString& _mailId)
    {
        Ui::gui_coll_helper collection(Ui::GetDispatcher()->create_collection(), true);

        collection.set_value_as_qstring("email", Email_);

        Ui::GetDispatcher()->post_message_to_core("mrim/get_key", collection.get(), this, [this, _mailId](core::icollection* _collection)
        {
            Utils::openMailBox(Email_, QString::fromUtf8(Ui::gui_coll_helper(_collection, false).get_value_as_string("key")), _mailId);
        });
    }

    void TrayIcon::hideAlerts()
    {
        MessageAlert_->hide();
        MessageAlert_->markShowed();

        updateIcon();
    }

    void TrayIcon::forceUpdateIcon()
    {
        updateUnreadsCounter();
        updateIcon();
    }

    void TrayIcon::resetState()
    {
        UnreadsCount_ = 0;
        first_start_ = true;

        updateIcon();
        hideEmailIcon();

        clearAllNotifications();
    }

    void TrayIcon::setVisible(bool visible)
    {
        systemTrayIcon_->setVisible(visible);
        if (emailSystemTrayIcon_)
            emailSystemTrayIcon_->setVisible(visible);
    }

    void TrayIcon::updateEmailIcon()
    {
        if (MailCount_ && canShowNotifications(NotificationType::email))
            showEmailIcon();
        else
            hideEmailIcon();
    }

    void TrayIcon::setMacIcon()
    {
#ifdef __APPLE__
        NotificationCenterManager::updateBadgeIcon(UnreadsCount_);

        QString state = MyInfo()->state().toLower();
        if (state != ql1s("offline"))
            state = qsl("online");

        const auto curTheme = MacSupport::currentTheme();
        const auto product = build::ProductNameShort();

        QString iconResource(qsl(":/menubar/%1_%2_%3%4_100").
            arg(product,
                state,
                curTheme,
                UnreadsCount_ > 0 ? qsl("_unread") : QString())
        );
        QIcon icon(Utils::parse_image_name(iconResource));
        systemTrayIcon_->setIcon(icon);

        emailIcon_ = QIcon(Utils::parse_image_name(qsl(":/menubar/mail_%1_100")).arg(curTheme));
        if (emailSystemTrayIcon_)
            emailSystemTrayIcon_->setIcon(emailIcon_);
#endif
    }

    void TrayIcon::updateAlertsPosition()
    {
        TrayPosition pos = getTrayPosition();
        QRect availableGeometry = QDesktopWidget().availableGeometry();

        int screenMarginX = Utils::scale_value(20) - Ui::get_gui_settings()->get_shadow_width();
        int screenMarginY = screenMarginX;
        int screenMarginYMention = screenMarginY;
        int screenMarginYMail = screenMarginY;

        if (MessageAlert_->isVisible())
        {
            screenMarginYMention += (MessageAlert_->height() - Utils::scale_value(12));
            screenMarginYMail = screenMarginYMention;
        }

        if (MentionAlert_->isVisible())
        {
            screenMarginYMail += (MentionAlert_->height() - Utils::scale_value(12));
        }

        switch (pos)
        {
        case TrayPosition::TOP_RIGHT:
            MessageAlert_->move(availableGeometry.topRight().x() - MessageAlert_->width() - screenMarginX, availableGeometry.topRight().y() + screenMarginY);
            MentionAlert_->move(availableGeometry.topRight().x() - MentionAlert_->width() - screenMarginX, availableGeometry.topRight().y() + screenMarginYMention);
            MailAlert_->move(availableGeometry.topRight().x() - MailAlert_->width() - screenMarginX, availableGeometry.topRight().y() + screenMarginYMail);
            break;

        case TrayPosition::BOTTOM_LEFT:
            MessageAlert_->move(availableGeometry.bottomLeft().x() + screenMarginX, availableGeometry.bottomLeft().y() - MessageAlert_->height() - screenMarginY);
            MentionAlert_->move(availableGeometry.bottomLeft().x() + screenMarginX, availableGeometry.bottomLeft().y() - MentionAlert_->height() - screenMarginYMention);
            MailAlert_->move(availableGeometry.bottomLeft().x() + screenMarginX, availableGeometry.bottomLeft().y() - MailAlert_->height() - screenMarginYMail);
            break;

        case TrayPosition::BOTOOM_RIGHT:
            MessageAlert_->move(availableGeometry.bottomRight().x() - MessageAlert_->width() - screenMarginX, availableGeometry.bottomRight().y() - MessageAlert_->height() - screenMarginY);
            MentionAlert_->move(availableGeometry.bottomRight().x() - MentionAlert_->width() - screenMarginX, availableGeometry.bottomRight().y() - MentionAlert_->height() - screenMarginYMention);
            MailAlert_->move(availableGeometry.bottomRight().x() - MailAlert_->width() - screenMarginX, availableGeometry.bottomRight().y() - MailAlert_->height() - screenMarginYMail);
            break;

        case TrayPosition::TOP_LEFT:
        default:
            MessageAlert_->move(availableGeometry.topLeft().x() + screenMarginX, availableGeometry.topLeft().y() + screenMarginY);
            MentionAlert_->move(availableGeometry.topLeft().x() + screenMarginX, availableGeometry.topLeft().y() + screenMarginYMention);
            MailAlert_->move(availableGeometry.topLeft().x() + screenMarginX, availableGeometry.topLeft().y() + screenMarginYMail);
            break;
        }
    }

    QIcon TrayIcon::createIcon(const bool _withBase)
    {
        auto createPixmap = [this](const int _size, const int _count, const bool _withBase)
        {
            QImage baseImage = _withBase ? TrayBase_.pixmap(QSize(_size, _size)).toImage() : QImage();
            return QPixmap::fromImage(Utils::iconWithCounter(_size, _count, Styling::getParameters().getColor(Styling::StyleVariable::SECONDARY_ATTENTION),
                                                             Styling::getParameters().getColor(Styling::StyleVariable::TEXT_SOLID_PERMANENT), std::move(baseImage)));
        };

        QIcon iconOverlay;
        if (UnreadsCount_ > 0)
        {
            iconOverlay.addPixmap(createPixmap(16, UnreadsCount_, _withBase));
            iconOverlay.addPixmap(createPixmap(32, UnreadsCount_, _withBase));
        }
        return iconOverlay;
    }

    bool TrayIcon::updateUnreadsCounter()
    {
        const auto prevCount = UnreadsCount_;
        UnreadsCount_ = Logic::getRecentsModel()->totalUnreads() + Logic::getUnknownsModel()->totalUnreads();

        return UnreadsCount_ != prevCount;
    }

    void TrayIcon::updateIcon(const bool _updateOverlay)
    {
#ifdef _WIN32
        if (_updateOverlay)
        {
            cleanupOverlayIcon();
            if (ptbl)
            {
                overlayIcon_ = UnreadsCount_ > 0 ? createHIconFromQIcon(createIcon(), GetSystemMetrics(SM_CXSMICON), GetSystemMetrics(SM_CYSMICON)) : nullptr;
                ptbl->SetOverlayIcon(MainWindow_->getParentWindow(), overlayIcon_, L"");
            }
            else
            {
                MainWindow_->setWindowIcon(UnreadsCount_ > 0 ? Unreads_ : Base_);
            }
        }
#endif //_WIN32

        if constexpr (platform::is_apple())
            setMacIcon();
        else
            systemTrayIcon_->setIcon(UnreadsCount_ > 0 ? createIcon(true): TrayBase_);
    }

    void TrayIcon::updateIconIfNeeded()
    {
        if (updateUnreadsCounter())
            updateIcon();
    }

    void TrayIcon::clearNotifications(const QString& aimId)
    {
        // We can remove notifications pushed by a previous application run
        if constexpr (!platform::is_apple())
        {
            if (aimId.isEmpty() || !Notifications_.contains(aimId))
            {
                updateIconIfNeeded();
                return;
            }
        }

        Notifications_.removeAll(aimId);
#if defined (_WIN32)
//        if (toastSupported())
//          ToastManager_->HideNotifications(aimId);
#elif defined (__APPLE__)
        if (ncSupported())
            NotificationCenterManager_->HideNotifications(aimId);
#endif //_WIN32

        updateIconIfNeeded();
    }

    void TrayIcon::clearAllNotifications()
    {
        if constexpr (!platform::is_apple())
        {
            if (Notifications_.empty())
            {
                updateIconIfNeeded();
                return;
            }
        }

        Notifications_.clear();
#if defined (__APPLE__)
        if (ncSupported())
            NotificationCenterManager_->removeAllNotifications();
#endif //__APPLE__

        updateIconIfNeeded();
    }

    void TrayIcon::dlgStates(const QVector<Data::DlgState>& _states)
    {
        std::set<QString> showedArray;
        bool playNotification = false;

        for (const auto& _state : _states)
        {
            const bool canNotify =
                _state.Visible_ &&
                !_state.Outgoing_ &&
                _state.UnreadCount_ != 0 &&
                !_state.hasMentionMe_ &&
                !_state.isSuspicious_ &&
                (!ShowedMessages_.contains(_state.AimId_) || (_state.LastMsgId_ != -1 && ShowedMessages_[_state.AimId_] < _state.LastMsgId_)) &&
                !_state.GetText().isEmpty() &&
                !Logic::getUnknownsModel()->contains(_state.AimId_) &&
                !Logic::getContactListModel()->isMuted(_state.AimId_);

            if (canNotify)
            {
                ShowedMessages_[_state.AimId_] = _state.LastMsgId_;

                const auto notifType = _state.AimId_ == ql1s("mail") ? NotificationType::email : NotificationType::messages;
                if (canShowNotifications(notifType))
                    showMessage(_state, AlertType::alertTypeMessage);

                playNotification = true;
            }

            if (_state.Visible_)
                showedArray.insert(_state.AimId_);
        }

        if (playNotification)
        {
#ifdef __APPLE__
            if (!MainWindow_->isUIActive() || MacSupport::previewIsShown())
#else
            if (!MainWindow_->isUIActive())
#endif //__APPLE__
                GetSoundsManager()->playSound(SoundsManager::Sound::IncomingMessage);
        }

        if (!showedArray.empty())
            markShowed(showedArray);

        updateIconIfNeeded();
    }

    void TrayIcon::mentionMe(const QString& _contact, Data::MessageBuddySptr _mention)
    {
        if (canShowNotifications(NotificationType::messages))
        {
            if (auto state = Logic::getRecentsModel()->getDlgState(_contact); state.YoursLastRead_ >= _mention->Id_ || state.LastReadMention_ >= _mention->Id_)
                return;

            Data::DlgState state;
            state.header_ = QChar(0xD83D) % QChar(0xDC4B) % ql1c(' ') % QT_TRANSLATE_NOOP("notifications", "You have been mentioned");
            state.AimId_ = _contact;
            state.mentionMsgId_ = _mention->Id_;
            state.mentionAlert_ = true;
            state.senderAimId_ = _mention->GetChatSender();
            state.Official_ = Logic::getContactListModel()->isOfficial(_contact);

            state.SetText(hist::MessageBuilder::formatRecentsText(*_mention).text_);
            showMessage(state, AlertType::alertTypeMentionMe);

            GetSoundsManager()->playSound(SoundsManager::Sound::IncomingMessage);
        }
    }

    void TrayIcon::newMail(const QString& email, const QString& from, const QString& subj, const QString& id)
    {
        Email_ = email;

        if (canShowNotifications(NotificationType::email))
        {
            Data::DlgState state;

            auto i1 = from.indexOf(ql1c('<'));
            auto i2 = from.indexOf(ql1c('>'));
            if (i1 != -1 && i2 != -1)
            {
                state.senderAimId_ = from.mid(i1 + 1, from.length() - i1 - (from.length() - i2 + 1));
                state.senderNick_ = from.mid(0, i1).trimmed();
            }
            else
            {
                state.senderAimId_ = from;
                state.senderNick_ = from;
            }

            state.AimId_ = qsl("mail");
            state.Friendly_ = from;
            state.header_ = Email_;
            state.MailId_ = id;

            state.SetText(subj);

            if constexpr (platform::is_apple())
                // Hide previous mail notifications
                clearNotifications(qsl("mail"));

            showMessage(state, AlertType::alertTypeEmail);
            GetSoundsManager()->playSound(SoundsManager::Sound::IncomingMail);
        }

        showEmailIcon();
    }

    void TrayIcon::mailStatus(const QString& email, unsigned count, bool init)
    {
        Email_ = email;
        MailCount_ = count;

        if (first_start_)
        {
            bool visible = !!count;
            if (visible)
            {
                showEmailIcon();
            }
            else
            {
                hideEmailIcon();
            }

            first_start_ = false;
        }
        else if (!count)
        {
            hideEmailIcon();
        }

        if (canShowNotifications(NotificationType::email))
        {
            if (!init && !InitMailStatusTimer_->isActive() && !MailAlert_->isVisible())
                return;

            if (count == 0 && init)
            {
                InitMailStatusTimer_->start();
                return;
            }

            Data::DlgState state;
            state.AimId_ = qsl("mail");
            state.header_ = Email_;
            state.Friendly_ = QT_TRANSLATE_NOOP("tray_menu", "New email");
            state.SetText(QString::number(count) % Utils::GetTranslator()->getNumberString(count,
                QT_TRANSLATE_NOOP3("tray_menu", " new email", "1"),
                QT_TRANSLATE_NOOP3("tray_menu", " new emails", "2"),
                QT_TRANSLATE_NOOP3("tray_menu", " new emails", "5"),
                QT_TRANSLATE_NOOP3("tray_menu", " new emails", "21")
                ));

            MailAlert_->updateMailStatusAlert(state);
            if (count == 0)
            {
                hideMailAlerts();
                return;
            }
            else if (MailAlert_->isVisible())
            {
                return;
            }

            showMessage(state, AlertType::alertTypeEmail);
        }
    }

    void TrayIcon::messageClicked(const QString& _aimId, const QString& mailId, const qint64 mentionId, const AlertType _alertType)
    {
        statistic::getGuiMetrics().eventNotificationClicked();

        if (!_aimId.isEmpty())
        {
            bool notificationCenterSupported = false;

            DeferredCallback action;

#if defined (_WIN32)
            if (!toastSupported())
#elif defined (__APPLE__)
            notificationCenterSupported = ncSupported();
#endif //_WIN32
            {
                switch (_alertType)
                {
                    case AlertType::alertTypeEmail:
                    {
                        if (!notificationCenterSupported)
                        {
                            MailAlert_->hide();
                            MailAlert_->markShowed();
                        }

                        action = [this, mailId]()
                        {

                            GetDispatcher()->post_stats_to_core(mailId.isEmpty() ?
                                                                core::stats::stats_event_names::alert_mail_common :
                                                                core::stats::stats_event_names::alert_mail_letter);

                            openMailBox(mailId);
                        };

                        break;
                    }
                    case AlertType::alertTypeMessage:
                    {
                        if (!notificationCenterSupported)
                        {
                            MessageAlert_->hide();
                            MessageAlert_->markShowed();
                        }

                        action = [_aimId]()
                        {
                            if (_aimId != Logic::getContactListModel()->selectedContact())
                                Utils::InterConnector::instance().getMainWindow()->skipRead();

                            Logic::getContactListModel()->setCurrent(_aimId, -1, true);
                        };

                        break;
                    }
                    case AlertType::alertTypeMentionMe:
                    {
                        if (!notificationCenterSupported)
                        {
                            MentionAlert_->hide();
                            MentionAlert_->markShowed();
                        }

                        action = [_aimId, mentionId]()
                        {
                            emit Logic::getContactListModel()->select(_aimId, mentionId, mentionId);
                        };

                        break;
                    }
                    default:
                    {
                        assert(false);
                        return;
                    }
                }

                if (action)
                {
                    if (!LocalPIN::instance()->locked())
                        action();
                    else
                        LocalPIN::instance()->setDeferredAction(new DeferredAction(std::move(action), this));
                }
            }
        }

        MainWindow_->activateFromEventLoop(MainWindow::ActivateReason::ByNotificationClick);
        emit Utils::InterConnector::instance().closeAnyPopupMenu();
        emit Utils::InterConnector::instance().closeAnyPopupWindow(Utils::CloseWindowInfo());
        GetDispatcher()->post_stats_to_core(core::stats::stats_event_names::alert_click);
    }

    QString alertType2String(const AlertType _alertType)
    {
        switch (_alertType)
        {
            case AlertType::alertTypeMessage:
                return qsl("message");
            case AlertType::alertTypeEmail:
                return qsl("mail");
            case AlertType::alertTypeMentionMe:
                return qsl("mention");
            default:
                assert(false);
                return qsl("unknown");
        }
    }

    void TrayIcon::showMessage(const Data::DlgState& state, const AlertType _alertType)
    {
        Notifications_ << state.AimId_;

        const auto isMail = (_alertType == AlertType::alertTypeEmail);
#if defined (_WIN32)
//        if (toastSupported())
//        {
//          ToastManager_->DisplayToastMessage(state.AimId_, state.GetText());
//          return;
//        }
#elif defined (__APPLE__)
        if (ncSupported())
        {
            auto displayName = isMail ? state.Friendly_ : Logic::GetFriendlyContainer()->getFriendly(state.AimId_);

            if (_alertType == AlertType::alertTypeMentionMe)
            {
                displayName = QChar(0xD83D) % QChar(0xDC4B) % ql1c(' ') % QT_TRANSLATE_NOOP("notifications", "You have been mentioned in %1").arg(displayName);
            }

            const bool isShowMessage = !get_gui_settings()->get_value<bool>(settings_hide_message_notification, false) && !Ui::LocalPIN::instance()->locked();
            const QString messageText = isShowMessage ? state.GetText() : QT_TRANSLATE_NOOP("notifications", "New message");

            NotificationCenterManager_->DisplayNotification(
                alertType2String(_alertType),
                state.AimId_,
                state.senderNick_,
                messageText,
                state.MailId_,
                displayName,
                QString::number(state.mentionMsgId_));
            return;
        }
#endif //_WIN32

        RecentMessagesAlert* alert = nullptr;
        switch (_alertType)
        {
        case AlertType::alertTypeMessage:
            alert = MessageAlert_;
            break;
        case AlertType::alertTypeEmail:
            alert = MailAlert_;
            break;
        case AlertType::alertTypeMentionMe:
            alert = MentionAlert_;
            break;
        default:
            assert(false);
            return;
        }

        alert->addAlert(state);

        TrayPosition pos = getTrayPosition();
        QRect availableGeometry = QDesktopWidget().availableGeometry();

        int screenMarginX = Utils::scale_value(20) - Ui::get_gui_settings()->get_shadow_width();
        int screenMarginY = Utils::scale_value(20) - Ui::get_gui_settings()->get_shadow_width();
        if (isMail && MessageAlert_->isVisible())
            screenMarginY += (MessageAlert_->height() - Utils::scale_value(12));

        switch (pos)
        {
        case TrayPosition::TOP_RIGHT:
            alert->move(availableGeometry.topRight().x() - alert->width() - screenMarginX, availableGeometry.topRight().y() + screenMarginY);
            break;

        case TrayPosition::BOTTOM_LEFT:
            alert->move(availableGeometry.bottomLeft().x() + screenMarginX, availableGeometry.bottomLeft().y() - alert->height() - screenMarginY);
            break;

        case TrayPosition::BOTOOM_RIGHT:
            alert->move(availableGeometry.bottomRight().x() - alert->width() - screenMarginX, availableGeometry.bottomRight().y() - alert->height() - screenMarginY);
            break;

        case TrayPosition::TOP_LEFT:
        default:
            alert->move(availableGeometry.topLeft().x() + screenMarginX, availableGeometry.topLeft().y() + screenMarginY);
            break;
        }

        alert->show();
        if (Menu_ && Menu_->isVisible())
            Menu_->raise();
    }

    TrayPosition TrayIcon::getTrayPosition() const
    {
        QRect availableGeometry = QDesktopWidget().availableGeometry();
        QRect iconGeometry = systemTrayIcon_->geometry();

        QString ag = qsl("availableGeometry x: %1, y: %2, w: %3, h: %4 ").arg(availableGeometry.x()).arg(availableGeometry.y()).arg(availableGeometry.width()).arg(availableGeometry.height());
        QString ig = qsl("iconGeometry x: %1, y: %2, w: %3, h: %4").arg(iconGeometry.x()).arg(iconGeometry.y()).arg(iconGeometry.width()).arg(iconGeometry.height());
        Log::trace(qsl("tray"), ag + ig);

        if (platform::is_linux() && iconGeometry.isEmpty())
            return TrayPosition::TOP_RIGHT;

        bool top = abs(iconGeometry.y() - availableGeometry.topLeft().y()) < abs(iconGeometry.y() - availableGeometry.bottomLeft().y());
        if (abs(iconGeometry.x() - availableGeometry.topLeft().x()) < abs(iconGeometry.x() - availableGeometry.topRight().x()))
            return top ? TrayPosition::TOP_LEFT : TrayPosition::BOTTOM_LEFT;
        else
            return top ? TrayPosition::TOP_RIGHT : TrayPosition::BOTOOM_RIGHT;
    }

    void TrayIcon::initEMailIcon()
    {
#ifdef __APPLE__
        emailIcon_ = QIcon(Utils::parse_image_name(qsl(":/menubar/mail_%1_100")).arg(MacSupport::currentTheme()));
#else
        emailIcon_ = QIcon(qsl(":/resources/main_window/tray_email.ico"));
#endif //__APPLE__
    }

    void TrayIcon::showEmailIcon()
    {
        if (emailSystemTrayIcon_ || !canShowNotifications(NotificationType::email))
            return;

        emailSystemTrayIcon_ = new QSystemTrayIcon(this);
        emailSystemTrayIcon_->setIcon(emailIcon_);
        emailSystemTrayIcon_->setVisible(true);
        connect(emailSystemTrayIcon_, &QSystemTrayIcon::activated, this, &TrayIcon::onEmailIconClick, Qt::QueuedConnection);

#ifdef __APPLE__
        if (ncSupported())
            NotificationCenterManager_->reinstallDelegate();
#endif //__APPLE__
    }

    void TrayIcon::hideEmailIcon()
    {
        if (!emailSystemTrayIcon_)
            return;

        emailSystemTrayIcon_->setVisible(false);
        delete emailSystemTrayIcon_;
        emailSystemTrayIcon_ = nullptr;

#ifdef __APPLE__
        if (ncSupported())
            NotificationCenterManager_->reinstallDelegate();
#endif //__APPLE__
    }

    void TrayIcon::hideMailAlerts()
    {
        if constexpr (platform::is_apple())
            clearNotifications(qsl("mail"));
        else
            MailAlert_->hide();
    }

    void TrayIcon::init()
    {
        MessageAlert_->hide();
        MailAlert_->hide();

        MainWindow_->setWindowIcon(Base_);

        updateUnreadsCounter();
        updateIcon(false);

        systemTrayIcon_->setToolTip(Utils::getAppTitle());

        initEMailIcon();

        if constexpr (!platform::is_apple())
        {
            Menu_ = new ContextMenu(MainWindow_);
            auto parentWindow = qobject_cast<MainWindow*>(parent());

            if constexpr (platform::is_linux())
                Menu_->addActionWithIcon(QIcon(), QT_TRANSLATE_NOOP("tray_menu", "Open"), parentWindow, [parentWindow]()
                {
                    parentWindow->activateFromEventLoop();
                });

            const auto iconPath = platform::is_windows() ? qsl(":/context_menu/quit") : QString();
            Menu_->addActionWithIcon(iconPath, QT_TRANSLATE_NOOP("tray_menu", "Quit"), parentWindow, &Ui::MainWindow::exit);
            systemTrayIcon_->setContextMenu(Menu_);
        }

        connect(systemTrayIcon_, &QSystemTrayIcon::activated, this, &TrayIcon::activated, Qt::QueuedConnection);
        systemTrayIcon_->show();
    }

    void TrayIcon::markShowed(const std::set<QString>& _aimIds)
    {
        Ui::gui_coll_helper collection(Ui::GetDispatcher()->create_collection(), true);

        core::ifptr<core::iarray> contacts_array(collection->create_array());
        contacts_array->reserve(_aimIds.size());

        for (const auto& aimid : _aimIds)
            contacts_array->push_back(collection.create_qstring_value(aimid).get());

        collection.set_value_as_array("contacts", contacts_array.get());
        Ui::GetDispatcher()->post_message_to_core("dlg_states/hide", collection.get());
    }

    bool TrayIcon::canShowNotificationsWin() const
    {
#ifdef _WIN32
        if (QSysInfo().windowsVersion() >= QSysInfo::WV_VISTA)
        {
            static QueryUserNotificationState query;
            if (!query)
            {
                HINSTANCE shell32 = LoadLibraryW(L"shell32.dll");
                if (shell32)
                {
                    query = (QueryUserNotificationState)GetProcAddress(shell32, "SHQueryUserNotificationState");
                }
            }

            if (query)
            {
                QUERY_USER_NOTIFICATION_STATE state;
                if (query(&state) == S_OK && state != QUNS_ACCEPTS_NOTIFICATIONS)
                    return false;
            }
        }
#endif //_WIN32
        return true;
    }

    void TrayIcon::mailBoxOpened()
    {
        hideEmailIcon();
    }

    void TrayIcon::onEmailIconClick(QSystemTrayIcon::ActivationReason)
    {
        GetDispatcher()->post_stats_to_core(core::stats::stats_event_names::tray_mail);

        openMailBox(QString());
    }

    bool TrayIcon::canShowNotifications(const NotificationType _notifType) const
    {
        // TODO: must be based on the type of notification - is it message, birthday-notify or contact-coming-notify.

        const auto paramName = _notifType == NotificationType::email
                ? settings_notify_new_mail_messages
                : settings_notify_new_messages;
        if (!get_gui_settings()->get_value<bool>(paramName, true))
            return false;

#ifdef _WIN32
        if (QSysInfo().windowsVersion() == QSysInfo::WV_XP)
        {
            static QuerySystemParametersInfo query;
            if (!query)
            {
                HINSTANCE user32 = LoadLibraryW(L"user32.dll");
                if (user32)
                {
                    query = (QuerySystemParametersInfo)GetProcAddress(user32, "SystemParametersInfoW");
                }
            }

            if (query)
            {
                BOOL result = FALSE;
                if (query(SPI_GETSCREENSAVERRUNNING, 0, &result, 0) && result)
                    return false;
            }
        }
        else
        {
            if (!canShowNotificationsWin())
                return false;
        }

        if (_notifType == NotificationType::email)
            return systemTrayIcon_->isSystemTrayAvailable() && systemTrayIcon_->supportsMessages();

#endif //_WIN32
#ifdef __APPLE__
        if (_notifType == NotificationType::email)
            return get_gui_settings()->get_value<bool>(settings_notify_new_mail_messages, true);

        return systemTrayIcon_->isSystemTrayAvailable() && systemTrayIcon_->supportsMessages() && (!MainWindow_->isUIActive() || MacSupport::previewIsShown());
#else
        static bool trayAvailable = systemTrayIcon_->isSystemTrayAvailable();
        static bool msgsSupport = systemTrayIcon_->supportsMessages();
        return _notifType != NotificationType::email && trayAvailable && msgsSupport && !MainWindow_->isUIActive();
#endif
    }

    void TrayIcon::activated(QSystemTrayIcon::ActivationReason reason)
    {
        bool validReason = reason == QSystemTrayIcon::Trigger;
        if constexpr (platform::is_linux())
            validReason |= (reason == QSystemTrayIcon::MiddleClick);

        if (validReason)
            MainWindow_->activate();
    }

    void TrayIcon::loggedIn()
    {
        connect(Logic::getRecentsModel(), &Logic::RecentsModel::dlgStatesHandled, this, &TrayIcon::dlgStates);
        connect(Logic::getRecentsModel(), &Logic::RecentsModel::updated, this, &TrayIcon::updateIconIfNeeded, Qt::QueuedConnection);
        connect(Logic::getRecentsModel(), &Logic::RecentsModel::readStateChanged, this, &TrayIcon::clearNotifications, Qt::QueuedConnection);

        connect(Logic::getUnknownsModel(), &Logic::UnknownsModel::dlgStatesHandled, this, &TrayIcon::dlgStates);
        connect(Logic::getUnknownsModel(), &Logic::UnknownsModel::updatedMessages, this, &TrayIcon::updateIconIfNeeded, Qt::QueuedConnection);
        connect(Logic::getUnknownsModel(), &Logic::UnknownsModel::readStateChanged, this, &TrayIcon::clearNotifications, Qt::QueuedConnection);

        connect(Logic::getContactListModel(), &Logic::ContactListModel::contactChanged, this, &TrayIcon::updateIconIfNeeded, Qt::QueuedConnection);
        connect(Logic::getContactListModel(), &Logic::ContactListModel::selectedContactChanged, this, &TrayIcon::clearNotifications, Qt::QueuedConnection);
        connect(Logic::getContactListModel(), &Logic::ContactListModel::contact_removed, this, &TrayIcon::clearNotifications, Qt::QueuedConnection);
    }

    void TrayIcon::loggedOut(const bool)
    {
        resetState();
    }

#if defined (_WIN32)
    bool TrayIcon::toastSupported()
    {
        return false;
        /*
        if (QSysInfo().windowsVersion() > QSysInfo::WV_WINDOWS8_1)
        {
        if (!ToastManager_)
        {
        ToastManager_ = std::make_unique<ToastManager>();
        connect(ToastManager_.get(), SIGNAL(messageClicked(QString)), this, SLOT(messageClicked(QString)), Qt::QueuedConnection);
        }
        return true;
        }
        return false;
        */
    }
#elif defined (__APPLE__)
    bool TrayIcon::ncSupported()
    {
        if (QSysInfo().macVersion() > QSysInfo::MV_10_7)
        {
            if (!NotificationCenterManager_)
            {
                NotificationCenterManager_ = std::make_unique<NotificationCenterManager>();
                connect(NotificationCenterManager_.get(), &NotificationCenterManager::messageClicked, this, &TrayIcon::messageClicked, Qt::QueuedConnection);
                connect(NotificationCenterManager_.get(), &NotificationCenterManager::osxThemeChanged, this, &TrayIcon::setMacIcon, Qt::QueuedConnection);
            }
            return true;
        }
        return false;
    }
#endif //_WIN32
}
