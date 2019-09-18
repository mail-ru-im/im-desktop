#include "stdafx.h"
#include "DetachedVideoWnd.h"
#include "../core_dispatcher.h"
#include "../../core/Voip/VoipManagerDefines.h"
#include "../utils/InterConnector.h"

#include "../utils/utils.h"
#include "VideoPanelHeader.h"
#include "VideoWindow.h"
#include "CommonUI.h"
#include "../controls/ContextMenu.h"
#include "../main_window/MainWindow.h"
#include "VoipTools.h"

#ifdef _WIN32
    #include "QDesktopWidget"
    #include <windows.h>
#endif

#ifdef __APPLE__
    #include "../utils/macos/mac_support.h"
    #include "macos/VideoFrameMacos.h"
#endif

const QSize cDetachedVideoWindowSize(240, 180);

enum { kAnimationDefDuration = 500 };

#define SHOW_HEADER_PANEL false

void Ui::MiniWindowVideoPanel::onHangUpButtonClicked()
{
    Ui::GetDispatcher()->getVoipController().setHangup();
}
void Ui::MiniWindowVideoPanel::onAudioOnOffClicked()
{
    Ui::GetDispatcher()->getVoipController().setSwitchACaptureMute();
}
void Ui::MiniWindowVideoPanel::onVideoOnOffClicked()
{
    Ui::GetDispatcher()->getVoipController().setSwitchVCaptureMute();
}

void Ui::MiniWindowVideoPanel::onVoipMediaLocalAudio(bool _enabled)
{
    if (micButton_)
    {
        micButton_->setProperty("CallEnableMic", _enabled);
        micButton_->setProperty("CallDisableMic", !_enabled);
        micButton_->setStyle(QApplication::style());
    }
}

void Ui::MiniWindowVideoPanel::onVoipMediaLocalVideo(bool _enabled)
{
    if (!videoButton_)
    {
        return;
    }

    localVideoEnabled_ = _enabled;

    updateVideoDeviceButtonsState();
}

void Ui::MiniWindowVideoPanel::onShareScreen()
{
    const QList<voip_proxy::device_desc>& screens = Ui::GetDispatcher()->getVoipController().screenList();
    int screenIndex = 0;

    if (!isScreenSharingEnabled_ && screens.size() > 1)
    {
        Ui::ContextMenu menu(this);
        Ui::ContextMenu::applyStyle(&menu, false, Utils::scale_value(15), Utils::scale_value(36));
        for (int i = 0; i < screens.size(); i++)
        {
            menu.addAction(QT_TRANSLATE_NOOP("voip_pages", "Screen") % ql1c(' ') % QString::number(i + 1), [i, this, screens]() {
                isScreenSharingEnabled_ = !isScreenSharingEnabled_;
                Ui::GetDispatcher()->getVoipController().switchShareScreen(!screens.empty() ? &screens[i] : nullptr);
                updateVideoDeviceButtonsState();
            });
        }

        menu.exec(QCursor::pos());
    }
    else
    {
        isScreenSharingEnabled_ = !isScreenSharingEnabled_;
        Ui::GetDispatcher()->getVoipController().switchShareScreen(!screens.empty() ? &screens[screenIndex] : nullptr);
        updateVideoDeviceButtonsState();
    }
}

void Ui::MiniWindowVideoPanel::onVoipVideoDeviceSelected(const voip_proxy::device_desc& desc)
{
    isScreenSharingEnabled_ = (desc.dev_type == voip_proxy::kvoipDevTypeVideoCapture && desc.video_dev_type == voip_proxy::kvoipDeviceDesktop);
    isCameraEnabled_ = (desc.dev_type == voip_proxy::kvoipDevTypeVideoCapture && desc.video_dev_type == voip_proxy::kvoipDeviceCamera);

    updateVideoDeviceButtonsState();
}


Ui::MiniWindowVideoPanel::MiniWindowVideoPanel(QWidget* _parent)
#ifdef __APPLE__
    : BaseBottomVideoPanel(_parent, Qt::Window | Qt::FramelessWindowHint | Qt::WindowDoesNotAcceptFocus | Qt::NoDropShadowWindowHint)
#elif defined(__linux__)
    : BaseBottomVideoPanel(_parent, Qt::Widget)
#else
    : BaseBottomVideoPanel(_parent)
