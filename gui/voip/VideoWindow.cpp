#include "stdafx.h"
#include "VideoWindow.h"
#include "DetachedVideoWnd.h"
#include "../../core/Voip/VoipManagerDefines.h"

#include "../core_dispatcher.h"
#include "../utils/gui_metrics.h"
#include "../main_window/containers/FriendlyContainer.h"
#include "../main_window/MainPage.h"
#include "../main_window/MainWindow.h"
#include "../gui_settings.h"
#include "MaskManager.h"
#include "VideoPanel.h"
#include "MaskPanel.h"
#include "secureCallWnd.h"

#include "SelectionContactsForConference.h"
#include "styles/ThemeParameters.h"
#include "../controls/CustomButton.h"
#include "../previewer/toast.h"

#ifdef __APPLE__
    #include "macos/VideoFrameMacos.h"
    #include "../utils/macos/mac_support.h"
#endif
namespace
{
    enum { kPreviewBorderOffset_leftRight = 20 };
    enum { kPreviewBorderOffset_topBottom = 20 };
    enum { kMinimumW = 460, kMinimumH = 400 };
    enum { kAnimationDefDuration = 500 };

    constexpr float kDefaultRelativeSizeW = 0.6f;
    constexpr float kDefaultRelativeSizeH = 0.7f;
    constexpr char videoWndWName[] = "video_window_w";
    constexpr char videoWndHName[] = "video_window_h";

    auto getToastVerOffset() noexcept
    {
        return Utils::scale_value(10);
    }

    bool windowIsOverlapped(platform_specific::GraphicsPanel* _window, const std::vector<QWidget*>& _exclude)
    {
        if (!_window)
            return false;

#ifdef _WIN32
        HWND target = (HWND)_window->frameId();
        if (!::IsWindowVisible(target))
            return false;

        RECT r;
        ::GetWindowRect(target, &r);

        const int overlapDepthPts = 90;
        const int ptsNumY = (r.bottom - r.top)/overlapDepthPts;
        const int ptsNumX = (r.right - r.left)/overlapDepthPts;

        std::vector<POINT> pts;
        pts.reserve(ptsNumY * ptsNumX);

        for (int j = 0; j < ptsNumY; ++j)
        {
            int ptY = r.top + overlapDepthPts*j;
            for (int i = 0; i < ptsNumX; ++i)
            {
                int ptX = r.left + overlapDepthPts*i;
                const POINT pt = { ptX, ptY };
                pts.push_back(pt);
            }
        }

        int ptsCounter = 0;
        for (auto it = pts.begin(); it != pts.end(); ++it)
        {
            const HWND top = ::WindowFromPoint( *it );
            bool isMyWnd = (top == target);
            for (auto itWindow = _exclude.begin(); itWindow != _exclude.end(); itWindow++)
            {
                if (*itWindow)
                    isMyWnd |= (top == (HWND)(*itWindow)->winId());
            }
            if (!isMyWnd)
                ++ptsCounter;
        }

        return (ptsCounter * 10) >= int(pts.size() * 4); // 40 % overlapping
#elif defined (__APPLE__)
        return platform_macos::windowIsOverlapped(_window, _exclude);
#elif defined (__linux__)
        return false;
#endif
    }
}

Ui::VideoWindowHeader::VideoWindowHeader(QWidget* parent)
    : Ui::MoveablePanel(parent, Qt::Widget)
{
    setAttribute(Qt::WA_NoSystemBackground, false);
    setAttribute(Qt::WA_TranslucentBackground, false);

    setAutoFillBackground(true);
    Utils::updateBgColor(this, Styling::getParameters().getColor(Styling::StyleVariable::BASE_BRIGHT_INVERSE));
    Utils::ApplyStyle(this, Styling::getParameters().getTitleQss());

    setObjectName(qsl("titleWidget"));
    setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);
    Testing::setAccessibleName(this, qsl("AS VoipWindow titleWidget"));
    auto mainLayout  = Utils::emptyHLayout(this);
    auto leftLayout  = Utils::emptyHLayout();
    auto rightLayout = Utils::emptyHLayout();

    auto logo = new QLabel(this);
    logo->setObjectName(qsl("windowIcon"));
    logo->setPixmap(Utils::renderSvgScaled(qsl(":/logo/logo_16"), QSize(16, 16)));
    logo->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    logo->setFocusPolicy(Qt::NoFocus);
    Testing::setAccessibleName(logo, qsl("AS VoipWindow logo"));
    leftLayout->addWidget(logo);
    leftLayout->setSizeConstraint(QLayout::SetMaximumSize);
    leftLayout->addStretch(1);

    mainLayout->addLayout(leftLayout, 1);

    title_ = new TextEmojiWidget(this, Fonts::appFontScaled(14, Fonts::FontWeight::SemiBold), Styling::getParameters().getColor(Styling::StyleVariable::TEXT_SOLID));
    title_->setFocusPolicy(Qt::NoFocus);
    title_->disableFixedPreferred();
    title_->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Expanding);
    title_->setEllipsis(true);
    title_->setText(QString());

    mainLayout->addSpacing(Utils::scale_value(8));
    Testing::setAccessibleName(title_, qsl("AS VoipWindow title"));
    mainLayout->addWidget(title_);

    rightLayout->addStretch(1);
    auto hideButton = new CustomButton(this, qsl(":/titlebar/minimize_button"), QSize(44, 28));
    Styling::Buttons::setButtonDefaultColors(hideButton);
    hideButton->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    Testing::setAccessibleName(hideButton, qsl("AS VoipWindow hideButton"));
    rightLayout->addWidget(hideButton);
    auto maximizeButton = new CustomButton(this, qsl(":/titlebar/expand_button"), QSize(44, 28));
    Styling::Buttons::setButtonDefaultColors(maximizeButton);
    maximizeButton->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    Testing::setAccessibleName(maximizeButton, qsl("AS VoipWindow maximizeButton"));
    rightLayout->addWidget(maximizeButton);
    auto closeButton = new CustomButton(this, qsl(":/titlebar/close_button"), QSize(44, 28));
    closeButton->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    Styling::Buttons::setButtonDefaultColors(closeButton);
    Testing::setAccessibleName(closeButton, qsl("AS VoipWindow closeButton"));
    rightLayout->addWidget(closeButton);

    mainLayout->addLayout(rightLayout, 1);

    auto btnBgHideHover = [=](bool _hovered) { hideButton->setBackgroundColor(Styling::getParameters().getColor(_hovered ? Styling::StyleVariable::BASE_BRIGHT_INVERSE_HOVER : Styling::StyleVariable::BASE_BRIGHT_INVERSE)); };
    auto btnBgHidePress = [=](bool _pressed) { hideButton->setBackgroundColor(Styling::getParameters().getColor(_pressed ? Styling::StyleVariable::BASE_BRIGHT : Styling::StyleVariable::BASE_BRIGHT_INVERSE)); };

    auto btnBgMaximizeHover = [=](bool _hovered) { maximizeButton->setBackgroundColor(Styling::getParameters().getColor(_hovered ? Styling::StyleVariable::BASE_BRIGHT_INVERSE_HOVER : Styling::StyleVariable::BASE_BRIGHT_INVERSE)); };
    auto btnBgMaximizePress = [=](bool _pressed) { maximizeButton->setBackgroundColor(Styling::getParameters().getColor(_pressed ? Styling::StyleVariable::BASE_BRIGHT : Styling::StyleVariable::BASE_BRIGHT_INVERSE)); };

    auto btnBgClose = [=](bool _state) { closeButton->setBackgroundColor(Styling::getParameters().getColor(_state ? Styling::StyleVariable::SECONDARY_ATTENTION : Styling::StyleVariable::BASE_BRIGHT_INVERSE)); };

    connect(hideButton, &QPushButton::clicked, this, &Ui::VideoWindowHeader::onMinimized);
    connect(maximizeButton, &QPushButton::clicked, this, &Ui::VideoWindowHeader::onFullscreen);
    connect(closeButton, &QPushButton::clicked, this, &Ui::VideoWindowHeader::onClose);

    connect(hideButton, &CustomButton::changedHover, this, btnBgHideHover);
    connect(hideButton, &CustomButton::changedPress, this, btnBgHidePress);
    connect(maximizeButton, &CustomButton::changedHover, this, btnBgMaximizeHover);
    connect(maximizeButton, &CustomButton::changedPress, this, btnBgMaximizePress);
    connect(closeButton, &CustomButton::changedHover, this, btnBgClose);
    connect(closeButton, &CustomButton::changedPress, this, btnBgClose);

    hideButton->setCursor(Qt::PointingHandCursor);
    maximizeButton->setCursor(Qt::PointingHandCursor);
    closeButton->setCursor(Qt::PointingHandCursor);

    show();
}

