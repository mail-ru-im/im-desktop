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
    enum { kAnimationDefDuration = 500 };

    auto getWidth()
    {
        return Utils::scale_value(284);
    }

    auto getMaxHeight()
    {
        return Utils::scale_value(212);
    }

    auto getCollapsedHeight()
    {
        return Utils::scale_value(52);
    }

    auto getDetachedVideoWindowSize() noexcept
    {
        return QSize(getWidth(), getMaxHeight());
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

    auto getTooltipSize()
    {
        return Utils::scale_value(QSize(166, 77));
    }

    auto getBorderRadius()
    {
        return Utils::scale_value(4);
    }

    constexpr std::chrono::milliseconds heightAnimDuration() noexcept { return std::chrono::milliseconds(150); }
    constexpr std::chrono::milliseconds tooltipShowDelay() noexcept { return std::chrono::milliseconds(400); }
}

void  Ui::MiniWindowVideoPanel::onResizeButtonClicked()
{
    const auto textMinimize = QT_TRANSLATE_NOOP("voip_pages", "Minimize");
    const auto textMaximize = QT_TRANSLATE_NOOP("voip_pages", "Maximize");
    if (resizeButton_->getTooltipText() == textMinimize)
        resizeButton_->setTooltipText(textMaximize);
    else
        resizeButton_->setTooltipText(textMinimize);

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

    if (isScreenSharingEnabled_)
        showScreenBorder(_desc.uid);
    else
        hideScreenBorder();
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


Ui::MiniWindowVideoPanel::~MiniWindowVideoPanel()
{
    hideScreenBorder();
}

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

void Ui::MiniWindowVideoPanel::showScreenBorder(std::string_view _uid)
{
    if (GetDispatcher()->getVoipController().isWebinar())
        return;

    hideScreenBorder();

    shareScreenFrame_ = new VideoPanelParts::ShareScreenFrame(_uid, getFrameBorderColor());
    connect(shareScreenFrame_, &VideoPanelParts::ShareScreenFrame::stopScreenSharing, this, &MiniWindowVideoPanel::onShareScreen);
}

void Ui::MiniWindowVideoPanel::hideScreenBorder()
{
    if (shareScreenFrame_)
        shareScreenFrame_->deleteLater();
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

    if (platform::is_linux())
    {
        move(0, rc.height() - rect().height());
    }
    else
    {
        // temporary hack (further redesign in next task)
        const auto dY = platform::is_windows() ? rc.y() : std::max(rc.y(), Utils::scale_value(23));
        move(rc.x(),  dY + rc.height() - rect().height());
    }

    setFixedWidth(rc.width());
    resizeButton_->setTooltipBoundingRect(rc);
    openCallButton_->setTooltipBoundingRect(rc);
}

void Ui::MiniWindowVideoPanel::paintEvent(QPaintEvent *_e)
{
    QWidget::paintEvent(_e);

    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    p.fillRect(rect(), Qt::transparent);
    p.setPen(Qt::NoPen);
    p.setBrush(Styling::getParameters().getColor(Styling::StyleVariable::BASE_GLOBALBLACK_PERMANENT));
    if constexpr (platform::is_apple())
    {
        const auto r = getBorderRadius();
        p.drawRoundedRect(rect(), r, r);
    }
    else
    {
        p.drawRect(rect());
    }
}


Ui::DetachedVideoWindow::DetachedVideoWindow(QWidget* _parent)
    : QWidget()
    , eventFilter_(nullptr)
    , parent_(_parent)
    , closedManualy_(false)
    , videoPanel_(new MiniWindowVideoPanel(this))
    , shadow_(new Ui::ShadowWindowParent(this))
    , rootWidget_(nullptr)
    , opacityAnimation_(new QVariantAnimation(this))
    , resizeAnimation_(new QVariantAnimation(this))
    , tooltipTimer_(nullptr)
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
        setWindowFlags(Qt::WindowStaysOnTopHint | Qt::Window | Qt::WindowDoesNotAcceptFocus);
        //setAttribute(Qt::WA_ShowWithoutActivating);
        setAttribute(Qt::WA_X11DoNotAcceptFocus);
        if constexpr (platform::is_linux())
            setWindowFlags(windowFlags() & ~Qt::WindowCloseButtonHint);
    }

