#include "stdafx.h"
#include "VideoPanel.h"
#include "DetachedVideoWnd.h"
#include "../core_dispatcher.h"
#include "../main_window/MainPage.h"
#include "../main_window/MainWindow.h"
#include "../utils/gui_metrics.h"
#include "SelectionContactsForConference.h"

#include "../controls/ContextMenu.h"

#include "../previewer/GalleryFrame.h"
#include "../previewer/toast.h"
#include "../styles/ThemeParameters.h"

#ifdef __APPLE__
#include "../utils/macos/mac_support.h"
#endif

namespace
{
    auto getButtonSize() noexcept
    {
        return Utils::scale_value(QSize(64, 86));
    }

    auto getButtonCircleSize() noexcept
    {
        return Utils::scale_value(44);
    }

    auto getButtonIconSize() noexcept
    {
        return QSize(32, 32);
    }

    auto getButtonTextFont()
    {
        return Fonts::appFontScaled(12, Fonts::FontWeight::Medium);
    }

    auto getButtonCircleVerOffset() noexcept
    {
        return Utils::scale_value(12);
    }

    auto getButtonTextVerOffset() noexcept
    {
        return Utils::scale_value(64);
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

    auto getPanelButtonsHorOffset() noexcept
    {
        return Utils::scale_value(2);
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
        return std::chrono::milliseconds(1000);
    }

    constexpr std::chrono::milliseconds getCheckOverlappedInterval() noexcept
    {
        return std::chrono::milliseconds(500);
    }
}

namespace Ui::VideoPanelParts
{
    PanelButton::PanelButton(QWidget* _parent, const QString& _text, const QString& _iconName, ButtonStyle _style)
        : ClickableWidget(_parent)
        , iconName_(_iconName)
        , textNormal_(Styling::getParameters().getColor(Styling::StyleVariable::TEXT_SOLID_PERMANENT))
        , textHovered_(Styling::getParameters().getColor(Styling::StyleVariable::TEXT_SOLID_PERMANENT_HOVER))
        , textPressed_(Styling::getParameters().getColor(Styling::StyleVariable::TEXT_SOLID_PERMANENT_ACTIVE))
        , textUnit_(nullptr)
    {
        const auto size = getButtonSize();

        setFixedSize(size);

        if (!_text.isEmpty())
        {
            textUnit_ = TextRendering::MakeTextUnit(_text, {}, TextRendering::LinksVisible::DONT_SHOW_LINKS);
            textUnit_->init(getButtonTextFont(), textNormal_, QColor(), QColor(), QColor(), TextRendering::HorAligment::CENTER, 1);
            textUnit_->evaluateDesiredSize();

            if (textUnit_->cachedSize().width() > size.width())
            {
                textUnit_->getHeight(size.width());
                textUnit_->elide(size.width());
            }

            textUnit_->setOffsets((size.width() - textUnit_->cachedSize().width()) / 2, getButtonTextVerOffset());
        }

        connect(this, &PanelButton::hoverChanged, this, qOverload<>(&PanelButton::update));
        connect(this, &PanelButton::pressChanged, this, qOverload<>(&PanelButton::update));

        updateStyle(_style);
    }

