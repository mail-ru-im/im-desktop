#include "stdafx.h"
#include "VideoPanel.h"
#include "DetachedVideoWnd.h"
#include "PanelButtons.h"
#include "../core_dispatcher.h"
#include "../main_window/MainPage.h"
#include "../main_window/MainWindow.h"
#include "../utils/gui_metrics.h"
#include "SelectionContactsForConference.h"

#include "../controls/ContextMenu.h"

#include "../previewer/GalleryFrame.h"
#include "../previewer/toast.h"
#include "../styles/ThemeParameters.h"
#include "../gui_settings.h"
#include "../../../common.shared/string_utils.h"

#ifdef __APPLE__
#include "../utils/macos/mac_support.h"
#endif

namespace
{
    auto getMenuIconSize() noexcept
    {
        return Utils::scale_value(QSize(20, 20));
    }

    auto getMenuIconColor()
    {
        return Styling::getParameters().getColorHex(Styling::StyleVariable::TEXT_SOLID_PERMANENT);
    }

    auto getMenuVerOffset() noexcept
    {
        return Utils::scale_value(16);
    }

    auto getPanelRoundedCornerRadius() noexcept
    {
        return Utils::scale_value(12);
    }

    auto getPanelBottomOffset() noexcept
    {
        return Utils::scale_value(12);
    }

    auto getPanelBackgroundColorLinux() noexcept
    {
        // color rgba(38, 38, 38, 100%) from video_panel_linux.qss
        return QColor(38, 38, 38, 255);
    }

    auto getPanelBackgroundColorHex()
    {
        if constexpr (platform::is_linux())
            return getPanelBackgroundColorLinux().name(QColor::HexArgb);
        else
            return Styling::getParameters().getColorHex(Styling::StyleVariable::GHOST_PERCEPTIBLE);
    }

    auto getToastVerOffset() noexcept
    {
        return Utils::scale_value(10);
    }

    auto getStopScreenSharingHeight() noexcept
    {
        return Utils::scale_value(19);
    }

    constexpr std::chrono::milliseconds getFullScreenSwitchingDelay() noexcept
    {
        return std::chrono::seconds(1);
    }

    constexpr std::chrono::milliseconds getCheckOverlappedInterval() noexcept
    {
        return std::chrono::milliseconds(500);
    }

    const QPixmap& getDeviceMark(bool _selected)
    {
        static const auto markSelected = Utils::renderSvg(qsl(":/voip/device_mark_checked"), getMenuIconSize(), Styling::getParameters().getColor(Styling::StyleVariable::SECONDARY_RAINBOW_EMERALD));
        static const auto mark = Utils::renderSvg(qsl(":/voip/device_mark"), getMenuIconSize(), getMenuIconColor());

        return _selected ? markSelected : mark;
    }

    enum class CallType
    {
        Vcs,
        PeerToPeer
    };

    auto addButtonLabel(CallType callType)
    {
        return callType == CallType::Vcs ? QT_TRANSLATE_NOOP("voip_video_panel", "Invite") : QT_TRANSLATE_NOOP("voip_video_panel", "Show\nparticipants");
    }

    auto addButtonIcon(CallType callType)
    {
        return callType == CallType::Vcs ? qsl(":/voip/call_share") : qsl(":/voip/show_participants");
    }
}

namespace Ui
{

