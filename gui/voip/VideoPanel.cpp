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

#ifdef __APPLE__
#include "../utils/macos/mac_support.h"
#endif

namespace
{
    auto getButtonSize() noexcept
    {
        return Utils::scale_value(QSize(70, 96));
    }

    auto getButtonCircleSize() noexcept
    {
        return Utils::scale_value(40);
    }

    auto getButtonIconSize() noexcept
    {
        return QSize(32, 32);
    }

    auto getMoreButtonSize() noexcept
    {
        return Utils::scale_value(20);
    }

    auto getMoreButtonBorder() noexcept
    {
        return Utils::scale_value(2);
    }

    auto getMoreButtonMarginLeft() noexcept
    {
        return Utils::scale_value(28);
    }

    auto getMoreButtonMarginTop() noexcept
    {
        return Utils::scale_value(26);
    }

    auto getMoreIconSize() noexcept
    {
        return Utils::scale_value(14);
    }

    auto getButtonTextFont()
    {
        return Fonts::appFontScaled(11, platform::is_apple() ? Fonts::FontWeight::Medium : Fonts::FontWeight::SemiBold);
    }

    auto getButtonCircleVerOffset() noexcept
    {
        return Utils::scale_value(12);
    }

    auto getButtonTextVerOffset() noexcept
    {
        return Utils::scale_value(60);
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

    auto getFrameBorderWidth() noexcept
    {
        return Utils::scale_value(2);
    }

    auto getFrameBorderColor()
    {
        return Styling::getParameters().getColor(Styling::StyleVariable::SECONDARY_RAINBOW_YELLOW);
    }

    auto getStopScreenSharingFont()
    {
        return Fonts::appFontScaled(12, Fonts::FontWeight::Normal);
    }

    auto getStopScreenSharingHeight() noexcept
    {
        return Utils::scale_value(19);
    }

    auto getStopScreenSharingTextPadding() noexcept
    {
        return Utils::scale_value(4);
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

    auto getShadowColor()
    {
        return QColor(0, 0, 0, 255 * 0.35);
    }
}

namespace Ui
{
    namespace VideoPanelParts
    {
        ScreenBorderLine::ScreenBorderLine(LinePosition _position, const Qt::WindowFlags& _wFlags, const QColor& _color)
            : QWidget(nullptr, _wFlags)
            , linePosition_(_position)
            , lineWidth_(getFrameBorderWidth())
            , bgColor_(_color)
            , checkOverlappedTimer_(nullptr)
        {
            setAttribute(Qt::WA_DeleteOnClose);
            setAttribute(Qt::WA_TranslucentBackground);
            setAttribute(Qt::WA_X11DoNotAcceptFocus);
            setAttribute(Qt::WA_TransparentForMouseEvents);

            if constexpr (platform::is_windows())
            {
                checkOverlappedTimer_ = new QTimer(this);
                checkOverlappedTimer_->setInterval(getCheckOverlappedInterval().count());

                connect(checkOverlappedTimer_, &QTimer::timeout, this, [this]()
                {
                    // try without isOverlapped (too slowly)
                    // if (isOverlapped())
                    raise();
                });

                checkOverlappedTimer_->start();
            }
        }

        ScreenBorderLine::~ScreenBorderLine()
        {
            if (checkOverlappedTimer_)
                checkOverlappedTimer_->stop();
        }

        void ScreenBorderLine::updateForGeometry(const QRect& _rect)
        {
            if (!_rect.isValid())
                return;

            switch (linePosition_)
            {
            case LinePosition::Left:
                setGeometry(_rect.x(), _rect.y() + lineWidth_, lineWidth_, _rect.height() - 2 * lineWidth_);
                break;
            case LinePosition::Top:
                setGeometry(_rect.x(), _rect.y(), _rect.width(), lineWidth_);
                break;
            case LinePosition::Right:
                setGeometry(_rect.x() + _rect.width() - lineWidth_, _rect.y() + lineWidth_, lineWidth_, _rect.height() - 2 * lineWidth_);
                break;
            case LinePosition::Bottom:
                setGeometry(_rect.x(), _rect.y() + _rect.height() - lineWidth_, _rect.width(), lineWidth_);
                break;
            }

            update();
        }

        void ScreenBorderLine::paintEvent(QPaintEvent* _event)
        {
            QPainter p(this);
            p.setRenderHints(QPainter::Antialiasing);
            p.setPen(Qt::NoPen);
            p.setBrush(bgColor_);
            p.drawRect(rect());
        }

        bool ScreenBorderLine::isOverlapped() const
        {
            if (!isVisible())
                return false;

#ifdef _WIN32
            auto hwnd = reinterpret_cast<HWND>(winId());

            RECT r;
            GetWindowRect(hwnd, &r);

            const int ptsNumX = r.right - r.left;
            const int ptsNumY = r.bottom - r.top;

            std::vector<POINT> pts;
            pts.reserve(ptsNumY * ptsNumX);

            for (int j = 0; j < ptsNumY; ++j)
            {
                int ptY = r.top + j;
                for (int i = 0; i < ptsNumX; ++i)
                    pts.emplace_back(POINT{ r.left + i, ptY });
            }

            for (const auto& point : pts)
                if (WindowFromPoint(point) != hwnd)
                    return true;
#endif

            return false;
        }

        ShareScreenFrame::ShareScreenFrame(std::string_view _uid, const QColor& _color, qreal _opacity)
            : QObject()
        {
            Qt::WindowFlags wFlags = Qt::Tool | Qt::FramelessWindowHint /*| Qt::WindowTransparentForInput*/ | Qt::WindowDoesNotAcceptFocus;
            if constexpr (platform::is_linux())
                wFlags |= Qt::BypassWindowManagerHint;
            else if constexpr (platform::is_windows())
                wFlags |= Qt::WindowStaysOnTopHint;
            else
                wFlags |= Qt::WindowStaysOnTopHint | Qt::NoDropShadowWindowHint;

            auto color = _color;
            color.setAlphaF(_opacity);

            borderLines_.reserve(4);
            borderLines_.emplace_back(new ScreenBorderLine(ScreenBorderLine::LinePosition::Left, wFlags, color));
            borderLines_.emplace_back(new ScreenBorderLine(ScreenBorderLine::LinePosition::Top, wFlags, color));
            borderLines_.emplace_back(new ScreenBorderLine(ScreenBorderLine::LinePosition::Right, wFlags, color));
            borderLines_.emplace_back(new ScreenBorderLine(ScreenBorderLine::LinePosition::Bottom, wFlags, color));

//          wFlags &= ~Qt::WindowTransparentForInput;
            buttonStopScreenSharing_ = new ClickableTextWidget(nullptr, getStopScreenSharingFont(), Styling::StyleVariable::BASE_GLOBALBLACK_PERMANENT);
            buttonStopScreenSharing_->installEventFilter(this);
            buttonStopScreenSharing_->overrideWindowFlags(wFlags);
            buttonStopScreenSharing_->setAttribute(Qt::WA_NoSystemBackground);
            buttonStopScreenSharing_->setCursor(Qt::PointingHandCursor);
            buttonStopScreenSharing_->setBackgroundColor(_color);
            buttonStopScreenSharing_->setWindowOpacity(_opacity);
            buttonStopScreenSharing_->setFixedHeight(getStopScreenSharingHeight());
            buttonStopScreenSharing_->setText(QT_TRANSLATE_NOOP("voip_video_panel", "Stop screen sharing"));
            buttonStopScreenSharing_->setLeftPadding(getStopScreenSharingTextPadding());
            buttonStopScreenSharing_->setFixedWidth(buttonStopScreenSharing_->sizeHint().width() + 2 * getStopScreenSharingTextPadding());
            connect(buttonStopScreenSharing_, &ClickableWidget::clicked, this, [this]() { Q_EMIT stopScreenSharing(QPrivateSignal()); });

            auto sharedScreen = QGuiApplication::primaryScreen();
            auto frameGeometry = sharedScreen->geometry();
            if constexpr (platform::is_linux())
            {
                frameGeometry = sharedScreen->virtualGeometry();
            }
            else
            {
                if (const auto voipScreens = GetDispatcher()->getVoipController().screenList(); voipScreens.size() > 1)
                {
                    auto voipScreen = std::find_if(voipScreens.begin(), voipScreens.end(), [_uid](const auto& _desc) { return _desc.uid == _uid; });
                    if (voipScreen != voipScreens.end())
                    {
                        const auto screens = QGuiApplication::screens();
                        if (int index = std::distance(voipScreens.begin(), voipScreen); index < screens.size())
                            frameGeometry = screens[index]->geometry();
                    }
                }
            }

            connect(sharedScreen, &QScreen::geometryChanged, this, &ShareScreenFrame::screenGeometryChanged);
            connect(sharedScreen, &QScreen::availableGeometryChanged, this, &ShareScreenFrame::screenGeometryChanged);
            connect(sharedScreen, &QScreen::virtualGeometryChanged, this, &ShareScreenFrame::screenGeometryChanged);

            screenGeometryChanged(frameGeometry);

            auto toast = new Toast(QT_TRANSLATE_NOOP("voip_video_panel", "Screen sharing enabled"));
            toast->setBackgroundColor(_color);
            toast->enableMoveAnimation(false);
            ToastManager::instance()->showToast(toast, frameGeometry.center());
        }

        ShareScreenFrame::~ShareScreenFrame()
        {
            ToastManager::instance()->hideToast();

            for (auto& line : borderLines_)
            {
                if (line)
                    line->close();
            }

            if (buttonStopScreenSharing_)
                buttonStopScreenSharing_->deleteLater();
        }

        void ShareScreenFrame::screenGeometryChanged(const QRect& _rect)
        {
            for (auto& line : borderLines_)
            {
                line->updateForGeometry(_rect);
#ifdef __APPLE__
                MacSupport::showInAllWorkspaces(line);
#else
                line->show();
#endif
            }

            if (buttonStopScreenSharing_)
            {
                uint posY = _rect.y() + getFrameBorderWidth();
                if constexpr (platform::is_linux())
                {
                    if (uint y = QGuiApplication::primaryScreen()->availableVirtualGeometry().y(); y > posY)
                        posY = y;
                }
                const auto pos = QPoint(_rect.center().x() - buttonStopScreenSharing_->width() / 2, posY);
                buttonStopScreenSharing_->move(pos);

#ifdef __APPLE__
                MacSupport::showInAllWorkspaces(buttonStopScreenSharing_);
#else
                buttonStopScreenSharing_->show();
#endif
            }
        }
    }

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
        , rootWidget_(nullptr)
        , stopCallButton_(nullptr)
        , videoButton_(nullptr)
        , moreButton_(nullptr)
        , addButton_(nullptr)
        , isFadedVisible_(false)
        , doPreventFadeIn_(false)
        , localVideoEnabled_(false)
        , isScreenSharingEnabled_(false)
        , isCameraEnabled_(true)
        , isParentMinimizedFromHere_(false)
        , isBigConference_(false)
        , isMasksAllowed_(true)
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

        shareScreenButton_ = addAndCreateButton(getScreensharingButtonText(PanelButton::ButtonStyle::Transparent), qsl(":/voip/sharescreen_icon"),
                                                PanelButton::ButtonStyle::Transparent, false,
                                                &VideoPanel::onShareScreen);

        addButton_ = addAndCreateButton(QT_TRANSLATE_NOOP("voip_video_panel", "Invite"), qsl(":/voip/add_contact"), PanelButton::ButtonStyle::Transparent, false, &VideoPanel::addButtonClicked);

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
    }

    VideoPanel::~VideoPanel()
    {
        closeMenu();
        hideScreenBorder();
        hideToast();
    }

    void VideoPanel::keyReleaseEvent(QKeyEvent* _e)
    {
        QWidget::keyReleaseEvent(_e);
        if (_e->key() == Qt::Key_Escape)
            Q_EMIT onkeyEscPressed();
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
                addMenuAction(MenuAction::ShareScreen, ql1c(' ') % QString::number(i + 1), [this, i]() { switchShareScreen(i); });

            showMenuAtButton(shareScreenButton_);

            resetPanelButtons(false);
        }
        else
        {
            switchShareScreen(0);
        }

        if (isScreenSharingEnabled_)
            Q_EMIT onShareScreenClickOn();
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
        bool showPopup = false;
        bool video = GetDispatcher()->getVoipController().checkPermissions(false, true, &showPopup);
        if (showPopup)
        {
            Q_EMIT needShowScreenPermissionsPopup(media::permissions::DeviceType::Camera);
            return;
        }
        if (!video)
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
                            showScreenBorder(std::move(uid));
                        });
                        return;
                    }
                }

                parent_->showMinimized();
                showScreenBorder(_desc.uid);
            }
        }
        else
        {
            hideScreenBorder();

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

        if (isScreenSharingEnabled_)
            shareScreenButton_->updateStyle(PanelButton::ButtonStyle::Normal, qsl(":/voip/sharescreen_icon"), getScreensharingButtonText(PanelButton::ButtonStyle::Normal));
        else
            shareScreenButton_->updateStyle(PanelButton::ButtonStyle::Transparent, qsl(":/voip/sharescreen_icon"), getScreensharingButtonText(PanelButton::ButtonStyle::Transparent));
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
                return QT_TRANSLATE_NOOP("voip_video_panel", "Invite");

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

    void VideoPanel::switchShareScreen(unsigned int _index)
    {
        const auto& screens = GetDispatcher()->getVoipController().screenList();

        const auto switchSharingImpl = [this, &screens](unsigned int _index)
        {
            isScreenSharingEnabled_ = !isScreenSharingEnabled_;
            GetDispatcher()->getVoipController().switchShareScreen(!screens.empty() && _index < static_cast<unsigned int>(screens.size()) ? &screens[_index] : nullptr);
            updateVideoDeviceButtonsState();
        };

        if constexpr (platform::is_apple())
        {
            if (!isScreenSharingEnabled_)
            {
                if (media::permissions::checkPermission(media::permissions::DeviceType::Screen) == media::permissions::Permission::Allowed)
                {
                    switchSharingImpl(_index);
                }
                else
                {
                    media::permissions::requestPermission(media::permissions::DeviceType::Screen, [](bool) {});
                    Q_EMIT needShowScreenPermissionsPopup(media::permissions::DeviceType::Screen);
                }
            }
            else
            {
                switchSharingImpl(_index);
            }
        }
        else
        {
            switchSharingImpl(_index);
        }
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

    void VideoPanel::showScreenBorder(std::string_view _uid)
    {
        if (GetDispatcher()->getVoipController().isWebinar())
            return;

        hideScreenBorder();

        shareScreenFrame_ = new VideoPanelParts::ShareScreenFrame(_uid, getFrameBorderColor());
        connect(shareScreenFrame_, &VideoPanelParts::ShareScreenFrame::stopScreenSharing, this, &VideoPanel::onShareScreen);
    }

    void VideoPanel::hideScreenBorder()
    {
        if (shareScreenFrame_)
            shareScreenFrame_->deleteLater();
    }

    void VideoPanel::showToast(const QString& _text, int _maxLineCount)
    {
        hideToast();

        if (parent_)
        {
            toast_ = new Toast(_text, parent_, _maxLineCount);
            toast_->setAttribute(Qt::WA_ShowWithoutActivating);
            if constexpr (platform::is_windows() || platform::is_apple())
                toast_->setWindowFlags(windowFlags());
            else if constexpr (platform::is_linux())
                toast_->setWindowFlags(Qt::Tool | Qt::FramelessWindowHint | Qt::BypassWindowManagerHint);

            const auto videoWindowRect = parent_->geometry();
            const auto yPos = platform::is_linux() ?
            videoWindowRect.bottom() - height() :
            (isFadedVisible_ ? geometry().y() : videoWindowRect.bottom() - toast_->height());
            ToastManager::instance()->showToast(toast_, QPoint(videoWindowRect.center().x(), yPos - getToastVerOffset()));
        }
    }

    bool VideoPanel::showPermissionPopup()
    {
        bool showPopup = false;
        Ui::GetDispatcher()->getVoipController().checkPermissions(true, false, &showPopup);
        if (showPopup)
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
        const auto isAddContact = !GetDispatcher()->getVoipController().isCallVCS() || GetDispatcher()->getVoipController().isCallPinnedRoom();
        const auto icon = isAddContact ? qsl(":/voip/add_contact") : qsl(":/voip/call_share");
        addButton_->updateStyle(PanelButton::ButtonStyle::Transparent, icon, QT_TRANSLATE_NOOP("voip_video_panel", "Invite"));
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
        bool showPopup = false;
        GetDispatcher()->getVoipController().checkPermissions(true, false, &showPopup);
        if (showPopup)
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

    void VideoPanel::callDestroyed()
    {
        isBigConference_ = false;
        isMasksAllowed_ = true;
        hideScreenBorder();
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

        addMenuAction(MenuAction::OpenMasks, &VideoPanel::onClickOpenMasks);

        if (GetDispatcher()->getVoipController().isCallVCS())
            addMenuAction(MenuAction::CopyVCSLink, &VideoPanel::onClickCopyVCSLink);

        showMenuAtButton(moreButton_);

        resetPanelButtons(false);
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
            case voip_proxy::kvoipDevTypeAudioPlayback: settingsName = ql1s(settings_speakers);    break;
            case voip_proxy::kvoipDevTypeAudioCapture:  settingsName = ql1s(settings_microphone); break;
            case voip_proxy::kvoipDevTypeVideoCapture:  settingsName = ql1s(settings_webcam);      break;
            case voip_proxy::kvoipDevTypeUndefined:
            default:
                assert(!"unexpected device type");
                return;
        };


        const auto uid = QString::fromStdString(_device.uid);
        if (get_gui_settings()->get_value<QString>(settingsName, QString()) != uid)
            get_gui_settings()->set_value<QString>(settingsName, uid);
    }
}