#include "stdafx.h"
#include "VideoPanel.h"
#include "VideoWindow.h"
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
    constexpr const char* kVoipDevTypeProperty = "voipDevType";

    auto getButtonIconSize() noexcept
    {
        return Utils::scale_value(QSize(40, 40));
    }

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

    auto getPanelBackgroundColorHex()
    {
        return Styling::getParameters().getColorHex(Styling::StyleVariable::GHOST_PERCEPTIBLE);
    }

    auto getToastVerOffset() noexcept
    {
        return Utils::scale_value(100);
    }

    auto getStopScreenSharingHeight() noexcept
    {
        return Utils::scale_value(19);
    }

    constexpr std::chrono::milliseconds getFullScreenSwitchingDelay() noexcept
    {
        return std::chrono::seconds(1);
    }

    QPixmap getDeviceMark(bool _selected)
    {
        const auto markSelected = Utils::renderSvg(qsl(":/voip/device_mark_checked"), getMenuIconSize(), Styling::getParameters().getColor(Styling::StyleVariable::SECONDARY_RAINBOW_EMERALD));
        const auto mark = Utils::renderSvg(qsl(":/voip/device_mark"), getMenuIconSize(), getMenuIconColor());

        return _selected ? markSelected : mark;
    }

    enum class CallType
    {
        Vcs,
        PeerToPeer
    };

    auto inviteButtonLabel(CallType _callType)
    {
        return _callType == CallType::Vcs ? QT_TRANSLATE_NOOP("voip_video_panel", "Invite") : QT_TRANSLATE_NOOP("voip_video_panel", "Show\nparticipants");
    }

    auto inviteButtonTooltip(CallType callType)
    {
        return callType == CallType::Vcs ? QT_TRANSLATE_NOOP("voip_video_panel", "Share call link") : QT_TRANSLATE_NOOP("voip_video_panel", "Call participants management");
    }

    auto inviteButtonIcon(CallType _callType)
    {
        return _callType == CallType::Vcs ? qsl(":/voip/call_share") : qsl(":/voip/show_participants");
    }
}

namespace Ui
{