    VideoPanel::VideoPanel(QWidget* _parent, QWidget* _container)
#ifdef __APPLE__
        : BaseBottomVideoPanel(_parent, Qt::Window | Qt::FramelessWindowHint | Qt::NoDropShadowWindowHint)
#elif defined(__linux__)
        : BaseBottomVideoPanel(_parent, Qt::Widget)
#else
        : BaseBottomVideoPanel(_parent)
#endif
        , container_(_container)
        , parent_(_parent)
    {
        if constexpr (!platform::is_linux())
        {
            setAttribute(Qt::WA_NoSystemBackground, true);
            setAttribute(Qt::WA_TranslucentBackground, true);
        }

        auto mainLayout = Utils::emptyHLayout(this);
        mainLayout->setAlignment(Qt::AlignTop);

        rootWidget_ = new QWidget(this);
        rootWidget_->setStyleSheet(qsl("background-color: %1; border-radius: %2;").arg(getPanelBackgroundColorHex()).arg(getPanelRoundedCornerRadius()));

        if constexpr (platform::is_linux())
            mainLayout->addStretch(1);

        mainLayout->addWidget(rootWidget_);

        if constexpr (platform::is_linux())
            mainLayout->addStretch(1);

        rootLayout_ = Utils::emptyHLayout(rootWidget_);
        rootLayout_->setAlignment(Qt::AlignTop);

        auto addAndCreateButton = [this] (const QString& _name, const QString& _icon, PanelButton::ButtonStyle _style, bool _hasMore, auto _slot) -> PanelButton*
        {
            auto btn = new PanelButton(rootWidget_, _name, _icon, PanelButton::ButtonSize::Big, _style, _hasMore);
            rootLayout_->addWidget(btn);
            connect(btn, &PanelButton::clicked, this, std::forward<decltype(_slot)>(_slot));
            return btn;
        };

        microphoneButton_ = addAndCreateButton(getMicrophoneButtonText(PanelButton::ButtonStyle::Transparent), qsl(":/voip/microphone_icon"),
                                               PanelButton::ButtonStyle::Transparent, true,
                                               &VideoPanel::onCaptureAudioOnOffClicked);

        connect(microphoneButton_, &PanelButton::moreButtonClicked, this, [this]()
        {
            onDeviceSettingsClick(microphoneButton_, voip_proxy::EvoipDevTypes::kvoipDevTypeAudioCapture);
        });

        speakerButton_ = addAndCreateButton(getSpeakerButtonText(PanelButton::ButtonStyle::Normal), qsl(":/voip/speaker_icon"),
                                            PanelButton::ButtonStyle::Normal, true,
                                            &VideoPanel::onSpeakerOnOffClicked);

        connect(speakerButton_, &PanelButton::moreButtonClicked, this, [this]()
        {
            onDeviceSettingsClick(speakerButton_, voip_proxy::EvoipDevTypes::kvoipDevTypeAudioPlayback);
        });

        videoButton_ = addAndCreateButton(getVideoButtonText(PanelButton::ButtonStyle::Transparent), qsl(":/voip/videocall_icon"),
                                          PanelButton::ButtonStyle::Normal, true,
                                          &VideoPanel::onVideoOnOffClicked);

        connect(videoButton_, &PanelButton::moreButtonClicked, this, [this]()
        {
            onDeviceSettingsClick(videoButton_, voip_proxy::EvoipDevTypes::kvoipDevTypeVideoCapture);
        });

        if (!isRunningUnderWayland())
        {
            shareScreenButton_ = addAndCreateButton(getScreensharingButtonText(PanelButton::ButtonStyle::Transparent), qsl(":/voip/sharescreen_icon"),
                PanelButton::ButtonStyle::Transparent, false, &VideoPanel::onShareScreen);
        }

        const auto callType = Ui::GetDispatcher()->getVoipController().isCallVCS() ?  CallType::Vcs : CallType::PeerToPeer;
        addButton_ = addAndCreateButton(addButtonLabel(callType), addButtonIcon(callType), PanelButton::ButtonStyle::Transparent, false, &VideoPanel::addButtonClicked);

        moreButton_ = addAndCreateButton(QT_TRANSLATE_NOOP("voip_video_panel", "More"), qsl(":/voip/morewide_icon"),
                                         PanelButton::ButtonStyle::Transparent, false,
                                         &VideoPanel::onMoreButtonClicked);


        stopCallButton_ = addAndCreateButton(QT_TRANSLATE_NOOP("voip_video_panel", "End\nMeeting"), qsl(":/voip/endcall_icon"),
                                             PanelButton::ButtonStyle::Red, false,
                                             &VideoPanel::onHangUpButtonClicked);

        connect(&GetDispatcher()->getVoipController(), &voip_proxy::VoipController::onVoipMediaLocalVideo, this, &VideoPanel::onVoipMediaLocalVideo);
        connect(&GetDispatcher()->getVoipController(), &voip_proxy::VoipController::onVoipCallNameChanged, this, &VideoPanel::onVoipCallNameChanged);
        //QObject::connect(&GetDispatcher()->getVoipController(), SIGNAL(onVoipMinimalBandwidthChanged(bool)), this, SLOT(onVoipMinimalBandwidthChanged(bool)), Qt::DirectConnection);
        connect(&GetDispatcher()->getVoipController(), &voip_proxy::VoipController::onVoipVideoDeviceSelected, this, &VideoPanel::onVoipVideoDeviceSelected);
        connect(&GetDispatcher()->getVoipController(), &voip_proxy::VoipController::onVoipMediaLocalAudio, this, &VideoPanel::onVoipMediaLocalAudio);

        connect(&GetDispatcher()->getVoipController(), &voip_proxy::VoipController::onVoipVolumeChanged, this, &VideoPanel::onVoipVolumeChanged);

        connect(&ScreenSharingManager::instance(), &ScreenSharingManager::needShowScreenPermissionsPopup, this, &VideoPanel::needShowScreenPermissionsPopup);
        connect(&ScreenSharingManager::instance(), &ScreenSharingManager::sharingStateChanged, this, &VideoPanel::onShareScreenStateChanged);
    }

    VideoPanel::~VideoPanel()
    {
        closeMenu();
        hideToast();
    }