#endif
    , mouseUnderPanel_(false)
    , parent_(_parent)
    , rootWidget_(nullptr)
    , micButton_(nullptr)
    , stopCallButton_(nullptr)
    , videoButton_(nullptr)
    , isTakling(false)
    , isFadedVisible(false)
    , localVideoEnabled_(false)
    , isScreenSharingEnabled_(false)
    , isCameraEnabled_(true)
{
    //setWindowFlags(Qt::SubWindow | Qt::FramelessWindowHint);
#ifndef __linux__
    setStyleSheet(Utils::LoadStyle(qsl(":/qss/video_panel_mini_window")));
    setProperty("VideoPanelMain", true);
    setAttribute(Qt::WA_NoSystemBackground, true);
    setAttribute(Qt::WA_TranslucentBackground, true);
#else
    setStyleSheet(Utils::LoadStyle(qsl(":/qss/video_panel_linux")));
    setProperty("VideoPanelMain", true);
#endif

    QVBoxLayout* rootLayout = Utils::emptyVLayout();
    rootLayout->setAlignment(Qt::AlignBottom);
#ifndef __linux__
    rootLayout->setContentsMargins(0, 0, 0, 0);
#endif
    setLayout(rootLayout);

    rootWidget_ = new QWidget(this);
    rootWidget_->setContentsMargins(0, 0, 0, 0);
    rootWidget_->setProperty("VideoPanel", true);
    layout()->addWidget(rootWidget_);

    QHBoxLayout* layoutTarget = Utils::emptyHLayout();
    layoutTarget->setAlignment(Qt::AlignVCenter);
    rootWidget_->setLayout(layoutTarget);

    QWidget* parentWidget = rootWidget_;
    auto addButton = [this, parentWidget, layoutTarget](const char* _propertyName, const char* _slot, QHBoxLayout* layout = nullptr)->QPushButton*
    {
        QPushButton* btn = new voipTools::BoundBox<QPushButton>(parentWidget);
        if (_propertyName != NULL)
        {
            btn->setProperty(_propertyName, true);
        }
        btn->setSizePolicy(QSizePolicy(QSizePolicy::Preferred, QSizePolicy::Expanding));
        btn->setCursor(QCursor(Qt::PointingHandCursor));
        btn->setFlat(true);
        (layout ? layout : layoutTarget)->addWidget(btn);

#ifdef __APPLE__
        connect(btn, &QPushButton::clicked, this, &MiniWindowVideoPanel::activateWindow, Qt::QueuedConnection);
#endif
        connect(btn, SIGNAL(clicked()), this, _slot, Qt::QueuedConnection);

        return btn;
    };

    layoutTarget->addStretch(1);
    stopCallButton_ = addButton("StopCallButton", SLOT(onHangUpButtonClicked()));
    layoutTarget->addStretch(1);
    micButton_ = addButton("CallEnableMic", SLOT(onAudioOnOffClicked()));
    micButton_->setProperty("CallDisableMic", false);
    layoutTarget->addStretch(1);
    shareScreenButton_ = addButton("ShareScreenButtonDisable", SLOT(onShareScreen()));
#ifdef __linux__
    shareScreenButton_->hide();
#else
    layoutTarget->addStretch(1);
#endif

    videoButton_ = addButton(NULL, SLOT(onVideoOnOffClicked()));
    layoutTarget->addStretch(1);

    resetHangupText();

    QObject::connect(&Ui::GetDispatcher()->getVoipController(), SIGNAL(onVoipMediaLocalAudio(bool)), this, SLOT(onVoipMediaLocalAudio(bool)), Qt::DirectConnection);
    QObject::connect(&Ui::GetDispatcher()->getVoipController(), SIGNAL(onVoipMediaLocalVideo(bool)), this, SLOT(onVoipMediaLocalVideo(bool)), Qt::DirectConnection);
    //QObject::connect(&Ui::GetDispatcher()->getVoipController(), SIGNAL(onVoipCallNameChanged(const voip_manager::ContactsList&)), this, SLOT(onVoipCallNameChanged(const voip_manager::ContactsList&)), Qt::DirectConnection);
    //QObject::connect(&Ui::GetDispatcher()->getVoipController(), SIGNAL(onVoipMinimalBandwidthChanged(bool)), this, SLOT(onVoipMinimalBandwidthChanged(bool)), Qt::DirectConnection);
    QObject::connect(&Ui::GetDispatcher()->getVoipController(), SIGNAL(onVoipVideoDeviceSelected(const voip_proxy::device_desc&)), this, SLOT(onVoipVideoDeviceSelected(const voip_proxy::device_desc&)), Qt::DirectConnection);

    videoPanelEffect_ = std::make_unique<Ui::UIEffects>(*this);
}

