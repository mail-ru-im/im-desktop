#include "stdafx.h"
#include "ScreenFrame.h"
#include "../styles/ThemeParameters.h"
#include "../styles/StyleVariable.h"
#include "../fonts.h"
#include "../previewer/toast.h"
#ifdef __APPLE__
#include "../utils/macos/mac_support.h"
#endif

#include "../core_dispatcher.h"

namespace
{
    constexpr std::chrono::milliseconds kAnimationDuration(150);
    constexpr std::chrono::milliseconds kMessageTimeout(4000);

    auto frameBorderColor(qreal _opacity = 1.0) noexcept
    {
        return Styling::getParameters().getColorHex(Styling::StyleVariable::SECONDARY_RAINBOW_YELLOW, _opacity);
    }

    auto frameFont() noexcept
    {
        return Fonts::appFontScaled(12, Fonts::FontWeight::Normal);
    }

    auto frameBorderWidth() noexcept
    {
        return Utils::scale_value(2);
    }

    auto textHPadding() noexcept
    {
        return Utils::scale_value(6);
    }

    auto buttonHeight() noexcept
    {
        return Utils::scale_value(19);
    }

    QString frameStyleSheet() noexcept
    {
        return qsl("border: %1px solid %2; background: transparent;").arg(frameBorderWidth()).arg(frameBorderColor());
    }

    QString buttonStyleSheet() noexcept
    {
        return qsl("padding-top: 0px; padding-left: %1px; padding-right: %1px; border: none; "
                   "background: %2;").arg(textHPadding()).arg(frameBorderColor());
    }
} // end anonimous namespace

namespace Ui
{

class ScreenFramePrivate
{
public:
    QScreen* screen_;
    QPropertyAnimation* fade_;
    ScreenFrame::Features features_;
    double opacity_;

    ScreenFramePrivate(QScreen* _screen, QWidget* _parent)
        : screen_(nullptr)
        , fade_(nullptr)
        , features_(ScreenFrame::NoFeatures)
    {
        if (_screen)
            screen_ = _screen;
        else if (_parent)
            screen_ = _parent->screen();
        else
            screen_ = qApp->primaryScreen();
    }