    bool VideoPanel::isRunningUnderWayland()
    {
        if constexpr (!platform::is_linux())
            return false;
        const char* xdg_session_type = std::getenv("XDG_SESSION_TYPE");
        if (!xdg_session_type || !su::starts_with(std::string_view(xdg_session_type), std::string_view("wayland")))
            return false;
        return bool(std::getenv("WAYLAND_DISPLAY"));
    }

    void VideoPanel::keyReleaseEvent(QKeyEvent* _e)
    {
        if (_e->key() == Qt::Key_Escape)
            Q_EMIT onkeyEscPressed();
        _e->ignore();
        BaseVideoPanel::keyReleaseEvent(_e);
    }

    void VideoPanel::paintEvent(QPaintEvent* _e)
    {
        QWidget::paintEvent(_e);

        if constexpr (platform::is_linux())
        {
            // temporary solution for linux
            QPainter p(this);
            p.fillRect(rect(), getPanelBackgroundColorLinux());
        }
    }

    void VideoPanel::moveEvent(QMoveEvent* _e)
    {
        QWidget::moveEvent(_e);
        closeMenu();
        hideToast();
    }

    bool VideoPanel::event(QEvent* _e)
    {
        if (_e->type() == QEvent::WindowDeactivate)
            hideToast();

        return QWidget::event(_e);
    }

    void VideoPanel::onClickGoChat()
    {
        const auto chatRoomCall = GetDispatcher()->getVoipController().isCallPinnedRoom();
        std::string contact;
        if (chatRoomCall)
            contact = GetDispatcher()->getVoipController().getChatId();
        else if (!activeContact_.empty())
            contact = activeContact_[0].contact;


        if (!contact.empty())
        {
            GetDispatcher()->getVoipController().openChat(QString::fromStdString(contact));
            Q_EMIT onGoToChatButton();
        }
    }

    void VideoPanel::setContacts(std::vector<voip_manager::Contact> _contacts, bool _activeCall)
    {
        activeContact_ = std::move(_contacts);
        isAddButtonEnabled_ = _activeCall && (int32_t)_contacts.size() < (Ui::GetDispatcher()->getVoipController().maxVideoConferenceMembers() - 1);
        addButton_->setEnabled(isAddButtonEnabled_);
    }

    void VideoPanel::enterEvent(QEvent* _e)
    {
        QWidget::enterEvent(_e);
        Q_EMIT onMouseEnter();
    }

    void VideoPanel::leaveEvent(QEvent* _e)
    {
        QWidget::leaveEvent(_e);
        Q_EMIT onMouseLeave();
    }

    void VideoPanel::resizeEvent(QResizeEvent* _e)
    {
        QWidget::resizeEvent(_e);
        closeMenu();
        hideToast();
    }

    void VideoPanel::onHangUpButtonClicked()
    {
        if (parent_ && parent_->isFullScreen())
            parent_->showNormal();
        GetDispatcher()->getVoipController().setHangup();
    }

    void VideoPanel::onShareScreen()
    {
        if (!GetDispatcher()->getVoipController().isLocalDesktopAllowed())
        {
            showToast(ToastType::DesktopNotAllowed);
            return;
        }

        const auto& screens = GetDispatcher()->getVoipController().screenList();

        if (!isScreenSharingEnabled_ && screens.size() > 1)
        {
            resetMenu();

            for (size_t i = 0; i < static_cast<size_t>(screens.size()); i++)
                addMenuAction(MenuAction::ShareScreen, ql1c(' ') % QString::number(i + 1), [i]() { ScreenSharingManager::instance().toggleSharing(i); });

            showMenuAtButton(shareScreenButton_);
            resetPanelButtons(false);
        }
        else
        {
            ScreenSharingManager::instance().toggleSharing(0);
        }
    }

    void VideoPanel::onVoipMediaLocalVideo(bool _enabled)
    {
        localVideoEnabled_ = _enabled && GetDispatcher()->getVoipController().isCamPermissionGranted();
        updateVideoDeviceButtonsState();
        statistic::getGuiMetrics().eventVideocallStartCapturing();
    }

    void VideoPanel::changeEvent(QEvent* _e)
    {
        QWidget::changeEvent(_e);
        if (_e->type() == QEvent::ActivationChange)
        {
            if (isActiveWindow() || (rootWidget_ && rootWidget_->isActiveWindow()))
            {
                if (container_)
                {
                    container_->raise();
                    raise();
                }
            }
        }
    }