    VideoPanel::VideoPanel(QWidget* _parent)
        : QWidget(_parent)
        , parent_(_parent)
    {
        setAttribute(Qt::WA_NoSystemBackground, true);
        setAttribute(Qt::WA_TranslucentBackground, true);
        setAttribute(Qt::WA_Hover, true);

        auto mainLayout = Utils::emptyHLayout(this);
        mainLayout->setAlignment(Qt::AlignTop);

        rootWidget_ = new QWidget(this);
        rootWidget_->setStyleSheet(qsl("background-color: %1; border-radius: %2;").arg(getPanelBackgroundColorHex()).arg(getPanelRoundedCornerRadius()));

        mainLayout->addWidget(rootWidget_);

        rootLayout_ = Utils::emptyHLayout(rootWidget_);
        rootLayout_->setAlignment(Qt::AlignTop);

        menu_ = new Previewer::CustomMenu;
        menu_->setAttribute(Qt::WA_DeleteOnClose, false);
        connect(menu_, &QMenu::aboutToShow, this, &VideoPanel::lockAnimations);
        connect(menu_, &QMenu::aboutToHide, this, [this]()
        {
            resetPanelButtons();
            Q_EMIT unlockAnimations();
        });

        microphoneButton_ = createButton(microphoneButtonText(false), true);
        microphoneButton_->setChecked(false);
        microphoneButton_->setIcon(microphoneIcon(microphoneButton_->isChecked()));
        microphoneButton_->setMenu(menu_);
        microphoneButton_->setKeySequence(Qt::ControlModifier + Qt::ShiftModifier + Qt::Key_A);
        microphoneButton_->setProperty(kVoipDevTypeProperty, voip_proxy::EvoipDevTypes::kvoipDevTypeAudioCapture);
        connect(microphoneButton_, &PanelToolButton::clicked, this, &VideoPanel::onCaptureAudioOnOffClicked);
        connect(microphoneButton_, &PanelToolButton::aboutToShowMenu, this, &VideoPanel::onDeviceSettingsClicked);
        rootLayout_->addWidget(microphoneButton_);

        speakerButton_ = createButton(speakerButtonText(true), true);
        speakerButton_->setChecked(true);
        speakerButton_->setIcon(speakerIcon(speakerButton_->isChecked()));
        speakerButton_->setMenu(menu_);
        speakerButton_->setKeySequence(Qt::ControlModifier + Qt::ShiftModifier + Qt::Key_D);
        speakerButton_->setProperty(kVoipDevTypeProperty, voip_proxy::EvoipDevTypes::kvoipDevTypeAudioPlayback);
        connect(speakerButton_, &PanelToolButton::clicked, this, &VideoPanel::onSpeakerOnOffClicked);
        connect(speakerButton_, &PanelToolButton::aboutToShowMenu, this, &VideoPanel::onDeviceSettingsClicked);
        rootLayout_->addWidget(speakerButton_);

        videoButton_ = createButton(speakerButtonText(false), true);
        videoButton_->setChecked(false);
        videoButton_->setIcon(videoIcon(speakerButton_->isChecked()));
        videoButton_->setMenu(menu_);
        videoButton_->setKeySequence(Qt::ControlModifier + Qt::ShiftModifier + Qt::Key_V);
        videoButton_->setProperty(kVoipDevTypeProperty, voip_proxy::EvoipDevTypes::kvoipDevTypeVideoCapture);
        connect(videoButton_, &PanelToolButton::clicked, this, &VideoPanel::onVideoOnOffClicked);
        connect(videoButton_, &PanelToolButton::aboutToShowMenu, this, &VideoPanel::onDeviceSettingsClicked);
        rootLayout_->addWidget(videoButton_);

        if (!isRunningUnderWayland())
        {
            shareScreenButton_ = createButton(screensharingButtonText(false), true);
            shareScreenButton_->setChecked(false);
            shareScreenButton_->setIcon(screensharingIcon());
            shareScreenButton_->installEventFilter(this);
            shareScreenButton_->setKeySequence(Qt::ControlModifier + Qt::ShiftModifier + Qt::Key_S);
            connect(shareScreenButton_, &PanelToolButton::clicked, this, &VideoPanel::onShareScreen);
            rootLayout_->addWidget(shareScreenButton_);
        }

        const auto callType = Ui::GetDispatcher()->getVoipController().isCallVCSOrLink() ? CallType::Vcs : CallType::PeerToPeer;
        inviteButton_ = createButton(inviteButtonLabel(callType), false);
        inviteButton_->setChecked(false);
        inviteButton_->setToolTip(inviteButtonTooltip(callType));
        inviteButton_->setIcon(inviteButtonIcon(callType));
        connect(inviteButton_, &PanelToolButton::clicked, this, &VideoPanel::inviteButtonClicked);
        rootLayout_->addWidget(inviteButton_);

        stopCallButton_ = createButton(QT_TRANSLATE_NOOP("voip_video_panel", "End\nMeeting"), false);
        stopCallButton_->setToolTip(QT_TRANSLATE_NOOP("voip_video_panel", "End meeting"));
        stopCallButton_->setIcon(hangupIcon());
        stopCallButton_->setButtonRole(PanelToolButton::Attention);
        connect(stopCallButton_, &PanelToolButton::clicked, this, &VideoPanel::requestHangup);
        rootLayout_->addWidget(stopCallButton_);

        connect(&GetDispatcher()->getVoipController(), &voip_proxy::VoipController::onVoipMediaLocalVideo, this, &VideoPanel::onVoipMediaLocalVideo);
        connect(&GetDispatcher()->getVoipController(), &voip_proxy::VoipController::onVoipCallNameChanged, this, &VideoPanel::onVoipCallNameChanged);
        connect(&GetDispatcher()->getVoipController(), &voip_proxy::VoipController::onVoipVideoDeviceSelected, this, &VideoPanel::onVoipVideoDeviceSelected);
        connect(&GetDispatcher()->getVoipController(), &voip_proxy::VoipController::onVoipMediaLocalAudio, this, &VideoPanel::onVoipMediaLocalAudio);

        connect(&GetDispatcher()->getVoipController(), &voip_proxy::VoipController::onVoipVolumeChanged, this, &VideoPanel::onVoipVolumeChanged);

        connect(&ScreenSharingManager::instance(), &ScreenSharingManager::needShowScreenPermissionsPopup, this, &VideoPanel::needShowScreenPermissionsPopup);
        connect(&ScreenSharingManager::instance(), &ScreenSharingManager::sharingStateChanged, this, &VideoPanel::onShareScreenStateChanged);

        connect(qApp, &QApplication::screenAdded, this, &VideoPanel::onScreensChanged);
        connect(qApp, &QApplication::screenRemoved, this, &VideoPanel::onScreensChanged);

        setFixedSize(mainLayout->minimumSize());
    }

