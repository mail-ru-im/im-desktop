#include "stdafx.h"
#include "ScreenFrame.h"
#include "DetachedVideoWnd.h"
#include "PanelButtons.h"
#include "../core_dispatcher.h"
#include "../utils/InterConnector.h"

#include "CommonUI.h"
#include "../previewer/GalleryFrame.h"
#include "../controls/ClickWidget.h"
#include "../controls/TooltipWidget.h"
#include "../main_window/MainWindow.h"
#include "../styles/ThemeParameters.h"
#include "../utils/graphicsEffects.h"

#include "../styles/ThemeParameters.h"

#include "VideoWindow.h"
#include "WindowController.h"

#ifdef _WIN32
    #include <windows.h>
#endif

#ifdef __APPLE__
    #include "../utils/macos/mac_support.h"
#endif

namespace
{
    auto getButtonIconSize() noexcept
    {
        return Utils::scale_value(QSize(36, 36));
    }

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

    auto getMaxHeight() noexcept
    {
        auto height = videoHeight() + getCollapsedHeight();
        height += headerHeight();
        return height;
    }

    auto panelsHeight() noexcept
    {
        return headerHeight() + getCollapsedHeight();
    }

    auto getDetachedVideoWindowSize() noexcept
    {
        return QSize(getWidth(), getMaxHeight());
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
        const auto enabled = _enabled && GetDispatcher()->getVoipController().isAudPermissionGranted();
        micButton_->setChecked(enabled);
        micButton_->setIcon(microphoneIcon(enabled));
        micButton_->setText(microphoneButtonText(enabled));
    }
}

void Ui::MiniWindowVideoPanel::onVoipMediaLocalVideo(bool _enabled)
{
    if (!videoButton_)
        return;
    localVideoEnabled_ = _enabled;
    updateVideoDeviceButtonsState();
}

void Ui::MiniWindowVideoPanel::showShareScreenMenu(int _screenCount)
{
    Q_EMIT needShowVideoWindow();
    menu_->close();
    menu_->clear();
    for (int i = 0; i < _screenCount; i++)
    {
        QAction* action = new QAction(QT_TRANSLATE_NOOP("voip_pages", "Screen") % ql1c(' ') % QString::number(i + 1), menu_);
        connect(action, &QAction::triggered, [this, i]() { Q_EMIT closeChooseScreenMenu(); ScreenSharingManager::instance().toggleSharing(i); });
        menu_->addAction(action, QPixmap{});
    }
    shareScreenButton_->showMenu(menu_);
}

void Ui::MiniWindowVideoPanel::onShareScreen()
{
    if (!Ui::GetDispatcher()->getVoipController().isLocalDesktopAllowed())
        return;

    const auto& screens = Ui::GetDispatcher()->getVoipController().screenList();
    const int screenCount = screens.size();
    if (!isScreenSharingEnabled_ && screenCount  > 1)
        showShareScreenMenu(screenCount);
    else
        ScreenSharingManager::instance().toggleSharing(0);
}

void Ui::MiniWindowVideoPanel::onShareScreenStateChanged(ScreenSharingManager::SharingState _state, int)
{
    isScreenSharingEnabled_ = (_state == ScreenSharingManager::SharingState::Shared);
    Q_EMIT onShareScreenClick(isScreenSharingEnabled_);
    updateVideoDeviceButtonsState();
}

void Ui::MiniWindowVideoPanel::onVoipVideoDeviceSelected(const voip_proxy::device_desc& _desc)
{
    isScreenSharingEnabled_ = (_desc.dev_type == voip_proxy::kvoipDevTypeVideoCapture && _desc.video_dev_type == voip_proxy::kvoipDeviceDesktop);
    isCameraEnabled_ = (_desc.dev_type == voip_proxy::kvoipDevTypeVideoCapture && _desc.video_dev_type == voip_proxy::kvoipDeviceCamera);
    updateVideoDeviceButtonsState();
}

