#include "stdafx.h"
#include "DetachedVideoWnd.h"
#include "PanelButtons.h"
#include "../core_dispatcher.h"
#include "../utils/InterConnector.h"

#include "CommonUI.h"
#include "../controls/ContextMenu.h"
#include "../controls/ClickWidget.h"
#include "../controls/TooltipWidget.h"
#include "../main_window/MainWindow.h"
#include "../styles/ThemeParameters.h"
#include "../utils/graphicsEffects.h"

#include "../styles/ThemeParameters.h"

#include "VideoWindow.h"

#ifdef _WIN32
    #include <windows.h>
#endif

#ifdef __APPLE__
    #include "../utils/macos/mac_support.h"
#endif

namespace
{
    auto getWidth() noexcept
    {
        return Utils::scale_value(284);
    }

    auto getCollapsedHeight() noexcept
    {
        return Utils::scale_value(52);
    }
    auto headerHeight() noexcept
    {
        return Utils::scale_value(20);
    }
    constexpr auto windowBorderWidth() noexcept
    {
        return platform::is_apple() ? 1 : 0;;
    }
    auto videoHeight() noexcept
    {
        return Utils::scale_value(160);
    }
    constexpr auto videoMinimalHeight() noexcept
    {
        return platform::is_apple() ? 1 : 0;
    }

    auto getMaxHeight() noexcept
    {
        auto height = videoHeight() + getCollapsedHeight();
        if constexpr (platform::is_linux())
            height += headerHeight();
        return height;
    }

    auto panelsHeight() noexcept
    {
        if constexpr (platform::is_linux())
            return headerHeight() + getCollapsedHeight();
        else
            return getCollapsedHeight();
    }

    auto getDetachedVideoWindowSize() noexcept
    {
        return QSize(getWidth(), platform::is_linux() ? getMaxHeight() : videoHeight());
    }

    auto getPanelBackgroundColorLinux() noexcept
    {
        return QColor(38, 38, 38, 255);
    }

    auto getButtonsOffset() noexcept
    {
        return Utils::scale_value(4);
    }

    auto getFrameBorderColor()
    {
        return Styling::getParameters().getColor(Styling::StyleVariable::SECONDARY_RAINBOW_YELLOW);
    }

    auto getTooltipText()
    {
        return QT_TRANSLATE_NOOP("voip_pages", "Move the window\nor open call\nwith click");
    }

    auto getTooltipSize() noexcept
    {
        return Utils::scale_value(QSize(166, 77));
    }

    auto getBorderRadius() noexcept
    {
        return Utils::scale_value(8);
    }

    auto titleFont()
    {
        if constexpr (platform::is_apple())
            return Fonts::appFontScaled(13, Fonts::FontFamily::SF_PRO_TEXT, Fonts::FontWeight::Medium);
        else
            return Fonts::appFontScaled(14, Fonts::FontFamily::SOURCE_SANS_PRO, Fonts::FontWeight::SemiBold);
    }

    auto getCornerSpacing() noexcept
    {
        return Utils::scale_value(4);
    }

    constexpr std::chrono::milliseconds heightAnimDuration() noexcept { return std::chrono::milliseconds(150); }
}

void  Ui::MiniWindowVideoPanel::onResizeButtonClicked()
{
    changeResizeButtonTooltip();
    Q_EMIT onResizeClicked();
}

void  Ui::MiniWindowVideoPanel::onOpenCallButtonClicked()
{
    Q_EMIT onOpenCallClicked();
}

void Ui::MiniWindowVideoPanel::onHangUpButtonClicked()
{
    Q_EMIT onHangClicked();
    Ui::GetDispatcher()->getVoipController().setHangup();
}

void Ui::MiniWindowVideoPanel::onAudioOnOffClicked()
{
    if (Ui::GetDispatcher()->getVoipController().isAudPermissionGranted())
        Ui::GetDispatcher()->getVoipController().setSwitchACaptureMute();
    else
        Q_EMIT onMicrophoneClick();
}

void Ui::MiniWindowVideoPanel::onVideoOnOffClicked()
{
    GetDispatcher()->getVoipController().checkPermissions(false, true, true);
    if (!Ui::GetDispatcher()->getVoipController().isCamPermissionGranted())
        return;
    Ui::GetDispatcher()->getVoipController().setSwitchVCaptureMute();
}

void Ui::MiniWindowVideoPanel::onVoipMediaLocalAudio(bool _enabled)
{
    if (micButton_)
    {
        const auto style = _enabled ? PanelButton::ButtonStyle::Normal : PanelButton::ButtonStyle::Transparent;
        const auto icon = _enabled ? qsl(":/voip/microphone_icon") : qsl(":/voip/microphone_off_icon");
        micButton_->updateStyle(style, icon);
        micButton_->setTooltipText(getMicrophoneButtonText(style, PanelButton::ButtonSize::Small));
    }
}