    void PanelButton::updateStyle(ButtonStyle _style, const QString& _iconName)
    {
        QColor iconColor;
        const auto params = Styling::getParameters();

        if (!_iconName.isEmpty())
            iconName_ = _iconName;

        switch (_style)
        {
        case ButtonStyle::Normal:
            iconColor = params.getColor(Styling::StyleVariable::BASE_GLOBALBLACK_PERMANENT);
            circleNormal_ = params.getColor(Styling::StyleVariable::TEXT_SOLID_PERMANENT);
            circleHovered_ = params.getColor(Styling::StyleVariable::TEXT_SOLID_PERMANENT_HOVER);
            circlePressed_ = params.getColor(Styling::StyleVariable::TEXT_SOLID_PERMANENT_ACTIVE);
            break;

        case ButtonStyle::Transparent:
            iconColor = params.getColor(Styling::StyleVariable::TEXT_SOLID_PERMANENT);
            circleNormal_ = params.getColor(Styling::StyleVariable::LUCENT_TERTIARY);
            circleHovered_ = params.getColor(Styling::StyleVariable::LUCENT_TERTIARY, 0.25);
            circlePressed_ = params.getColor(Styling::StyleVariable::LUCENT_TERTIARY, 0.20);
            break;

        case ButtonStyle::Red:
            iconColor = params.getColor(Styling::StyleVariable::TEXT_SOLID_PERMANENT);
            circleNormal_ = params.getColor(Styling::StyleVariable::SECONDARY_ATTENTION);
            circleHovered_ = params.getColor(Styling::StyleVariable::SECONDARY_ATTENTION_HOVER);
            circlePressed_ = params.getColor(Styling::StyleVariable::SECONDARY_ATTENTION_ACTIVE);
            break;

        default:
            Q_UNREACHABLE();
            return;
        }

        if (!iconName_.isEmpty())
            icon_ = Utils::renderSvgScaled(iconName_, getButtonIconSize(), iconColor);

        update();
    }

    void PanelButton::paintEvent(QPaintEvent* _event)
    {
        QColor circleColor;
        QColor textColor;

        if (isEnabled() && isPressed())
        {
            circleColor = circlePressed_;
            textColor = textPressed_;
        }
        else if (isEnabled() && isHovered())
        {
            circleColor = circleHovered_;
            textColor = textHovered_;
        }
        else
        {
            circleColor = circleNormal_;
            textColor = textNormal_;
        }

        QPainter p(this);
        p.setRenderHints(QPainter::Antialiasing | QPainter::SmoothPixmapTransform);
        p.setPen(Qt::NoPen);

        const auto circlePlace = QRect((width() - getButtonCircleSize()) / 2, getButtonCircleVerOffset(), getButtonCircleSize(), getButtonCircleSize());
        if (circleColor.isValid())
        {
            p.setBrush(circleColor);
            p.drawEllipse(circlePlace);
        }

        const auto ratio = Utils::scale_bitmap_ratio();
        p.drawPixmap(circlePlace.x() + (circlePlace.width() - icon_.width() / ratio) / 2,
                     circlePlace.y() + (circlePlace.height() - icon_.height() / ratio) / 2,
                     icon_);

        if (textUnit_)
        {
            textUnit_->setColor(textColor);
            textUnit_->draw(p, Ui::TextRendering::VerPosition::TOP);
        }
    }

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

//        wFlags &= ~Qt::WindowTransparentForInput;
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

        auto toast = new Ui::Toast(QT_TRANSLATE_NOOP("voip_video_panel", "Screen sharing enabled"));
        toast->setBackgroundColor(_color);
        toast->enableMoveAnimation(false);
        Ui::ToastManager::instance()->showToast(toast, frameGeometry.center());
    }

