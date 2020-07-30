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

#ifdef _WIN32
    #include <windows.h>
#endif

#ifdef __APPLE__
    #include "../utils/macos/mac_support.h"
    #include "macos/VideoFrameMacos.h"
#endif

namespace
{
    enum { kAnimationDefDuration = 500 };

    auto getDetachedVideoWindowSize() noexcept
    {
        if constexpr (platform::is_linux())
            return Utils::scale_value(QSize(240, 224));
        else
            return Utils::scale_value(QSize(240, 180));
    }

    auto getButtonSize() noexcept
    {
        return Utils::scale_value(QSize(40, 40));
    }

    auto getPanelBackgroundColorLinux() noexcept
    {
        return QColor(38, 38, 38, 255);
    }

    auto getPanelOffsetFromButton() noexcept
    {
        return Utils::scale_value(22);
    }

    auto getButtonsVerOffset() noexcept
    {
        if constexpr (platform::is_linux())
            return Utils::scale_value(4);
        else
            return Utils::scale_value(0);
    }
}

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
    , isFadedVisible_(false)
    , localVideoEnabled_(false)
    , isScreenSharingEnabled_(false)
    , isCameraEnabled_(true)
{
    if constexpr (!platform::is_linux())
    {
        setAttribute(Qt::WA_NoSystemBackground, true);
        setAttribute(Qt::WA_TranslucentBackground, true);
    }

    setStyleSheet(Utils::LoadStyle(qsl(":/qss/video_panel_mini_window")));
    setProperty("VideoPanelMain", true);

    auto rootLayout = Utils::emptyVLayout();

    if constexpr (platform::is_linux())
        rootLayout->setAlignment(Qt::AlignVCenter);
    else
        rootLayout->setAlignment(Qt::AlignBottom);

    rootLayout->setContentsMargins(0, getButtonsVerOffset(), 0, getButtonsVerOffset());
    setLayout(rootLayout);

    rootWidget_ = new QWidget(this);
    rootWidget_->setContentsMargins(0, 0, 0, 0);
    rootWidget_->setProperty("VideoPanel", true);
    rootLayout->addWidget(rootWidget_);

    auto layoutTarget = Utils::emptyHLayout();
    layoutTarget->setAlignment(Qt::AlignVCenter);
    rootWidget_->setLayout(layoutTarget);

    auto addButton = [this, parent = rootWidget_, layoutTarget](const char* _propertyName, auto _slot) -> QPushButton*
    {
        auto btn = new QPushButton(parent);
        if (_propertyName != NULL)
            btn->setProperty(_propertyName, true);
        btn->setSizePolicy(QSizePolicy(QSizePolicy::Preferred, QSizePolicy::Expanding));
        btn->setCursor(QCursor(Qt::PointingHandCursor));
        btn->setFlat(true);

        layoutTarget->addWidget(btn);

        if constexpr (platform::is_apple())
            connect(btn, &QPushButton::clicked, this, [this](){ if (!isActiveWindow()) activateWindow(); });

        connect(btn, &QPushButton::clicked, this, std::forward<decltype(_slot)>(_slot));

        return btn;
    };

    layoutTarget->addStretch(1);
    stopCallButton_ = addButton("StopCallButton", &MiniWindowVideoPanel::onHangUpButtonClicked);
    layoutTarget->addStretch(1);
    micButton_ = addButton("CallEnableMic", &MiniWindowVideoPanel::onAudioOnOffClicked);
    micButton_->setProperty("CallDisableMic", false);
    layoutTarget->addStretch(1);
    shareScreenButton_ = addButton("ShareScreenButtonDisable", &MiniWindowVideoPanel::onShareScreen);
    layoutTarget->addStretch(1);
    videoButton_ = addButton(NULL, &MiniWindowVideoPanel::onVideoOnOffClicked);
    layoutTarget->addStretch(1);

    resetHangupText();

    connect(&Ui::GetDispatcher()->getVoipController(), &voip_proxy::VoipController::onVoipMediaLocalAudio, this, &MiniWindowVideoPanel::onVoipMediaLocalAudio);
    connect(&Ui::GetDispatcher()->getVoipController(), &voip_proxy::VoipController::onVoipMediaLocalVideo, this, &MiniWindowVideoPanel::onVoipMediaLocalVideo);
    //QObject::connect(&Ui::GetDispatcher()->getVoipController(), SIGNAL(onVoipCallNameChanged(const voip_manager::ContactsList&)), this, SLOT(onVoipCallNameChanged(const voip_manager::ContactsList&)), Qt::DirectConnection);
    //QObject::connect(&Ui::GetDispatcher()->getVoipController(), SIGNAL(onVoipMinimalBandwidthChanged(bool)), this, SLOT(onVoipMinimalBandwidthChanged(bool)), Qt::DirectConnection);
    connect(&Ui::GetDispatcher()->getVoipController(), &voip_proxy::VoipController::onVoipVideoDeviceSelected, this, &MiniWindowVideoPanel::onVoipVideoDeviceSelected);

}

void Ui::MiniWindowVideoPanel::fadeIn(unsigned int _kAnimationDefDuration)
{
    if (!isFadedVisible_)
    {
        isFadedVisible_ = true;
        if constexpr (!platform::is_linux())
            BaseVideoPanel::fadeIn(_kAnimationDefDuration);
    }
}