void Ui::MiniWindowVideoPanel::onScreensChanged()
{
    if (qApp->screens().count() < 2 && menu_->isVisible())
        menu_->hide();
}


Ui::MiniWindowVideoPanel::MiniWindowVideoPanel(QWidget* _parent)
    : QWidget(_parent)
    , parent_(_parent)
    , rootWidget_(nullptr)
    , micButton_(nullptr)
    , stopCallButton_(nullptr)
    , videoButton_(nullptr)
    , localVideoEnabled_(false)
    , isScreenSharingEnabled_(false)
    , isCameraEnabled_(true)
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

    layoutTarget->addStretch(1);
    resizeButton_ = createButton(ql1s(":/voip/resize"), QT_TRANSLATE_NOOP("voip_pages", "Minimize"), Qt::AlignRight, true);
    connect(resizeButton_, &TransparentPanelButton::clicked, this, &MiniWindowVideoPanel::onResizeButtonClicked);
    layoutTarget->addWidget(resizeButton_);

    micButton_ = createButton(microphoneIcon(false), microphoneButtonText(false), true);
    micButton_->setChecked(false);
    micButton_->setKeySequence(Qt::ControlModifier + Qt::ShiftModifier + Qt::Key_A);
    connect(micButton_, &PanelToolButton::clicked, this, &MiniWindowVideoPanel::onAudioOnOffClicked);
    layoutTarget->addWidget(micButton_);

    videoButton_ = createButton(videoIcon(true), videoButtonText(true), true);
    videoButton_->setChecked(true);
    videoButton_->setKeySequence(Qt::ControlModifier + Qt::ShiftModifier + Qt::Key_V);
    connect(videoButton_, &PanelToolButton::clicked, this, &MiniWindowVideoPanel::onVideoOnOffClicked);
    layoutTarget->addWidget(videoButton_);

    shareScreenButton_ = createButton(screensharingIcon(), screensharingButtonText(false), true);
    shareScreenButton_->setChecked(false);
    shareScreenButton_->installEventFilter(this);
    shareScreenButton_->setKeySequence(Qt::ControlModifier + Qt::ShiftModifier + Qt::Key_S);
    connect(shareScreenButton_, &PanelToolButton::clicked, this, &MiniWindowVideoPanel::onShareScreen);
    layoutTarget->addWidget(shareScreenButton_);

    stopCallButton_ = createButton(hangupIcon(), QT_TRANSLATE_NOOP("voip_video_panel_mini", "End meeting"), false);
    stopCallButton_->setToolTip(QT_TRANSLATE_NOOP("voip_video_panel_mini", "End meeting"));
    stopCallButton_->setButtonRole(PanelToolButton::Attention);
    connect(stopCallButton_, &PanelToolButton::clicked, this, &MiniWindowVideoPanel::onHangUpClicked);
    layoutTarget->addWidget(stopCallButton_);

    openCallButton_ = createButton(ql1s(":/voip/maximize"), QT_TRANSLATE_NOOP("voip_pages", "Raise call window"), Qt::AlignLeft, false);
    connect(openCallButton_, &TransparentPanelButton::clicked, this, &MiniWindowVideoPanel::onOpenCallButtonClicked);
    layoutTarget->addWidget(openCallButton_);
    layoutTarget->addStretch(1);

    connect(&Ui::GetDispatcher()->getVoipController(), &voip_proxy::VoipController::onVoipMediaLocalAudio, this, &MiniWindowVideoPanel::onVoipMediaLocalAudio);
    connect(&Ui::GetDispatcher()->getVoipController(), &voip_proxy::VoipController::onVoipMediaLocalVideo, this, &MiniWindowVideoPanel::onVoipMediaLocalVideo);
    connect(&Ui::GetDispatcher()->getVoipController(), &voip_proxy::VoipController::onVoipVideoDeviceSelected, this, &MiniWindowVideoPanel::onVoipVideoDeviceSelected);

    connect(&ScreenSharingManager::instance(), &ScreenSharingManager::needShowScreenPermissionsPopup, this, &MiniWindowVideoPanel::needShowScreenPermissionsPopup);
    connect(&ScreenSharingManager::instance(), &ScreenSharingManager::sharingStateChanged, this, &MiniWindowVideoPanel::onShareScreenStateChanged);

    menu_ = new Previewer::CustomMenu;
    menu_->setAttribute(Qt::WA_DeleteOnClose, false);
    menu_->setFeatures(Previewer::CustomMenu::ScreenAware | Previewer::CustomMenu::TargetAware);
    menu_->setArrowSize({});
    connect(qApp, &QApplication::screenAdded, this, &MiniWindowVideoPanel::onScreensChanged);
    connect(qApp, &QApplication::screenRemoved, this, &MiniWindowVideoPanel::onScreensChanged);
}