    void VideoPanel::onVideoOnOffClicked()
    {
        bool video = GetDispatcher()->getVoipController().checkPermissions(false, true, true);
        if (!video)
        {
            Q_EMIT needShowScreenPermissionsPopup(media::permissions::DeviceType::Camera);
            return;
        }
        if (!Ui::GetDispatcher()->getVoipController().isCamPermissionGranted())
            return;
        if (!GetDispatcher()->getVoipController().isLocalCamAllowed())
        {
            showToast(ToastType::CamNotAllowed);
            return;
        }

        if (!(isCameraEnabled_ && localVideoEnabled_))
            Q_EMIT onCameraClickOn();

        GetDispatcher()->getVoipController().setSwitchVCaptureMute();
    }

    void VideoPanel::setVisible(bool _visible)
    {
        if (!_visible)
        {
            closeMenu();
            hideToast();
        }

        QWidget::setVisible(_visible);
    }

    void VideoPanel::updatePosition(const QWidget& _parent)
    {
        if constexpr (platform::is_linux())
        {
            BaseBottomVideoPanel::updatePosition(_parent);
        }
        else
        {
            const auto rc = parent_->geometry();
            move(rc.x() + (rc.width() - width()) / 2, rc.y() + rc.height() - rect().height() - getPanelBottomOffset());
        }

        if (isFadedIn())
            updateToastPosition();
    }

    void VideoPanel::fadeIn(unsigned int _kAnimationDefDuration)
    {
        if (!isFadedVisible_ && !doPreventFadeIn_)
        {
            isFadedVisible_ = true;
            if constexpr (!platform::is_linux())
                BaseVideoPanel::fadeIn(_kAnimationDefDuration);

            updateToastPosition();
        }
    }

    void VideoPanel::fadeOut(unsigned int _kAnimationDefDuration)
    {
        if (isFadedVisible_)
        {
            isFadedVisible_ = false;
            if constexpr (!platform::is_linux())
                BaseVideoPanel::fadeOut(_kAnimationDefDuration);
        }
    }

    bool VideoPanel::isFadedIn()
    {
        return isFadedVisible_;
    }

    void VideoPanel::setPreventFadeIn(bool _doPrevent)
    {
        doPreventFadeIn_ = _doPrevent;
    }

    bool VideoPanel::isActiveWindow()
    {
        return QWidget::isActiveWindow();
    }

    void VideoPanel::onVoipVideoDeviceSelected(const voip_proxy::device_desc& _desc)
    {
        isScreenSharingEnabled_ = (_desc.dev_type == voip_proxy::kvoipDevTypeVideoCapture && _desc.video_dev_type == voip_proxy::kvoipDeviceDesktop);
        isCameraEnabled_        = (_desc.dev_type == voip_proxy::kvoipDevTypeVideoCapture && _desc.video_dev_type == voip_proxy::kvoipDeviceCamera);
        updateVideoDeviceButtonsState();

        if (isScreenSharingEnabled_)
        {
            if (parent_ && parent_->isVisible() && !parent_->isMinimized())
            {
                isParentMinimizedFromHere_ = true;
                prevParentState_ = parent_->windowState();

                if constexpr (platform::is_apple())
                {
                    if (parent_->isFullScreen())
                    {
                        parent_->showNormal();
                        QTimer::singleShot(getFullScreenSwitchingDelay().count(), [this, uid = _desc.uid]()
                        {
                            parent_->showMinimized();
                        });
                        return;
                    }
                }

                parent_->showMinimized();
            }
        }
        else
        {
            if (isParentMinimizedFromHere_)
            {
                isParentMinimizedFromHere_ = false;
                if (parent_ && parent_->isMinimized())
                {
                    Q_EMIT parentWindowWillDeminiaturize();
                    switch (prevParentState_)
                    {
                        case Qt::WindowFullScreen:
                            parent_->showFullScreen();
                            break;
                        case Qt::WindowMaximized:
                            parent_->showMaximized();
                            break;
                        default:
                            parent_->showNormal();
                            break;
                    }
                    Q_EMIT parentWindowDidDeminiaturize();
                    parent_->raise();
                    parent_->activateWindow();
                }
            }
        }
    }

    void VideoPanel::updateVideoDeviceButtonsState()
    {
        if (localVideoEnabled_ && isCameraEnabled_)
            videoButton_->updateStyle(PanelButton::ButtonStyle::Normal, qsl(":/voip/videocall_icon"), getVideoButtonText(PanelButton::ButtonStyle::Normal));
        else
            videoButton_->updateStyle(PanelButton::ButtonStyle::Transparent, qsl(":/voip/videocall_off_icon"), getVideoButtonText(PanelButton::ButtonStyle::Transparent));

        if (shareScreenButton_)
        {
            if (isScreenSharingEnabled_)
                shareScreenButton_->updateStyle(PanelButton::ButtonStyle::Normal, qsl(":/voip/sharescreen_icon"), getScreensharingButtonText(PanelButton::ButtonStyle::Normal));
            else
                shareScreenButton_->updateStyle(PanelButton::ButtonStyle::Transparent, qsl(":/voip/sharescreen_icon"), getScreensharingButtonText(PanelButton::ButtonStyle::Transparent));
        }
    }