    VideoPanel::~VideoPanel()
    {
        closeMenu();
        hideToast();
        menu_->deleteLater();
    }

    PanelToolButton* VideoPanel::createButton(const QString& _text, bool _checkable) const
    {
        PanelToolButton* button = new PanelToolButton(_text, const_cast<VideoPanel*>(this));
        button->setIconSize(getButtonIconSize());
        button->setButtonStyle(Qt::ToolButtonTextUnderIcon);
        button->setCursor(Qt::PointingHandCursor);
        button->setCheckable(_checkable);
        button->setMenuAlignment(Qt::AlignTop | Qt::AlignHCenter);
        return button;
    }

    void VideoPanel::showShareScreenMenu(int _screenCount)
    {
        resetMenu();
        for (size_t i = 0; i < static_cast<size_t>(_screenCount); i++)
            addMenuAction(MenuAction::ShareScreen, ql1c(' ') % QString::number(i + 1), [i]() { ScreenSharingManager::instance().toggleSharing(i); });
        shareScreenButton_->showMenu(menu_);
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
        QWidget::keyReleaseEvent(_e);
    }

    void VideoPanel::moveEvent(QMoveEvent* _e)
    {
        QWidget::moveEvent(_e);
        closeMenu();
        hideToast();
    }

    void VideoPanel::resizeEvent(QResizeEvent* _e)
    {
        QWidget::resizeEvent(_e);
        closeMenu();
        hideToast();
    }

    void VideoPanel::hideEvent(QHideEvent* _e)
    {
        QWidget::hideEvent(_e);
        closeMenu();
        hideToast();
    }

    bool VideoPanel::event(QEvent* _e)
    {
        if (_e->type() == QEvent::WindowDeactivate)
            hideToast();

        return QWidget::event(_e);
    }

    bool VideoPanel::eventFilter(QObject* _watched, QEvent* _event)
    {
        if (_watched == shareScreenButton_ && _event->type() == QEvent::MouseButtonPress)
        {
            if (static_cast<QMouseEvent*>(_event)->button() != Qt::LeftButton)
                return QWidget::eventFilter(_watched, _event);

            const bool isLocalDesktopAllowed = GetDispatcher()->getVoipController().isLocalDesktopAllowed();
            const auto& screens = GetDispatcher()->getVoipController().screenList();
            const int screenCount = screens.size();
            if (!isScreenSharingEnabled_ && isLocalDesktopAllowed && screenCount > 1)
            {
                showShareScreenMenu(screenCount);
                resetPanelButtons(false);
                return true;
            }
        }
        return QWidget::eventFilter(_watched, _event);
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
        isInviteButtonEnabled_ = _activeCall && (int32_t)_contacts.size() < (Ui::GetDispatcher()->getVoipController().maxVideoConferenceMembers() - 1);
        inviteButton_->setEnabled(isInviteButtonEnabled_);
    }