bool Ui::VideoWindowHeader::uiWidgetIsActive() const
{
    return parentWidget()->isActiveWindow();
}

void Ui::VideoWindowHeader::updatePosition(const QWidget& parent)
{
}

void Ui::VideoWindowHeader::setTitle(const QString& text)
{
    title_->setText(text);
}

void Ui::VideoWindowHeader::mouseDoubleClickEvent(QMouseEvent * e)
{
    if (e->button() == Qt::LeftButton)
        Q_EMIT onFullscreen();
}

// Did user click to one of these button.
struct Ui::VideoWindow::ButtonStatisticData
{
    bool buttonMic = false;
    bool buttonSpeaker = false;
    bool buttonChat = false;
    bool buttonCamera = false;
    bool buttonSecurity = false;
    bool masks = false;
    bool buttonAddToCall = false;

    int currentTime = 0;

    enum {kMinTimeForStatistic = 5}; // for calls from 5 seconds.
};

Ui::VideoWindow::VideoWindow()
    : QWidget()
    , videoPanel_(new(std::nothrow) VideoPanel(this, this))
    , maskPanel_(nullptr)
    , transparentPanelOutgoingWidget_(nullptr)
    , detachedWnd_(new DetachedVideoWindow(this))
    , checkOverlappedTimer_(this)
    , showPanelTimer_(this)
    , outgoingNotAccepted_(false)
    //, secureCallWnd_(nullptr)
    , shadow_(this)
    , lastBottomOffset_(0)
    , lastTopOffset_(0)
    , startTalking(false)
    //, enableSecureCall_(false)
    , maskPanelState(SMP_HIDE)
#ifdef __APPLE__
    , fullscreenNotification_(*this)
    , isFullscreenAnimation_(false)
    , changeSpaceAnimation_(false)
#endif
{
    callDescription.time = 0;
    buttonStatistic_ = std::make_unique<ButtonStatisticData>();
#ifdef __APPLE__
    headerHeight_ = platform_macos::getWidgetHeaderHeight(*this);
#endif
#ifdef _WIN32
    header_ = new VideoWindowHeader(this);
    headerHeight_ = header_->height();
#endif
    // TODO: use panels' height
    maskPanel_ = std::unique_ptr<MaskPanel>(new(std::nothrow) MaskPanel(this, this, Utils::scale_value(30), Utils::scale_value(56)));

    if constexpr (platform::is_windows())
    {
        setWindowFlags(Qt::Window | Qt::CustomizeWindowHint);
    }
    else
    {
        // We have a problem on Mac with WA_ShowWithoutActivating and WindowDoesNotAcceptFocus.
        // If video window is showing with these flags,, we can not activate ICQ mainwindow
        setWindowFlags(Qt::Window /*| Qt::WindowDoesNotAcceptFocus*//*| Qt::WindowMinimizeButtonHint | Qt::WindowCloseButtonHint | Qt::WindowMaximizeButtonHint*/);
        //setAttribute(Qt::WA_ShowWithoutActivating);
        setAttribute(Qt::WA_X11DoNotAcceptFocus);
    }

    QVBoxLayout* rootLayout = Utils::emptyVLayout();
    rootLayout->setAlignment(Qt::AlignVCenter);
    setLayout(rootLayout);

#ifdef _WIN32
    Testing::setAccessibleName(header_, qsl("AS VoipWindow header"));
    rootLayout->addWidget(header_);
#endif

    panels_.push_back(videoPanel_.get());
    panels_.push_back(maskPanel_.get());

#ifndef _WIN32
    transparentPanels_.push_back(maskPanel_.get());
#endif

#ifndef STRIP_VOIP
    rootWidget_ = platform_specific::GraphicsPanel::create(this, panels_, true, true);
    rootWidget_->setContentsMargins(0, 0, 0, 0);
    rootWidget_->setAttribute(Qt::WA_UpdatesDisabled);
    rootWidget_->setSizePolicy(QSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding));

    if constexpr (platform::is_linux())
    {
        rootLayout->setContentsMargins(0, 0, 0, 0);
        rootLayout->addWidget(rootWidget_, 1);
        rootLayout->addWidget(videoPanel_.get());
        rootLayout->addWidget(maskPanel_.get());
    }
    else
    {
        rootWidget_->setAttribute(Qt::WA_NoSystemBackground, true);
        rootWidget_->setAttribute(Qt::WA_TranslucentBackground, true);

        Testing::setAccessibleName(rootWidget_, qsl("AS VoipWindow rootWidget"));
        rootLayout->addWidget(rootWidget_);
    }