    template<typename Slot>
    void VideoPanel::addMenuAction(const QPixmap& _icon, const QString& _text, Slot&& _slot)
    {
        if (menu_)
        {
            auto a = new QAction(_text, menu_);
            connect(a, &QAction::triggered, this, std::forward<Slot>(_slot));
            menu_->addAction(a, _icon);
        }
    }

    template<typename Slot>
    void VideoPanel::addMenuAction(VideoPanel::MenuAction _action, const QString& _addText, Slot&& _slot)
    {
        if (menu_)
        {
            QString text = menuActionText(_action) % _addText;
            auto a = new QAction(text, menu_);
            connect(a, &QAction::triggered, this, std::forward<Slot>(_slot));
            menu_->addAction(a, Utils::renderSvg(menuActionIconPath(_action), getMenuIconSize(), getMenuIconColor()));
        }
    }

    template<typename Slot>
    void VideoPanel::addMenuAction(VideoPanel::MenuAction _action, Slot&& _slot)
    {
        addMenuAction(_action, {}, std::forward<Slot>(_slot));
    }

    QString VideoPanel::menuActionIconPath(MenuAction _action)
    {
        switch (_action)
        {
            case MenuAction::GoToChat:
                return qsl(":/context_menu/goto");

            case MenuAction::ConferenceAllTheSame:
                return qsl(":/context_menu/conference_all_icon");

            case MenuAction::ConferenceOneIsBig:
                return qsl(":/context_menu/conference_one_icon");

            case MenuAction::OpenMasks:
                return qsl(":/settings/sticker");

            case MenuAction::AddUser:
                return qsl(":/header/add_user");

            case MenuAction::CallSettings:
                return qsl(":/settings_icon_sidebar");

            case MenuAction::ShareScreen:
                return QString();

            case MenuAction::CopyVCSLink:
                return qsl(":/copy_link_icon");

            case MenuAction::InviteVCS:
                return qsl(":/voip/share_to_chat");

            default:
                Q_UNREACHABLE();
                return QString();
        }
    }

    QString VideoPanel::menuActionText(MenuAction _action)
    {
        switch (_action)
        {
            case MenuAction::GoToChat:
                return QT_TRANSLATE_NOOP("voip_video_panel", "Go to chat");

            case MenuAction::ConferenceAllTheSame:
                return QT_TRANSLATE_NOOP("voip_video_panel", "Show all");

            case MenuAction::ConferenceOneIsBig:
                return QT_TRANSLATE_NOOP("voip_video_panel", "Show one");

            case MenuAction::OpenMasks:
                return QT_TRANSLATE_NOOP("voip_video_panel", "Open masks");

            case MenuAction::AddUser:
                return QT_TRANSLATE_NOOP("voip_video_panel", "Add user");

            case MenuAction::CallSettings:
                return QT_TRANSLATE_NOOP("voip_video_panel", "Settings");

            case MenuAction::ShareScreen:
                return QT_TRANSLATE_NOOP("voip_video_panel", "Screen");

            case MenuAction::CopyVCSLink:
                return QT_TRANSLATE_NOOP("voip_video_panel", "Copy link");

            case MenuAction::InviteVCS:
                return addButtonLabel(CallType::Vcs);

            default:
                Q_UNREACHABLE();
                return QString();
        }
    }

    void VideoPanel::resetMenu()
    {
        if (menu_)
            menu_->deleteLater();

        menu_ = new Previewer::CustomMenu();
        connect(menu_, &QObject::destroyed, this, [this]() { resetPanelButtons(); });
    }

    void VideoPanel::showMenuAt(const QPoint& _pos)
    {
        if (menu_)
            menu_->showAtPos(_pos);
    }

    void VideoPanel::showMenuAtButton(const PanelButton* _button)
    {
        const auto buttonRect = _button->rect();
        const auto pos = QPoint(buttonRect.x() + buttonRect.width() / 2, buttonRect.y() + getMenuVerOffset());
        showMenuAt(_button->mapToGlobal(pos));
    }

    void VideoPanel::closeMenu()
    {
        if (menu_)
            menu_->close();
    }

    void VideoPanel::onShareScreenStateChanged(ScreenSharingManager::SharingState _state, int)
    {
        isScreenSharingEnabled_ = (_state == ScreenSharingManager::SharingState::Shared);
        Q_EMIT onShareScreenClick(isScreenSharingEnabled_);
        updateVideoDeviceButtonsState();
    }

    void VideoPanel::resetPanelButtons(bool _enable)
    {
        const auto buttons = findChildren<PanelButton*>();
        for (auto button : buttons)
        {
            button->setEnabled(_enable);
            button->setHovered(false);
            button->setPressed(false);
        }
        addButton_->setEnabled(isAddButtonEnabled_ && _enable);
    }