    ShareScreenFrame::~ShareScreenFrame()
    {
        Ui::ToastManager::instance()->hideToast();

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

Ui::VideoPanel::VideoPanel(QWidget* _parent, QWidget* _container)
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
    , isFadedVisible_(false)
    , doPreventFadeIn_(false)
    , localVideoEnabled_(false)
    , isScreenSharingEnabled_(false)
    , isCameraEnabled_(true)
    , isConferenceAll_(true)
    , isActiveCall_(false)
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
    rootWidget_->setContentsMargins(getPanelButtonsHorOffset(), 0, getPanelButtonsHorOffset(), 0);
    rootWidget_->setStyleSheet(qsl("background-color: %1; border-radius: %2;").arg(getPanelBackgroundColorHex()).arg(getPanelRoundedCornerRadius()));

    if constexpr (platform::is_linux())
        mainLayout->addStretch(1);

    mainLayout->addWidget(rootWidget_);

    if constexpr (platform::is_linux())
        mainLayout->addStretch(1);

    auto rootLayout = Utils::emptyHLayout(rootWidget_);
    rootLayout->setAlignment(Qt::AlignTop);

    auto addAndCreateButton = [this, &rootLayout] (const QString& _name, const QString& _icon, VideoPanelParts::PanelButton::ButtonStyle _style, auto _slot) -> VideoPanelParts::PanelButton*
    {
        auto btn = new VideoPanelParts::PanelButton(rootWidget_, _name, _icon, _style);
        rootLayout->addWidget(btn);
        connect(btn, &VideoPanelParts::PanelButton::clicked, this, std::forward<decltype(_slot)>(_slot));
        return btn;
    };

    videoButton_ = addAndCreateButton(QT_TRANSLATE_NOOP("voip_video_panel", "Video"), qsl(":/voip/videocall_icon"),
                                      VideoPanelParts::PanelButton::ButtonStyle::Normal,
                                      &VideoPanel::onVideoOnOffClicked);

    microphoneButton_ = addAndCreateButton(QT_TRANSLATE_NOOP("voip_video_panel", "Mic"), qsl(":/voip/microphone_icon"),
                                           VideoPanelParts::PanelButton::ButtonStyle::Transparent,
                                           &VideoPanel::onCaptureAudioOnOffClicked);

    speakerButton_ = addAndCreateButton(QT_TRANSLATE_NOOP("voip_video_panel", "Sound"), qsl(":/voip/speaker_icon"),
                                        VideoPanelParts::PanelButton::ButtonStyle::Normal,
                                        &VideoPanel::onSpeakerOnOffClicked);

    shareScreenButton_ = addAndCreateButton(QT_TRANSLATE_NOOP("voip_video_panel", "Screen"), qsl(":/voip/sharescreen_icon"),
                                            VideoPanelParts::PanelButton::ButtonStyle::Transparent,
                                            &VideoPanel::onShareScreen);

    stopCallButton_ = addAndCreateButton(QT_TRANSLATE_NOOP("voip_video_panel", "End"), qsl(":/voip/endcall_icon"),
                                         VideoPanelParts::PanelButton::ButtonStyle::Red,
                                         &VideoPanel::onHangUpButtonClicked);

    moreButton_ = addAndCreateButton(QT_TRANSLATE_NOOP("voip_video_panel", "More"), qsl(":/voip/morewide_icon"),
                                     VideoPanelParts::PanelButton::ButtonStyle::Transparent,
                                     &VideoPanel::onMoreButtonClicked);

    connect(&Ui::GetDispatcher()->getVoipController(), &voip_proxy::VoipController::onVoipMediaLocalVideo, this, &VideoPanel::onVoipMediaLocalVideo);
    connect(&Ui::GetDispatcher()->getVoipController(), &voip_proxy::VoipController::onVoipCallNameChanged, this, &VideoPanel::onVoipCallNameChanged);
    //QObject::connect(&Ui::GetDispatcher()->getVoipController(), SIGNAL(onVoipMinimalBandwidthChanged(bool)), this, SLOT(onVoipMinimalBandwidthChanged(bool)), Qt::DirectConnection);
    connect(&Ui::GetDispatcher()->getVoipController(), &voip_proxy::VoipController::onVoipVideoDeviceSelected, this, &VideoPanel::onVoipVideoDeviceSelected);
    connect(&Ui::GetDispatcher()->getVoipController(), &voip_proxy::VoipController::onVoipMediaLocalAudio, this, &VideoPanel::onVoipMediaLocalAudio);

    connect(&Ui::GetDispatcher()->getVoipController(), &voip_proxy::VoipController::onVoipVolumeChanged, this, &VideoPanel::onVoipVolumeChanged);
}

Ui::VideoPanel::~VideoPanel()
{
    closeMenu();
    hideScreenBorder();
    hideToast();
}

void Ui::VideoPanel::keyReleaseEvent(QKeyEvent* _e)
{
    QWidget::keyReleaseEvent(_e);
    if (_e->key() == Qt::Key_Escape)
        Q_EMIT onkeyEscPressed();
}

void Ui::VideoPanel::paintEvent(QPaintEvent* _e)
{
    QWidget::paintEvent(_e);

    if constexpr (platform::is_linux())
    {
        // temporary solution for linux
        QPainter p(this);
        p.fillRect(rect(), getPanelBackgroundColorLinux());
    }
}

void Ui::VideoPanel::moveEvent(QMoveEvent* _e)
{
    QWidget::moveEvent(_e);
    closeMenu();
    hideToast();
}

bool Ui::VideoPanel::event(QEvent* _e)
{
    if (_e->type() == QEvent::WindowDeactivate)
        hideToast();

    return QWidget::event(_e);
}

void Ui::VideoPanel::onClickGoChat()
{
    const auto chatRoomCall = GetDispatcher()->getVoipController().isCallPinnedRoom();
    std::string contact;
    if (chatRoomCall)
        contact = GetDispatcher()->getVoipController().getChatId();
    else if (!activeContact_.empty())
        contact = activeContact_[0].contact;


    if (!contact.empty())
    {
        Ui::GetDispatcher()->getVoipController().openChat(QString::fromStdString(contact));
        Q_EMIT onGoToChatButton();
    }
}

void Ui::VideoPanel::setContacts(std::vector<voip_manager::Contact> _contacts, bool _activeCall)
{
    activeContact_ = std::move(_contacts);
    isActiveCall_ = _activeCall;
}

void Ui::VideoPanel::enterEvent(QEvent* _e)
{
    QWidget::enterEvent(_e);
    Q_EMIT onMouseEnter();
}

void Ui::VideoPanel::leaveEvent(QEvent* _e)
{
    QWidget::leaveEvent(_e);
    Q_EMIT onMouseLeave();
}

void Ui::VideoPanel::resizeEvent(QResizeEvent* _e)
{
    QWidget::resizeEvent(_e);
    closeMenu();
    hideToast();
}

void Ui::VideoPanel::onHangUpButtonClicked()
{
    Ui::GetDispatcher()->getVoipController().setHangup();
}

void Ui::VideoPanel::onShareScreen()
{
    if (!Ui::GetDispatcher()->getVoipController().isLocalDesktopAllowed())
    {
        showToast(ToastType::DesktopNotAllowed);
        return;
    }

    const auto& screens = Ui::GetDispatcher()->getVoipController().screenList();

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

void Ui::VideoPanel::onVoipMediaLocalVideo(bool _enabled)
{
    localVideoEnabled_ = _enabled && Ui::GetDispatcher()->getVoipController().isCamPermissionGranted();
    updateVideoDeviceButtonsState();
    statistic::getGuiMetrics().eventVideocallStartCapturing();
}

void Ui::VideoPanel::changeEvent(QEvent* _e)
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

void Ui::VideoPanel::onVideoOnOffClicked()
{
    bool show_popup = false;
    bool video = Ui::GetDispatcher()->getVoipController().checkPermissions(false, true, &show_popup);
    if (show_popup)
    {
        Q_EMIT needShowScreenPermissionsPopup(media::permissions::DeviceType::Camera);
        return;
    }
    if (!video)
        return;
    if (!Ui::GetDispatcher()->getVoipController().isLocalCamAllowed())
    {
        showToast(ToastType::CamNotAllowed);
        return;
    }

    if (!(isCameraEnabled_ && localVideoEnabled_))
        Q_EMIT onCameraClickOn();

    Ui::GetDispatcher()->getVoipController().setSwitchVCaptureMute();
}

void Ui::VideoPanel::setVisible(bool _visible)
{
    if (!_visible)
    {
        closeMenu();
        hideToast();
    }

    QWidget::setVisible(_visible);
}

void Ui::VideoPanel::updatePosition(const QWidget& _parent)
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

void Ui::VideoPanel::fadeIn(unsigned int _kAnimationDefDuration)
{
    if (!isFadedVisible_ && !doPreventFadeIn_)
    {
        isFadedVisible_ = true;
        if constexpr (!platform::is_linux())
            BaseVideoPanel::fadeIn(_kAnimationDefDuration);
    }
}

void Ui::VideoPanel::fadeOut(unsigned int _kAnimationDefDuration)
{
    if (isFadedVisible_)
    {
        isFadedVisible_ = false;
        if constexpr (!platform::is_linux())
            BaseVideoPanel::fadeOut(_kAnimationDefDuration);
    }
}

bool Ui::VideoPanel::isFadedIn()
{
    return isFadedVisible_;
}

void Ui::VideoPanel::setPreventFadeIn(bool _doPrevent)
{
    doPreventFadeIn_ = _doPrevent;
}

bool Ui::VideoPanel::isActiveWindow()
{
    return QWidget::isActiveWindow();
}

void Ui::VideoPanel::onVoipVideoDeviceSelected(const voip_proxy::device_desc& _desc)
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

void Ui::VideoPanel::updateVideoDeviceButtonsState()
{
    if (localVideoEnabled_ && isCameraEnabled_)
        videoButton_->updateStyle(VideoPanelParts::PanelButton::ButtonStyle::Normal, qsl(":/voip/videocall_icon"));
    else
        videoButton_->updateStyle(VideoPanelParts::PanelButton::ButtonStyle::Transparent, qsl(":/voip/videocall_off_icon"));

    if (localVideoEnabled_ && isScreenSharingEnabled_)
        shareScreenButton_->updateStyle(VideoPanelParts::PanelButton::ButtonStyle::Normal);
    else
        shareScreenButton_->updateStyle(VideoPanelParts::PanelButton::ButtonStyle::Transparent);
}

template<typename Slot>
void Ui::VideoPanel::addMenuAction(Ui::VideoPanel::MenuAction _action, const QString& _addText, Slot&& _slot)
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
void Ui::VideoPanel::addMenuAction(Ui::VideoPanel::MenuAction _action, Slot&& _slot)
{
    addMenuAction(_action, {}, std::forward<Slot>(_slot));
}

QString Ui::VideoPanel::menuActionIconPath(MenuAction _action)
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

QString Ui::VideoPanel::menuActionText(MenuAction _action)
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
        return QT_TRANSLATE_NOOP("voip_video_panel", "Invite");

    default:
        Q_UNREACHABLE();
        return QString();
    }
}