void Ui::MiniWindowVideoPanel::fadeIn(unsigned int kAnimationDefDuration)
{
    isFadedVisible = true;
#ifndef __linux__
    videoPanelEffect_->fadeIn(kAnimationDefDuration);
#endif
}

void Ui::MiniWindowVideoPanel::fadeOut(unsigned int kAnimationDefDuration)
{
    isFadedVisible = false;
#ifndef __linux__
    videoPanelEffect_->fadeOut(kAnimationDefDuration);
#endif
}

bool Ui::MiniWindowVideoPanel::isFadedIn()
{
    return isFadedVisible;
}

bool Ui::MiniWindowVideoPanel::isUnderMouse()
{
    return mouseUnderPanel_;
}

void Ui::MiniWindowVideoPanel::resetHangupText()
{
    if (stopCallButton_)
    {
        stopCallButton_->setText(QString());
        stopCallButton_->repaint();
    }
}

void Ui::MiniWindowVideoPanel::updateVideoDeviceButtonsState()
{
    bool enableCameraButton = localVideoEnabled_ && isCameraEnabled_;
    bool enableScreenButton = localVideoEnabled_ && isScreenSharingEnabled_;

    videoButton_->setProperty("CallEnableCam", enableCameraButton);
    videoButton_->setProperty("CallDisableCam", !enableCameraButton);

    shareScreenButton_->setProperty("ShareScreenButtonDisable", !enableScreenButton);
    shareScreenButton_->setProperty("ShareScreenButtonEnable", enableScreenButton);

    videoButton_->setStyle(QApplication::style());
    shareScreenButton_->setStyle(QApplication::style());
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
        emit onMouseEnter();
    }
}

void Ui::MiniWindowVideoPanel::leaveEvent(QEvent* _e)
{
    QWidget::leaveEvent(_e);

    mouseUnderPanel_ = false;
    emit onMouseLeave();
}

void Ui::MiniWindowVideoPanel::resizeEvent(QResizeEvent* _e)
{
    QWidget::resizeEvent(_e);

#ifdef __APPLE__
    // Forced set fixed size, because under mac we use cocoa to change panel size.
    setFixedSize(size());
#endif
}

void Ui::MiniWindowVideoPanel::updatePosition(const QWidget& parent)
{
    // We have code dublication here,
    // because Mac and Qt have different coords systems.
    // We can convert Mac coords to Qt, but we need to add
    // special cases for multi monitor systems.
    int offsetFromButton = Utils::scale_value(22);
#ifdef __APPLE__
    //auto rc = platform_macos::getWindowRect(*parentWidget());
    QRect parentRect = platform_macos::getWidgetRect(*parentWidget());
    platform_macos::setWindowPosition(*this,
        QRect(parentRect.left(),
            parentRect.top() + offsetFromButton,
            parentRect.width(),
            height()));
#elif defined(__linux__)
    auto rc = parentWidget()->geometry();
    move(0, rc.height() - rect().height() - offsetFromButton);
    setFixedWidth(rc.width());
#else
    auto rc = parentWidget()->geometry();
    move(rc.x(), rc.y() + rc.height() - rect().height() - offsetFromButton);
    setFixedWidth(rc.width());
#endif
}


Ui::DetachedVideoWindow::DetachedVideoWindow(QWidget* parent)
    : QWidget()
    , showPanelTimer_(this)
    , parent_(parent)
    , closedManualy_(false)
    , videoPanel_(new MiniWindowVideoPanel(this))
    , shadow_ (new Ui::ShadowWindowParent(this))
{
    setFixedSize(Utils::scale_value(cDetachedVideoWindowSize));

    horizontalLayout_ = Utils::emptyHLayout(this);
    horizontalLayout_->setAlignment(Qt::AlignVCenter);
    QMetaObject::connectSlotsByName(this);

#ifdef _WIN32
    setWindowFlags(Qt::WindowStaysOnTopHint | Qt::FramelessWindowHint | Qt::SubWindow);
#else
        // We have problem on Mac with WA_ShowWithoutActivating and WindowDoesNotAcceptFocus.
        // If video window is showing with this flags, we cannot activate main ICQ window.
        // UPDATED: Looks like for mini video window it works ok with  Qt::WindowDoesNotAcceptFocus
        // and Qt::WA_ShowWithoutActivating.
        setWindowFlags(Qt::WindowStaysOnTopHint | Qt::Window | Qt::WindowDoesNotAcceptFocus);
        //setAttribute(Qt::WA_ShowWithoutActivating);
        setAttribute(Qt::WA_X11DoNotAcceptFocus);
#endif

#ifndef STRIP_VOIP
    std::vector<QPointer<Ui::BaseVideoPanel>> panels;
    rootWidget_ = platform_specific::GraphicsPanel::create(this, panels, false, false);
    rootWidget_->setContentsMargins(0, 0, 0, 0);
    rootWidget_->setAttribute(Qt::WA_UpdatesDisabled);
    rootWidget_->setSizePolicy(QSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding));
    layout()->addWidget(rootWidget_);