void Ui::MiniWindowVideoPanel::onVoipMediaLocalVideo(bool _enabled)
{
    if (!videoButton_)
        return;

    localVideoEnabled_ = _enabled;
    updateVideoDeviceButtonsState();
}

void Ui::MiniWindowVideoPanel::onShareScreen()
{
    if (!Ui::GetDispatcher()->getVoipController().isLocalDesktopAllowed())
        return;

    const QList<voip_proxy::device_desc>& screens = Ui::GetDispatcher()->getVoipController().screenList();

    const auto switchSharingImpl = [this, &screens](const int _index)
    {
        isScreenSharingEnabled_ = !isScreenSharingEnabled_;
        Q_EMIT onShareScreenClick(isScreenSharingEnabled_);
        Ui::GetDispatcher()->getVoipController().switchShareScreen(!screens.empty() ? &screens[_index] : nullptr);
        updateVideoDeviceButtonsState();
    };

    const auto switchSharing = [this, &switchSharingImpl](const int _index)
    {
        if constexpr (platform::is_apple())
        {
            if (!isScreenSharingEnabled_)
            {
                const auto p = media::permissions::checkPermission(media::permissions::DeviceType::Screen);
                if (p == media::permissions::Permission::Allowed)
                {
                    switchSharingImpl(_index);
                }
                else
                {
                    media::permissions::requestPermission(media::permissions::DeviceType::Screen, [](bool){});
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
    };

    if (!isScreenSharingEnabled_ && screens.size() > 1)
    {
        Ui::ContextMenu menu(this);
        Ui::ContextMenu::applyStyle(&menu, false, Utils::scale_value(15), Utils::scale_value(36));
        for (int i = 0; i < screens.size(); i++)
        {
            menu.addAction(QT_TRANSLATE_NOOP("voip_pages", "Screen") % ql1c(' ') % QString::number(i + 1), [i, switchSharing]()
            {
                switchSharing(i);
            });
        }
        menu.exec(QCursor::pos());
    } else
    {
        switchSharing(0);
    }
}

void Ui::MiniWindowVideoPanel::onVoipVideoDeviceSelected(const voip_proxy::device_desc& _desc)
{
    isScreenSharingEnabled_ = (_desc.dev_type == voip_proxy::kvoipDevTypeVideoCapture && _desc.video_dev_type == voip_proxy::kvoipDeviceDesktop);
    isCameraEnabled_ = (_desc.dev_type == voip_proxy::kvoipDevTypeVideoCapture && _desc.video_dev_type == voip_proxy::kvoipDeviceCamera);

    updateVideoDeviceButtonsState();
}


Ui::MiniWindowVideoPanel::MiniWindowVideoPanel(QWidget* _parent)
    : BaseBottomVideoPanel(_parent)
    , parent_(_parent)
    , rootWidget_(nullptr)
    , micButton_(nullptr)
    , stopCallButton_(nullptr)
    , videoButton_(nullptr)
    , localVideoEnabled_(false)
    , isScreenSharingEnabled_(false)
    , isCameraEnabled_(true)
    , mouseUnderPanel_(false)
{
    setFixedHeight(getCollapsedHeight());
    auto rootLayout = Utils::emptyVLayout();

    rootLayout->setAlignment(Qt::AlignVCenter);
    setLayout(rootLayout);

    rootWidget_ = new QWidget(this);
    rootLayout->addWidget(rootWidget_);

    auto layoutTarget = Utils::emptyHLayout(rootWidget_);
    layoutTarget->setSpacing(getButtonsOffset());
    layoutTarget->setAlignment(Qt::AlignVCenter);

    auto addButton = [this, parent = rootWidget_, layoutTarget](const QString& _icon, PanelButton::ButtonStyle _style, auto _slot) -> PanelButton*
    {
        auto btn = new PanelButton(parent, QString(), _icon, PanelButton::ButtonSize::Small, _style);

        layoutTarget->addWidget(btn);

        if constexpr (platform::is_apple())
            connect(btn, &PanelButton::clicked, this, [this](){ if (!isActiveWindow()) activateWindow(); });

        connect(btn, &PanelButton::clicked, this, std::forward<decltype(_slot)>(_slot));

        return btn;
    };

    auto addTransparentButton = [this, parent = rootWidget_, layoutTarget](const QString& _icon, const QString& _text, Qt::Alignment _align, bool _isAnimated, auto _slot) -> TransparentPanelButton*
    {
        auto btn = new TransparentPanelButton(parent, _icon, _text, _align, _isAnimated);

        layoutTarget->addWidget(btn);

        if constexpr (platform::is_apple())
            connect(btn, &PanelButton::clicked, this, [this]() { if (!isActiveWindow()) activateWindow(); });

        connect(btn, &PanelButton::clicked, this, std::forward<decltype(_slot)>(_slot));

        return btn;
    };

    layoutTarget->addStretch(1);
    resizeButton_ = addTransparentButton(qsl(":/voip/resize"), QT_TRANSLATE_NOOP("voip_pages", "Minimize"), Qt::AlignRight, true, &MiniWindowVideoPanel::onResizeButtonClicked);
    micButton_ = addButton(qsl(":/voip/microphone_icon"), PanelButton::ButtonStyle::Transparent, &MiniWindowVideoPanel::onAudioOnOffClicked);
    videoButton_ = addButton(qsl(":/voip/videocall_icon"), PanelButton::ButtonStyle::Normal, &MiniWindowVideoPanel::onVideoOnOffClicked);
    shareScreenButton_ = addButton(qsl(":/voip/sharescreen_icon"), PanelButton::ButtonStyle::Transparent, &MiniWindowVideoPanel::onShareScreen);
    stopCallButton_ = addButton(qsl(":/voip/endcall_icon"), PanelButton::ButtonStyle::Red, &MiniWindowVideoPanel::onHangUpButtonClicked);
    stopCallButton_->setTooltipText(QT_TRANSLATE_NOOP("voip_video_panel_mini", "End meeting"));
    openCallButton_ = addTransparentButton(qsl(":/voip/maximize"), QT_TRANSLATE_NOOP("voip_pages", "Raise call window"), Qt::AlignLeft, false, &MiniWindowVideoPanel::onOpenCallButtonClicked);
    layoutTarget->addStretch(1);

    connect(&Ui::GetDispatcher()->getVoipController(), &voip_proxy::VoipController::onVoipMediaLocalAudio, this, &MiniWindowVideoPanel::onVoipMediaLocalAudio);
    connect(&Ui::GetDispatcher()->getVoipController(), &voip_proxy::VoipController::onVoipMediaLocalVideo, this, &MiniWindowVideoPanel::onVoipMediaLocalVideo);
    //QObject::connect(&Ui::GetDispatcher()->getVoipController(), SIGNAL(onVoipCallNameChanged(const voip_manager::ContactsList&)), this, SLOT(onVoipCallNameChanged(const voip_manager::ContactsList&)), Qt::DirectConnection);
    //QObject::connect(&Ui::GetDispatcher()->getVoipController(), SIGNAL(onVoipMinimalBandwidthChanged(bool)), this, SLOT(onVoipMinimalBandwidthChanged(bool)), Qt::DirectConnection);
    connect(&Ui::GetDispatcher()->getVoipController(), &voip_proxy::VoipController::onVoipVideoDeviceSelected, this, &MiniWindowVideoPanel::onVoipVideoDeviceSelected);

}


Ui::MiniWindowVideoPanel::~MiniWindowVideoPanel() = default;

bool Ui::MiniWindowVideoPanel::isUnderMouse()
{
    return mouseUnderPanel_;
}

void Ui::MiniWindowVideoPanel::updateVideoDeviceButtonsState()
{
    const auto videoEnabled = localVideoEnabled_ && isCameraEnabled_;
    const auto videoStyle = videoEnabled ? PanelButton::ButtonStyle::Normal : PanelButton::ButtonStyle::Transparent;
    const auto videoIcon = videoEnabled ? qsl(":/voip/videocall_icon") : qsl(":/voip/videocall_off_icon");
    const auto shareStyle = isScreenSharingEnabled_ ? PanelButton::ButtonStyle::Normal : PanelButton::ButtonStyle::Transparent;
    const auto shareIcon = isScreenSharingEnabled_ ? qsl(":/voip/sharescreen_icon") : qsl(":/voip/sharescreen_icon");

    videoButton_->updateStyle(videoStyle, videoIcon);
    videoButton_->setTooltipText(getVideoButtonText(videoStyle, PanelButton::ButtonSize::Small));

    shareScreenButton_->updateStyle(shareStyle, shareIcon);
    shareScreenButton_->setTooltipText(getScreensharingButtonText(shareStyle, PanelButton::ButtonSize::Small));
}

void Ui::MiniWindowVideoPanel::changeResizeButtonTooltip()
{
    static const auto textMinimize = QT_TRANSLATE_NOOP("voip_pages", "Minimize");
    static const auto textMaximize = QT_TRANSLATE_NOOP("voip_pages", "Maximize");
    if (resizeButton_->getTooltipText() == textMinimize)
        resizeButton_->setTooltipText(textMaximize);
    else
        resizeButton_->setTooltipText(textMinimize);
}

void Ui::MiniWindowVideoPanel::changeResizeButtonState()
{
    changeResizeButtonTooltip();
    resizeButton_->changeState();
}

void Ui::MiniWindowVideoPanel::changeEvent(QEvent* _e)
{
    QWidget::changeEvent(_e);
    if (_e->type() == QEvent::ActivationChange)
    {
        if (isActiveWindow() || (rootWidget_ && rootWidget_->isActiveWindow()))
        {
            if (parent_)
            {
                parent_->raise();
                raise();
            }
        }
    }
}

void Ui::MiniWindowVideoPanel::enterEvent(QEvent* _e)
{
    QWidget::enterEvent(_e);
    if (!mouseUnderPanel_)
    {
        mouseUnderPanel_ = true;
        Q_EMIT onMouseEnter();
    }
}

void Ui::MiniWindowVideoPanel::leaveEvent(QEvent* _e)
{
    QWidget::leaveEvent(_e);
    mouseUnderPanel_ = false;
    Q_EMIT onMouseLeave();
}

void Ui::MiniWindowVideoPanel::resizeEvent(QResizeEvent* _e)
{
    QWidget::resizeEvent(_e);
    if constexpr (platform::is_apple())
    {
        // Forced set fixed size, because under mac we use cocoa to change panel size.
        setFixedSize(size());
    }
}

void Ui::MiniWindowVideoPanel::mousePressEvent(QMouseEvent* _e)
{
    Q_EMIT mousePressed(QMouseEvent(*_e));
}

void Ui::MiniWindowVideoPanel::mouseMoveEvent(QMouseEvent* _e)
{
    Q_EMIT mouseMoved(QMouseEvent(*_e));
}

void Ui::MiniWindowVideoPanel::mouseDoubleClickEvent(QMouseEvent* _e)
{
    Q_EMIT mouseDoubleClicked();
}

void Ui::MiniWindowVideoPanel::updatePosition(const QWidget& _parent)
{
    const auto& rc = _parent.geometry();

    if constexpr (platform::is_windows())
    {
        move(rc.x(), rc.y() + rc.height());
    }
    else if constexpr (platform::is_apple())
    {
        auto height = rc.y();
        height += (rc.height() == videoMinimalHeight() ? -windowBorderWidth() : rc.height());
        move(rc.x() - windowBorderWidth(), height);
    }

    setFixedWidth(rc.width() + 2 * windowBorderWidth());
    resizeButton_->setTooltipBoundingRect(rc);
    openCallButton_->setTooltipBoundingRect(rc);
}

void Ui::MiniWindowVideoPanel::paintEvent(QPaintEvent* _event)
{
    QWidget::paintEvent(_event);
    const auto r = rect();
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    p.fillRect(r, Qt::transparent);
    p.setPen(Qt::NoPen);
    p.setBrush(Styling::getParameters().getColor(Styling::StyleVariable::BASE_GLOBALBLACK_PERMANENT));
    if constexpr (platform::is_apple())
    {
        const auto radius = getBorderRadius();
        p.drawRect(0, 0, r.width(), radius);
        p.drawRoundedRect(r, radius, radius);
    }
    else
    {
        p.drawRect(r);
    }
}


Ui::DetachedVideoWindow::DetachedVideoWindow(QWidget* _parent)
    : QWidget()
    , eventFilter_(nullptr)
    , parent_(_parent)
    , closedManualy_(false)
    , videoPanel_(new MiniWindowVideoPanel(this))
    , header_(new MiniWindowHeader(this))
    , videoPanels_{ videoPanel_, header_ }
    , shadow_(new Ui::ShadowWindowParent(this))
    , rootWidget_(nullptr)
    , opacityAnimation_(new QVariantAnimation(this))
    , resizeAnimation_(new QVariantAnimation(this))
    , tooltipTimer_(nullptr)
    , mode_(WindowMode::Full)
{
    setFixedSize(getDetachedVideoWindowSize());
    setStyleSheet(qsl("background: transparent;"));

    auto rootLayout = Utils::emptyVLayout(this);
    rootLayout->setAlignment(Qt::AlignVCenter);
    setLayout(rootLayout);

    QMetaObject::connectSlotsByName(this);

    if constexpr (platform::is_windows())
    {
        setWindowFlags(Qt::WindowStaysOnTopHint | Qt::FramelessWindowHint | Qt::SubWindow);
    }
    else
    {
        // We have problem on Mac with WA_ShowWithoutActivating and WindowDoesNotAcceptFocus.
        // If video window is showing with this flags, we cannot activate main ICQ window.
        // UPDATED: Looks like for mini video window it works ok with  Qt::WindowDoesNotAcceptFocus
        // and Qt::WA_ShowWithoutActivating.
        setWindowFlags(Qt::WindowStaysOnTopHint | Qt::Window | Qt::FramelessWindowHint);
        //setAttribute(Qt::WA_ShowWithoutActivating);
        setAttribute(Qt::WA_X11DoNotAcceptFocus);
    }

#ifndef STRIP_VOIP
    rootWidget_ = platform_specific::GraphicsPanel::create(this, videoPanels_, false, false);
    rootWidget_->setAttribute(Qt::WA_UpdatesDisabled);
    rootWidget_->setFixedHeight(videoHeight());
    rootWidget_->setFixedWidth(width());

    if (platform::is_linux())
    {
        rootLayout->addWidget(header_);
        rootLayout->addWidget(rootWidget_);
        rootLayout->addSpacerItem(new QSpacerItem(0, 0, QSizePolicy::Expanding, QSizePolicy::Expanding));
        rootLayout->addWidget(videoPanel_);
    }
#endif

    eventFilter_ = new ResizeEventFilter(videoPanels_, shadow_->getShadowWidget(), this);
    installEventFilter(eventFilter_);

    if constexpr (!platform::is_linux())
    {
        setAttribute(Qt::WA_UpdatesDisabled);
        setAttribute(Qt::WA_ShowWithoutActivating);
    }

    connect(&Ui::GetDispatcher()->getVoipController(), &voip_proxy::VoipController::onVoipWindowRemoveComplete, this, &DetachedVideoWindow::onVoipWindowRemoveComplete);
    connect(&Ui::GetDispatcher()->getVoipController(), &voip_proxy::VoipController::onVoipWindowAddComplete, this, &DetachedVideoWindow::onVoipWindowAddComplete);
    connect(&Ui::GetDispatcher()->getVoipController(), &voip_proxy::VoipController::onVoipCallDestroyed, this, &DetachedVideoWindow::onVoipCallDestroyed);
    if (const auto mw = Utils::InterConnector::instance().getMainWindow())
    {
        connect(mw, &Ui::MainWindow::mouseMoved, this, [this](const QPoint& _pos)
        {
            if (!isVisible())
                return;

            QRect r(geometry().topLeft(), rootWidget_->size());
            if (r.contains(mapToGlobal(_pos)))
                startTooltipTimer();
        });
    }

    if (videoPanel_)
    {
        connect(videoPanel_, &MiniWindowVideoPanel::onMouseLeave, this, &DetachedVideoWindow::onPanelMouseLeave);
        connect(videoPanel_, &MiniWindowVideoPanel::onMouseEnter, this, &DetachedVideoWindow::onPanelMouseEnter);
        connect(videoPanel_, &MiniWindowVideoPanel::needShowScreenPermissionsPopup, this, [this](media::permissions::DeviceType type)
        {
            activateMainVideoWindow();
            Q_EMIT needShowScreenPermissionsPopup(type);
        });
        connect(videoPanel_, &MiniWindowVideoPanel::onMicrophoneClick, this, &DetachedVideoWindow::onMicrophoneClick);
        connect(videoPanel_, &MiniWindowVideoPanel::onShareScreenClick, this, &DetachedVideoWindow::onShareScreenClick);
        connect(videoPanel_, &MiniWindowVideoPanel::onOpenCallClicked, this, &DetachedVideoWindow::activateMainVideoWindow);
        connect(videoPanel_, &MiniWindowVideoPanel::onResizeClicked, this, &DetachedVideoWindow::onPanelClickedResize);
        connect(videoPanel_, &MiniWindowVideoPanel::onHangClicked, this, &DetachedVideoWindow::onPanelClickedClose);

        connect(videoPanel_, &MiniWindowVideoPanel::mouseDoubleClicked, this, &DetachedVideoWindow::activateMainVideoWindow);
        connect(videoPanel_, &MiniWindowVideoPanel::mousePressed, this, &DetachedVideoWindow::onMousePress);
        connect(videoPanel_, &MiniWindowVideoPanel::mouseMoved, this, &DetachedVideoWindow::onMouseMove);
    }

    const auto screenRect = Utils::mainWindowScreen()->geometry();
    const auto detachedWndRect = rect();

    auto detachedWndPos = screenRect.topRight();
    detachedWndPos.setX(detachedWndPos.x() - detachedWndRect.width() - 0.01f * screenRect.width());
    detachedWndPos.setY(detachedWndPos.y() + 0.05f * screenRect.height());

    const QRect rc(detachedWndPos.x(), detachedWndPos.y(), detachedWndRect.width(), detachedWndRect.height());
    setGeometry(rc);

    resizeAnimation_->setDuration(heightAnimDuration().count());
    resizeAnimation_->setEasingCurve(QEasingCurve::InOutSine);
    connect(resizeAnimation_, &QVariantAnimation::valueChanged, this, [this](const QVariant& value)
    {
        const auto val = value.toInt();
        if constexpr (platform::is_linux())
            setFixedHeight(val + panelsHeight());
        else
            setFixedHeight(val);
        rootWidget_->setFixedHeight(val);
        update();
    });
    if (platform::is_apple())
    {
        connect(resizeAnimation_, &QVariantAnimation::finished, this, [this]
        {
            if  (height() > videoMinimalHeight())
                header_->setCollapsed(false);
        });
    }

    opacityAnimation_->setDuration(heightAnimDuration().count());
    connect(opacityAnimation_, &QVariantAnimation::valueChanged, this, [this](const QVariant& value)
    {
        rootWidget_->setOpacity(value.toDouble());
    });
    connect(opacityAnimation_, &QVariantAnimation::finished, this, [this]()
    {
        if (QAbstractAnimation::Backward == opacityAnimation_->direction())
            hide();
    });

    // Round rect.
    //QPainterPath path(QPointF(0, 0));
    //path.addRoundRect(rect(), Utils::scale_value(8));
    //QRegion region(path.toFillPolygon().toPolygon());
    //setMask(region);
}

void Ui::DetachedVideoWindow::onVoipCallDestroyed(const voip_manager::ContactEx& _contactEx)
{
    if (_contactEx.current_call)
    {
        // in this moment destroyed call is active, e.a. call_count + 1
        closedManualy_ = false;
    }
}

void Ui::DetachedVideoWindow::onPanelClickedClose()
{
    activateMainVideoWindow();
    closedManualy_ = true;
    hide();
}

void Ui::DetachedVideoWindow::onPanelClickedResize()
{
    if (height() > panelsHeight())
    {
        mode_ = WindowMode::Compact;
        resizeAnimated(ResizeDirection::Minimize);
    }
    else
    {
        mode_ = WindowMode::Full;
        resizeAnimated(ResizeDirection::Maximize);
    }
}

void Ui::DetachedVideoWindow::onPanelMouseEnter()
{
    if (videoPanel_)
    {
        updatePanels();
        Ui::GetDispatcher()->getVoipController().passWindowHover(rootWidget_->frameId(), true);
    }
    startTooltipTimer();
}

void Ui::DetachedVideoWindow::onPanelMouseLeave()
{
    const QPoint p = mapFromGlobal(QCursor::pos());
    if (videoPanel_ && !rect().contains(p))
        Ui::GetDispatcher()->getVoipController().passWindowHover(rootWidget_->frameId(), false);
    Tooltip::hide();
    if (tooltipTimer_)
        tooltipTimer_->stop();
}

void Ui::DetachedVideoWindow::mousePressEvent(QMouseEvent* _e)
{
    pressPos_ = QCursor::pos();
    mousePressed_ = true;
    onMousePress(*_e);
}

void Ui::DetachedVideoWindow::mouseReleaseEvent(QMouseEvent *_e)
{
    if (Utils::clicked(pressPos_, QCursor::pos()))
        activateMainVideoWindow();
    pressPos_ = QPoint();
    mousePressed_ = false;
}

void Ui::DetachedVideoWindow::mouseMoveEvent(QMouseEvent* _e)
{
    Tooltip::hide();
    startTooltipTimer();
    onMouseMove(*_e);
}

void Ui::DetachedVideoWindow::onMousePress(const QMouseEvent& _e)
{
    posDragBegin_ = _e.pos();
}

void Ui::DetachedVideoWindow::onMouseMove(const QMouseEvent& _e)
{
    if (_e.buttons() & Qt::LeftButton)
    {
#ifdef __APPLE__
        if (auto handle = windowHandle())
            handle->startSystemMove();
#else
        const auto diff = _e.pos() - posDragBegin_;
        const auto newpos = pos() + diff;
        move(newpos);
#endif
    }
}

void Ui::DetachedVideoWindow::enterEvent(QEvent* _e)
{
    QWidget::enterEvent(_e);
    Tooltip::forceShow(true);
    onPanelMouseEnter();
}

void Ui::DetachedVideoWindow::leaveEvent(QEvent* _e)
{
    QWidget::leaveEvent(_e);
    Tooltip::forceShow(false);
    onPanelMouseLeave();
}

void Ui::DetachedVideoWindow::paintEvent(QPaintEvent* _e)
{
    QWidget::paintEvent(_e);

    if constexpr (platform::is_linux())
    {
        QPainter p(this);
        p.fillRect(rect(), getPanelBackgroundColorLinux());
    }
}

void Ui::DetachedVideoWindow::mouseDoubleClickEvent(QMouseEvent* /*e*/)
{
    activateMainVideoWindow();
}

quintptr Ui::DetachedVideoWindow::getContentWinId()
{
    return (quintptr)rootWidget_->frameId();
}

void Ui::DetachedVideoWindow::onVoipWindowRemoveComplete(quintptr _winId)
{
    if (_winId == rootWidget_->frameId())
    {
        rootWidget_->freeNative();
        hide();
    }
    Utils::InterConnector::instance().getMainWindow()->updateMainMenu();
}

void Ui::DetachedVideoWindow::onVoipWindowAddComplete(quintptr _winId)
{
    if (_winId == rootWidget_->frameId())
    {
        if constexpr (platform::is_linux())
        {
            // workaround for final glfw-window collapse, cause it has the asynchronous
            // nature and may not be able to query the final size after window creation
            if (mode_ == WindowMode::Compact)
            {
                rootWidget_->setFixedHeight(1);
                QTimer::singleShot(0, this, [this](){ rootWidget_->setFixedHeight(0); });
            }
        }

        updatePanels();
#ifdef __APPLE__
        MacSupport::showInAllWorkspaces(this);
#else
        showNormal();
#endif
    }
    Utils::InterConnector::instance().getMainWindow()->updateMainMenu();
}

void Ui::DetachedVideoWindow::resizeAnimated(ResizeDirection _dir)
{
    if (resizeAnimation_->state() == QVariantAnimation::Running)
        return;

    const auto newHeight = _dir == ResizeDirection::Maximize ? videoHeight() : videoMinimalHeight();
    resizeAnimation_->stop();
    resizeAnimation_->setStartValue(rootWidget_->height());
    resizeAnimation_->setEndValue(newHeight);
    resizeAnimation_->start();

    const auto opacity = _dir == ResizeDirection::Maximize ? 1. : 0.;
    const auto curOpacity = _dir == ResizeDirection::Maximize ? 0. : 1.;
    opacityAnimation_->stop();
    opacityAnimation_->setStartValue(curOpacity);
    opacityAnimation_->setEndValue(opacity);
    opacityAnimation_->start();

    if constexpr (platform::is_apple())
    {
        if (_dir == ResizeDirection::Minimize)
            header_->setCollapsed(true);
    }
}

void Ui::DetachedVideoWindow::onTooltipTimer()
{
    if (!isVisible() || resizeAnimation_->state() == QVariantAnimation::Running)
        return;

    const auto pos = QCursor::pos();
    QRect r(geometry().topLeft(), rootWidget_->size());
    if (r.contains(pos))
        Tooltip::show(getTooltipText(), QRect(QCursor::pos(), Utils::scale_value(QSize())), getTooltipSize());
}

void Ui::DetachedVideoWindow::showFrame(WindowMode _mode)
{
    rootWidget_->initNative(platform_specific::ViewResize::Adjust, rootWidget_->height() ? QSize() : getDetachedVideoWindowSize());
    im_assert(rootWidget_->frameId());
    if (rootWidget_->frameId())
        Ui::GetDispatcher()->getVoipController().setWindowAdd((quintptr)rootWidget_->frameId(), "", false, false, 0);

    setWindowMode(_mode);
}

void Ui::DetachedVideoWindow::hideFrame()
{
    if (rootWidget_->frameId())
        Ui::GetDispatcher()->getVoipController().setWindowRemove((quintptr)rootWidget_->frameId());
}

void Ui::DetachedVideoWindow::showEvent(QShowEvent* _e)
{
    QWidget::showEvent(_e);

    for (auto panel : videoPanels_)
        panel->setVisible(true);

    updatePanels();
}

bool Ui::DetachedVideoWindow::closedManualy()
{
    return closedManualy_;
}

void Ui::DetachedVideoWindow::hideEvent(QHideEvent* _e)
{
    QWidget::hideEvent(_e);
    if (videoPanel_)
    {
        for (auto panel : videoPanels_)
            panel->setVisible(false);
        Ui::GetDispatcher()->getVoipController().passWindowHover(rootWidget_->winId(), false);
    }
}

quintptr Ui::DetachedVideoWindow::getVideoFrameId() const
{
    return (quintptr)rootWidget_->frameId();
}

Ui::DetachedVideoWindow::~DetachedVideoWindow()
{
    removeEventFilter(eventFilter_);
    delete eventFilter_;
}

void Ui::DetachedVideoWindow::changeEvent(QEvent* _e)
{
    QWidget::changeEvent(_e);
}

void Ui::DetachedVideoWindow::updatePanels() const
{
    if (!rootWidget_ || !videoPanel_)
    {
        im_assert(false);
        return;
    }

    if constexpr (platform::is_apple())
    {
        rootWidget_->clearPanels();
        std::vector<QPointer<Ui::BaseVideoPanel>> panels;

        for (auto panel : videoPanels_)
        {
            if (panel->isVisible())
                panels.push_back(panel);
        }

        rootWidget_->addPanels(panels);

        for (auto panel : videoPanels_)
        {
            if (panel->isVisible())
                panel->updatePosition(*this);
        }
    }
}

void Ui::DetachedVideoWindow::activateMainVideoWindow()
{
    if (parent_)
    {
        if (auto p = qobject_cast<Ui::VideoWindow*>(parent_))
            p->raiseWindow();
    }
}

bool Ui::DetachedVideoWindow::isMinimized() const
{
    return QWidget::isMinimized() && (videoPanel_ && !videoPanel_->isVisible());
}

void Ui::DetachedVideoWindow::setWindowTitle(const QString& _title)
{
    header_->setTitle(_title);
}

void Ui::DetachedVideoWindow::setWindowMode(WindowMode _mode)
{
    if (_mode != WindowMode::Current && mode_ != _mode)
    {
        mode_ = _mode;

        if (_mode == WindowMode::Compact)
        {
            setFixedHeight(platform::is_linux() ? panelsHeight() : 0);
            rootWidget_->setFixedHeight(0);
            rootWidget_->setOpacity(0.);
            header_->setCollapsed(true);
        }
        else
        {
            setFixedHeight(getDetachedVideoWindowSize().height());
            rootWidget_->setFixedHeight(videoHeight());
            rootWidget_->setOpacity(1.);
        }

        videoPanel_->changeResizeButtonState();
    }
}

void Ui::DetachedVideoWindow::moveToCorner()
{
    const auto screenRect = QDesktopWidget().availableGeometry(this);
    const auto wndRect = rect();
    auto wndPos = screenRect.topRight();
    wndPos.setX(wndPos.x() - wndRect.width() - getCornerSpacing());
    auto yPosition = wndPos.y() + getCornerSpacing();
    if constexpr (platform::is_windows() || platform::is_apple())
        yPosition += headerHeight();
    wndPos.setY(yPosition);
    setGeometry(wndPos.x(), wndPos.y(), wndRect.width(), wndRect.height());
}

void Ui::DetachedVideoWindow::startTooltipTimer()
{
    if (!tooltipTimer_)
    {
        tooltipTimer_ = new QTimer(this);
        tooltipTimer_->setSingleShot(true);
        tooltipTimer_->setInterval(Tooltip::getDefaultShowDelay());
        connect(tooltipTimer_, &QTimer::timeout, this, &DetachedVideoWindow::onTooltipTimer);
    }
    tooltipTimer_->start();
}

Ui::MiniWindowHeader::MiniWindowHeader(QWidget* _parent)
    : MoveablePanel(_parent)
    , collapsed_(false)
{
    if (platform::is_apple())
    {
        setFixedHeight(headerHeight() + videoHeight());
        const auto borderWidth = windowBorderWidth();
        const auto width = getWidth() + 2 * borderWidth;
        setFixedWidth(width);
        QRegion mask(rect());
        setMask(mask.subtracted(QRegion(borderWidth, headerHeight(), getWidth(), videoHeight())));
    }
    else
    {
        setFixedHeight(headerHeight());
        setFixedWidth(getWidth());
    }

    text_ = TextRendering::MakeTextUnit(QString());
    text_->init(titleFont(), Styling::getParameters().getColor(Styling::StyleVariable::TEXT_SOLID_PERMANENT));
    text_->evaluateDesiredSize();
}

void Ui::MiniWindowHeader::updatePosition(const QWidget& _parent)
{
    const auto& rc = _parent.geometry();

    if constexpr (platform::is_windows())
        move(rc.x(), rc.y() - rect().height());
    else if constexpr (platform::is_apple())
        move(rc.x() - windowBorderWidth(), rc.y() - headerHeight());

    setFixedWidth(rc.width() + 2 * windowBorderWidth());
}

void Ui::MiniWindowHeader::setTitle(const QString& _title)
{
    text_->setText(_title);
    text_->evaluateDesiredSize();
    updateTitleOffsets();
    update();
}

void Ui::MiniWindowHeader::setCollapsed(bool _collapsed)
{
    if (platform::is_apple())
    {
        collapsed_ = _collapsed;
        auto newHeight = collapsed_ ? (headerHeight() + windowBorderWidth()) : (headerHeight() + videoHeight());
        setFixedHeight(newHeight);
    }
}

void Ui::MiniWindowHeader::paintEvent(QPaintEvent* _event)
{
    QWidget::paintEvent(_event);

    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    p.setPen(Qt::NoPen);
    static const auto color = Styling::getParameters().getColor(Styling::StyleVariable::LUCENT_TERTIARY);
    if constexpr (platform::is_apple())
    {
        auto w = rect().width();
        auto h = headerHeight();
        const auto r = getBorderRadius();

        p.setBrush(Qt::black);
        p.drawRoundedRect(0, 0, w, h + r, r, r);
        p.setBrush(color);
        p.drawRoundedRect(0, 0, w, h + r, r, r);

        p.setCompositionMode(QPainter::CompositionMode_Source);
        p.setBrush(Qt::transparent);

        const auto yPosition = collapsed_ ? (h + windowBorderWidth()) : h;
        const auto drawHeight = collapsed_ ? (r - windowBorderWidth()) : r;
        p.drawRect(0, yPosition, w, drawHeight);

        if (!collapsed_)
        {
            p.setPen({ Qt::black, 2.0 });
            QLine left(0, h + windowBorderWidth(), 0, videoHeight() + h);
            p.drawLine(left);
            QLine right(w, h + windowBorderWidth(), w, videoHeight() + h);
            p.drawLine(right);
        }
    }
    else
    {
        p.fillRect(rect(), Qt::black);
        p.fillRect(rect(), color);
    }

    text_->draw(p, TextRendering::VerPosition::MIDDLE);
}

void Ui::MiniWindowHeader::resizeEvent(QResizeEvent* event)
{
    updateTitleOffsets();
}

bool Ui::MiniWindowHeader::uiWidgetIsActive() const
{
    if (parentWidget())
        return parentWidget()->isActiveWindow();
    return false;
}

void Ui::MiniWindowHeader::updateTitleOffsets()
{
    const auto r = rect();
    text_->setOffsets(r.width() / 2 - text_->cachedSize().width() / 2, headerHeight() / 2);
}