Ui::MiniWindowVideoPanel::~MiniWindowVideoPanel()
{
    menu_->close();
    menu_->deleteLater();
}

Ui::PanelToolButton* Ui::MiniWindowVideoPanel::createButton(const QString& _icon, const QString& _text, bool _checkable)
{
    PanelToolButton* button = new PanelToolButton(this);
    button->setIconSize(getButtonIconSize());
    button->setButtonStyle(Qt::ToolButtonIconOnly);
    button->setIcon(_icon);
    button->setText(_text);
    button->setCheckable(_checkable);
    button->setCursor(Qt::PointingHandCursor);
    if constexpr (platform::is_apple())
        connect(button, &PanelToolButton::clicked, this, [this]() { if (!isActiveWindow()) activateWindow(); });
    return button;
}

Ui::TransparentPanelButton * Ui::MiniWindowVideoPanel::createButton(const QString& _icon, const QString& _text, Qt::Alignment _align, bool _isAnimated)
{
    auto btn = new TransparentPanelButton(rootWidget_, _icon, _text, _align, _isAnimated);
    if constexpr (platform::is_apple())
        connect(btn, &TransparentPanelButton::clicked, this, [this]() { if (!isActiveWindow()) activateWindow(); });
    return btn;
}

void Ui::MiniWindowVideoPanel::updateVideoDeviceButtonsState()
{
    const auto videoEnabled = localVideoEnabled_ && isCameraEnabled_;
    videoButton_->setChecked(videoEnabled);
    videoButton_->setIcon(videoIcon(videoEnabled));
    videoButton_->setText(videoButtonText(videoEnabled));
    shareScreenButton_->setChecked(isScreenSharingEnabled_);
    shareScreenButton_->setText(screensharingButtonText(videoEnabled));
}

void Ui::MiniWindowVideoPanel::changeResizeButtonTooltip()
{
    static const auto textMinimize = QT_TRANSLATE_NOOP("voip_pages", "Minimize");
    static const auto textMaximize = QT_TRANSLATE_NOOP("voip_pages", "Maximize");
    resizeButton_->setTooltipText(resizeButton_->getTooltipText() == textMinimize ? textMaximize : textMinimize);
}

void Ui::MiniWindowVideoPanel::changeResizeButtonState()
{
    changeResizeButtonTooltip();
    resizeButton_->changeState();
}

bool Ui::MiniWindowVideoPanel::eventFilter(QObject* _watched, QEvent* _event)
{
    if (_watched == shareScreenButton_ && _event->type() == QEvent::MouseButtonPress)
    {
        if (static_cast<QMouseEvent*>(_event)->button() != Qt::LeftButton)
            return QWidget::eventFilter(_watched, _event);

        const bool isLocalDesktopAllowed = GetDispatcher()->getVoipController().isLocalDesktopAllowed();
        const auto& screens = GetDispatcher()->getVoipController().screenList();
        const int screenCount = screens.count();
        if (!isScreenSharingEnabled_ && isLocalDesktopAllowed && screenCount > 1)
        {
            showShareScreenMenu(screenCount);
            return true;
        }
    }
    return QWidget::eventFilter(_watched, _event);
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
    const auto radius = getBorderRadius();
    p.drawRect(0, 0, r.width(), radius);
    p.drawRoundedRect(r, radius, radius);
}

