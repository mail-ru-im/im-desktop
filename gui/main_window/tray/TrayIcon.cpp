#include "stdafx.h"
#include "TrayIcon.h"

#include "RecentMessagesAlert.h"
#include "../MainWindow.h"
#include "../MainPage.h"
#include "../contact_list/ContactListModel.h"
#include "../containers/FriendlyContainer.h"
#include "../contact_list/RecentItemDelegate.h"
#include "../contact_list/RecentsModel.h"
#include "../contact_list/UnknownsModel.h"
#ifndef STRIP_AV_MEDIA
#include "../sounds/SoundsManager.h"
#endif // !STRIP_AV_MEDIA
#include "../../core_dispatcher.h"
#include "../../gui_settings.h"
#include "../../url_config.h"
#include "../../my_info.h"
#include "../../controls/ContextMenu.h"
#include "../../utils/gui_coll_helper.h"
#include "../../utils/InterConnector.h"
#include "../../utils/utils.h"
#include "../../utils/features.h"
#include "../../utils/log/log.h"
#include "../history_control/history/MessageBuilder.h"
#include "../history_control/ChatEventInfo.h"
#include "utils/gui_metrics.h"
#include "main_window/LocalPIN.h"
#include "styles/ThemeParameters.h"
#include "../common.shared/config/config.h"
#include "../common.shared/string_utils.h"

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
    enum class hIconSize
    {
        small,
        big
    };

    HICON createHIconFromQIcon(const QIcon& _icon, const hIconSize _size)
    {
        if (!_icon.isNull())
        {
            const int xSize = GetSystemMetrics(_size == hIconSize::small ? SM_CXSMICON : SM_CXICON);
            const int ySize = GetSystemMetrics(_size == hIconSize::small ? SM_CYSMICON : SM_CYICON);
            const QPixmap pm = _icon.pixmap(_icon.actualSize(QSize(xSize, ySize)));
            if (!pm.isNull())
                return qt_pixmapToWinHICON(pm);
        }
        return nullptr;
    }

    bool isTaskbarIconsSmall()
    {
        QSettings s(qsl("HKEY_CURRENT_USER\\Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\Advanced"), QSettings::NativeFormat);
        return s.value(qsl("TaskbarSmallIcons"), 0).toInt() == 1;
    }
#endif //_WIN32

#ifdef __linux__
    quint32 djbStringHash(const QString& string)
    {
        quint32 hash = 5381;
        const auto chars = string.toLatin1();
        for (auto c : chars)
            hash = (hash << 5) + hash + c;
        return hash;
    }
#endif

    bool containsStrangerEvent(const Data::DlgState& _state)
    {
        if (auto pChatEvt = _state.lastMessage_->GetChatEvent())
            return pChatEvt->eventType() == core::chat_event_type::warn_about_stranger ||
                pChatEvt->eventType() == core::chat_event_type::no_longer_stranger;
        return false;
    }

#ifdef __APPLE__
    QString getThemeForTray()
    {
        return MacSupport::themeTypeToString(MacSupport::currentThemeForTray());
    }
#endif
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
        , MailAlert_(new RecentMessagesAlert(AlertType::alertTypeEmail))
        , MainWindow_(parent)
        , Base_(qsl(":/logo/ico"))
        , Unreads_(qsl(":/logo/ico_unread"))
#ifdef _WIN32
        , TrayBase_(qsl(":/logo/ico_tray"))
        , TrayUnreads_(qsl(":/logo/ico_unread_tray"))
#else
        , TrayBase_(qsl(":/logo/ico"))
        , TrayUnreads_(qsl(":/logo/ico_unread"))
#endif //_WIN32
        , InitMailStatusTimer_(nullptr)
        , UnreadsCount_(0)
        , MailCount_(0)
#ifdef _WIN32
        , ptbl(nullptr)
        , flashing_(false)
#endif //_WIN32
    {
#ifdef _WIN32
        ::CoInitializeEx(NULL, COINIT_APARTMENTTHREADED);
        HRESULT hr = CoCreateInstance(CLSID_TaskbarList, NULL, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&ptbl));
        if (FAILED(hr))
            ptbl = nullptr;

        for (auto& icon : winIcons_)
            icon = nullptr;