void Ui::MiniWindowVideoPanel::fadeOut(unsigned int _kAnimationDefDuration)
{
    if (isFadedVisible_)
    {
        isFadedVisible_ = false;
        if constexpr (!platform::is_linux())
            BaseVideoPanel::fadeOut(_kAnimationDefDuration);
    }
}

bool Ui::MiniWindowVideoPanel::isFadedIn()
{
    return isFadedVisible_;
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

void Ui::MiniWindowVideoPanel::updatePosition(const QWidget& _parent)
{
    const auto rc = parentWidget()->geometry();

    if (platform::is_linux())
        move(0, rc.height() - rect().height());
    else
        move(rc.x(), rc.y() + rc.height() - rect().height() - getPanelOffsetFromButton());

    setFixedWidth(rc.width());
}


Ui::DetachedVideoWindow::DetachedVideoWindow(QWidget* _parent)
    : QWidget()
    , showPanelTimer_(this)
    , parent_(_parent)
    , closedManualy_(false)
    , videoPanel_(new MiniWindowVideoPanel(this))
    , shadow_(new Ui::ShadowWindowParent(this))
{
    setFixedSize(getDetachedVideoWindowSize());

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
    }

#ifndef STRIP_VOIP
    std::vector<QPointer<Ui::BaseVideoPanel>> panels = { videoPanel_ };
    rootWidget_ = platform_specific::GraphicsPanel::create(this, panels, false, false);
    rootWidget_->setContentsMargins(0, 0, 0, 0);
    rootWidget_->setAttribute(Qt::WA_UpdatesDisabled);
    rootWidget_->setSizePolicy(QSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding));

    if constexpr (!platform::is_linux())
    {
        rootLayout->addWidget(rootWidget_);
    }
    else
    {
        rootLayout->addWidget(rootWidget_, 1);
        rootLayout->addWidget(videoPanel_);
    }

#endif

    std::vector<QPointer<BaseVideoPanel>> videoPanels = { videoPanel_ };
    eventFilter_ = new ResizeEventFilter(videoPanels, shadow_->getShadowWidget(), this);
    installEventFilter(eventFilter_);

    if constexpr (!platform::is_linux())
    {
        setAttribute(Qt::WA_UpdatesDisabled);
        setAttribute(Qt::WA_ShowWithoutActivating);
    }

    connect(&showPanelTimer_, &QTimer::timeout, this, &DetachedVideoWindow::checkPanelsVis);
    showPanelTimer_.setInterval(1500);

    connect(&Ui::GetDispatcher()->getVoipController(), &voip_proxy::VoipController::onVoipWindowRemoveComplete, this, &DetachedVideoWindow::onVoipWindowRemoveComplete);
    connect(&Ui::GetDispatcher()->getVoipController(), &voip_proxy::VoipController::onVoipWindowAddComplete, this, &DetachedVideoWindow::onVoipWindowAddComplete);
    connect(&Ui::GetDispatcher()->getVoipController(), &voip_proxy::VoipController::onVoipCallDestroyed, this, &DetachedVideoWindow::onVoipCallDestroyed);

    //setMinimumSize(Utils::scale_value(320), Utils::scale_value(80));

    if (videoPanel_)
    {
        videoPanel_->fadeOut(0);
        connect(videoPanel_, &MiniWindowVideoPanel::onMouseLeave, this, &DetachedVideoWindow::onPanelMouseLeave);
        connect(videoPanel_, &MiniWindowVideoPanel::onMouseEnter, this, &DetachedVideoWindow::onPanelMouseEnter);
        connect(videoPanel_, &MiniWindowVideoPanel::needShowScreenPermissionsPopup, this, [this](media::permissions::DeviceType type)
        {
            activateMainVideoWindow();
            Q_EMIT needShowScreenPermissionsPopup(type);
        });
    }

    const auto screenRect = Utils::mainWindowScreen()->geometry();
    const auto detachedWndRect = rect();

    auto detachedWndPos = screenRect.topRight();
    detachedWndPos.setX(detachedWndPos.x() - detachedWndRect.width() - 0.01f * screenRect.width());
    detachedWndPos.setY(detachedWndPos.y() + 0.05f * screenRect.height());

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
    if (_contactEx.current_call)
    {
        // in this moment destroyed call is active, e.a. call_count + 1
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
        hide();
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
    assert(rootWidget_->frameId());
    if (rootWidget_->frameId())
        Ui::GetDispatcher()->getVoipController().setWindowAdd((quintptr)rootWidget_->frameId(), "", false, false, 0);
    //shadow_->showShadow();
}

void Ui::DetachedVideoWindow::hideFrame()
{
    assert(rootWidget_->frameId());
    if (rootWidget_->frameId())
        Ui::GetDispatcher()->getVoipController().setWindowRemove((quintptr)rootWidget_->frameId());
    //shadow_->hideShadow();

    if (videoPanel_)
        videoPanel_->hide();
}

void Ui::DetachedVideoWindow::showEvent(QShowEvent* _e)
{
    QWidget::showEvent(_e);

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
        if (parent_->isMinimized())
        {
            Q_EMIT windowWillDeminiaturize();
            parent_->showNormal();
            Q_EMIT windowDidDeminiaturize();
        }
        parent_->raise();
        parent_->activateWindow();
        //hide();
    }
}

bool Ui::DetachedVideoWindow::isMinimized() const
{
    return QWidget::isMinimized() && (videoPanel_ && !videoPanel_->isVisible());
}