#endif //__linux__

    std::vector<QPointer<BaseVideoPanel>> videoPanels;

    if (videoPanel_)
    {
        videoPanels.push_back(videoPanel_);
    }

    eventFilter_ = new ResizeEventFilter(videoPanels, shadow_->getShadowWidget(), this);
    installEventFilter(eventFilter_);

    setAttribute(Qt::WA_UpdatesDisabled);
    setAttribute(Qt::WA_ShowWithoutActivating);

    connect(&showPanelTimer_, SIGNAL(timeout()), this, SLOT(checkPanelsVis()), Qt::QueuedConnection);
    showPanelTimer_.setInterval(1500);

    connect(&Ui::GetDispatcher()->getVoipController(), SIGNAL(onVoipWindowRemoveComplete(quintptr)), this, SLOT(onVoipWindowRemoveComplete(quintptr)), Qt::DirectConnection);
    connect(&Ui::GetDispatcher()->getVoipController(), SIGNAL(onVoipWindowAddComplete(quintptr)), this, SLOT(onVoipWindowAddComplete(quintptr)), Qt::DirectConnection);
    connect(&Ui::GetDispatcher()->getVoipController(), SIGNAL(onVoipCallDestroyed(const voip_manager::ContactEx&)), this, SLOT(onVoipCallDestroyed(const voip_manager::ContactEx&)), Qt::DirectConnection);

    //setMinimumSize(Utils::scale_value(320), Utils::scale_value(80));

    QDesktopWidget dw;
    const auto screenRect = dw.screenGeometry(dw.primaryScreen());
    const auto detachedWndRect = rect();

    auto detachedWndPos = screenRect.topRight();
    detachedWndPos.setX(detachedWndPos.x() - detachedWndRect.width() - 0.01f * screenRect.width());
    detachedWndPos.setY(detachedWndPos.y() + 0.05f * screenRect.height());

    if (videoPanel_)
    {
        videoPanel_->fadeOut(0);
        connect(videoPanel_, SIGNAL(onMouseLeave()), this, SLOT(onPanelMouseLeave()), Qt::QueuedConnection);
        connect(videoPanel_, SIGNAL(onMouseEnter()), this, SLOT(onPanelMouseEnter()), Qt::QueuedConnection);
    }

    const QRect rc(detachedWndPos.x(), detachedWndPos.y(), detachedWndRect.width(), detachedWndRect.height());
    setGeometry(rc);

    // Round rect.
    //QPainterPath path(QPointF(0, 0));
    //path.addRoundRect(rect(), Utils::scale_value(8));
    //QRegion region(path.toFillPolygon().toPolygon());
    //setMask(region);
}

void Ui::DetachedVideoWindow::onVoipCallDestroyed(const voip_manager::ContactEx& _contactEx)
{
    if (_contactEx.call_count <= 1)
    { // in this moment destroyed call is active, e.a. call_count + 1
        closedManualy_ = false;
    }
}

void Ui::DetachedVideoWindow::onPanelClickedClose()
{
    closedManualy_ = true;
    hide();
}
void Ui::DetachedVideoWindow::onPanelClickedMinimize()
{
    //showMinimized();
}
void Ui::DetachedVideoWindow::onPanelClickedMaximize()
{
    //if (_parent) {
    //_parent->showNormal();
    //_parent->activateWindow();
    //hide();
    //}
}