void Ui::VideoPanel::resetMenu()
{
    if (menu_)
        menu_->deleteLater();

    menu_ = new Previewer::CustomMenu();
    connect(menu_, &QObject::destroyed, this, [this]() { resetPanelButtons(); });
}

void Ui::VideoPanel::showMenuAt(const QPoint& _pos)
{
    if (menu_)
        menu_->showAtPos(_pos);
}

void Ui::VideoPanel::showMenuAtButton(const VideoPanelParts::PanelButton* _button)
{
    const auto buttonRect = _button->rect();
    const auto pos = QPoint(buttonRect.x() + buttonRect.width() / 2, buttonRect.y() + getMenuVerOffset());
    showMenuAt(_button->mapToGlobal(pos));
}

void Ui::VideoPanel::closeMenu()
{
    if (menu_)
        menu_->close();
}

void Ui::VideoPanel::switchShareScreen(unsigned int _index)
{
    const auto& screens = Ui::GetDispatcher()->getVoipController().screenList();

    const auto switchSharingImpl = [this, &screens](unsigned int _index)
    {
        isScreenSharingEnabled_ = !isScreenSharingEnabled_;
        Ui::GetDispatcher()->getVoipController().switchShareScreen(!screens.empty() && _index < static_cast<unsigned int>(screens.size()) ? &screens[_index] : nullptr);
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

void Ui::VideoPanel::resetPanelButtons(bool _enable)
{
    const auto buttons = findChildren<VideoPanelParts::PanelButton*>();
    for (auto button : buttons)
    {
        button->setEnabled(_enable);
        button->setHovered(false);
        button->setPressed(false);
    }
}

void Ui::VideoPanel::showScreenBorder(std::string_view _uid)
{
    if (Ui::GetDispatcher()->getVoipController().isWebinar())
        return;

    hideScreenBorder();

    shareScreenFrame_ = new VideoPanelParts::ShareScreenFrame(_uid, getFrameBorderColor());
    connect(shareScreenFrame_, &VideoPanelParts::ShareScreenFrame::stopScreenSharing, this, &VideoPanel::onShareScreen);
}

void Ui::VideoPanel::hideScreenBorder()
{
    if (shareScreenFrame_)
        shareScreenFrame_->deleteLater();
}

void Ui::VideoPanel::showToast(const QString& _text, int _maxLineCount)
{
    hideToast();

    if (parent_)
    {
        toast_ = new Ui::Toast(_text, parent_, _maxLineCount);
        toast_->setAttribute(Qt::WA_ShowWithoutActivating);
        if constexpr (platform::is_windows() || platform::is_apple())
            toast_->setWindowFlags(windowFlags());
        else if constexpr (platform::is_linux())
            toast_->setWindowFlags(Qt::Tool | Qt::FramelessWindowHint | Qt::BypassWindowManagerHint);

        const auto videoWindowRect = parent_->geometry();
        const auto yPos = platform::is_linux() ?
            videoWindowRect.bottom() - height() :
            (isFadedVisible_ ? geometry().y() : videoWindowRect.bottom() - toast_->height());
        Ui::ToastManager::instance()->showToast(toast_, QPoint(videoWindowRect.center().x(), yPos - getToastVerOffset()));
    }
}

void Ui::VideoPanel::showToast(ToastType _type)
{
    switch (_type)
    {
    case ToastType::CamNotAllowed:
    case ToastType::DesktopNotAllowed:
    {
        if (Ui::GetDispatcher()->getVoipController().isWebinar())
        {
            showToast(QT_TRANSLATE_NOOP("voip_video_panel", "Only the creator of the webinar can show the video"));
        }
        else
        {
            const auto maxUsers = Ui::GetDispatcher()->getVoipController().maxUsersWithVideo();
            showToast(QT_TRANSLATE_NOOP("voip_video_panel", "Ask one of the participants to turn off the video. In calls with over %1 people only one video can be shown").arg(maxUsers), 2);
        }
        break;
    }
    case ToastType::MasksNotAllowed:
    {
        if (Ui::GetDispatcher()->getVoipController().isWebinar())
        {
            showToast(QT_TRANSLATE_NOOP("voip_video_panel", "Only the creator of the webinar can use masks"));
        }
        else
        {
            const auto usersBound = Ui::GetDispatcher()->getVoipController().lowerBoundForBigConference();
            showToast(QT_TRANSLATE_NOOP("voip_video_panel", "Masks aren't available in calls with more than %1 members").arg(usersBound), 2);
        }
        break;
    }
    case ToastType::MicNotAllowed:
    {
        if (Ui::GetDispatcher()->getVoipController().isWebinar())
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
    default:
        break;
    }
}

void Ui::VideoPanel::updateToastPosition()
{
    if (toast_)
    {
        const auto rc = parent_->geometry();
        toast_->move(QPoint(rc.center().x() - toast_->width() / 2, (!platform::is_linux() ? geometry().y() : rc.bottom() - height()) - toast_->height() - getToastVerOffset()));
    }
}

void Ui::VideoPanel::hideToast()
{
    if (toast_)
        Ui::ToastManager::instance()->hideToast();
}

void Ui::VideoPanel::onCaptureAudioOnOffClicked()
{
    bool show_popup = false;
    Ui::GetDispatcher()->getVoipController().checkPermissions(true, false, &show_popup);
    if (show_popup)
    {
        Q_EMIT needShowScreenPermissionsPopup(media::permissions::DeviceType::Microphone);
        return;
    }
    if (!Ui::GetDispatcher()->getVoipController().isLocalAudAllowed())
    {
        showToast(ToastType::MicNotAllowed);
        return;
    }

    Ui::GetDispatcher()->getVoipController().setSwitchACaptureMute();
    Q_EMIT onMicrophoneClick();
}

void Ui::VideoPanel::onVoipMediaLocalAudio(bool _enabled)
{
    if (microphoneButton_)
    {
        if (_enabled && Ui::GetDispatcher()->getVoipController().isAudPermissionGranted())
            microphoneButton_->updateStyle(VideoPanelParts::PanelButton::ButtonStyle::Normal, qsl(":/voip/microphone_icon"));
        else
            microphoneButton_->updateStyle(VideoPanelParts::PanelButton::ButtonStyle::Transparent, qsl(":/voip/microphone_off_icon"));
    }
}

bool Ui::VideoPanel::isPreventFadeOut() const
{
    return menu_ && menu_->isActiveWindow();
}

void Ui::VideoPanel::changeConferenceMode(voip_manager::VideoLayout _layout)
{
    isConferenceAll_ = _layout == voip_manager::AllTheSame;
}

void Ui::VideoPanel::callDestroyed()
{
    isBigConference_ = false;
    isMasksAllowed_ = true;
    hideScreenBorder();
}

void Ui::VideoPanel::onSpeakerOnOffClicked()
{
    Ui::GetDispatcher()->getVoipController().setSwitchAPlaybackMute();
    Q_EMIT onSpeakerClick();
}

void Ui::VideoPanel::onVoipVolumeChanged(const std::string& _deviceType, int _vol)
{
    if (_deviceType == "audio_playback")
    {
        if (_vol > 0)
            speakerButton_->updateStyle(VideoPanelParts::PanelButton::ButtonStyle::Normal, qsl(":/voip/speaker_icon"));
        else
            speakerButton_->updateStyle(VideoPanelParts::PanelButton::ButtonStyle::Transparent, qsl(":/voip/speaker_off_icon"));
    }
}

void Ui::VideoPanel::onMoreButtonClicked()
{
    resetMenu();

    if (!Ui::GetDispatcher()->getVoipController().isCallVCS())
    {
        if (activeContact_.size() == 1 || GetDispatcher()->getVoipController().isCallPinnedRoom())
            addMenuAction(MenuAction::GoToChat, &VideoPanel::onClickGoChat);

        addMenuAction(isConferenceAll_ ? MenuAction::ConferenceOneIsBig : MenuAction::ConferenceAllTheSame, &VideoPanel::onChangeConferenceMode);
    }

    addMenuAction(MenuAction::OpenMasks, &VideoPanel::onClickOpenMasks);

    if (isActiveCall_ && !Ui::GetDispatcher()->getVoipController().isCallVCS() || GetDispatcher()->getVoipController().isCallPinnedRoom())
        addMenuAction(MenuAction::AddUser, &VideoPanel::addUserToConference);

    if (Ui::GetDispatcher()->getVoipController().isCallVCS())
    {
        addMenuAction(MenuAction::CopyVCSLink, &VideoPanel::onClickCopyVCSLink);
        addMenuAction(MenuAction::InviteVCS, &VideoPanel::onClickInviteVCS);
    }

    addMenuAction(MenuAction::CallSettings, &VideoPanel::onClickSettings);

    showMenuAtButton(moreButton_);

    resetPanelButtons(false);
}

void Ui::VideoPanel::onChangeConferenceMode()
{
    isConferenceAll_ = !isConferenceAll_;
    Q_EMIT updateConferenceMode(isConferenceAll_ ? voip_manager::AllTheSame : voip_manager::OneIsBig);
}

void Ui::VideoPanel::onClickSettings()
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

void Ui::VideoPanel::onClickOpenMasks()
{
    if (!isMasksAllowed_)
    {
        showToast(ToastType::MasksNotAllowed);
        return;
    }

    Q_EMIT onShowMaskPanel();
}

void Ui::VideoPanel::onVoipCallNameChanged(const voip_manager::ContactsList& _contacts)
{
    const auto lowerBound = Ui::GetDispatcher()->getVoipController().lowerBoundForBigConference() - 1;
    isBigConference_ = _contacts.contacts.size() > static_cast<size_t>(lowerBound);

    if (isBigConference_ || !Ui::GetDispatcher()->getVoipController().isLocalCamAllowed())
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

void Ui::VideoPanel::onClickCopyVCSLink()
{
    if (const auto url = Ui::GetDispatcher()->getVoipController().getConferenceUrl(); url.isEmpty())
    {
        showToast(ToastType::EmptyLink);
    }
    else
    {
        QApplication::clipboard()->setText(url);
        showToast(ToastType::LinkCopied);
    }
}

void Ui::VideoPanel::onClickInviteVCS()
{
    if (const auto url = Ui::GetDispatcher()->getVoipController().getConferenceUrl(); url.isEmpty())
        showToast(ToastType::EmptyLink);
    else
        Q_EMIT inviteVCSUrl(url);
}