Ui::DetachedVideoWindow::DetachedVideoWindow(QWidget* _parent)
    : QWidget()
    , parent_(_parent)
    , pressPos_(kInvalidPoint)
    , resizeAnimation_(new QVariantAnimation(this))
{
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
        setWindowFlags(Qt::Window | Qt::WindowStaysOnTopHint | Qt::FramelessWindowHint);
        setAttribute(Qt::WA_X11DoNotAcceptFocus);
    }
    setFixedSize(getDetachedVideoWindowSize());
    setAttribute(Qt::WA_NoSystemBackground, true);
    setAttribute(Qt::WA_TranslucentBackground, true);
    setAttribute(Qt::WA_Hover, true);

    auto rootLayout = Utils::emptyVLayout(this);
    rootLayout->setAlignment(Qt::AlignTop);
    setLayout(rootLayout);

    QMetaObject::connectSlotsByName(this);

    shadow_ = std::unique_ptr<Ui::ShadowWindowParent>(new Ui::ShadowWindowParent(this));
    header_ = new MiniWindowHeader(this);
    rootLayout->addWidget(header_);
#ifndef STRIP_VOIP
    rootWidget_ = new OGLRenderWidget(this);
    rootWidget_->setMiniWindow(true);
    rootWidget_->getWidget()->setMinimumHeight(0);
    rootWidget_->getWidget()->setMaximumHeight(videoHeight());
    rootWidget_->getWidget()->setFixedWidth(width());
    rootWidget_->getWidget()->setSizePolicy(QSizePolicy(QSizePolicy::Fixed, QSizePolicy::Expanding));
    rootLayout->addWidget(rootWidget_->getWidget());
#endif

    // create panels after main widget, so it can be visible without raise()
    videoPanel_ = new MiniWindowVideoPanel(this);
    videoPanel_->setAttribute(Qt::WA_Hover);
    rootLayout->addWidget(videoPanel_);
    header_->show();
    videoPanel_->show();

    connect(&Ui::GetDispatcher()->getVoipController(), &voip_proxy::VoipController::onVoipMediaLocalAudio, this, [this](bool _enabled)
    {
        microphoneEnabled_ = _enabled && GetDispatcher()->getVoipController().isAudPermissionGranted();
        rootWidget_->localMediaChanged(microphoneEnabled_, videoEnabled_);
    });
    connect(&Ui::GetDispatcher()->getVoipController(), &voip_proxy::VoipController::onVoipMediaLocalVideo, this, [this](bool _enabled)
    {
        videoEnabled_ = _enabled;
        rootWidget_->localMediaChanged(microphoneEnabled_, videoEnabled_);
    });
    connect(&Ui::GetDispatcher()->getVoipController(), &voip_proxy::VoipController::onVoipCallDestroyed, this, &DetachedVideoWindow::onVoipCallDestroyed);
    connect(&Ui::GetDispatcher()->getVoipController(), &voip_proxy::VoipController::onVoipCallNameChanged, this, &DetachedVideoWindow::onVoipCallNameChanged);

    connect(videoPanel_, &MiniWindowVideoPanel::needShowScreenPermissionsPopup, this, [this](media::permissions::DeviceType type)
    {
        Q_EMIT activateMainVideoWindow();
        Q_EMIT needShowScreenPermissionsPopup(type);
    });

    connect(videoPanel_, &MiniWindowVideoPanel::onMicrophoneClick, this, &DetachedVideoWindow::onMicrophoneClick);
    connect(videoPanel_, &MiniWindowVideoPanel::onShareScreenClick, this, &DetachedVideoWindow::onShareScreenClick);
    connect(videoPanel_, &MiniWindowVideoPanel::onOpenCallClicked, this, &DetachedVideoWindow::activateMainVideoWindow);
    connect(videoPanel_, &MiniWindowVideoPanel::onResizeClicked, this, &DetachedVideoWindow::onPanelClickedResize);
    connect(videoPanel_, &MiniWindowVideoPanel::onHangUpClicked, this, &DetachedVideoWindow::onPanelClickedClose);

    connect(this, &DetachedVideoWindow::onShareScreenClick, this, [this](bool _on){ getRender()->setScreenSharing(_on); });

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
        setFixedHeight(val + panelsHeight());
        update();
    });

    WindowController* windowController = new WindowController(this);
    windowController->setOptions(WindowController::Dragging | WindowController::RequireMouseEvents);
    windowController->setBorderWidth(0);
    windowController->setWidget(this, this);
}