    void VideoPanel::showToast(const QString& _text, int _maxLineCount)
    {
        hideToast();

        if (parent_)
        {
            toast_ = new Toast(_text, parent_, _maxLineCount);
            toast_->setAttribute(Qt::WA_ShowWithoutActivating);
            toast_->setFocusPolicy(Qt::NoFocus);
            if constexpr (platform::is_windows())
            {
                toast_->setWindowFlags(windowFlags() | Qt::WindowDoesNotAcceptFocus);
            }
            else if constexpr (platform::is_apple())
            {
                const auto flags = parent_->isFullScreen()
                    ? Qt::Tool | Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint | Qt::WindowDoesNotAcceptFocus
                    : windowFlags() | Qt::WindowDoesNotAcceptFocus;
                toast_->setWindowFlags(flags);
            }
            else if constexpr (platform::is_linux())
            {
                toast_->setWindowFlags(Qt::Tool | Qt::FramelessWindowHint | Qt::BypassWindowManagerHint | Qt::WindowDoesNotAcceptFocus);
            }

            if constexpr (!platform::is_linux())
            {
                connect(toast_, &Toast::appeared, this, [isFadedVisiblePrev = isFadedVisible_, this]()
                {
                    if (isFadedVisiblePrev != isFadedVisible_)
                        updateToastPosition();
                });
            }

            const auto videoWindowRect = parent_->geometry();
            const auto yPos = platform::is_linux()
                ? videoWindowRect.bottom() - height()
                : (isFadedVisible_ ? geometry().y() : videoWindowRect.bottom() - toast_->height());
            ToastManager::instance()->showToast(toast_, QPoint(videoWindowRect.center().x(), yPos - getToastVerOffset()));
        }
    }

    bool VideoPanel::showPermissionPopup()
    {
        Ui::GetDispatcher()->getVoipController().checkPermissions(true, false, false);
        if (!Ui::GetDispatcher()->getVoipController().isAudPermissionGranted())
        {
            Q_EMIT needShowScreenPermissionsPopup(media::permissions::DeviceType::Microphone);
            return true;
        }
        if (!Ui::GetDispatcher()->getVoipController().isLocalAudAllowed())
        {
            showToast(ToastType::MicNotAllowed);
            return true;
        }

        return false;
    }

    void VideoPanel::updatePanelButtons()
    {
        const auto callType = Ui::GetDispatcher()->getVoipController().isCallVCS() ? CallType::Vcs : CallType::PeerToPeer;
        addButton_->updateStyle(PanelButton::ButtonStyle::Transparent, addButtonIcon(callType), addButtonLabel(callType));
    }

    void VideoPanel::showToast(ToastType _type)
    {
        switch (_type)
        {
            case ToastType::CamNotAllowed:
            case ToastType::DesktopNotAllowed:
            {
                if (GetDispatcher()->getVoipController().isWebinar())
                {
                    showToast(QT_TRANSLATE_NOOP("voip_video_panel", "Only the creator of the webinar can show the video"));
                }
                else
                {
                    const auto maxUsers = GetDispatcher()->getVoipController().maxUsersWithVideo();
                    showToast(QT_TRANSLATE_NOOP("voip_video_panel", "Ask one of the participants to turn off the video. In calls with over %1 people only one video can be shown").arg(maxUsers), 2);
                }
                break;
            }
            case ToastType::MasksNotAllowed:
            {
                if (GetDispatcher()->getVoipController().isWebinar())
                {
                    showToast(QT_TRANSLATE_NOOP("voip_video_panel", "Only the creator of the webinar can use masks"));
                }
                else
                {
                    const auto usersBound = GetDispatcher()->getVoipController().lowerBoundForBigConference();
                    showToast(QT_TRANSLATE_NOOP("voip_video_panel", "Masks aren't available in calls with more than %1 members").arg(usersBound), 2);
                }
                break;
            }
            case ToastType::MicNotAllowed:
            {
                if (GetDispatcher()->getVoipController().isWebinar())
                    showToast(QT_TRANSLATE_NOOP("voip_video_panel", "Only the creator of the webinar can use a microphone"));
                break;
            }
            case ToastType::LinkCopied:
            {
                showToast(QT_TRANSLATE_NOOP("voip_video_panel", "Link copied"));
                break;
            }
            case ToastType::EmptyLink:
            {
                showToast(QT_TRANSLATE_NOOP("voip_video_panel", "Wait for connection"));
                break;
            }
            case ToastType::DeviceUnavailable:
            {
                showToast(QT_TRANSLATE_NOOP("voip_video_panel", "No devices found"));
                break;
            }
            default:
                break;
        }
    }