#endif

    QIcon icon(qsl(":/logo/ico"));
    setWindowIcon(icon);

    eventFilter_ = new ResizeEventFilter(panels_, shadow_.getShadowWidget(), this);
    installEventFilter(eventFilter_);

    if (videoPanel_)
    {
        connect(videoPanel_.get(), &VideoPanel::onMouseEnter, this, &VideoWindow::onPanelMouseEnter);
        connect(videoPanel_.get(), &VideoPanel::onMouseLeave, this, &VideoWindow::onPanelMouseLeave);
        connect(videoPanel_.get(), &VideoPanel::onkeyEscPressed, this, &VideoWindow::onEscPressed);

        connect(videoPanel_.get(), &VideoPanel::onShowMaskPanel, this, &VideoWindow::onShowMaskPanel);
        connect(videoPanel_.get(), &VideoPanel::closeActiveMask, this, &VideoWindow::onCloseActiveMask);

        connect(videoPanel_.get(), &VideoPanel::onCameraClickOn, this, &VideoWindow::onSetPreviewPrimary);
        connect(videoPanel_.get(), &VideoPanel::onShareScreenClickOn, this, &VideoWindow::onScreenSharing);

        connect(videoPanel_.get(), &VideoPanel::needShowScreenPermissionsPopup, this, &VideoWindow::showScreenPermissionsPopup);

        connect(videoPanel_.get(), &VideoPanel::onMicrophoneClick, this, [this]() {buttonStatistic_->buttonMic = true; });
        connect(videoPanel_.get(), &VideoPanel::onGoToChatButton, this, [this]() {buttonStatistic_->buttonChat = true; });
        connect(videoPanel_.get(), &VideoPanel::onCameraClickOn, this, [this]() {buttonStatistic_->buttonCamera = true; });

        connect(videoPanel_.get(), &VideoPanel::updateConferenceMode, this, &VideoWindow::updateConferenceMode);
        connect(videoPanel_.get(), &VideoPanel::addUserToConference, this, &VideoWindow::onAddUserClicked);
        connect(videoPanel_.get(), &VideoPanel::inviteVCSUrl, this, &VideoWindow::onInviteVCSUrl);

        connect(videoPanel_.get(), &VideoPanel::onSpeakerClick, this, [this]() {buttonStatistic_->buttonSpeaker = true; });
        connect(videoPanel_.get(), &VideoPanel::addUserToConference, this, [this]() {buttonStatistic_->buttonAddToCall = true; });
    }

    if (maskPanel_)
    {
        connect(maskPanel_.get(), &MaskPanel::makePreviewPrimary, this, &VideoWindow::onSetPreviewPrimary);
        connect(maskPanel_.get(), &MaskPanel::makeInterlocutorPrimary, this, qOverload<>(&VideoWindow::onSetContactPrimary));
        connect(maskPanel_.get(), &MaskPanel::onShowMaskList, this, &VideoWindow::onShowMaskList);

        connect(maskPanel_.get(), &MaskPanel::getCallStatus, this, &VideoWindow::getCallStatus);
        connect(maskPanel_.get(), &MaskPanel::onHideMaskPanel, this, &VideoWindow::onHideMaskPanel);
    }

    //connect(&Ui::GetDispatcher()->getVoipController(), &voip_proxy::VoipController::onVoipUpdateCipherState, this, &VideoWindow::onVoipUpdateCipherState);
    connect(&Ui::GetDispatcher()->getVoipController(), &voip_proxy::VoipController::onVoipCallTimeChanged, this, &VideoWindow::onVoipCallTimeChanged);
    connect(&Ui::GetDispatcher()->getVoipController(), &voip_proxy::VoipController::onVoipCallNameChanged, this, &VideoWindow::onVoipCallNameChanged);
    connect(&Ui::GetDispatcher()->getVoipController(), &voip_proxy::VoipController::onVoipMouseTapped, this, &VideoWindow::onVoipMouseTapped);
    connect(&Ui::GetDispatcher()->getVoipController(), &voip_proxy::VoipController::onVoipCallDestroyed, this, &VideoWindow::onVoipCallDestroyed);
    connect(&Ui::GetDispatcher()->getVoipController(), &voip_proxy::VoipController::onVoipMediaRemoteVideo, this, &VideoWindow::onVoipMediaRemoteVideo);
    connect(&Ui::GetDispatcher()->getVoipController(), &voip_proxy::VoipController::onVoipWindowRemoveComplete, this, &VideoWindow::onVoipWindowRemoveComplete);
    connect(&Ui::GetDispatcher()->getVoipController(), &voip_proxy::VoipController::onVoipWindowAddComplete, this, &VideoWindow::onVoipWindowAddComplete);
    connect(&Ui::GetDispatcher()->getVoipController(), &voip_proxy::VoipController::onVoipCallOutAccepted, this, &VideoWindow::onVoipCallOutAccepted);
    connect(&Ui::GetDispatcher()->getVoipController(), &voip_proxy::VoipController::onVoipCallCreated, this, &VideoWindow::onVoipCallCreated);
    //QObject::connect(&Ui::GetDispatcher()->getVoipController(), &voip_proxy::VoipController::onVoipChangeWindowLayout, this, &VideoWindow::onVoipChangeWindowLayout, Qt::DirectConnection);
    connect(&Ui::GetDispatcher()->getVoipController(), &voip_proxy::VoipController::onVoipCallConnected, this, &VideoWindow::onVoipCallConnected);
    connect(&Ui::GetDispatcher()->getVoipController(), &voip_proxy::VoipController::onVoipMainVideoLayoutChanged, this, &VideoWindow::onVoipMainVideoLayoutChanged);

#ifdef _WIN32
    connect(header_, &VideoWindowHeader::onMinimized, this, &Ui::VideoWindow::onPanelClickedMinimize);
    connect(header_, &VideoWindowHeader::onFullscreen, this, &Ui::VideoWindow::onPanelClickedMaximize);
    connect(header_, &VideoWindowHeader::onClose, this, &Ui::VideoWindow::onPanelClickedClose);
#endif

    connect(&checkOverlappedTimer_, &QTimer::timeout, this, &VideoWindow::checkOverlap);
    checkOverlappedTimer_.setInterval(500);
    checkOverlappedTimer_.start();

    connect(&showPanelTimer_, &QTimer::timeout, this, &VideoWindow::checkPanelsVis);
#ifdef __APPLE__
    connect(&fullscreenNotification_, &platform_macos::FullScreenNotificaton::fullscreenAnimationStart, this, &VideoWindow::fullscreenAnimationStart);
    connect(&fullscreenNotification_, &platform_macos::FullScreenNotificaton::fullscreenAnimationFinish, this, &VideoWindow::fullscreenAnimationFinish);
    connect(&fullscreenNotification_, &platform_macos::FullScreenNotificaton::activeSpaceDidChange, this, &VideoWindow::activeSpaceDidChange);
    connect(detachedWnd_.get(), &DetachedVideoWindow::windowWillDeminiaturize, this, &VideoWindow::windowWillDeminiaturize);
    connect(detachedWnd_.get(), &DetachedVideoWindow::windowDidDeminiaturize, this, &VideoWindow::windowDidDeminiaturize);
    if (videoPanel_)
    {
        connect(videoPanel_.get(), &VideoPanel::parentWindowWillDeminiaturize, this, &VideoWindow::windowWillDeminiaturize);
        connect(videoPanel_.get(), &VideoPanel::parentWindowDidDeminiaturize, this, &VideoWindow::windowDidDeminiaturize);
    }
#endif

    connect(detachedWnd_.get(), &DetachedVideoWindow::needShowScreenPermissionsPopup, this, &VideoWindow::showScreenPermissionsPopup);

    showPanelTimer_.setInterval(1500);
    if (detachedWnd_)
        detachedWnd_->hideFrame();

    setMinimumSize(Utils::scale_value(kMinimumW), Utils::scale_value(kMinimumH));
    //saveMinSize(minimumSize());

    resizeToDefaultSize();
}