void Ui::DetachedVideoWindow::onVoipCallDestroyed(const voip_manager::ContactEx& _contactEx)
{
    if (_contactEx.current_call)
    {
        // in this moment destroyed call is active, e.a. call_count + 1
        rootWidget_->clear();
        closedManualy_ = false;
    }
    ScreenSharingManager::instance().stopSharing();
}

void Ui::DetachedVideoWindow::onVoipCallNameChanged(const voip_manager::ContactsList& _contacts)
{
    rootWidget_->updatePeers(_contacts);
}

void Ui::DetachedVideoWindow::onPanelClickedClose()
{
    Q_EMIT activateMainVideoWindow();
    closedManualy_ = true;
    hide();
    Q_EMIT requestHangup();
}

void Ui::DetachedVideoWindow::onPanelClickedResize()
{
    if (mode_ == WindowMode::Full)
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
    startTooltipTimer();
}

void Ui::DetachedVideoWindow::onPanelMouseLeave()
{
    Tooltip::hide();
    if (tooltipTimer_)
        tooltipTimer_->stop();
}

void Ui::DetachedVideoWindow::mousePressEvent(QMouseEvent* _e)
{
    pressPos_ = _e->globalPos();
    QWidget::mousePressEvent(_e);
}

void Ui::DetachedVideoWindow::mouseReleaseEvent(QMouseEvent *_e)
{
    if (pressPos_ != kInvalidPoint && Utils::clicked(pressPos_, _e->globalPos()))
    {
        Tooltip::hide();
        Q_EMIT activateMainVideoWindow();
        return;
    }
    pressPos_ = kInvalidPoint;
    QWidget::mouseReleaseEvent(_e);
}

void Ui::DetachedVideoWindow::mouseMoveEvent(QMouseEvent* _e)
{
    Tooltip::hide();
    startTooltipTimer();
    QWidget::mouseMoveEvent(_e);
}

void Ui::DetachedVideoWindow::enterEvent(QEvent* _e)
{
    QWidget::enterEvent(_e);
    Tooltip::forceShow(true);
    onPanelMouseEnter();
    activateWindow();
}

void Ui::DetachedVideoWindow::leaveEvent(QEvent* _e)
{
    QWidget::leaveEvent(_e);
    Tooltip::forceShow(false);
    onPanelMouseLeave();
}

void Ui::DetachedVideoWindow::mouseDoubleClickEvent(QMouseEvent* /*e*/)
{
    Tooltip::hide();
    Q_EMIT activateMainVideoWindow();
}

void Ui::DetachedVideoWindow::contextMenuEvent(QContextMenuEvent*)
{
    // If user press "menu" key on keyboard then QContextMenuEvent in
    // globalPos() method will return the center of widget, which
    // is inacceptable. Since we can't distinguish from key presses and mouse
    // presses within the QContextMenuEvent we have to take a cursor position
    // to retrieve actual mouse position to show menu there
    const QPoint globalPos = QCursor::pos();
    const QPoint localPos = rootWidget_->getWidget()->mapFromGlobal(globalPos);
    if (QMenu* menu = rootWidget_->createContextMenu(localPos))
        menu->exec(globalPos);
}