#ifndef STRIP_VOIP
    std::vector<QPointer<Ui::BaseVideoPanel>> panels = { videoPanel_ };
    rootWidget_ = platform_specific::GraphicsPanel::create(this, panels, false, false);
    rootWidget_->setAttribute(Qt::WA_UpdatesDisabled);
    rootWidget_->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    rootWidget_->setFixedHeight(height() - getCollapsedHeight());

    rootLayout->addWidget(rootWidget_);
    rootLayout->addSpacerItem(new QSpacerItem(0, 0, QSizePolicy::Expanding, QSizePolicy::Expanding));
    rootLayout->addWidget(videoPanel_);
#endif

    std::vector<QPointer<BaseVideoPanel>> videoPanels = { videoPanel_ };
    eventFilter_ = new ResizeEventFilter(videoPanels, shadow_->getShadowWidget(), this);
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
        const auto rwHeight = val - getCollapsedHeight();
        setFixedHeight(val);
        rootWidget_->setFixedHeight(rwHeight);
        update();
    });

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
    if (height() > getCollapsedHeight())
        resizeAnimated(ResizeDirection::Minimize);
    else
        resizeAnimated(ResizeDirection::Maximize);
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
    onMousePress(*_e);
}

void Ui::DetachedVideoWindow::mouseReleaseEvent(QMouseEvent *_e)
{
    if (Utils::clicked(pressPos_, QCursor::pos()))
        activateMainVideoWindow();
    pressPos_ = QPoint();
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
        const auto diff = _e.pos() - posDragBegin_;
        const auto newpos = pos() + diff;
        move(newpos);
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

    const auto newHeight = _dir == ResizeDirection::Maximize ? getMaxHeight() : getCollapsedHeight();
    resizeAnimation_->stop();
    resizeAnimation_->setStartValue(height());
    resizeAnimation_->setEndValue(newHeight);
    resizeAnimation_->start();

    const auto opacity = _dir == ResizeDirection::Maximize ? 1. : 0.;
    const auto curOpacity = _dir == ResizeDirection::Maximize ? 0. : 1.;
    opacityAnimation_->stop();
    opacityAnimation_->setStartValue(curOpacity);
    opacityAnimation_->setEndValue(opacity);
    opacityAnimation_->start();
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

void Ui::DetachedVideoWindow::showFrame()
{
    rootWidget_->initNative(platform_specific::ViewResize::Adjust);
    assert(rootWidget_->frameId());
    if (rootWidget_->frameId())
        Ui::GetDispatcher()->getVoipController().setWindowAdd((quintptr)rootWidget_->frameId(), "", false, false, 0);
    if (videoPanel_)
        videoPanel_->show();
    //shadow_->showShadow();
}

void Ui::DetachedVideoWindow::hideFrame()
{
    if (rootWidget_->frameId())
        Ui::GetDispatcher()->getVoipController().setWindowRemove((quintptr)rootWidget_->frameId());
    //shadow_->hideShadow();

    if (videoPanel_)
        videoPanel_->hide();
}

void Ui::DetachedVideoWindow::showEvent(QShowEvent* _e)
{
    QWidget::showEvent(_e);

    videoPanel_->setVisible(true);
    if constexpr (platform::is_linux())
        if (videoPanel_)
            videoPanel_->show();

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
        videoPanel_->setVisible(false);
        videoPanel_->hide();
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
        assert(false);
        return;
    }

    if constexpr (platform::is_apple())
    {
        rootWidget_->clearPanels();
        std::vector<QPointer<Ui::BaseVideoPanel>> panels;

        if (videoPanel_ && videoPanel_->isVisible())
            panels.push_back(videoPanel_);

        rootWidget_->addPanels(panels);

        if (videoPanel_ && videoPanel_->isVisible())
            videoPanel_->updatePosition(*this);
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

void Ui::DetachedVideoWindow::startTooltipTimer()
{
    if (!tooltipTimer_)
    {
        tooltipTimer_ = new QTimer(this);
        tooltipTimer_->setSingleShot(true);
        tooltipTimer_->setInterval(tooltipShowDelay());
        connect(tooltipTimer_, &QTimer::timeout, this, &DetachedVideoWindow::onTooltipTimer);
    }
    tooltipTimer_->start();
}