Ui::VideoWindow::~VideoWindow()
{
    checkOverlappedTimer_.stop();
#ifndef STRIP_VOIP
    rootWidget_->clearPanels();
#endif
    removeEventFilter(eventFilter_);
    delete eventFilter_;
}

quintptr Ui::VideoWindow::getContentWinId()
{
    return rootWidget_->frameId();
}

void Ui::VideoWindow::onPanelMouseEnter()
{
    showPanelTimer_.stop();
    fadeInPanels(kAnimationDefDuration);
}

void Ui::VideoWindow::onVoipMediaRemoteVideo(const voip_manager::VideoEnable& videoEnable)
{
    hasRemoteVideo_[QString::fromStdString(videoEnable.contact.contact)] = videoEnable.enable;
    checkPanelsVisibility();
    if (currentContacts_.size() <= 1)
    {
        if (hasRemoteVideoInCall())
        {
            tryRunPanelsHideTimer();
        }
        else
        {
            showPanelTimer_.stop();
            fadeInPanels(kAnimationDefDuration);
        }
    }
}

void Ui::VideoWindow::onPanelMouseLeave()
{
    tryRunPanelsHideTimer();
}

#ifndef __APPLE__
// Qt has a bug with double click and fullscreen switching under macos
void Ui::VideoWindow::mouseDoubleClickEvent(QMouseEvent* _e)
{
    QWidget::mouseDoubleClickEvent(_e);
    _switchFullscreen();
}
#endif

#ifndef _WIN32 //We have our own mouse event handler on win32
void Ui::VideoWindow::enterEvent(QEvent* _e)
{
    QWidget::enterEvent(_e);
    showPanelTimer_.stop();
    fadeInPanels(kAnimationDefDuration);
    tryRunPanelsHideTimer();
}

void Ui::VideoWindow::leaveEvent(QEvent* _e)
{
    QWidget::leaveEvent(_e);
    tryRunPanelsHideTimer();
}

void Ui::VideoWindow::moveEvent(QMoveEvent* event)
{
    QWidget::moveEvent(event);
    //hideSecurityDialog();
}

bool Ui::VideoWindow::event(QEvent* _event)
{
    if constexpr (platform::is_apple())
    {
        if (_event->type() == QEvent::WindowActivate || _event->type() == QEvent::InputMethodQuery)
        {
            for (auto& panel : panels_)
                if (panel && panel->isVisible())
                     panel->activateWindow();
        }
    }

    return QWidget::event(_event);
}


void Ui::VideoWindow::mouseMoveEvent(QMouseEvent* _e)
{
    if (!resendMouseEventToPanel(_e))
    {
        showPanelTimer_.stop();
        fadeInPanels(kAnimationDefDuration);
        tryRunPanelsHideTimer();
    }
}

void Ui::VideoWindow::mouseReleaseEvent(QMouseEvent* event)
{
    if (!resendMouseEventToPanel(event))
    {
#ifdef __APPLE__
        if (event->button() == Qt::LeftButton)
        {
            // Own double click processing
            auto prevTime = doubleClickTime_.elapsed();
            if (prevTime == 0 || prevTime > platform_macos::doubleClickInterval())
            {
                doubleClickMousePos_ = event->pos();
                doubleClickTime_.start();
            }

            bool doubleClick = prevTime != 0 && prevTime < platform_macos::doubleClickInterval();

            if (doubleClick && doubleClickMousePos_ == event->pos())
            {
                QWidget::mouseDoubleClickEvent(event);
                _switchFullscreen();
            }
            else
#endif
            if (maskPanel_->isVisible())
            {
                onSetContactPrimary();
                onHideMaskPanel();
            }
#ifdef __APPLE__
        }
#endif
    }
}

void Ui::VideoWindow::mousePressEvent(QMouseEvent * event)
{
    if (!resendMouseEventToPanel(event))
        event->ignore();
}

void Ui::VideoWindow::wheelEvent(QWheelEvent * event)
{
    if (!resendMouseEventToPanel(event))
        event->ignore();
}

template <typename E> bool Ui::VideoWindow::resendMouseEventToPanel(E* event_)
{
    bool res = false;
    for (auto panel : transparentPanels_)
    {
        if (panel->isVisible() && (panel->rect().contains(event_->pos()) || panel->isGrabMouse()))
        {
            QApplication::sendEvent(panel, event_);
            res = true;
            break;
        }
    }
    return res;
}

#endif

bool Ui::VideoWindow::isOverlapped() const
{
    std::vector<QWidget*> excludeWnd;

    for (auto panel : panels_)
        excludeWnd.push_back(panel);

    //if (secureCallWnd_ && secureCallWnd_->isVisible())
    //    excludeWnd.push_back(secureCallWnd_);

    return windowIsOverlapped(rootWidget_, excludeWnd);
}

void Ui::VideoWindow::checkOverlap()
{
    if (!isVisible())
        return;
    assert(rootWidget_ != nullptr);
    if (!rootWidget_)
        return;

    // We do not change normal video window to small video window, if
    // there is active modal window, because it can lost focus and
    // close automatically.
    if (detachedWnd_ && QApplication::activeModalWidget() == nullptr)
    {
        bool platformState = false;
#ifdef __APPLE__
        platformState  = isFullscreenAnimation_ || changeSpaceAnimation_;
#endif
        // Additional to overlapping we show small video window, if VideoWindow was minimized.
        if (!detachedWnd_->closedManualy() && (isOverlapped() || isMinimized()) && !platformState)
        {
            if constexpr (platform::is_windows())
                QApplication::alert(this);

            if constexpr (platform::is_apple())
                if (isFullScreen())
                    return;

            if (!miniWindowShown)
            {
                miniWindowShown = true;
                detachedWnd_->showFrame();
            }
        } else
        {
            if (miniWindowShown)
            {
                miniWindowShown = false;
                detachedWnd_->hideFrame();
            }
        }
    }
}

void Ui::VideoWindow::checkPanelsVis()
{
    // Do not hide panel if set flag or cursor is under it.
    if (videoPanel_ && (videoPanel_->isPreventFadeOut() || videoPanel_->frameGeometry().contains(QCursor().pos())))
        return;

    showPanelTimer_.stop();
    fadeOutPanels(kAnimationDefDuration);
}

void Ui::VideoWindow::onVoipCallTimeChanged(unsigned _secElapsed, bool _hasCall)
{
    callDescription.time = _hasCall ? _secElapsed : 0;
    if (_hasCall)
        buttonStatistic_->currentTime = _secElapsed;
    updateWindowTitle();
}

void Ui::VideoWindow::onPanelClickedMinimize()
{
    showMinimized();
}

void Ui::VideoWindow::onPanelClickedMaximize()
{
    _switchFullscreen();
}

void Ui::VideoWindow::onPanelClickedClose()
{
    Ui::GetDispatcher()->getVoipController().setHangup();
}

void Ui::VideoWindow::hideFrame()
{
    rootWidget_->exitTalk();
    Ui::GetDispatcher()->getVoipController().setWindowRemove((quintptr)rootWidget_->frameId());
}