#elif defined(__APPLE__)
        NotificationCenterManager_ = std::make_unique<NotificationCenterManager>();
        connect(NotificationCenterManager_.get(), &NotificationCenterManager::messageClicked, this, &TrayIcon::messageClicked, Qt::QueuedConnection);
        connect(NotificationCenterManager_.get(), &NotificationCenterManager::osxThemeChanged, this, &TrayIcon::setMacIcon, Qt::QueuedConnection);
#endif //_WIN32
        init();

        InitMailStatusTimer_ = new QTimer(this);
        InitMailStatusTimer_->setInterval(init_mail_timeout);
        InitMailStatusTimer_->setSingleShot(true);

        mailStatusReceivedTimer_ = new QTimer(this);
        mailStatusReceivedTimer_->setInterval(init_mail_timeout);
        mailStatusReceivedTimer_->setSingleShot(true);
        mailStatusReceivedTimer_->start();
        connect(mailStatusReceivedTimer_, &QTimer::timeout, this, [this](){ mailStatusReceived_ = true; });

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
        connect(Ui::GetDispatcher(), &core_dispatcher::messagesPatched, this, &TrayIcon::deleteMessage, Qt::QueuedConnection);

        connect(&Utils::InterConnector::instance(), &Utils::InterConnector::historyControlPageFocusIn, this, &TrayIcon::clearNotifications, Qt::QueuedConnection);
        connect(&Utils::InterConnector::instance(), &Utils::InterConnector::mailBoxOpened, this, &TrayIcon::mailBoxOpened, Qt::QueuedConnection);
        connect(&Utils::InterConnector::instance(), &Utils::InterConnector::logout, this, &TrayIcon::resetState, Qt::QueuedConnection);

        if constexpr (platform::is_apple())
        {
            connect(&Utils::InterConnector::instance(), &Utils::InterConnector::mailBoxOpened, this, [this]()
            {
                clearNotifications(qsl("mail"));
            });
            connect(&Utils::InterConnector::instance(), &Utils::InterConnector::trayIconThemeChanged, this, &TrayIcon::forceUpdateIcon);

            setVisible(get_gui_settings()->get_value(settings_show_in_menubar, false));
        }
    }

    TrayIcon::~TrayIcon()
    {
        cleanupWinIcons();
        clearAllNotifications();
#ifdef _WIN32
        CoUninitialize();
#endif
    }

    void TrayIcon::cleanupWinIcons()
    {
#ifdef _WIN32
        for (auto& icon : winIcons_)
        {
            DestroyIcon(icon);
            icon = nullptr;
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
        MailCount_ = 0;
        Email_.clear();

        forceUpdateIcon();
        updateEmailIcon();

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
        if (state != u"offline")
            state = qsl("online");

        const auto curTheme = getThemeForTray();
        QString iconResource(ql1s(":/menubar/%1_%2%3_100").
            arg(state, curTheme, UnreadsCount_ > 0 ? ql1s("_unread") : QLatin1String())
        );
        QIcon icon(Utils::parse_image_name(iconResource));
        systemTrayIcon_->setIcon(icon);

        auto eIcon = Utils::renderSvg(ql1s(":/menubar/mail_%1").arg(curTheme));
        Utils::check_pixel_ratio(eIcon);
        emailIcon_ = QIcon(eIcon);
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
#if defined(_WIN32)
        if (_updateOverlay)
        {
            const auto hwnd = reinterpret_cast<HWND>(MainWindow_->winId());
            const auto resetIcons = [this, hwnd](const QIcon& _icon)
            {
                cleanupWinIcons();
                winIcons_[IconType::small] = createHIconFromQIcon(_icon, hIconSize::small);
                winIcons_[IconType::big] = createHIconFromQIcon(_icon, hIconSize::big);
                winIcons_[IconType::overlay] = UnreadsCount_ > 0 ? createHIconFromQIcon(createIcon(), hIconSize::small) : nullptr;

                SendMessage(hwnd, WM_SETICON, ICON_SMALL, LPARAM(winIcons_[IconType::small]));
                SendMessage(hwnd, WM_SETICON, ICON_BIG, LPARAM(winIcons_[IconType::big]));
            };

            if (ptbl && !isTaskbarIconsSmall())
            {
                resetIcons(Base_);
                ptbl->SetOverlayIcon(hwnd, winIcons_[IconType::overlay], L"");
            }
            else
            {
                resetIcons(UnreadsCount_ > 0 ? Unreads_ : Base_);
            }
        }
#elif defined(__linux__)
        static const auto hasUnity = QDBusInterface(qsl("com.canonical.Unity"), qsl("/")).isValid();
        if (hasUnity)
        {
            static const auto launcherUrl = []() -> QString
            {
                const auto executableInfo = QFileInfo(QCoreApplication::applicationFilePath());
                const QString desktopFileName = executableInfo.fileName() % u"desktop.desktop";
                return u"application://" % desktopFileName;
            }();

            QVariantMap unityProperties;
            unityProperties.insert(qsl("count-visible"), UnreadsCount_ > 0);
            if (UnreadsCount_ > 0)
                unityProperties.insert(qsl("count"), (qint64)(UnreadsCount_ > 9999 ? 9999 : UnreadsCount_));

            QDBusMessage signal = QDBusMessage::createSignal(
                        qsl("/com/canonical/unity/launcherentry/") % QString::number(djbStringHash(launcherUrl)),
                        qsl("com.canonical.Unity.LauncherEntry"),
                        qsl("Update"));
            signal << launcherUrl;
            signal << unityProperties;
            QDBusConnection::sessionBus().send(signal);
        }
#endif

        if constexpr (platform::is_apple())
            setMacIcon();
        else
            systemTrayIcon_->setIcon(UnreadsCount_ > 0 ? createIcon(true): TrayBase_);

        animateTaskbarIcon();
    }

    void TrayIcon::updateIconIfNeeded()
    {
        if (updateUnreadsCounter())
            updateIcon();
    }

    void TrayIcon::animateTaskbarIcon()
    {
        bool nowFlashing = false;
#ifdef _WIN32
        nowFlashing = flashing_;

        const auto flash = [this](const auto _flag, const auto _count, const auto _to)
        {
            FLASHWINFO fi;
            fi.cbSize = sizeof(FLASHWINFO);
            fi.hwnd = reinterpret_cast<HWND>(MainWindow_->winId());
            fi.uCount = _count;
            fi.dwTimeout = _to;
            fi.dwFlags = _flag;
            FlashWindowEx(&fi);

            flashing_ = _flag != FLASHW_STOP;
        };

        if (UnreadsCount_ == 0)
            flash(FLASHW_STOP, 0, 0);
#endif

        static int prevCount = UnreadsCount_;
        const auto canAnimate =
            (UnreadsCount_ > prevCount || nowFlashing) &&
            UnreadsCount_ > 0 &&
            get_gui_settings()->get_value<bool>(settings_alert_tray_icon, false) &&
            !MainWindow_->isActive();
        prevCount = UnreadsCount_;

        if (canAnimate)
        {
#if defined(__APPLE__)
            NotificationCenterManager_->animateDockIcon();
#elif defined(_WIN32)
            UINT timeOutMs = GetCaretBlinkTime();
            if (!timeOutMs || timeOutMs == INFINITE)
                timeOutMs = 250;

            flash(FLASHW_TRAY | FLASHW_TIMERNOFG, 10, timeOutMs);
#else
            qApp->alert(MainWindow_, 1000);
#endif
        }
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
        NotificationCenterManager_->HideNotifications(aimId);
#endif //_WIN32

        updateIconIfNeeded();
    }

    void TrayIcon::clearAllNotifications()
    {
        if constexpr (!platform::is_apple())
        {
            if (Notifications_.isEmpty())
            {
                updateIconIfNeeded();
                return;
            }
        }

        Notifications_.clear();
#if defined (__APPLE__)
        NotificationCenterManager_->removeAllNotifications();
#endif //__APPLE__

        updateIconIfNeeded();
    }

    void TrayIcon::dlgStates(const QVector<Data::DlgState>& _states)
    {
        std::set<QString> showedArray;
        for (const auto& _state : _states)
        {
            const bool canNotify =
                    !containsStrangerEvent(_state) &&
                    _state.Visible_ &&
                    !_state.Outgoing_ &&
                    _state.UnreadCount_ != 0 &&
                    !_state.hasMentionMe_ &&
                    !_state.isSuspicious_ &&
                    (!ShowedMessages_.contains(_state.AimId_) || (_state.LastMsgId_ != -1 && ShowedMessages_[_state.AimId_] < _state.LastMsgId_)) &&
                    _state.HasText() &&
                    !Logic::getContactListModel()->isThread(_state.AimId_) &&
                    !Logic::getUnknownsModel()->contains(_state.AimId_) &&
                    !Logic::getContactListModel()->isMuted(_state.AimId_);

            if (canNotify)
            {
                ShowedMessages_[_state.AimId_] = _state.LastMsgId_;

                const auto notifType = _state.AimId_ == u"mail" ? NotificationType::email : NotificationType::messages;
                if (canShowNotifications(notifType))
                    showMessage(_state, _state.AimId_ == u"mail" ? AlertType::alertTypeEmail : AlertType::alertTypeMessage);
            }

            if (_state.Visible_)
                showedArray.insert(_state.AimId_);
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

            if (Logic::getContactListModel()->isThread(_contact))
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
        }
    }

    void TrayIcon::deleteMessage(const Data::PatchedMessage& _messages)
    {
        if (!Features::removeDeletedFromNotifications())
            return;

        if constexpr (!platform::is_apple())
        {
            for (const auto message : _messages.msgIds_)
            {
                MessageAlert_->removeAlert(_messages.aimId_, message);
                MentionAlert_->removeAlert(_messages.aimId_, message);
            }

            updateAlertsPosition();
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

            if (mailStatusReceived_)
                showMessage(state, AlertType::alertTypeEmail);
        }

        showEmailIcon();
    }

    void TrayIcon::mailStatus(const QString& email, unsigned count, bool init)
    {
        Email_ = email;
        MailCount_ = count;

        updateEmailIcon();

        if (!canShowNotifications(NotificationType::email))
            return;

        if (!init && !InitMailStatusTimer_->isActive() && !MailAlert_->isVisible())
            return;

        if (count == 0 && init)
        {
            InitMailStatusTimer_->start();
            return;
        }

        mailStatusReceived_ = true;

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

    void TrayIcon::messageClicked(const QString& _aimId, const QString& mailId, const qint64 mentionId, const AlertType _alertType)
    {
        statistic::getGuiMetrics().eventNotificationClicked();
        MainWindow::ActivateReason reason = MainWindow::ActivateReason::ByNotificationClick;

        if (!_aimId.isEmpty())
        {
            bool notificationCenterSupported = false;

            DeferredCallback action;

#if defined (_WIN32)
            if (!toastSupported())
#elif defined (__APPLE__)
            notificationCenterSupported = true;
#endif //_WIN32
            {
                const auto hideIfNotSupported = [notificationCenterSupported](RecentMessagesAlert* _alert)
                {
                    if (!notificationCenterSupported)
                    {
                        _alert->hide();
                        _alert->markShowed();
                    }
                };
                switch (_alertType)
                {
                    case AlertType::alertTypeEmail:
                    {
                        hideIfNotSupported(MailAlert_);
                        action = [this, mailId]()
                        {

                            GetDispatcher()->post_stats_to_core(mailId.isEmpty() ?
                                                                core::stats::stats_event_names::alert_mail_common :
                                                                core::stats::stats_event_names::alert_mail_letter);

                            openMailBox(mailId);
                        };
                        reason = MainWindow::ActivateReason::ByMailClick;
                        break;
                    }
                    case AlertType::alertTypeMessage:
                    {
                        hideIfNotSupported(MessageAlert_);
                        action = [_aimId]()
                        {
                            Utils::InterConnector::instance().getMainWindow()->showMessengerPage();
                            if (_aimId != Logic::getContactListModel()->selectedContact())
                                Utils::InterConnector::instance().getMainWindow()->skipRead();

                            Utils::InterConnector::instance().openDialog(_aimId);
                        };

                        break;
                    }
                    case AlertType::alertTypeMentionMe:
                    {
                        hideIfNotSupported(MentionAlert_);

                        action = [_aimId, mentionId]()
                        {
                            Q_EMIT Logic::getContactListModel()->select(_aimId, mentionId);
                        };

                        break;
                    }
                    default:
                    {
                        im_assert(false);
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

        MainWindow_->activateFromEventLoop(reason);
        Q_EMIT Utils::InterConnector::instance().closeAnyPopupMenu();
        Q_EMIT Utils::InterConnector::instance().closeAnyPopupWindow(Utils::CloseWindowInfo());
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
                im_assert(false);
                return qsl("unknown");
        }
    }

    void TrayIcon::showMessage(Data::DlgState _state, const AlertType _alertType)
    {
        if (!canShowNotifications(_alertType == AlertType::alertTypeEmail ? NotificationType::email : NotificationType::messages))
            return;

        _state.draft_ = {}; // do not show draft alert
        Notifications_.push_back(_state.AimId_);

#ifndef STRIP_AV_MEDIA
        GetSoundsManager()->playSound(_alertType ==  AlertType::alertTypeEmail ? SoundType::IncomingMail : SoundType::IncomingMessage);
#endif // !STRIP_AV_MEDIA

#if defined (__APPLE__)
        bool messageTextVisible = !get_gui_settings()->get_value<bool>(settings_hide_message_notification, false);
        bool senderNameVisible = !get_gui_settings()->get_value<bool>(settings_hide_sender_notification, false);

        if (Features::hideMessageInfoEnabled())
        {
            messageTextVisible = false;
            senderNameVisible = false;
        }

        if (Features::hideMessageTextEnabled() || LocalPIN::instance()->locked())
            messageTextVisible = false;

        QString displayName = formatNotificationTitle(_state, _alertType, senderNameVisible);
        QString senderNick = formatNotificationSubtitle(_state, _alertType, senderNameVisible);
        QString messageText = formatNotificationText(_state, _alertType, senderNameVisible, messageTextVisible, false);

        NotificationCenterManager_->DisplayNotification(
            alertType2String(_alertType),
            _state.AimId_,
            senderNick,
            messageText,
            _state.MailId_,
            displayName,
            QString::number(_state.mentionMsgId_));
        return;
#endif //__APPLE__

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
            im_assert(false);
            return;
        }

        alert->addAlert(_state);

        TrayPosition pos = getTrayPosition();
        QRect availableGeometry = QDesktopWidget().availableGeometry();

        int screenMarginX = Utils::scale_value(20) - Ui::get_gui_settings()->get_shadow_width();
        int screenMarginY = Utils::scale_value(20) - Ui::get_gui_settings()->get_shadow_width();
        const auto isMail = (_alertType == AlertType::alertTypeEmail);
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

        if (alert->isHidden())
        {   // These weird dances around alert->show() help to avoid alert widget being shown
            // with old content and not got repainted in proper time. It happened occasionally.
            const auto oldOpacity = alert->windowOpacity();
            alert->setWindowOpacity(0.0);
            QCoreApplication::processEvents();
            alert->show();
            QCoreApplication::processEvents();
            alert->setWindowOpacity(oldOpacity);
        }
        else
        {
            alert->show();
        }

        if (Menu_ && Menu_->isVisible())
            Menu_->raise();
    }

    TrayPosition TrayIcon::getTrayPosition() const
    {
        QRect availableGeometry = QDesktopWidget().availableGeometry();
        QRect iconGeometry = systemTrayIcon_->geometry();

        QString ag = qsl("availableGeometry x: %1, y: %2, w: %3, h: %4 ").arg(availableGeometry.x()).arg(availableGeometry.y()).arg(availableGeometry.width()).arg(availableGeometry.height());
        QString ig = qsl("iconGeometry x: %1, y: %2, w: %3, h: %4").arg(iconGeometry.x()).arg(iconGeometry.y()).arg(iconGeometry.width()).arg(iconGeometry.height());
        Log::trace(u"tray", ag + ig);

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
        emailIcon_ = QIcon(Utils::parse_image_name(ql1s(":/menubar/mail_%1_100")).arg(getThemeForTray()));
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
            {
                Menu_->addActionWithIcon(QIcon(), QT_TRANSLATE_NOOP("tray_menu", "Open"), parentWindow, [parentWindow]()
                    {
                        parentWindow->activateFromEventLoop();
                    });
            }

            auto onOffNotifications_ = [this](bool _setOn){ return Menu_->addActionWithIcon(
                platform::is_windows() ? qsl(":/context_menu/mute") + (_setOn ? qsl("") : qsl("_off")) : QString(),
                QT_TRANSLATE_NOOP("context_menu", ql1s("Turn %1 notifications").arg(_setOn ? qsl("on") : qsl("off")).toStdString().c_str()),
                get_gui_settings(),
                [_setOn]()
                {
#ifndef STRIP_VOIP
                    GetDispatcher()->getVoipController().setMuteSounds(_setOn);
#endif
                    get_gui_settings()->set_value<bool>(settings_sounds_enabled, _setOn);
                    get_gui_settings()->set_value<bool>(settings_notify_new_messages, _setOn);
                    get_gui_settings()->set_value<bool>(settings_notify_new_mail_messages, _setOn);
                }
            );};

            disableNotifications_ = onOffNotifications_(false);
            enableNotifications_ = onOffNotifications_(true);

            const auto iconPath = platform::is_windows() ? qsl(":/context_menu/quit") : QString();
            Menu_->addActionWithIcon(iconPath, QT_TRANSLATE_NOOP("tray_menu", "Close"), parentWindow, &Ui::MainWindow::exit);

            systemTrayIcon_->setContextMenu(Menu_);
        }

        connect(systemTrayIcon_, &QSystemTrayIcon::activated, this, &TrayIcon::activated, Qt::QueuedConnection);
        if constexpr (platform::is_linux())
            connect(Menu_, &ContextMenu::aboutToShow, this, &TrayIcon::showContextMenu, Qt::QueuedConnection);

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
        QUERY_USER_NOTIFICATION_STATE state;
        if (SHQueryUserNotificationState(&state) == S_OK && state != QUNS_ACCEPTS_NOTIFICATIONS)
        {
            Log::write_network_log(su::concat("QueryUserNotificationState returned ", std::to_string((int)state)));
            return false;
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
        const auto isMail = _notifType == NotificationType::email;
        const auto paramName = isMail ? settings_notify_new_mail_messages : settings_notify_new_messages;
        if (!get_gui_settings()->get_value<bool>(paramName, true))
            return false;

        if (isMail && !getUrlConfig().isMailConfigPresent())
            return false;

#ifdef __linux__
        static const bool trayCanShowMessages = systemTrayIcon_->isSystemTrayAvailable() && systemTrayIcon_->supportsMessages();
#else
        const bool trayCanShowMessages = systemTrayIcon_->isSystemTrayAvailable() && systemTrayIcon_->supportsMessages();
#endif

        const bool uiActive = isMail ? false : MainWindow_->isUIActive();
        const bool showWithActiveUI = get_gui_settings()->get_value<bool>(settings_notify_new_messages_with_active_ui, settings_notify_new_messages_with_active_ui_default());

#ifdef __APPLE__
        if (isMail)
            return showWithActiveUI || get_gui_settings()->get_value<bool>(settings_notify_new_mail_messages, true);

        return trayCanShowMessages && (!uiActive || showWithActiveUI || MacSupport::previewIsShown());
#else
        if constexpr (platform::is_windows())
            if (!canShowNotificationsWin())
                return false;

        return trayCanShowMessages && (!uiActive || showWithActiveUI);
#endif
    }

    void TrayIcon::activated(QSystemTrayIcon::ActivationReason reason)
    {
        bool validReason = reason == QSystemTrayIcon::Trigger;
        if constexpr (platform::is_linux())
            validReason |= (reason == QSystemTrayIcon::MiddleClick);

        if (validReason)
            MainWindow_->activate();

        if (reason == QSystemTrayIcon::Context)
        {
            if constexpr (platform::is_windows())
                showContextMenu();
        }
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

        forceUpdateIcon();
    }

    void TrayIcon::loggedOut()
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
#endif //_WIN32

    void TrayIcon::showContextMenu()
    {
        const auto notify = get_gui_settings()->get_value<bool>(settings_notify_new_messages, true);
        disableNotifications_->setVisible(notify);
        enableNotifications_->setVisible(!notify);

        const QRect geomTrayIcon = systemTrayIcon_->geometry();
        const QRect geomMenu = Menu_->geometry();
        switch (getTrayPosition())
        {
        case TrayPosition::TOP_RIGHT:
            Menu_->move(geomTrayIcon.x() + geomTrayIcon.width() - geomMenu.width(), geomTrayIcon.y() + geomTrayIcon.height());
            break;
        case TrayPosition::BOTTOM_LEFT:
            Menu_->move(geomTrayIcon.x(), geomTrayIcon.y() - geomMenu.height());
            break;
        case TrayPosition::BOTOOM_RIGHT:
            Menu_->move(geomTrayIcon.x() + geomTrayIcon.width() - geomMenu.width(), geomTrayIcon.y() - geomMenu.height());
            break;
        case TrayPosition::TOP_LEFT:
        default:
            Menu_->move(geomTrayIcon.x(), geomTrayIcon.y() + geomTrayIcon.height());
            break;
        }    
    }
}