    void VideoPanel::onShareScreen()
    {
        if (!GetDispatcher()->getVoipController().isLocalDesktopAllowed())
        {
            showToast(VideoWindowToastProvider::Type::DesktopNotAllowed);
            return;
        }

        const auto& screens = GetDispatcher()->getVoipController().screenList();
        const int screenCount = screens.size();
        if (!isScreenSharingEnabled_ && screenCount > 1)
        {
            showShareScreenMenu(screenCount);
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
            showToast(VideoWindowToastProvider::Type::CamNotAllowed);
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
                        QTimer::singleShot(getFullScreenSwitchingDelay().count(), [this]()
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
                    parent_->raise();
                    parent_->activateWindow();
                }
            }
        }
    }

    void VideoPanel::updateVideoDeviceButtonsState()
    {
        const bool hasVideo = localVideoEnabled_ && isCameraEnabled_;
        videoButton_->setChecked(hasVideo);
        videoButton_->setIcon(videoIcon(hasVideo));
        videoButton_->setText(videoButtonText(hasVideo));

        if (shareScreenButton_)
        {
            shareScreenButton_->setText(screensharingButtonText(isScreenSharingEnabled_));
            shareScreenButton_->setChecked(isScreenSharingEnabled_);
            shareScreenButton_->setIcon(screensharingIcon());
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
                return ql1s(":/context_menu/goto");

            case MenuAction::ConferenceAllTheSame:
                return ql1s(":/context_menu/conference_all_icon");

            case MenuAction::ConferenceOneIsBig:
                return ql1s(":/context_menu/conference_one_icon");

            case MenuAction::AddUser:
                return ql1s(":/header/add_user");

            case MenuAction::CallSettings:
                return ql1s(":/settings_icon_sidebar");

            case MenuAction::ShareScreen:
                return QString();

            case MenuAction::CopyVCSLink:
                return ql1s(":/copy_link_icon");

            case MenuAction::InviteVCS:
                return ql1s(":/voip/share_to_chat");

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

            case MenuAction::AddUser:
                return QT_TRANSLATE_NOOP("voip_video_panel", "Add user");

            case MenuAction::CallSettings:
                return QT_TRANSLATE_NOOP("voip_video_panel", "Settings");

            case MenuAction::ShareScreen:
                return QT_TRANSLATE_NOOP("voip_video_panel", "Screen");

            case MenuAction::CopyVCSLink:
                return QT_TRANSLATE_NOOP("voip_video_panel", "Copy link");

            case MenuAction::InviteVCS:
                return inviteButtonLabel(CallType::Vcs);

            default:
                Q_UNREACHABLE();
                return QString();
        }
    }

    void VideoPanel::resetMenu()
    {
        if (menu_)
        {
            menu_->close();
            menu_->clear();
        }
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
        const auto buttons = findChildren<PanelToolButton*>(QString{}, Qt::FindDirectChildrenOnly);
        for (auto button : buttons)
            button->setEnabled(_enable);
        inviteButton_->setEnabled(isInviteButtonEnabled_ && _enable);
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
            showToast(VideoWindowToastProvider::Type::MicNotAllowed);
            return true;
        }

        return false;
    }

    void VideoPanel::updatePanelButtons()
    {
        const auto callType = Ui::GetDispatcher()->getVoipController().isCallVCSOrLink() ? CallType::Vcs : CallType::PeerToPeer;
        inviteButton_->setIcon(inviteButtonIcon(callType));
        inviteButton_->setText(inviteButtonLabel(callType));
    }

    void VideoPanel::setConferenceMode(bool _isConferenceAll)
    {
        isConferenceAll_ = _isConferenceAll;
    }

    void VideoPanel::showToast(VideoWindowToastProvider::Type _type)
    {
        VideoWindowToastProvider::instance().show(_type);
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
            showToast(VideoWindowToastProvider::Type::MicNotAllowed);
            return;
        }

        GetDispatcher()->getVoipController().setSwitchACaptureMute();
        Q_EMIT onMicrophoneClick();
    }

    void VideoPanel::onVoipMediaLocalAudio(bool _enabled)
    {
        if (microphoneButton_)
        {
            const bool checked = _enabled && GetDispatcher()->getVoipController().isAudPermissionGranted();
            microphoneButton_->setChecked(checked);
            microphoneButton_->setText(microphoneButtonText(checked));
            microphoneButton_->setIcon(microphoneIcon(checked));
        }
    }

    bool VideoPanel::isPreventFadeOut() const
    {
        return menu_ && menu_->isActiveWindow();
    }

    void VideoPanel::callDestroyed()
    {
        isConferenceAll_ = true;
        isBigConference_ = false;
        ScreenSharingManager::instance().stopSharing();
    }

    void VideoPanel::onSpeakerOnOffClicked()
    {
        GetDispatcher()->getVoipController().setSwitchAPlaybackMute();
        Q_EMIT onSpeakerClick();
    }

    void VideoPanel::onVoipVolumeChanged(const std::string& _deviceType, int _vol)
    {
        if (_deviceType != "audio_playback")
            return;

        const bool hasAudio = _vol > 0;
        speakerButton_->setChecked(hasAudio);
        speakerButton_->setText(speakerButtonText(hasAudio));
        speakerButton_->setIcon(speakerIcon(hasAudio));
    }

    void VideoPanel::onChangeConferenceMode()
    {
        isConferenceAll_ = !isConferenceAll_;
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

    void VideoPanel::onVoipCallNameChanged(const voip_manager::ContactsList& _contacts)
    {
        const auto lowerBound = GetDispatcher()->getVoipController().lowerBoundForBigConference() - 1;
        isBigConference_ = _contacts.contacts.size() > static_cast<size_t>(lowerBound);

        if (!isConferenceAll_ && _contacts.contacts.size() <= 1)
            onChangeConferenceMode();

        const auto isVcsCall = Ui::GetDispatcher()->getVoipController().isCallVCS();
        inviteButton_->setBadgeCount(isVcsCall ? 0 : _contacts.contacts.size() + 1);
    }

    void VideoPanel::onClickCopyVCSLink()
    {
        if (const auto url = GetDispatcher()->getVoipController().getConferenceUrl(); url.isEmpty())
        {
            showToast(VideoWindowToastProvider::Type::EmptyLink);
        }
        else
        {
            QApplication::clipboard()->setText(url);
            showToast(VideoWindowToastProvider::Type::LinkCopied);
        }
    }

    void VideoPanel::onClickInviteVCS()
    {
        if (const auto url = GetDispatcher()->getVoipController().getConferenceUrl(); url.isEmpty())
            showToast(VideoWindowToastProvider::Type::EmptyLink);
        else
            Q_EMIT inviteVCSUrl(url);
    }

    void VideoPanel::onDeviceSettingsClicked()
    {
        const QAbstractButton* _button = qobject_cast<QAbstractButton*>(sender());
        if (!_button)
            return;

        const QVariant userData = _button->property(kVoipDevTypeProperty);
        if (!userData.isValid())
            return;

        const voip_proxy::EvoipDevTypes _type = static_cast<voip_proxy::EvoipDevTypes>(userData.toInt());

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

            resetPanelButtons(false);
        }
        else
        {
            showToast(VideoWindowToastProvider::Type::DeviceUnavailable);
        }
    }

    void VideoPanel::inviteButtonClicked()
    {
        const auto isAddContact = !GetDispatcher()->getVoipController().isCallVCSOrLink() || GetDispatcher()->getVoipController().isCallPinnedRoom();

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

    void VideoPanel::onScreensChanged()
    {
        if (qApp->screens().count() < 2 && menu_->isVisible())
            menu_->hide();
    }
}