void Ui::VideoWindow::showFrame()
{
    Ui::GetDispatcher()->getVoipController().setWindowAdd((quintptr)rootWidget_->frameId(), "", true, false, 0);

    offsetWindow(Utils::scale_value(kPreviewBorderOffset_topBottom), Utils::scale_value(kPreviewBorderOffset_topBottom));

    if constexpr (platform::is_apple())
        activateWindow();
}

void Ui::VideoWindow::onVoipWindowRemoveComplete(quintptr _winId)
{
    if (_winId == rootWidget_->frameId())
    {
        callMethodProxy(qsl("hide"));
        //hide();
    }
}

void Ui::VideoWindow::updatePanels() const
{
    if (!rootWidget_)
    {
        assert(false);
        return;
    }

    if constexpr (platform::is_apple())
    {
        rootWidget_->clearPanels();
        std::vector<QPointer<Ui::BaseVideoPanel>> panels;

        for (auto panel : panels_)
        {
            if (panel && panel->isVisible())
                panels.push_back(panel);
        }

        rootWidget_->addPanels(panels);

        for (auto panel : panels_)
        {
            if (panel && panel->isVisible())
                panel->updatePosition(*this);
        }
    }
}

void Ui::VideoWindow::onVoipWindowAddComplete(quintptr _winId)
{
    if (_winId == rootWidget_->frameId())
    {
        updatePanels();
        show();
    }
}

void Ui::VideoWindow::hideEvent(QHideEvent* _ev)
{
    QWidget::hideEvent(_ev);
    checkPanelsVisibility(true);
    if (detachedWnd_)
        detachedWnd_->hideFrame();

    shadow_.hideShadow();
}

bool Ui::VideoWindow::nativeEvent(const QByteArray& _data, void* _message, long* _result)
{
#ifdef _WIN32
    auto msg = static_cast<MSG*>(_message);

    if (msg->message == WM_SETCURSOR)
    {
        showPanelTimer_.stop();
        fadeInPanels(kAnimationDefDuration);
        tryRunPanelsHideTimer();
    }
    else if (msg->message == WM_NCACTIVATE)
    {
        *_result = 1;
        return true;
    }
    else if (msg->message == WM_NCCALCSIZE)
    {
        auto params = reinterpret_cast<LPNCCALCSIZE_PARAMS>(msg->lParam);
        auto& rect = (msg->wParam == TRUE) ? params->rgrc[0] : *reinterpret_cast<LPRECT>(msg->lParam);
        // remove 6px top border
        rect.top -= 6;
    }
    else if (msg->message == WM_NCPAINT)
    {
        // sometimes the top border of a non-client area appears above the header widget
        // so just update it
        header_->update();
    }
#endif

    return false;
}

void Ui::VideoWindow::showEvent(QShowEvent* _ev)
{
    QWidget::showEvent(_ev);
    checkPanelsVisibility();

    showPanelTimer_.stop();
    fadeInPanels(kAnimationDefDuration);

    tryRunPanelsHideTimer();

    if (detachedWnd_)
        detachedWnd_->hideFrame();

    shadow_.showShadow();

    updateUserName();
    Utils::InterConnector::instance().getMainWindow()->updateMainMenu();
}

void Ui::VideoWindow::onVoipMouseTapped(quintptr _hwnd, const std::string& _tapType, const std::string& contact)
{
    const bool dblTap = _tapType == "double";
    const bool over   = _tapType == "over";
    const bool single = _tapType == "single";

    if (detachedWnd_ && detachedWnd_->getVideoFrameId() == _hwnd)
    {
        if constexpr (platform::is_windows() || platform::is_linux())
        {
            if (dblTap)
                raise();
        }
    }
    else if ((quintptr)rootWidget_->frameId() == _hwnd)
    {
        if (dblTap)
        {
             _switchFullscreen();
        }
        else if (over)
        {
            showPanelTimer_.stop();
            fadeInPanels(kAnimationDefDuration);
            tryRunPanelsHideTimer();
        }
        else if (single)
        {
            if (maskPanel_->isVisible())
                onHideMaskPanel();
            else if (currentLayout_.layout == voip_manager::OneIsBig)
                onSetContactPrimary(contact);
        }
    }
}

void Ui::VideoWindow::onVoipCallOutAccepted(const voip_manager::ContactEx& /*contact_ex*/)
{
}

void Ui::VideoWindow::onVoipCallCreated(const voip_manager::ContactEx& _contactEx)
{
    statistic::getGuiMetrics().eventVoipStartCalling();
    if (!_contactEx.incoming)
    {
        outgoingNotAccepted_ = true;
        startTalking = false;
        rootWidget_->createdTalk(Ui::GetDispatcher()->getVoipController().isCallVCS());
    }
    //if (_contactEx.connection_count == 1)
    //    createdNewCall();
}

void Ui::VideoWindow::checkPanelsVisibility(bool _forceHide /*= false*/)
{
    if (isHidden() || isMinimized() || _forceHide)
    {
        hidePanels();
        return;
    }

#ifdef _WIN32
    if (header_)
        header_->setVisible(!isInFullscreen());
#endif

    if (videoPanel_)
        videoPanel_->setVisible(maskPanelState != SMP_SHOW);
    if (maskPanel_)
        maskPanel_->setVisible(maskPanelState != SMP_HIDE);

    if (rootWidget_)
        updatePanels();
}

void Ui::VideoWindow::_switchFullscreen()
{
    if (!isInFullscreen())
        showFullScreen();
    else
        showNormal();

    checkPanelsVisibility();

    fadeInPanels(kAnimationDefDuration);
    tryRunPanelsHideTimer();
}

void Ui::VideoWindow::onVoipCallNameChanged(const voip_manager::ContactsList& _contacts)
{
    if (_contacts.contacts.empty())
        return;

    // Hide secure for conference.
    //if (secureCallWnd_ && secureCallWnd_->isVisible() && _contacts.contacts.size() > 1)
    //    secureCallWnd_->close();

    auto res = std::find(_contacts.windows.begin(), _contacts.windows.end(), (void*)rootWidget_->frameId());
    if (res != _contacts.windows.end())
    {
        currentContacts_ = _contacts.contacts;
    }
    callDescription.name = _contacts.conference_name;
    updateUserName();
    removeUnneededRemoteVideo();
    //checkCurrentAspectState();
}

void Ui::VideoWindow::updateWindowTitle()
{
    setWindowTitle(QString::fromStdString(callDescription.name));
}

void Ui::VideoWindow::paintEvent(QPaintEvent *_e)
{
    return QWidget::paintEvent(_e);
}

void Ui::VideoWindow::changeEvent(QEvent* _e)
{
    QWidget::changeEvent(_e);
    if (_e->type() == QEvent::WindowStateChange)
    {
        if constexpr (platform::is_apple())
        {
            // On Mac we can go to normal size using OSX.
            // We need to update buttons and panel states.
            if (_e->spontaneous())
            {
                checkPanelsVisibility();
                if (!!rootWidget_)
                    rootWidget_->fullscreenModeChanged(isInFullscreen());
            }
        }
    }
}