void Ui::DetachedVideoWindow::onPanelMouseEnter()
{
    if (videoPanel_)
    {
        videoPanel_->setVisible(isVisible());
        updatePanels();
        videoPanel_->fadeIn(kAnimationDefDuration);
        Ui::GetDispatcher()->getVoipController().passWindowHover(rootWidget_->frameId(), true);
    }
}

void Ui::DetachedVideoWindow::onPanelMouseLeave()
{
    const QPoint p = mapFromGlobal(QCursor::pos());
    if (videoPanel_ && !rect().contains(p))
    {
        //updatePanels();
        videoPanel_->fadeOut(kAnimationDefDuration);
        Ui::GetDispatcher()->getVoipController().passWindowHover(rootWidget_->frameId(), false);
    }
}

void Ui::DetachedVideoWindow::mousePressEvent(QMouseEvent* _e)
{
    posDragBegin_ = _e->pos();
}

void Ui::DetachedVideoWindow::mouseMoveEvent(QMouseEvent* _e)
{
    if (_e->buttons() & Qt::LeftButton)
    {
        QPoint diff = _e->pos() - posDragBegin_;
        QPoint newpos = this->pos() + diff;

        this->move(newpos);
    }
}

void Ui::DetachedVideoWindow::enterEvent(QEvent* _e)
{
    QWidget::enterEvent(_e);
    onPanelMouseEnter();
    //showPanelTimer_.stop();
}

void Ui::DetachedVideoWindow::leaveEvent(QEvent* _e)
{
    QWidget::leaveEvent(_e);

    onPanelMouseLeave();
    //showPanelTimer_.start();
}

void Ui::DetachedVideoWindow::mouseDoubleClickEvent(QMouseEvent* /*e*/)
{
    if (parent_)
    {
        if (parent_->isMinimized())
        {
            emit windowWillDeminiaturize();
            parent_->showNormal();
            emit windowDidDeminiaturize();
        }
        parent_->raise();
        parent_->activateWindow();
        //hide();
    }
}

quintptr Ui::DetachedVideoWindow::getContentWinId()
{
    return (quintptr)rootWidget_->frameId();
}

void Ui::DetachedVideoWindow::onVoipWindowRemoveComplete(quintptr _winId)
{
    if (_winId == rootWidget_->frameId())
    {
        hide();
    }
    Utils::InterConnector::instance().getMainWindow()->updateMainMenu();
}

void Ui::DetachedVideoWindow::onVoipWindowAddComplete(quintptr _winId)
{
    if (_winId == rootWidget_->frameId())
    {
        updatePanels();
        showNormal();
    }
    Utils::InterConnector::instance().getMainWindow()->updateMainMenu();
}

void Ui::DetachedVideoWindow::showFrame()
{
#ifndef __linux__
    assert(rootWidget_->frameId());
    if (rootWidget_->frameId())
    {
        Ui::GetDispatcher()->getVoipController().setWindowAdd((quintptr)rootWidget_->frameId(), false, false, 0);
    }
    //shadow_->showShadow();
#endif
}

void Ui::DetachedVideoWindow::hideFrame()
{
#ifndef __linux__
    assert(rootWidget_->frameId());
    if (rootWidget_->frameId())
    {
        Ui::GetDispatcher()->getVoipController().setWindowRemove((quintptr)rootWidget_->frameId());
    }
    //shadow_->hideShadow();
#endif
    if (videoPanel_)
    {
        videoPanel_->hide();
    }
}

void Ui::DetachedVideoWindow::showEvent(QShowEvent* _e)
{
    QWidget::showEvent(_e);
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
        videoPanel_->fadeOut(kAnimationDefDuration);
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

void Ui::DetachedVideoWindow::checkPanelsVis()
{
    showPanelTimer_.stop();
}

void Ui::DetachedVideoWindow::updatePanels() const
{
    if (!rootWidget_ || !videoPanel_)
    {
        assert(false);
        return;
    }

#ifdef __APPLE__

    rootWidget_->clearPanels();
    std::vector<QPointer<Ui::BaseVideoPanel>> panels;

    if (videoPanel_ && videoPanel_->isVisible())
    {
        panels.push_back(videoPanel_);
    }

    rootWidget_->addPanels(panels);

    if (videoPanel_ && videoPanel_->isVisible())
    {
        videoPanel_->updatePosition(*this);
    }
#endif
}

bool Ui::DetachedVideoWindow::isMinimized() const
{
    return QWidget::isMinimized() && (videoPanel_ && !videoPanel_->isVisible());
}