    void VideoPanel::updateToastPosition()
    {
        if (toast_)
        {
            const auto rc = parent_->geometry();
            toast_->move(QPoint(rc.center().x() - toast_->width() / 2, (!platform::is_linux() ? geometry().y() : rc.bottom() - height()) - toast_->height() - getToastVerOffset()));
        }
    }

    void VideoPanel::hideToast()
    {
        if (toast_)
            ToastManager::instance()->hideToast();
    }

    void VideoPanel::onCaptureAudioOnOffClicked()
    {
        GetDispatcher()->getVoipController().checkPermissions(true, false, false);
        if (!Ui::GetDispatcher()->getVoipController().isAudPermissionGranted())
        {
            Q_EMIT needShowScreenPermissionsPopup(media::permissions::DeviceType::Microphone);
            return;
        }
        if (!GetDispatcher()->getVoipController().isLocalAudAllowed())
        {
            showToast(ToastType::MicNotAllowed);
            return;
        }

        GetDispatcher()->getVoipController().setSwitchACaptureMute();
        Q_EMIT onMicrophoneClick();
    }

    void VideoPanel::onVoipMediaLocalAudio(bool _enabled)
    {
        if (microphoneButton_)
        {
            if (_enabled && GetDispatcher()->getVoipController().isAudPermissionGranted())
                microphoneButton_->updateStyle(PanelButton::ButtonStyle::Normal, qsl(":/voip/microphone_icon"), getMicrophoneButtonText(PanelButton::ButtonStyle::Normal));
            else
                microphoneButton_->updateStyle(PanelButton::ButtonStyle::Transparent, qsl(":/voip/microphone_off_icon"), getMicrophoneButtonText(PanelButton::ButtonStyle::Transparent));
        }
    }

    bool VideoPanel::isPreventFadeOut() const
    {
        return menu_ && menu_->isActiveWindow();
    }

    void VideoPanel::changeConferenceMode(voip_manager::VideoLayout _layout)
    {
        isConferenceAll_ = _layout == voip_manager::AllTheSame;
    }

    void VideoPanel::callDestroyed()
    {
        isBigConference_ = false;
        isMasksAllowed_ = true;
        ScreenSharingManager::instance().stopSharing();
    }

    void VideoPanel::onSpeakerOnOffClicked()
    {
        GetDispatcher()->getVoipController().setSwitchAPlaybackMute();
        Q_EMIT onSpeakerClick();
    }

    void VideoPanel::onVoipVolumeChanged(const std::string& _deviceType, int _vol)
    {
        if (_deviceType == "audio_playback")
        {
            if (_vol > 0)
                speakerButton_->updateStyle(PanelButton::ButtonStyle::Normal, qsl(":/voip/speaker_icon"), getSpeakerButtonText(PanelButton::ButtonStyle::Normal));
            else
                speakerButton_->updateStyle(PanelButton::ButtonStyle::Transparent, qsl(":/voip/speaker_off_icon"), getSpeakerButtonText(PanelButton::ButtonStyle::Transparent));
        }
    }

    void VideoPanel::onMoreButtonClicked()
    {
        resetMenu();

        if (!GetDispatcher()->getVoipController().isCallVCS() && (activeContact_.size() == 1 || GetDispatcher()->getVoipController().isCallPinnedRoom()))
            addMenuAction(MenuAction::GoToChat, &VideoPanel::onClickGoChat);

        if (!GetDispatcher()->getVoipController().isCallVCS())
            addMenuAction(isConferenceAll_ ? MenuAction::ConferenceOneIsBig : MenuAction::ConferenceAllTheSame, &VideoPanel::onChangeConferenceMode);

        if (!GetDispatcher()->getVoipController().isHideControlsWhenRemDesktopSharing())
            addMenuAction(MenuAction::OpenMasks, &VideoPanel::onClickOpenMasks);

        if (GetDispatcher()->getVoipController().isCallVCS())
            addMenuAction(MenuAction::CopyVCSLink, &VideoPanel::onClickCopyVCSLink);

        showMenuAtButton(moreButton_);

        resetPanelButtons(false);
    }

    void VideoPanel::onChangeConferenceMode()
    {
        isConferenceAll_ = !isConferenceAll_;
        Q_EMIT updateConferenceMode(isConferenceAll_ ? voip_manager::AllTheSame : voip_manager::OneIsBig);
    }

    void VideoPanel::onClickSettings()
    {
        if (auto mainPage = MainPage::instance())
        {
            if (auto wnd = static_cast<MainWindow*>(mainPage->window()))
            {
                wnd->raise();
                wnd->activate();
            }
            mainPage->settingsTabActivate(Utils::CommonSettingsType::CommonSettingsType_VoiceVideo);
        }
    }