void Ui::VideoWindow::closeEvent(QCloseEvent* _e)
{
    _e->ignore();
    Ui::GetDispatcher()->getVoipController().setHangup();
}

void Ui::VideoWindow::resizeEvent(QResizeEvent* _e)
{
    QWidget::resizeEvent(_e);

    if constexpr (!platform::is_linux())
    {
        bool bFadeIn = false;
        std::for_each(panels_.begin(), panels_.end(), [&bFadeIn](BaseVideoPanel* panel)
        {
            if (panel->isVisible())
            {
                bFadeIn = bFadeIn || panel->isFadedIn();
                panel->forceFinishFade();
            }
        });

        if (bFadeIn)
            tryRunPanelsHideTimer();
    }

    //hideSecurityDialog();
    Ui::GetDispatcher()->getVoipController().updateLargeState((quintptr)rootWidget_->frameId(), width() > voip_manager::kWindowLargeWidth);
}

void Ui::VideoWindow::onVoipCallDestroyed(const voip_manager::ContactEx& _contactEx)
{
    auto res = std::find(_contactEx.windows.begin(), _contactEx.windows.end(), (void*)getContentWinId());
    if (res == _contactEx.windows.end())
        return;

    //unuseAspect();
    hasRemoteVideo_.clear();
    outgoingNotAccepted_ = false;
    startTalking = false;

#ifdef __APPLE__
    // On mac if video window is fullscreen and on active, we make it active.
    // It needs to correct close video window.
    if (isFullScreen() && !isActiveWindow() && !videoPanel_->isActiveWindow())
    {
        // Make video window active and wait finish of switch animation.
        callMethodProxy(qsl("activateWindow"));
        callMethodProxy(qsl("raise"));
        changeSpaceAnimation_ = true;
    }
    else if (isMinimized())
    {
        // Qt has but with miniature mode:
        // If window if miniature it ignore hide call.
        windowWillDeminiaturize();
        showNormal();
        windowDidDeminiaturize();
    }
#endif

    callMethodProxy(qsl("onEscPressed"));

    //onEscPressed();

    if (_contactEx.current_call)
    {
        if (maskPanel_)
            maskPanel_->callDestroyed();

        if (videoPanel_)
            videoPanel_->callDestroyed();
    }

    currentContacts_.clear();
    sendStatistic();
}

void Ui::VideoWindow::escPressed()
{
    onEscPressed();
}

void Ui::VideoWindow::onEscPressed()
{
    if (isInFullscreen())
        _switchFullscreen();
}

bool Ui::VideoWindow::getWindowSizeFromSettings(int& _nWidth, int& _nHeight)
{
    bool res = false;
    if (qt_gui_settings* settings = Ui::get_gui_settings())
    {
        const int tmpW = settings->get_value<int>(videoWndWName, -1);
        const int tmpH = settings->get_value<int>(videoWndHName, -1);

        if (tmpW > 0 && tmpH > 0)
        {
            _nWidth  = tmpW;
            _nHeight = tmpH;
            res = true;
        }
    }
    return res;
}

void Ui::VideoWindow::fullscreenAnimationStart()
{
#ifdef __APPLE__
    isFullscreenAnimation_ = true;
    rootWidget_->fullscreenAnimationStart();
#endif
}

void Ui::VideoWindow::fullscreenAnimationFinish()
{
#ifdef __APPLE__
    if (isFullscreenAnimation_)
    {
        isFullscreenAnimation_ = false;
        rootWidget_->fullscreenAnimationFinish();
        executeCommandList();
    }
#endif
}

void Ui::VideoWindow::activeSpaceDidChange()
{
#ifdef __APPLE__
    if (changeSpaceAnimation_)
    {
        changeSpaceAnimation_ = false;
        executeCommandList();
    }
#endif
}

void Ui::VideoWindow::windowWillDeminiaturize()
{
    if constexpr (platform::is_apple())
        rootWidget_->windowWillDeminiaturize();
}

void Ui::VideoWindow::windowDidDeminiaturize()
{
    if constexpr (platform::is_apple())
        rootWidget_->windowDidDeminiaturize();
}

void Ui::VideoWindow::executeCommandList()
{
#ifdef __APPLE__
    QMutableListIterator<QString> iterator(commandList_);
    while (iterator.hasNext())
    {
        if (!isFullscreenAnimation_ && !changeSpaceAnimation_)
        {
            callMethodProxy(iterator.next());
            iterator.remove();
        }
        else
        {
            // Animation is in progress.
            break;
        }
    }
#endif
}

void Ui::VideoWindow::callMethodProxy(const QString& _method)
{
#ifdef __APPLE__
    if (isFullscreenAnimation_ || changeSpaceAnimation_)
    {
        commandList_.push_back(_method);
    }
    else
#endif
    {
        QMetaObject::invokeMethod(this, _method.toLatin1().data(), /*bQueued ? Qt::QueuedConnection :*/ Qt::DirectConnection);
    }
}

void Ui::VideoWindow::offsetWindow(int _bottom, int _top)
{
    lastBottomOffset_ = _bottom;
    lastTopOffset_ = _top;

    Ui::GetDispatcher()->getVoipController().setWindowOffsets(
        (quintptr)rootWidget_->frameId(),
        Utils::scale_value(kPreviewBorderOffset_leftRight) * Utils::scale_bitmap_ratio(),
        _top * Utils::scale_bitmap_ratio(),
        Utils::scale_value(kPreviewBorderOffset_leftRight) * Utils::scale_bitmap_ratio(),
        _bottom * Utils::scale_bitmap_ratio()
    );
}

void Ui::VideoWindow::updateUserName()
{
    if (currentContacts_.empty())
        return;

    if (!Ui::GetDispatcher()->getVoipController().isCallVCS())
    {
        std::vector<std::string> friendlyNames;
        friendlyNames.reserve(currentContacts_.size());
        for (const auto& x : currentContacts_)
            friendlyNames.push_back(Logic::GetFriendlyContainer()->getFriendly(QString::fromStdString(x.contact)).toStdString());
        callDescription.name = voip_proxy::VoipController::formatCallName(friendlyNames, QT_TRANSLATE_NOOP("voip_pages", "and").toUtf8().constData());
        assert(!callDescription.name.empty());
    }

    if (videoPanel_)
        videoPanel_->setContacts(currentContacts_, startTalking);

    updateWindowTitle();
}

void Ui::VideoWindow::fadeInPanels(int kAnimationDefDuration)
{
    Ui::GetDispatcher()->getVoipController().passWindowHover(rootWidget_->frameId(), true);

    if (!videoPanel_->isFadedIn() && maskPanelState == SMP_HIDE)
    {
        std::for_each(panels_.begin(), panels_.end(), [kAnimationDefDuration](BaseVideoPanel* panel)
        {
            if (panel->isVisible())
                panel->fadeIn(kAnimationDefDuration);
        });
    }
}

void Ui::VideoWindow::fadeOutPanels(int kAnimationDefDuration)
{
    std::for_each(panels_.begin(), panels_.end(), [kAnimationDefDuration](BaseVideoPanel* panel)
    {
        panel->fadeOut(kAnimationDefDuration);
    });

    Ui::GetDispatcher()->getVoipController().passWindowHover(rootWidget_->frameId(), false);
}