    void init(ScreenFrame* _q)
    {
        assert(screen_ != nullptr);
        QObject::connect(screen_, &QScreen::geometryChanged, _q, &ScreenFrame::changeGeometry);

        fade_ = new QPropertyAnimation(_q, "windowOpacity", _q);
        fade_->setDuration(kAnimationDuration.count());
        fade_->setStartValue(0.0);
        fade_->setEndValue(0.5);
        QObject::connect(fade_, &QPropertyAnimation::valueChanged, _q, &ScreenFrame::fade);
        QObject::connect(fade_, &QPropertyAnimation::finished, _q, &ScreenFrame::disappear);

        if constexpr (platform::is_linux())
        {
            // Give a special attributes on Linux:
            // - Qt::WA_X11NetWmWindowTypeDesktop allows window to occupy all screen geometry
            // - Qt::WA_X11NetWmWindowTypeToolTip allows window to be on top of any other windows
            _q->setAttribute(Qt::WA_X11NetWmWindowTypeDesktop);
            _q->setAttribute(Qt::WA_X11NetWmWindowTypeToolTip);
        }
    }
};

ScreenFrame::ScreenFrame(QScreen* _screen, QWidget* _parent)
    : QFrame(nullptr, preferredWindowFlags())
    , d(std::make_unique<ScreenFramePrivate>(_screen, _parent))
{
    d->init(this);
}

ScreenFrame::ScreenFrame(QWidget* _parent)
    : ScreenFrame(_parent ? _parent->screen() : qApp->primaryScreen(), nullptr)
{
}

ScreenFrame::~ScreenFrame()
{
}

Qt::WindowFlags ScreenFrame::preferredWindowFlags()
{
    if constexpr (platform::is_apple())
        return Qt::FramelessWindowHint | Qt::Tool | Qt::NoDropShadowWindowHint | Qt::WindowDoesNotAcceptFocus | Qt::WindowStaysOnTopHint;
    else if constexpr (platform::is_windows())
        return Qt::FramelessWindowHint | Qt::ToolTip | Qt::NoDropShadowWindowHint | Qt::WindowDoesNotAcceptFocus | Qt::WindowStaysOnTopHint;
    else if constexpr (platform::is_linux())
        return Qt::FramelessWindowHint | Qt::ToolTip;
    else
        return Qt::FramelessWindowHint | Qt::ToolTip;
}

void ScreenFrame::setFeatures(ScreenFrame::Features _features)
{
    d->features_ = _features;
    if (d->features_ & (FadeIn | FadeOut))
        setWindowOpacity(static_cast<double>(isVisible()));
}

ScreenFrame::Features ScreenFrame::features() const
{
    return d->features_;
}

void ScreenFrame::setScreen(QScreen* _screen)
{
    if (_screen)
    {
        disconnect(d->screen_, &QScreen::geometryChanged, this, &ScreenFrame::changeGeometry);
        d->screen_ = _screen;
        changeGeometry(_screen->geometry());
        connect(d->screen_, &QScreen::geometryChanged, this, &ScreenFrame::changeGeometry);
    }
}

void ScreenFrame::changeGeometry(const QRect& _screenRect)
{
    if (geometry() == _screenRect)
        return;

    if constexpr (platform::is_apple())
    {
        // We need to delay actual geometry changes, since
        // MacOS is unable process screen changes so fast
        QTimer::singleShot(std::chrono::milliseconds(1000), this, [this, _screenRect]()
        {
            setGeometry(_screenRect);
        });
    }
    else
    {
        setGeometry(_screenRect);
        updateMask();
    }
}

void ScreenFrame::fade(const QVariant& _opacity)
{
    setWindowOpacity(_opacity.toDouble());
    update();
}

void ScreenFrame::fadeIn()
{
    d->fade_->stop();
    d->fade_->setCurrentTime(0);
    d->fade_->setDirection(QPropertyAnimation::Forward);
    d->fade_->start();
}

void ScreenFrame::fadeOut()
{
    d->fade_->stop();
    d->fade_->setCurrentTime(0);
    d->fade_->setDirection(QPropertyAnimation::Backward);
    d->fade_->start();
}

void ScreenFrame::disappear()
{
    if (d->fade_->direction() == QPropertyAnimation::Backward)
        close();
}

void ScreenFrame::paintEvent(QPaintEvent*)
{
    QStyleOptionFrame opt;
    initStyleOption(&opt);

    QStylePainter painter(this);
    painter.drawPrimitive(QStyle::PE_Frame, opt);
}

void ScreenFrame::showEvent(QShowEvent* _event)
{
#ifdef __APPLE__
    MacSupport::showInAllWorkspaces(this);
#endif
    if (geometry() != d->screen_->geometry())
        setGeometry(d->screen_->geometry());

    if (d->features_ & FadeIn)
        fadeIn();
}

void ScreenFrame::closeEvent(QCloseEvent* _event)
{
    Q_EMIT closed();
    if (!(d->features_ & FadeOut))
    {
        QWidget::closeEvent(_event);
        return;
    }

    switch (d->fade_->direction())
    {
    case QPropertyAnimation::Forward:
        if (d->fade_->state() == QPropertyAnimation::Running)
            d->fade_->stop();
        break;
    case QPropertyAnimation::Backward:
        if (d->fade_->state() == QPropertyAnimation::Stopped)
        {
            QWidget::closeEvent(_event);
            return;
        }
        break;
    }

    if (d->fade_->state() != QPropertyAnimation::Running)
        fadeOut();

    _event->ignore();
}

void ScreenFrame::resizeEvent(QResizeEvent* _event)
{
    QFrame::resizeEvent(_event);
    updateMask();
}

void ScreenFrame::moveEvent(QMoveEvent* _event)
{
    QFrame::moveEvent(_event);
    updateMask();
}

void ScreenFrame::updateMask()
{
    QRegion mask = rect();
    int w = frameWidth();
    mask -= rect().adjusted(w, w, -w, -w);
    const auto children = findChildren<QWidget*>();
    for (auto child : children)
        if (child->isVisible())
            mask |= child->geometry();

    setMask(mask);
    repaint();
}

class SharingScreenFramePrivate
{
public:
    QPushButton* closeButton_;
    QVBoxLayout* layout_;
    QPointer<Toast> toastPtr_;
};

SharingScreenFrame::SharingScreenFrame(QScreen* _screen, QWidget* _parent)
    : ScreenFrame(_screen, _parent)
    , d(std::make_unique<SharingScreenFramePrivate>())
{
    init();
}

SharingScreenFrame::SharingScreenFrame(QWidget* _parent)
    : ScreenFrame(_parent)
    , d(std::make_unique<SharingScreenFramePrivate>())
{
    init();
}

SharingScreenFrame::~SharingScreenFrame() = default;

void SharingScreenFrame::clearMessage()
{
    ToastManager::instance()->hideToast();
}

void SharingScreenFrame::hideMessage()
{
    ToastManager::instance()->hideToast();
}

void SharingScreenFrame::resizeEvent(QResizeEvent* _event)
{
    // For Linux OS we add margins for system areas, since currently
    // we cannot intercept any input events in that areas.
    // Workaround for this can be implemented in near future.
    if constexpr (platform::is_linux())
    {
        QRect screenRect = screen()->geometry();
        QRect availRect = screen()->availableGeometry();
        QMargins margins;
        margins.setLeft(availRect.left() - screenRect.left());
        margins.setRight(screenRect.right() - availRect.right());
        margins.setTop(availRect.top() - screenRect.top());
        margins.setBottom(screenRect.bottom() - availRect.bottom());
        layout()->setContentsMargins(margins);
    }
    ScreenFrame::resizeEvent(_event);
}

void SharingScreenFrame::showMessage(const QString& _text, std::chrono::milliseconds _timeout)
{
    auto toast = new Toast(_text);
    toast->setBackgroundColor(frameBorderColor());
    toast->enableMoveAnimation(false);
    toast->setVisibilityDuration(_timeout);
    ToastManager::instance()->showToast(toast, geometry().center());
    connect(this, &ScreenFrame::closed, ToastManager::instance(), &ToastManager::hideToast);
}

void SharingScreenFrame::init()
{
    setAutoFillBackground(false);
    setAttribute(Qt::WA_TranslucentBackground, true);
    setAttribute(Qt::WA_OpaquePaintEvent, false);
    setAttribute(Qt::WA_TransparentForMouseEvents, true);
    setStyleSheet(frameStyleSheet());
    setFont(frameFont());

    d->closeButton_ = new QPushButton(this);
    d->closeButton_->setFixedHeight(buttonHeight());
    d->closeButton_->setCursor(Qt::PointingHandCursor);
    d->closeButton_->setFont(frameFont());
    d->closeButton_->setText(QT_TRANSLATE_NOOP("voip_video_panel", "Stop screen sharing"));
    d->closeButton_->setStyleSheet(buttonStyleSheet());
    d->closeButton_->setAutoFillBackground(false);
    d->closeButton_->setAttribute(Qt::WA_TranslucentBackground, true);
    d->closeButton_->setAttribute(Qt::WA_OpaquePaintEvent, false);
    d->closeButton_->setAttribute(Qt::WA_TransparentForMouseEvents, false);
    connect(d->closeButton_, &QPushButton::clicked, this, &ScreenFrame::close);

    d->layout_ = new QVBoxLayout(this);
    d->layout_->setContentsMargins(0, 0, 0, 0);
    d->layout_->setSpacing(0);
    d->layout_->addWidget(d->closeButton_, 0, Qt::AlignTop | Qt::AlignHCenter);
}

QScreen* sharingScreen(std::string_view _uid)
{
    QScreen* sharedScreen = QGuiApplication::primaryScreen();
    if (const auto voipScreens = Ui::GetDispatcher()->getVoipController().screenList(); voipScreens.size() > 1)
    {
        auto voipScreen = std::find_if(voipScreens.begin(), voipScreens.end(), [_uid](const auto& _desc) { return _desc.uid == _uid; });
        if (voipScreen != voipScreens.end())
        {
            const auto screens = QGuiApplication::screens();
            if (int index = std::distance(voipScreens.begin(), voipScreen); index < screens.size())
                sharedScreen = screens[index];
        }
    }
    return sharedScreen;
}

ScreenSharingManager::ScreenSharingManager()
    : QObject(nullptr)
    , currentIndex_(-1)
    , isCameraSelected_(false)

{
    connect(&Ui::GetDispatcher()->getVoipController(), &voip_proxy::VoipController::onVoipVideoDeviceSelected, this, &ScreenSharingManager::onVoipVideoDeviceSelected);
}

ScreenSharingManager& ScreenSharingManager::instance()
{
    static ScreenSharingManager inst;
    return inst;
}

void ScreenSharingManager::startSharing(int _screenIndex)
{
    auto& voipController = Ui::GetDispatcher()->getVoipController();
    const auto& screens = voipController.screenList();
    if (_screenIndex >= screens.size())
        return; // invalid voip screen index or screen list is empty

    const auto device = &screens[_screenIndex];

    if (voipController.isWebinar())
    {
        currentIndex_ = _screenIndex;
        voipController.switchShareScreen(device);
        Q_EMIT sharingStateChanged(SharingState::Shared, currentIndex_);
        return; // disable sharing frame for webinar mode
    }

    QScreen* screen = sharingScreen(device->uid);
    if (frame_ == nullptr)
    {
        frame_ = new SharingScreenFrame(screen, nullptr);
        frame_->setFeatures(ScreenFrame::FadeIn | ScreenFrame::FadeOut);
        frame_->setAttribute(Qt::WA_DeleteOnClose);
        connect(frame_, &SharingScreenFrame::destroyed, this, [this]()
        {
             if (!isCameraSelected_)
                 Ui::GetDispatcher()->getVoipController().switchShareScreen(nullptr);
             Q_EMIT sharingStateChanged(SharingState::NonShared, -1);
        });
        frame_->show();
    }
    else
    {
        if (currentIndex_ == _screenIndex)
            return;

        frame_->setScreen(screen);
    }

    currentIndex_ = _screenIndex;
    voipController.switchShareScreen(device);

    Q_EMIT sharingStateChanged(SharingState::Shared, currentIndex_);

    frame_->showMessage(QT_TRANSLATE_NOOP("voip_video_panel", "Screen sharing enabled"), kMessageTimeout);
}

void ScreenSharingManager::stopSharing()
{
    if (frame_)
        frame_->close();  // the sharingStateChanged() signal will be emitted after frame closed
}

void ScreenSharingManager::toggleSharing(int _screenIndex)
{
    const auto switchSharingImpl = [this](unsigned int _index)
    {
        if (state() == SharingState::NonShared)
            startSharing(_index);
        else
            stopSharing();
    };

    if constexpr (platform::is_apple())
    {
        if (state() == SharingState::NonShared)
        {
            if (media::permissions::checkPermission(media::permissions::DeviceType::Screen) == media::permissions::Permission::Allowed)
            {
                switchSharingImpl(_screenIndex);
            }
            else
            {
                media::permissions::requestPermission(media::permissions::DeviceType::Screen, [](bool) {});
                Q_EMIT needShowScreenPermissionsPopup(media::permissions::DeviceType::Screen);
            }
        }
        else
        {
            switchSharingImpl(_screenIndex);
        }
    }
    else
    {
        switchSharingImpl(_screenIndex);
    }
}

void ScreenSharingManager::onVoipVideoDeviceSelected(const voip_proxy::device_desc& _desc)
{
    isCameraSelected_ = (_desc.dev_type == voip_proxy::kvoipDevTypeVideoCapture &&
                        (_desc.video_dev_type == voip_proxy::kvoipDeviceCamera || _desc.video_dev_type == voip_proxy::kvoipDeviceVirtualCamera));
    if (isCameraSelected_)
        stopSharing();
}

ScreenSharingManager::SharingState ScreenSharingManager::state() const
{
    return (frame_ == nullptr ? SharingState::NonShared : SharingState::Shared);
}

} // end namespace Ui