void Ui::DetachedVideoWindow::resizeAnimated(ResizeDirection _dir)
{
    if (resizeAnimation_->state() == QVariantAnimation::Running)
        return;

    const auto newHeight = _dir == ResizeDirection::Maximize ? videoHeight() : 0;
    resizeAnimation_->stop();
    resizeAnimation_->setStartValue(_dir == ResizeDirection::Maximize ? 0 : videoHeight());
    resizeAnimation_->setEndValue(newHeight);
    resizeAnimation_->start();
}

void Ui::DetachedVideoWindow::onTooltipTimer()
{
    if (!isVisible() || resizeAnimation_->state() == QVariantAnimation::Running)
        return;

    const auto pos = QCursor::pos();
    QRect r(geometry().topLeft(), header_->size());
    if (r.contains(pos))
        Tooltip::show(getTooltipText(), QRect(QCursor::pos(), Utils::scale_value(QSize())), getTooltipSize());
}

void Ui::DetachedVideoWindow::showFrame(WindowMode _mode)
{
    setWindowMode(_mode);
#ifdef __APPLE__
    MacSupport::showInAllWorkspaces(this);
#else
    showNormal();
#endif
    Utils::InterConnector::instance().getMainWindow()->updateMainMenu();
}

void Ui::DetachedVideoWindow::hideFrame()
{
    hide();
    Utils::InterConnector::instance().getMainWindow()->updateMainMenu();
}

void Ui::DetachedVideoWindow::showEvent(QShowEvent* _e)
{
    QWidget::showEvent(_e);
    rootWidget_->getWidget()->update();
    activateWindow();
}

bool Ui::DetachedVideoWindow::closedManualy()
{
    return closedManualy_;
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
}

bool Ui::DetachedVideoWindow::isMinimized() const
{
    const auto isMin = QWidget::isMinimized();
    if constexpr (platform::is_linux())
        return isMin;

    return isMin && (videoPanel_ && !videoPanel_->isVisible());
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
            setFixedHeight(panelsHeight());
        }
        else
        {
            setFixedHeight(getDetachedVideoWindowSize().height());
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
    : QWidget(_parent)
{
    setFixedHeight(headerHeight());
    setFixedWidth(getWidth());
    setMouseTracking(true);

    text_ = TextRendering::MakeTextUnit(QString());
    text_->init({ titleFont(), Styling::ThemeColorKey{ Styling::StyleVariable::TEXT_SOLID_PERMANENT } });
    text_->evaluateDesiredSize();
}

void Ui::MiniWindowHeader::setTitle(const QString& _title)
{
    text_->setText(_title);
    text_->evaluateDesiredSize();
    updateTitleOffsets();
    update();
}

void Ui::MiniWindowHeader::paintEvent(QPaintEvent* _event)
{
    QWidget::paintEvent(_event);

    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    p.setPen(Qt::NoPen);
    const auto color = Styling::getParameters().getColor(Styling::StyleVariable::LUCENT_TERTIARY);
    auto w = width();
    auto h = height();
    const auto r = getBorderRadius();

    p.setBrush(Qt::black);
    p.drawRoundedRect(0, 0, w, h + r, r, r);
    p.setBrush(color);
    p.drawRoundedRect(0, 0, w, h + r, r, r);

    p.setCompositionMode(QPainter::CompositionMode_Source);
    p.setBrush(Qt::transparent);

    p.drawRect(0, h, w, r);

    p.setPen({ Qt::black, 2.0 });
    const QLine left(0, h + windowBorderWidth(), 0, videoHeight() + h);
    p.drawLine(left);
    const QLine right(w, h + windowBorderWidth(), w, videoHeight() + h);
    p.drawLine(right);

    text_->draw(p, TextRendering::VerPosition::MIDDLE);
}

void Ui::MiniWindowHeader::resizeEvent(QResizeEvent* event)
{
    updateTitleOffsets();
}

void Ui::MiniWindowHeader::updateTitleOffsets()
{
    const auto r = rect();
    text_->setOffsets(r.width() / 2 - text_->cachedSize().width() / 2, headerHeight() / 2);
}