void Ui::VideoWindow::hidePanels()
{
    // in linux don't hide the panels
    if constexpr (platform::is_linux())
        return;

    std::for_each(panels_.begin(), panels_.end(), [] (BaseVideoPanel* panel)
    {
        panel->hide();
    });
}

void Ui::VideoWindow::onSetPreviewPrimary()
{
    // Don't switch view for video conference.
    onSetContactPrimary(PREVIEW_RENDER_NAME);
}

void Ui::VideoWindow::onSetContactPrimary()
{
    if (!currentContacts_.empty())
        onSetContactPrimary(currentContacts_[0].contact);
}

void Ui::VideoWindow::onSetContactPrimary(const std::string& contact)
{
    if (currentLayout_.layout == voip_manager::OneIsBig && !Ui::GetDispatcher()->getVoipController().isCallVCS())
        Ui::GetDispatcher()->getVoipController().setWindowSetPrimary(getContentWinId(), contact.c_str());
}

void Ui::VideoWindow::onShowMaskList()
{
    showPanelTimer_.stop();
    fadeInPanels(kAnimationDefDuration);
}

void Ui::VideoWindow::onHideMaskList()
{
    tryRunPanelsHideTimer();
}

bool Ui::VideoWindow::isActiveWindow() const
{
    bool bPanelIsActive = false;
    for (const auto& panel : panels_)
        bPanelIsActive = bPanelIsActive || (panel && panel->isActiveWindow());

    return !isMinimized() && (bPanelIsActive || QWidget::isActiveWindow());
}

void Ui::VideoWindow::getCallStatus(bool& isAccepted)
{
    isAccepted = !outgoingNotAccepted_;
}

void Ui::VideoWindow::tryRunPanelsHideTimer()
{
    // We hide panel if all are true:
    // 1. Security window is not opened.
    // 2. Mask panel is closed.
    // Removed:
    // 3. Has remove video or now is conference.
    // 4. Have at least one established connection.

    const bool haveSecurityWnd = false;//secureCallWnd_ && secureCallWnd_->isVisible();
    if (/*(hasRemoteVideoInCall() || currentContacts_.size() > 1) &&*/ !haveSecurityWnd && (maskPanel_ == nullptr || !maskPanel_->isOpened())
        /*&& Ui::GetDispatcher()->getVoipController().hasEstablishCall()*/)
    {
        showPanelTimer_.start();
    }
}

/*void Ui::VideoWindow::hideSecurityDialog()
{
    if (secureCallWnd_ && secureCallWnd_->isVisible())
    {
        secureCallWnd_->hide();
        onSecureCallWndClosed();
    }
}*/

void Ui::VideoWindow::onMaskListAnimationFinished(bool out)
{
    if (!out)
    {
        if (outgoingNotAccepted_)
            maskPanel_->chooseFirstMask();
    }
}

void Ui::VideoWindow::updateConferenceMode(voip_manager::VideoLayout layout)
{
    Ui::GetDispatcher()->getVoipController().setWindowVideoLayout(rootWidget_->frameId(), layout);

    if constexpr (platform::is_apple())
    {
        if (videoPanel_ && videoPanel_->isVisible())
            videoPanel_->activateWindow();
    }
}

bool Ui::VideoWindow::hasRemoteVideoInCall()
{
    return hasRemoteVideo_.size() == 1 && hasRemoteVideo_.begin().value();
}

bool Ui::VideoWindow::hasRemoteVideoForConference()
{
    bool res = false;
    std::for_each(currentContacts_.begin(), currentContacts_.end(), [&res, this](const voip_manager::Contact& contact)
    {
        const auto contactString = QString::fromStdString(contact.contact);
        res = res || (hasRemoteVideo_.count(contactString) > 0 && hasRemoteVideo_[contactString]);
    });
    return res;
}

void Ui::VideoWindow::removeUnneededRemoteVideo()
{
    QHash <QString, bool> tempHasRemoteVideo;

    std::for_each(currentContacts_.begin(), currentContacts_.end(), [&tempHasRemoteVideo, this](const voip_manager::Contact& contact)
    {
        const auto contactString = QString::fromStdString(contact.contact);
        tempHasRemoteVideo[contactString] = (hasRemoteVideo_.count(contactString) > 0 && hasRemoteVideo_[contactString]);
    });
    hasRemoteVideo_.swap(tempHasRemoteVideo);
}

void Ui::VideoWindow::onVoipCallConnected(const voip_manager::ContactEx& _contactEx)
{
    if (!startTalking)
    {
        startTalking = true;
        updateUserName();
        rootWidget_->startedTalk();
    }
}

void Ui::VideoWindow::onAddUserClicked()
{
    FullVideoWindowPanel dialogParent(this);

    // Correct attach to video window.
    panels_.push_back(&dialogParent);
    eventFilter_->addPanel(&dialogParent);
    updatePanels();
    
    videoPanel_->setPreventFadeIn(true);
    videoPanel_->fadeOut(kAnimationDefDuration);

    dialogParent.updatePosition(*this);
    dialogParent.show();

    showAddUserToVideoConverenceDialogVideoWindow(this, &dialogParent);

    videoPanel_->setPreventFadeIn(false);

    // Remove panel from video window.
    eventFilter_->removePanel(&dialogParent);
    panels_.pop_back();
    updatePanels();
}

void  Ui::VideoWindow::onVoipMainVideoLayoutChanged(const voip_manager::MainVideoLayout& mainLayout)
{
    if (rootWidget_ && (WId)mainLayout.hwnd == rootWidget_->frameId())
    {
        currentLayout_ = mainLayout;
        outgoingNotAccepted_ = (mainLayout.type == voip_manager::MVL_OUTGOING);
        videoPanel_->changeConferenceMode(mainLayout.layout);
        checkPanelsVisibility();
    }
}

/*void Ui::VideoWindow::updateTopPanelSecureCall()
{
}*/

bool Ui::VideoWindow::isInFullscreen() const
{
    if constexpr (platform::is_linux())
        return isFullScreen() || isMaximized();
    else
        return isFullScreen();
}

void Ui::VideoWindow::setWindowTitle(const QString& text)
{
#ifdef _WIN32
    header_->setTitle(text);
#endif
    QWidget::setWindowTitle(text);
}

void Ui::VideoWindow::onShowMaskPanel()
{
    if (!maskPanel_->isVisible())
    {
        if constexpr (!platform::is_linux())
        {
            videoPanel_->fadeOut(kAnimationDefDuration);
            maskPanel_->fadeIn(kAnimationDefDuration);

            maskPanelState = SMP_ANIMATION;
            checkPanelsVisibility();

            QTimer::singleShot(kAnimationDefDuration, this, &Ui::VideoWindow::setShowMaskPanelState);
        }
        else
        {
            setShowMaskPanelState();
        }

        onSetPreviewPrimary();
    }

    buttonStatistic_->masks = true;
}