    void VideoPanel::onClickOpenMasks()
    {
        if (!isMasksAllowed_)
        {
            showToast(ToastType::MasksNotAllowed);
            return;
        }

        Q_EMIT onShowMaskPanel();
    }

    void VideoPanel::onVoipCallNameChanged(const voip_manager::ContactsList& _contacts)
    {
        const auto lowerBound = GetDispatcher()->getVoipController().lowerBoundForBigConference() - 1;
        isBigConference_ = _contacts.contacts.size() > static_cast<size_t>(lowerBound);

        if (isBigConference_ || !GetDispatcher()->getVoipController().isLocalCamAllowed())
        {
            if (isMasksAllowed_)
            {
                isMasksAllowed_ = false;
                Q_EMIT closeActiveMask();
            }
        }
        else
        {
            isMasksAllowed_ = true;
        }
        const auto isVcsCall = Ui::GetDispatcher()->getVoipController().isCallVCS();
        addButton_->setCount(isVcsCall ? 0 : _contacts.contacts.size() + 1);
    }

    void VideoPanel::onClickCopyVCSLink()
    {
        if (const auto url = GetDispatcher()->getVoipController().getConferenceUrl(); url.isEmpty())
        {
            showToast(ToastType::EmptyLink);
        }
        else
        {
            QApplication::clipboard()->setText(url);
            showToast(ToastType::LinkCopied);
        }
    }

    void VideoPanel::onClickInviteVCS()
    {
        if (const auto url = GetDispatcher()->getVoipController().getConferenceUrl(); url.isEmpty())
            showToast(ToastType::EmptyLink);
        else
            Q_EMIT inviteVCSUrl(url);
    }

    void VideoPanel::onDeviceSettingsClick(const PanelButton* _button, voip_proxy::EvoipDevTypes _type)
    {
        std::vector<voip_proxy::device_desc> devices;
        devices = GetDispatcher()->getVoipController().deviceList(_type);
        // Remove non camera devices.
        devices.erase(std::remove_if(devices.begin(), devices.end(), [](const voip_proxy::device_desc& desc)
        {
            return (desc.dev_type == voip_proxy::kvoipDevTypeVideoCapture && desc.video_dev_type != voip_proxy::kvoipDeviceCamera);
        }), devices.end());

        if (!devices.empty())
        {
            resetMenu();

            auto defaultActiveDevice = GetDispatcher()->getVoipController().activeDevice(_type);
            if ((*defaultActiveDevice).name.empty() && !devices.empty())
                defaultActiveDevice->name = devices.front().name;

            for (const auto& dev : devices)
            {
                const auto changeActiveDevice = [this, dev]()
                {
                    onActiveDeviceChange(dev);
                };

                const auto isActiveDevice = (*defaultActiveDevice).name == dev.name;
                addMenuAction(getDeviceMark(isActiveDevice), QString::fromStdString(dev.name), changeActiveDevice);
            }


            showMenuAtButton(_button);

            resetPanelButtons(false);
        }
        else
        {
            showToast(ToastType::DeviceUnavailable);
        }
    }

    void VideoPanel::addButtonClicked()
    {
        const auto isAddContact = !GetDispatcher()->getVoipController().isCallVCS() || GetDispatcher()->getVoipController().isCallPinnedRoom();

        if (isAddContact)
            Q_EMIT addUserToConference();
        else
            Q_EMIT onClickInviteVCS();
    }

    void VideoPanel::onActiveDeviceChange(const voip_proxy::device_desc& _device)
    {
#ifdef _WIN32
        QString defaultFlagSettingName;
        switch (_device.dev_type)
        {
            case voip_proxy::kvoipDevTypeAudioPlayback:
                defaultFlagSettingName = ql1s(settings_speakers_is_default);
                break;
            case voip_proxy::kvoipDevTypeAudioCapture:
                defaultFlagSettingName = ql1s(settings_microphone_is_default);
                break;
            default:
                break;
        }
        get_gui_settings()->set_value<bool>(defaultFlagSettingName, _device.uid == DEFAULT_DEVICE_UID);
#endif

        GetDispatcher()->getVoipController().setActiveDevice(_device);

        QString settingsName;
        switch (_device.dev_type) {
            case voip_proxy::kvoipDevTypeAudioPlayback: settingsName = ql1s(settings_speakers); break;
            case voip_proxy::kvoipDevTypeAudioCapture:  settingsName = ql1s(settings_microphone); break;
            case voip_proxy::kvoipDevTypeVideoCapture:  settingsName = ql1s(settings_webcam); break;
            case voip_proxy::kvoipDevTypeUndefined:
            default:
                im_assert(!"unexpected device type");
                return;
        };


        const auto uid = QString::fromStdString(_device.uid);
        if (get_gui_settings()->get_value<QString>(settingsName, QString()) != uid)
            get_gui_settings()->set_value<QString>(settingsName, uid);
    }
}