void Ui::VideoWindow::onHideMaskPanel()
{
    if (maskPanel_->isVisible())
    {
        if constexpr (!platform::is_linux())
        {
            videoPanel_->fadeIn(kAnimationDefDuration);
            maskPanel_->fadeOut(kAnimationDefDuration);

            maskPanelState = SMP_ANIMATION;
            checkPanelsVisibility();

            QTimer::singleShot(kAnimationDefDuration, this, &Ui::VideoWindow::setHideMaskPanelState);
        }
        else
        {
            setHideMaskPanelState();
        }

        onSetContactPrimary();
    }
}

void Ui::VideoWindow::onCloseActiveMask()
{
    if (maskPanel_)
    {
        onHideMaskPanel();
        maskPanel_->selectMask(nullptr);
    }
}

void Ui::VideoWindow::setShowMaskPanelState()
{
    maskPanelState = SMP_SHOW;
    checkPanelsVisibility();
}

void Ui::VideoWindow::setHideMaskPanelState()
{
    maskPanelState = SMP_HIDE;
    checkPanelsVisibility();
}

void Ui::VideoWindow::keyPressEvent(QKeyEvent* _e)
{
    if constexpr (!platform::is_apple())
    {
        const auto key = _e->key();
        if (_e->modifiers() == Qt::ControlModifier)
        {
            if (key == Qt::Key_Q || key == Qt::Key_W)
            {
                close();
                if (key == Qt::Key_Q)
                    Q_EMIT finished();
            }
        }
    }

    QWidget::keyPressEvent(_e);
}

void Ui::VideoWindow::keyReleaseEvent(QKeyEvent* _e)
{
    QWidget::keyReleaseEvent(_e);
    if (_e->key() == Qt::Key_Escape)
        Q_EMIT onEscPressed();
}

void Ui::VideoWindow::onScreenSharing()
{
    onSetContactPrimary();
    if (maskPanel_)
        maskPanel_->selectMask(nullptr);
}

void Ui::VideoWindow::createdNewCall()
{
    // Moved window to center of screen of main window.
    resizeToDefaultSize();
    // Reset statistic data.
    buttonStatistic_ = std::make_unique<ButtonStatisticData>();
}

void Ui::VideoWindow::resizeToDefaultSize()
{
    Ui::MainWindow* wnd = static_cast<Ui::MainWindow*>(MainPage::instance()->window());
    if (wnd)
    {
        QRect screenRect;
        if constexpr (platform::is_windows())
            screenRect = QApplication::desktop()->availableGeometry(wnd->getScreen());
        else
            screenRect = QApplication::desktop()->availableGeometry(wnd);

        setGeometry(
            QStyle::alignedRect(
                Qt::LeftToRight,
                Qt::AlignCenter,
                QSize(screenRect.size().width() * kDefaultRelativeSizeW, screenRect.size().height() * kDefaultRelativeSizeH),
                screenRect
            )
        );
    }
}

void Ui::VideoWindow::showScreenPermissionsPopup(media::permissions::DeviceType type)
{
    static bool isShow = false;
    if (!isShow)
    {
        QScopedValueRollback scoped(isShow, true);

        FullVideoWindowPanel dialogParent(this);

        // Correct attach to video window.
        panels_.push_back(&dialogParent);
        eventFilter_->addPanel(&dialogParent);
        updatePanels();

        dialogParent.updatePosition(*this);
        dialogParent.show();

        bool res = false;
        if (media::permissions::DeviceType::Screen == type)
        {
            res = Utils::GetConfirmationWithTwoButtons(QT_TRANSLATE_NOOP("input_widget", "Cancel"), QT_TRANSLATE_NOOP("input_widget", "Open settings"),
                QT_TRANSLATE_NOOP("voip_pages", "To share screen you need to allow access to the screen recording in the system settings"),
                QT_TRANSLATE_NOOP("voip_pages", "Screen recording permissions"), &dialogParent);
        } else if (media::permissions::DeviceType::Microphone == type)
        {
            res = Utils::GetConfirmationWithTwoButtons(QT_TRANSLATE_NOOP("input_widget", "Cancel"), QT_TRANSLATE_NOOP("input_widget", "Open settings"),
                QT_TRANSLATE_NOOP("voip_pages", "To use microphone you need to allow access to the microphone in the system settings"),
                QT_TRANSLATE_NOOP("voip_pages", "Microphone permissions"), &dialogParent);
        } else if (media::permissions::DeviceType::Camera == type)
        {
            res = Utils::GetConfirmationWithTwoButtons(QT_TRANSLATE_NOOP("input_widget", "Cancel"), QT_TRANSLATE_NOOP("input_widget", "Open settings"),
                QT_TRANSLATE_NOOP("voip_pages", "To use camera you need to allow access to the camera in the system settings"),
                QT_TRANSLATE_NOOP("voip_pages", "Camera permissions"), &dialogParent);
        }
        if (res)
            media::permissions::openPermissionSettings(type);

        // Remove panel from video window.
        eventFilter_->removePanel(&dialogParent);
        panels_.pop_back();
        updatePanels();
    }
}

void Ui::VideoWindow::onInviteVCSUrl(const QString& _url)
{
    FullVideoWindowPanel dialogParent(this);

    // Correct attach to video window.
    panels_.push_back(&dialogParent);
    eventFilter_->addPanel(&dialogParent);
    updatePanels();

    dialogParent.updatePosition(*this);
    dialogParent.show();

    showInviteVCSDialogVideoWindow(&dialogParent, _url);

    // Remove panel from video window.
    eventFilter_->removePanel(&dialogParent);
    panels_.pop_back();
    updatePanels();
}

void Ui::VideoWindow::sendStatistic()
{
    if (buttonStatistic_ && buttonStatistic_->currentTime >= ButtonStatisticData::kMinTimeForStatistic)
    {
        core::stats::event_props_type props;
        auto yesOrNo = [](bool value) -> std::string { return value ? "Yes" : "No"; };
        props.emplace_back("ButtonMic", yesOrNo(buttonStatistic_->buttonMic));
        props.emplace_back("ButtonSpeaker", yesOrNo(buttonStatistic_->buttonSpeaker));
        props.emplace_back("ButtonChat", yesOrNo(buttonStatistic_->buttonChat));
        props.emplace_back("ButtonCamera", yesOrNo(buttonStatistic_->buttonCamera));
        props.emplace_back("ButtonSecurity", yesOrNo(buttonStatistic_->buttonSecurity));
        props.emplace_back("Masks", yesOrNo(buttonStatistic_->masks));
        props.emplace_back("ButtonAddToCall", yesOrNo(buttonStatistic_->buttonAddToCall));
        Ui::GetDispatcher()->post_stats_to_core(core::stats::stats_event_names::voip_calls_interface_buttons, props);
    }
}

bool Ui::VideoWindow::isMinimized() const
{
    return QWidget::isMinimized() || (detachedWnd_ && detachedWnd_->isMinimized());
}

Ui::VideoPanel* Ui::VideoWindow::getVideoPanel() const
{
    return videoPanel_.get();
}

void Ui::VideoWindow::showToast(const QString& _text, int _maxLineCount)
{
    if (auto videoPanel = videoPanel_.get())
        videoPanel->showToast(_text, _maxLineCount);
}
