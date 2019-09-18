#include "stdafx.h"
#include "VideoWindow.h"
#include "DetachedVideoWnd.h"
#include "../../core/Voip/VoipManagerDefines.h"

#include "../core_dispatcher.h"
#include "../utils/utils.h"
#include "../utils/gui_metrics.h"
#include "../main_window/friendly/FriendlyContainer.h"
#include "../main_window/MainPage.h"
#include "../main_window/MainWindow.h"
#include "../gui_settings.h"
#include "MaskManager.h"

#include "VideoFrame.h"
#include "VoipTools.h"
#include "VideoPanel.h"
#include "VideoPanelHeader.h"
#include "MaskPanel.h"
#include "secureCallWnd.h"

#include "SelectionContactsForConference.h"
#include <QDesktopWidget>

#include "styles/ThemeParameters.h"
#include "../controls/CustomButton.h"

#ifdef __APPLE__
    #include "macos/VideoFrameMacos.h"
    #include "../utils/macos/mac_support.h"
#endif

extern std::string getFotmatedTime(unsigned _ts);
namespace
{
    enum { kPreviewBorderOffset_leftRight = 16 };
    enum { kPreviewBorderOffset_topBottom = 24 };
    enum { kPreviewBorderOffset_with_video_panel = 60 };
    enum { kPreviewBorderOffset_with_mask_panel = 77 };
    enum { kMinimumW = 460, kMinimumH = 400 };
    static const float kDefaultRelativeSizeW = 0.6f;
    static const float kDefaultRelativeSizeH = 0.7f;
    //enum { kDefaultW = kMinimumW, kDefaultH = kMinimumH };
    enum { kAnimationDefDuration = 500 };
    enum { kMinHorizontalWidth = 560};

    const std::string videoWndWName = "video_window_w";
    const std::string videoWndHName = "video_window_h";

    bool windowIsOverlapped(platform_specific::GraphicsPanel* _window, const std::vector<QWidget*>& _exclude)
    {
        if (!_window)
        {
            return false;
        }

#ifdef _WIN32
        HWND target = (HWND)_window->frameId();
        if (!::IsWindowVisible(target))
        {
            return false;
        }

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

            bool isMyWnd = top == target;
            for (auto itWindow = _exclude.begin(); itWindow != _exclude.end(); itWindow++)
            {
                if (*itWindow)
                {
                    isMyWnd |= top == (HWND)(*itWindow)->winId();
                }
            }

            if (!isMyWnd) {
                ++ptsCounter;
            }
        }

        return (ptsCounter * 10) >= int(pts.size() * 4); // 40 % overlapping
#elif defined (__APPLE__)
        return platform_macos::windowIsOverlapped(_window, _exclude);
#elif defined (__linux__)
        return false;
#endif
    }
}


Ui::VideoWindowHeader::VideoWindowHeader(QWidget* parent) : Ui::MoveablePanel(parent, Qt::Widget)
{
    setAttribute(Qt::WA_NoSystemBackground, false);
    setAttribute(Qt::WA_TranslucentBackground, false);

    setAutoFillBackground(true);
    Utils::updateBgColor(this, Styling::getParameters().getColor(Styling::StyleVariable::BASE_BRIGHT_INVERSE));

    Utils::ApplyStyle(this, Styling::getParameters().getTitleQss());

    setObjectName(qsl("titleWidget"));
    setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);
    Testing::setAccessibleName(this, qsl("AS titleWidget_"));
    auto mainLayout  = Utils::emptyHLayout(this);
    auto leftLayout  = Utils::emptyHLayout();
    auto rightLayout = Utils::emptyHLayout();

    auto logo = new QLabel(this);
    logo->setObjectName(qsl("windowIcon"));
    logo->setPixmap(Utils::renderSvgScaled(build::GetProductVariant(qsl(":/logo/logo_icq"), qsl(":/logo/logo_agent"), qsl(":/logo/logo_biz_16"), qsl(":/logo/logo_dit")), QSize(16, 16)));
    logo->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    logo->setFocusPolicy(Qt::NoFocus);
    Testing::setAccessibleName(logo, qsl("AS vw logo"));
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
    Testing::setAccessibleName(title_, qsl("AS vw title_"));
    mainLayout->addWidget(title_);

    rightLayout->addStretch(1);
    auto hideButton = new CustomButton(this, qsl(":/titlebar/minimize_button"), QSize(44, 28));
    Styling::Buttons::setButtonDefaultColors(hideButton);
    hideButton->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    Testing::setAccessibleName(hideButton, qsl("AS vw hideButton"));
    rightLayout->addWidget(hideButton);
    auto maximizeButton = new CustomButton(this, qsl(":/titlebar/expand_button"), QSize(44, 28));
    Styling::Buttons::setButtonDefaultColors(maximizeButton);
    maximizeButton->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    Testing::setAccessibleName(maximizeButton, qsl("AS vw maximizeButton"));
    rightLayout->addWidget(maximizeButton);
    auto closeButton = new CustomButton(this, qsl(":/titlebar/close_button"), QSize(44, 28));
    closeButton->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    Styling::Buttons::setButtonDefaultColors(closeButton);
    Testing::setAccessibleName(closeButton, qsl("AS vw closeButton"));
    rightLayout->addWidget(closeButton);

    mainLayout->addLayout(rightLayout, 1);

    auto btnBgHideHover = [=](bool _hovered) {hideButton->setBackgroundColor(Styling::getParameters().getColor(_hovered ? Styling::StyleVariable::BASE_BRIGHT_INVERSE_HOVER : Styling::StyleVariable::BASE_BRIGHT_INVERSE)); };
    auto btnBgHidePress = [=](bool _pressed) {hideButton->setBackgroundColor(Styling::getParameters().getColor(_pressed ? Styling::StyleVariable::BASE_BRIGHT : Styling::StyleVariable::BASE_BRIGHT_INVERSE)); };

    auto btnBgMaximizeHover = [=](bool _hovered) {maximizeButton->setBackgroundColor(Styling::getParameters().getColor(_hovered ? Styling::StyleVariable::BASE_BRIGHT_INVERSE_HOVER : Styling::StyleVariable::BASE_BRIGHT_INVERSE)); };
    auto btnBgMaximizePress = [=](bool _pressed) {maximizeButton->setBackgroundColor(Styling::getParameters().getColor(_pressed ? Styling::StyleVariable::BASE_BRIGHT : Styling::StyleVariable::BASE_BRIGHT_INVERSE)); };

    auto btnBgClose = [=](bool _state) {closeButton->setBackgroundColor(Styling::getParameters().getColor(_state ? Styling::StyleVariable::SECONDARY_ATTENTION : Styling::StyleVariable::BASE_BRIGHT_INVERSE)); };

    connect(hideButton,     &QPushButton::clicked, this, &Ui::VideoWindowHeader::onMinimized);
    connect(maximizeButton, &QPushButton::clicked, this, &Ui::VideoWindowHeader::onFullscreen);
    connect(closeButton,    &QPushButton::clicked, this, &Ui::VideoWindowHeader::onClose);

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

void Ui::VideoWindowHeader::updatePosition(const QWidget& parent){}

void Ui::VideoWindowHeader::setTitle(const QString& text)
{
    title_->setText(text);
}

void Ui::VideoWindowHeader::mouseDoubleClickEvent(QMouseEvent * e)
{
    if (e->button() == Qt::LeftButton)
    {
        emit onFullscreen();
    }
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
    , videoPanel_(new(std::nothrow) voipTools::BoundBox<VideoPanel>(this, this))
    , maskPanel_(nullptr)
    , transparentPanelOutgoingWidget_(nullptr)
#ifndef __linux__
    , detachedWnd_(new DetachedVideoWindow(this))
#endif
    , checkOverlappedTimer_(this)
    , showPanelTimer_(this)
    , outgoingNotAccepted_(false)
    , secureCallWnd_(nullptr)
    , shadow_(this)

    , lastBottomOffset_(0)
    , lastTopOffset_(0)
    , startTalking(false)
    , isLoadSizeFromSettings_(false)
    , enableSecureCall_(false)
    , maskPanelState(SMP_HIDE)
#ifdef __APPLE__
    , _fullscreenNotification(*this)
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

    headerPanel_    = std::unique_ptr<VideoPanelHeader>(new(std::nothrow) voipTools::BoundBox<VideoPanelHeader>(this, headerHeight_));

    // TODO: use panels' height
    maskPanel_ = std::unique_ptr<MaskPanel>(new(std::nothrow) voipTools::BoundBox <MaskPanel>(this, this, Utils::scale_value(30), Utils::scale_value(56)));

#ifdef _WIN32
    setWindowFlags(Qt::Window | Qt::CustomizeWindowHint);
#else
    // We have a problem on Mac with WA_ShowWithoutActivating and WindowDoesNotAcceptFocus.
    // If video window is showing with these flags,, we can not activate ICQ mainwindow
    setWindowFlags(Qt::Window /*| Qt::WindowDoesNotAcceptFocus*//*| Qt::WindowMinimizeButtonHint | Qt::WindowCloseButtonHint | Qt::WindowMaximizeButtonHint*/);
    //setAttribute(Qt::WA_ShowWithoutActivating);
    setAttribute(Qt::WA_X11DoNotAcceptFocus);
#endif

    QVBoxLayout* rootLayout = Utils::emptyVLayout();
    rootLayout->setAlignment(Qt::AlignVCenter);
    setLayout(rootLayout);

#ifdef _WIN32
    Testing::setAccessibleName(header_, qsl("AS vw header_"));
    rootLayout->addWidget(header_);
#endif

    panels_.push_back(videoPanel_.get());
    if (!!headerPanel_)
    {
        panels_.push_back(headerPanel_.get());
    }


    panels_.push_back(maskPanel_.get());

#ifndef _WIN32
    transparentPanels_.push_back(maskPanel_.get());
#endif


#ifdef __linux__
    rootLayout->setContentsMargins(0, 0, 0, 0);
    //layout()->addWidget(topPanelOutgoing_.get());
    layout()->addWidget(headerPanel_.get());
#endif


#ifndef STRIP_VOIP
    rootWidget_ = platform_specific::GraphicsPanel::create(this, panels_, true, true);
    rootWidget_->setContentsMargins(0, 0, 0, 0);
    rootWidget_->setAttribute(Qt::WA_UpdatesDisabled);
    rootWidget_->setSizePolicy(QSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding));

#ifndef __linux__
    rootWidget_->setAttribute(Qt::WA_NoSystemBackground, true);
    rootWidget_->setAttribute(Qt::WA_TranslucentBackground, true);
#endif

#ifdef __linux__
    rootLayout->addWidget(rootWidget_, 1);
    rootLayout->addWidget(videoPanel_.get());
    rootLayout->addWidget(maskPanel_.get());
#else
    Testing::setAccessibleName(rootWidget_, qsl("AS vw rootWidget_"));
    rootLayout->addWidget(rootWidget_);
#endif

#endif //__linux__

    QIcon icon(build::GetProductVariant(qsl(":/logo/ico_icq"), qsl(":/logo/ico_agent"), qsl(":/logo/ico_biz"), qsl(":/logo/ico_dit")));
    setWindowIcon(icon);

    eventFilter_ = new ResizeEventFilter(panels_, shadow_.getShadowWidget(), this);
    installEventFilter(eventFilter_);

    if (videoPanel_)
    {
        videoPanel_->setFullscreenMode(isInFullscreen());

        connect(videoPanel_.get(), SIGNAL(onMouseEnter()), this, SLOT(onPanelMouseEnter()), Qt::QueuedConnection);
        connect(videoPanel_.get(), SIGNAL(onMouseLeave()), this, SLOT(onPanelMouseLeave()), Qt::QueuedConnection);
        connect(videoPanel_.get(), SIGNAL(onFullscreenClicked()), this, SLOT(onPanelFullscreenClicked()), Qt::QueuedConnection);
        connect(videoPanel_.get(), SIGNAL(onkeyEscPressed()), this, SLOT(onEscPressed()), Qt::QueuedConnection);

        connect(videoPanel_.get(), SIGNAL(autoHideToolTip(bool&)),  this, SLOT(autoHideToolTip(bool&)));
        connect(videoPanel_.get(), SIGNAL(companionName(QString&)), this, SLOT(companionName(QString&)));
        connect(videoPanel_.get(), SIGNAL(showToolTip(bool &)), this, SLOT(showToolTip(bool &)));
        connect(videoPanel_.get(), &VideoPanel::isVideoWindowActive, this, [this](bool& isActive)
                {
                    isActive = isActiveWindow();
                });
        connect(videoPanel_.get(), SIGNAL(onShowMaskPanel()), this, SLOT(onShowMaskPanel()));

        connect(videoPanel_.get(), SIGNAL(onCameraClickOn()), this, SLOT(onSetPreviewPrimary()));
        connect(videoPanel_.get(), SIGNAL(onShareScreenClickOn()), this, SLOT(onScreenSharing()));

        connect(videoPanel_.get(), &VideoPanel::onMicrophoneClick, this, [this]() {buttonStatistic_->buttonMic = true; });
        connect(videoPanel_.get(), &VideoPanel::onGoToChatButton, this, [this]() {buttonStatistic_->buttonChat = true; });
        connect(videoPanel_.get(), &VideoPanel::onCameraClickOn, this, [this]() {buttonStatistic_->buttonCamera = true; });
    }

    if (!!headerPanel_)
    {
        connect(headerPanel_.get(), SIGNAL(onMouseEnter()), this, SLOT(onPanelMouseEnter()), Qt::QueuedConnection);
        connect(headerPanel_.get(), SIGNAL(onMouseLeave()), this, SLOT(onPanelMouseLeave()), Qt::QueuedConnection);

        connect(headerPanel_.get(), SIGNAL(onSecureCallClicked(const QRect&)), this, SLOT(onSecureCallClicked(const QRect&)), Qt::QueuedConnection);
        connect(headerPanel_.get(), &VideoPanelHeader::updateConferenceMode, this, &VideoWindow::updateConferenceMode);
        connect(headerPanel_.get(), &VideoPanelHeader::addUserToConference, this, &VideoWindow::onAddUserClicked);

        connect(headerPanel_.get(),  &VideoPanelHeader::onPlaybackClick, this, [this]() {buttonStatistic_->buttonSpeaker = true; });
        connect(headerPanel_.get(), &VideoPanelHeader::addUserToConference, this, [this]() {buttonStatistic_->buttonAddToCall = true; });

        connect(this, &VideoWindow::onCreateNewCall, headerPanel_.get(), &VideoPanelHeader::onCreateNewCall);
        connect(this, &VideoWindow::onStartedTalk, headerPanel_.get(), &VideoPanelHeader::onStartedTalk);
    }

    if (!!maskPanel_)
    {
        connect(maskPanel_.get(), SIGNAL(makePreviewPrimary()), this, SLOT(onSetPreviewPrimary()), Qt::QueuedConnection);
        connect(maskPanel_.get(), SIGNAL(makeInterlocutorPrimary()), this, SLOT(onSetContactPrimary()), Qt::QueuedConnection);
        connect(maskPanel_.get(), SIGNAL(onShowMaskList()), this, SLOT(onShowMaskList()), Qt::QueuedConnection);

        connect(maskPanel_.get(), SIGNAL(getCallStatus(bool&)), this, SLOT(getCallStatus(bool&)), Qt::DirectConnection);
        connect(maskPanel_.get(), SIGNAL(onHideMaskPanel()), this, SLOT(onHideMaskPanel()));

        videoPanel_->setSelectedMask(maskPanel_->getFirstMask());
    }

    QObject::connect(&Ui::GetDispatcher()->getVoipController(), SIGNAL(onVoipUpdateCipherState(const voip_manager::CipherState&)), this, SLOT(onVoipUpdateCipherState(const voip_manager::CipherState&)), Qt::DirectConnection);
    QObject::connect(&Ui::GetDispatcher()->getVoipController(), SIGNAL(onVoipCallTimeChanged(unsigned,bool)), this, SLOT(onVoipCallTimeChanged(unsigned,bool)), Qt::DirectConnection);
    QObject::connect(&Ui::GetDispatcher()->getVoipController(), SIGNAL(onVoipCallNameChanged(const voip_manager::ContactsList&)), this, SLOT(onVoipCallNameChanged(const voip_manager::ContactsList&)), Qt::DirectConnection);
    QObject::connect(&Ui::GetDispatcher()->getVoipController(), SIGNAL(onVoipMouseTapped(quintptr, const std::string&, const std::string&)), this, SLOT(onVoipMouseTapped(quintptr,const std::string&, const std::string&)), Qt::DirectConnection);
    QObject::connect(&Ui::GetDispatcher()->getVoipController(), SIGNAL(onVoipCallDestroyed(const voip_manager::ContactEx&)), this, SLOT(onVoipCallDestroyed(const voip_manager::ContactEx&)), Qt::DirectConnection);
    QObject::connect(&Ui::GetDispatcher()->getVoipController(), SIGNAL(onVoipMediaRemoteVideo(const voip_manager::VideoEnable&)), this, SLOT(onVoipMediaRemoteVideo(const voip_manager::VideoEnable&)), Qt::DirectConnection);
    QObject::connect(&Ui::GetDispatcher()->getVoipController(), SIGNAL(onVoipWindowRemoveComplete(quintptr)), this, SLOT(onVoipWindowRemoveComplete(quintptr)), Qt::DirectConnection);
    QObject::connect(&Ui::GetDispatcher()->getVoipController(), SIGNAL(onVoipWindowAddComplete(quintptr)), this, SLOT(onVoipWindowAddComplete(quintptr)), Qt::DirectConnection);
    QObject::connect(&Ui::GetDispatcher()->getVoipController(), SIGNAL(onVoipCallOutAccepted(const voip_manager::ContactEx&)), this, SLOT(onVoipCallOutAccepted(const voip_manager::ContactEx&)), Qt::DirectConnection);
    QObject::connect(&Ui::GetDispatcher()->getVoipController(), SIGNAL(onVoipCallCreated(const voip_manager::ContactEx&)), this, SLOT(onVoipCallCreated(const voip_manager::ContactEx&)), Qt::DirectConnection);
    //QObject::connect(&Ui::GetDispatcher()->getVoipController(), SIGNAL(onVoipButtonTapped(const voip_manager::ButtonTap&)), this, SLOT(onVoipButtonTapped(const voip_manager::ButtonTap&)), Qt::DirectConnection);
    //QObject::connect(&Ui::GetDispatcher()->getVoipController(), &voip_proxy::VoipController::onVoipChangeWindowLayout,
    //    this, &VideoWindow::onVoipChangeWindowLayout, Qt::DirectConnection);
    QObject::connect(&Ui::GetDispatcher()->getVoipController(), SIGNAL(onVoipCallConnected(const voip_manager::ContactEx&)), this, SLOT(onVoipCallConnected(const voip_manager::ContactEx&)), Qt::DirectConnection);
    QObject::connect(&Ui::GetDispatcher()->getVoipController(), SIGNAL(onVoipMainVideoLayoutChanged(const voip_manager::MainVideoLayout&)), this, SLOT(onVoipMainVideoLayoutChanged(const voip_manager::MainVideoLayout&)), Qt::DirectConnection);

#ifdef _WIN32
    connect(header_, &VideoWindowHeader::onMinimized,  this, &Ui::VideoWindow::onPanelClickedMinimize);
    connect(header_, &VideoWindowHeader::onFullscreen, this, &Ui::VideoWindow::onPanelClickedMaximize);
    connect(header_, &VideoWindowHeader::onClose,      this, &Ui::VideoWindow::onPanelClickedClose);
#endif

    connect(&checkOverlappedTimer_, SIGNAL(timeout()), this, SLOT(checkOverlap()), Qt::QueuedConnection);
    checkOverlappedTimer_.setInterval(500);
    checkOverlappedTimer_.start();

    connect(&showPanelTimer_, SIGNAL(timeout()), this, SLOT(checkPanelsVis()), Qt::QueuedConnection);
#ifdef __APPLE__
    connect(&_fullscreenNotification, SIGNAL(fullscreenAnimationStart()), this, SLOT(fullscreenAnimationStart()));
    connect(&_fullscreenNotification, SIGNAL(fullscreenAnimationFinish()), this, SLOT(fullscreenAnimationFinish()));
    connect(&_fullscreenNotification, SIGNAL(activeSpaceDidChange()), this, SLOT(activeSpaceDidChange()));
    connect(detachedWnd_.get(), SIGNAL(windowWillDeminiaturize()), this, SLOT(windowWillDeminiaturize()));
    connect(detachedWnd_.get(), SIGNAL(windowDidDeminiaturize()), this, SLOT(windowDidDeminiaturize()));
#endif

    showPanelTimer_.setInterval(1500);
    if (!!detachedWnd_)
    {
        detachedWnd_->hideFrame();
    }

    setMinimumSize(Utils::scale_value(kMinimumW), Utils::scale_value(kMinimumH));
    //saveMinSize(minimumSize());

    resizeToDefaultSize();
}

Ui::VideoWindow::~VideoWindow()
{
    checkOverlappedTimer_.stop();

#ifndef STRIP_VOIP
    rootWidget_->clearPanels();
#endif //__linux__

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

#ifndef __APPLE__
// Qt has a bug with double click and fullscreen switching under macos
void Ui::VideoWindow::mouseDoubleClickEvent(QMouseEvent* _e)
{
    QWidget::mouseDoubleClickEvent(_e);
    _switchFullscreen();
}
#endif

void Ui::VideoWindow::moveEvent(QMoveEvent * event)
{
    QWidget::moveEvent(event);

    hideSecurityDialog();
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

void Ui::VideoWindow::mouseReleaseEvent(QMouseEvent * event)
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
    {
        event->ignore();
    }
}

void Ui::VideoWindow::wheelEvent(QWheelEvent * event)
{
    if (!resendMouseEventToPanel(event))
    {
        event->ignore();
    }
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
    {
        excludeWnd.push_back(panel);
    }

    if (secureCallWnd_ && secureCallWnd_->isVisible())
    {
        excludeWnd.push_back(secureCallWnd_);
    }

    return windowIsOverlapped(rootWidget_, excludeWnd);
}

void Ui::VideoWindow::checkOverlap()
{
    if (!isVisible())
    {
        return;
    }

    assert(!!rootWidget_);
    if (!rootWidget_)
    {
        return;
    }

    // We do not change normal video widnow to small video vindow, if
    // there is active modal window, becuase it can lost focus and
    // close automatically.
    if (!!detachedWnd_ && QApplication::activeModalWidget() == nullptr)
    {

        bool platformState = false;
#ifdef __APPLE__
        platformState  = isFullscreenAnimation_ || changeSpaceAnimation_;
#endif
        // Additional to overlapping we show small video window, if VideoWindow was minimized.
        if (!detachedWnd_->closedManualy() &&
            (isOverlapped() || isMinimized()) && !platformState)
        {
#ifdef _WIN32
            QApplication::alert(this);
#endif
            if (platform::is_apple() && isFullScreen())
                return;

            detachedWnd_->showFrame();
        }
        else
        {
            detachedWnd_->hideFrame();
        }
    }
}

void Ui::VideoWindow::checkPanelsVis()
{
    // Do not hide panels if cursor is under it.
    QWidget* panels[] = { videoPanel_.get(), headerPanel_.get()};
    auto cursorPosition = QCursor().pos();
    for (QWidget* panel : panels)
    {
        if (panel)
        {
            if (panel->frameGeometry().contains(cursorPosition))
            {
                return;
            }
        }
    }

    showPanelTimer_.stop();

    fadeOutPanels(kAnimationDefDuration);

    offsetWindow(Utils::scale_value(kPreviewBorderOffset_topBottom), Utils::scale_value(kPreviewBorderOffset_topBottom));
}

void Ui::VideoWindow::onVoipCallTimeChanged(unsigned _secElapsed, bool _hasCall)
{
    if (!!headerPanel_)
    {
        //headerPanel_->setTime(_secElapsed, _hasCall);
        callDescription.time = _hasCall ? _secElapsed : 0;

        if (_hasCall)
        {
            buttonStatistic_->currentTime = _secElapsed;
        }
        updateWindowTitle();
    }
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
    Ui::GetDispatcher()->getVoipController().setWindowRemove((quintptr)rootWidget_->frameId());
}

void Ui::VideoWindow::showFrame()
{
    Ui::GetDispatcher()->getVoipController().setWindowAdd((quintptr)rootWidget_->frameId(), true, false, (videoPanel_->heightOfCommonPanel() + Utils::scale_value(5)) * Utils::scale_bitmap_ratio());

#ifdef __linux__
    int bottom = Utils::scale_value(kPreviewBorderOffset_with_video_panel);
#else
    int bottom = Utils::scale_value(kPreviewBorderOffset_with_video_panel);
#endif

    offsetWindow(bottom, headerPanel_->geometry().height() + Utils::scale_value(kPreviewBorderOffset_topBottom));

#ifdef __APPLE__
    isLoadSizeFromSettings_ = false;
#endif
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

#ifdef __APPLE__

    rootWidget_->clearPanels();
    std::vector<QPointer<Ui::BaseVideoPanel>> panels;

    for (auto panel : panels_)
    {
        if (panel && panel->isVisible())
        {
            panels.push_back(panel);
        }
    }

    rootWidget_->addPanels(panels);

    for (auto panel : panels_)
    {
        if (panel && panel->isVisible())
        {
            panel->updatePosition(*this);
        }
    }
#endif
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
    if (!!detachedWnd_)
    {
        detachedWnd_->hideFrame();
    }

    shadow_.hideShadow();
}

void Ui::VideoWindow::showEvent(QShowEvent* _ev)
{
    QWidget::showEvent(_ev);
    checkPanelsVisibility();

    showPanelTimer_.stop();
    fadeInPanels(kAnimationDefDuration);

    tryRunPanelsHideTimer();

    if (!!detachedWnd_)
    {
        detachedWnd_->hideFrame();
    }

    // If ICQ is in normal mode, we reload window size from settings.
    //if (!isInFullscreen() && !isLoadSizeFromSettings_)
   // {
        //int savedW = Utils::scale_value(kDefaultW);
        //int savedH = Utils::scale_value(kDefaultH);
        //resize(savedW, savedH);

        //int savedW = 0;
        //int savedH = 0;
//        if (getWindowSizeFromSettings(savedW, savedH))
//        {
//            resize(savedW, savedH);
//#ifdef __APPLE__
//            isLoadSizeFromSettings_ = true;
//#endif
//        }
    //}


    showNormal();
    activateWindow();
    videoPanel_->setFullscreenMode(isInFullscreen());

    shadow_.showShadow();

    updateUserName();
    Utils::InterConnector::instance().getMainWindow()->updateMainMenu();
}

void Ui::VideoWindow::onVoipMouseTapped(quintptr _hwnd, const std::string& _tapType, const std::string& contact)
{
    const bool dblTap = _tapType == "double";
    const bool over   = _tapType == "over";
    const bool single = _tapType == "single";

    if (!!detachedWnd_ && detachedWnd_->getVideoFrameId() == _hwnd)
    {
        if (dblTap)
        {
#ifdef _WIN32
            raise();
#endif
        }
    }
    else if ((quintptr)rootWidget_->frameId() == _hwnd)
    {
        if (dblTap)
        {
            //bool oldLayoutMode = currentLayout_.layout == voip_manager::AllTheSame;
            //if (oldLayoutMode)
            //{
            //    headerPanel_->switchConferenceMode();
            //    Ui::GetDispatcher()->getVoipController().setWindowSetPrimary(getContentWinId(), contact.c_str());
            //}
            //else
            //{
                _switchFullscreen();
            //}
        }
        else if (over)
        {
            showPanelTimer_.stop();

            fadeInPanels(kAnimationDefDuration);

            tryRunPanelsHideTimer();
        }
        else if (single)
        {
            //    onSetContactPrimary();
            if (maskPanel_->isVisible())
            {
                onHideMaskPanel();
            }
            else
            {
                if (currentLayout_.layout == voip_manager::OneIsBig)
                {
                    onSetContactPrimary(contact);
                }
            }
        }
    }
}

void Ui::VideoWindow::onPanelFullscreenClicked()
{
    _switchFullscreen();
}

void Ui::VideoWindow::onVoipCallOutAccepted(const voip_manager::ContactEx& /*contact_ex*/)
{
    //outgoingNotAccepted_ = false;
}

void Ui::VideoWindow::onVoipCallCreated(const voip_manager::ContactEx& _contactEx)
{
    statistic::getGuiMetrics().eventVoipStartCalling();

    if (!_contactEx.incoming)
    {
        outgoingNotAccepted_ = true;

        if (_contactEx.connection_count == 1)
        {
            startTalking = false;
            rootWidget_->createdTalk();
        }
    }

    if (_contactEx.connection_count == 1)
    {
        createdNewCall();
    }
}

void Ui::VideoWindow::checkPanelsVisibility(bool _forceHide /*= false*/)
{
    videoPanel_->setFullscreenMode(isInFullscreen());

    if (isHidden() || isMinimized() || _forceHide)
    {
        hidePanels();
        return;
    }


#ifdef _WIN32
    if (header_)
    {
        header_->setVisible(!isInFullscreen());
        if (headerPanel_)
        {
            headerPanel_->setTopOffset(header_->isVisible() ? header_->height() : 0);
        }
    }
#endif

#ifdef __APPLE__
    if (headerPanel_)
    {
        headerPanel_->setTopOffset(!isInFullscreen() ? headerHeight_ : 0);
    }
#endif

    if (headerPanel_)
    {
        headerPanel_->show();
    }
    if (maskPanel_)
    {
        maskPanel_->setVisible(maskPanelState != SMP_HIDE);
    }
    if (videoPanel_)
    {
        videoPanel_->setVisible(maskPanelState != SMP_SHOW);
    }

    //showPanel(maskPanel_.get());
    //videoPanel_->hide();

    if (!!rootWidget_)
    {
        updatePanels();
    }
}

void Ui::VideoWindow::_switchFullscreen()
{
    if (!isInFullscreen())
    {
        showFullScreen();
    }
    else
    {
        showNormal();
    }

    checkPanelsVisibility();

    if (!!rootWidget_)
    {
        //rootWidget_->fullscreenModeChanged(isInFullscreen());
    }

    fadeInPanels(kAnimationDefDuration);

    tryRunPanelsHideTimer();
}

void Ui::VideoWindow::onVoipCallNameChanged(const voip_manager::ContactsList& _contacts)
{
    if(_contacts.contacts.empty())
    {
        return;
    }

    // Hide secure for conference.
    if (secureCallWnd_ && secureCallWnd_->isVisible() && _contacts.contacts.size() > 1)
    {
        secureCallWnd_->close();
    }

    auto res = std::find(_contacts.windows.begin(), _contacts.windows.end(), (void*)rootWidget_->frameId());

    //if (currentContacts_.empty() || currentContacts_.at(0) == _contacts.at(0))
    if (res != _contacts.windows.end())
    {
        currentContacts_ = _contacts.contacts;

        updateTopPanelSecureCall();
    }

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
#ifdef __APPLE__
        // On Mac we can go to normal size using OSX.
        // We need to update buttons and panel states.
        if (_e->spontaneous())
        {
            checkPanelsVisibility();
            if (!!rootWidget_)
            {
                rootWidget_->fullscreenModeChanged(isInFullscreen());
            }
        }
#endif
    }
}

void Ui::VideoWindow::closeEvent(QCloseEvent* _e)
{
    QWidget::closeEvent(_e);
    Ui::GetDispatcher()->getVoipController().setHangup();
}

void Ui::VideoWindow::resizeEvent(QResizeEvent* _e)
{
    QWidget::resizeEvent(_e);

    //bool isSystemEvent = _e->spontaneous();
    //qt_gui_settings* settings = Ui::get_gui_settings();
    // Save size in settings only if it is system or user event.
    // Ignore any inside window size changing.
    //if (isSystemEvent && settings)
    //{
    //    settings->set_value<int>(videoWndWName.c_str(), width());
    //    settings->set_value<int>(videoWndHName.c_str(), height());
    //}


#ifndef __linux__
    bool bFadeIn = false;
    std::for_each(panels_.begin(), panels_.end(), [&bFadeIn](BaseVideoPanel* panel) {
        if (panel->isVisible())
        {
            bFadeIn = bFadeIn || panel->isFadedIn();
            panel->forceFinishFade();
        }
    });

    if (bFadeIn)
    {
        tryRunPanelsHideTimer();
    }
#endif

#ifdef _WIN32
    int borderWidth = Utils::scale_value(2);
    const auto fg = frameGeometry();
    const auto ge = geometry();

    /* It is better to use setMask, but for Windows it breaks fadein/out animation for ownered panesl.
    We use Win API here to make small border.
    borderWidth = std::min(ge.top() - fg.top(),
                   std::min(fg.right() - ge.right(),
                   std::min(fg.bottom() - ge.bottom(),
                   std::min(ge.left() - fg.left(),
                   borderWidth))));

    QRegion reg(-borderWidth, -borderWidth, ge.width() + 2*borderWidth, ge.height() + 2*borderWidth);
    setMask(reg);
    */

    borderWidth = std::max(
        std::min(ge.top() - fg.top(),
            std::min(fg.right() - ge.right(),
                std::min(fg.bottom() - ge.bottom(),
                    ge.left() - fg.left()))), borderWidth) - borderWidth;

    HRGN rectRegion = CreateRectRgn(borderWidth, borderWidth, fg.width() - borderWidth, fg.height() - borderWidth);
    if (rectRegion)
    {
        SetWindowRgn((HWND)winId(), rectRegion, FALSE);
        DeleteObject (rectRegion);
    }
#endif

    //auto rc = rect();
    //if (rc.width() >= Utils::scale_value(kMinHorizontalWidth))
    //{
    //    // Vertical mode
    //    maskPanel_->setPanelMode(QBoxLayout::TopToBottom);
    //}
    //else
    //{
    //    // Horizontal mode
    //    maskPanel_->setPanelMode(QBoxLayout::LeftToRight);
    //}

    hideSecurityDialog();

    Ui::GetDispatcher()->getVoipController().updateLargeState((quintptr)rootWidget_->frameId(), width() > voip_manager::kWindowLargeWidth);
}

void Ui::VideoWindow::onVoipCallDestroyed(const voip_manager::ContactEx& _contactEx)
{
    auto res = std::find(_contactEx.windows.begin(), _contactEx.windows.end(), (void*)getContentWinId());

    if (res == _contactEx.windows.end())
    {
        return;
    }

    if (_contactEx.connection_count <= 1)
    {
        //unuseAspect();
        hasRemoteVideo_.clear();
        outgoingNotAccepted_ = false;
        startTalking = false;

#ifdef __APPLE__
        // On mac if video window is fullscreen and on active, we make it active.
        // It needs to correct close video window.
        if (isFullScreen() && !isActiveWindow() && !headerPanel_->isActiveWindow() &&
            !videoPanel_->isActiveWindow())
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

        if (secureCallWnd_)
        {
            secureCallWnd_->hide();
        }

        if (!!headerPanel_)
        {
            headerPanel_->enableSecureCall(false);
        }

        if (maskPanel_ && _contactEx.call_count <= 1)
        {
            maskPanel_->callDestroyed();
        }

        currentContacts_.clear();

        sendStatistic();
    }

    // Remove destroyed contact from list.
    currentContacts_.erase(std::remove(currentContacts_.begin(), currentContacts_.end(), _contactEx.contact),
        currentContacts_.end());
}

void Ui::VideoWindow::escPressed()
{
    onEscPressed();
}

void Ui::VideoWindow::onEscPressed()
{
    if (isInFullscreen())
    {
        _switchFullscreen();
    }
}

void Ui::VideoWindow::onSecureCallWndOpened()
{
    onPanelMouseEnter();
    if (!!headerPanel_)
    {
        headerPanel_->setSecureWndOpened(true);
    }
}

void Ui::VideoWindow::onSecureCallWndClosed()
{
    onPanelMouseLeave();
    if (!!headerPanel_)
    {
        headerPanel_->setSecureWndOpened(false);
    }

    delete secureCallWnd_;
    secureCallWnd_ = nullptr;

}

void Ui::VideoWindow::onVoipUpdateCipherState(const voip_manager::CipherState& _state)
{
    enableSecureCall_ = voip_manager::CipherState::kCipherStateEnabled == _state.state;
    if (!!headerPanel_)
    {
        updateTopPanelSecureCall();
    }

    if (secureCallWnd_ && enableSecureCall_)
    {
        secureCallWnd_->setSecureCode(_state.secureCode);
    }
}

void Ui::VideoWindow::onSecureCallClicked(const QRect& _rc)
{
    if (!secureCallWnd_)
    {
        secureCallWnd_ = new SecureCallWnd(this);
        connect(secureCallWnd_, SIGNAL(onSecureCallWndOpened()), this, SLOT(onSecureCallWndOpened()), Qt::QueuedConnection);
        connect(secureCallWnd_, SIGNAL(onSecureCallWndClosed()), this, SLOT(onSecureCallWndClosed()), Qt::QueuedConnection);
    }

    if (!secureCallWnd_)
    {
        return;
    }

    const QPoint windowTCPt((_rc.left() + _rc.right()) * 0.5f + Utils::scale_value(2), _rc.bottom() - Utils::scale_value(4));
    const QPoint secureCallWndTLPt(windowTCPt.x() - secureCallWnd_->width()*0.5f, windowTCPt.y());

    voip_manager::CipherState cipherState;
    Ui::GetDispatcher()->getVoipController().getSecureCode(cipherState);

    if (voip_manager::CipherState::kCipherStateEnabled == cipherState.state)
    {
        secureCallWnd_->setSecureCode(cipherState.secureCode);
        secureCallWnd_->move(secureCallWndTLPt);
        secureCallWnd_->show();
        secureCallWnd_->raise();
        secureCallWnd_->setFocus(Qt::NoFocusReason);
    }

    buttonStatistic_->buttonSecurity = true;
}


bool Ui::VideoWindow::getWindowSizeFromSettings(int& _nWidth, int& _nHeight)
{
    bool res = false;
    if (qt_gui_settings* settings = Ui::get_gui_settings())
    {
        const int tmpW = settings->get_value<int>(videoWndWName.c_str(), -1);
        const int tmpH = settings->get_value<int>(videoWndHName.c_str(), -1);

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
#ifdef __APPLE__
    rootWidget_->windowWillDeminiaturize();
#endif
}

void Ui::VideoWindow::windowDidDeminiaturize()
{
#ifdef __APPLE__
    rootWidget_->windowDidDeminiaturize();
#endif
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

void Ui::VideoWindow::autoHideToolTip(bool& autoHide)
{
    autoHide = !hasRemoteVideoInCall();
}

void Ui::VideoWindow::companionName(QString& name)
{
    name = QString::fromStdString(callDescription.name);
}

void Ui::VideoWindow::showToolTip(bool &show)
{
    show = !isMinimized() && !isOverlapped();
}

void Ui::VideoWindow::updateUserName()
{
    if (currentContacts_.empty())
    {
        return;
    }

    std::vector<std::string> friendlyNames;
    friendlyNames.reserve(currentContacts_.size());
    for (const auto& x : currentContacts_)
    {
        friendlyNames.push_back(Logic::GetFriendlyContainer()->getFriendly(QString::fromStdString(x.contact)).toStdString());
    }

    auto name = voip_proxy::VoipController::formatCallName(friendlyNames, QT_TRANSLATE_NOOP("voip_pages", "and").toUtf8().constData());
    assert(!name.empty());

    if (!!videoPanel_)
    {
        videoPanel_->setContacts(currentContacts_);
    }

    if (headerPanel_)
    {
        headerPanel_->setContacts(currentContacts_);
    }

    callDescription.name = name;
    updateWindowTitle();
}

void Ui::VideoWindow::fadeInPanels(int kAnimationDefDuration)
{
    Ui::GetDispatcher()->getVoipController().passWindowHover(rootWidget_->frameId(), true);

    if (!videoPanel_->isFadedIn() && maskPanelState == SMP_HIDE)
    {
#ifdef __linux__
        int bottom = Utils::scale_value(kPreviewBorderOffset_with_video_panel);
#else
        int bottom = Utils::scale_value(kPreviewBorderOffset_with_video_panel);
#endif

        offsetWindow(bottom, headerPanel_->geometry().height() + Utils::scale_value(kPreviewBorderOffset_topBottom));

        std::for_each(panels_.begin(), panels_.end(), [kAnimationDefDuration](BaseVideoPanel* panel) {
            if (panel->isVisible())
            {
                panel->fadeIn(kAnimationDefDuration);
            }
        });
    }
}

void Ui::VideoWindow::fadeOutPanels(int kAnimationDefDuration)
{
    std::for_each(panels_.begin(), panels_.end(), [kAnimationDefDuration](BaseVideoPanel* panel) {
        panel->fadeOut(kAnimationDefDuration);
    });

    Ui::GetDispatcher()->getVoipController().passWindowHover(rootWidget_->frameId(), false);

    offsetWindow(Utils::scale_value(kPreviewBorderOffset_topBottom), Utils::scale_value(kPreviewBorderOffset_topBottom));
}

void Ui::VideoWindow::hidePanels()
{
    std::for_each(panels_.begin(), panels_.end(), [] (BaseVideoPanel* panel){
        panel->hide();
    });
}

void Ui::VideoWindow::onSetPreviewPrimary()
{
    // Don't switch view for video conference.
    onSetContactPrimary(PREVIEW_RENDER_NAME);
    //Ui::GetDispatcher()->getVoipController().setWindowSetPrimary(getContentWinId(), PREVIEW_RENDER_NAME);
}

void Ui::VideoWindow::onSetContactPrimary()
{
    onSetContactPrimary(currentContacts_[0].contact);
    //Ui::GetDispatcher()->getVoipController().setWindowSetPrimary(getContentWinId(), currentContacts_[0].contact.c_str());
}

void Ui::VideoWindow::onSetContactPrimary(const std::string& contact)
{
    if (currentLayout_.layout == voip_manager::OneIsBig)
    {
        Ui::GetDispatcher()->getVoipController().setWindowSetPrimary(getContentWinId(), contact.c_str());
    }
}

void Ui::VideoWindow::showPanel(QWidget* widget)
{
    if (widget)
    {
        widget->show();
    }
}

void Ui::VideoWindow::hidePanel(QWidget* widget)
{
    if (widget)
    {
        widget->hide();
    }
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
    for (auto panel : panels_)
    {
        bPanelIsActive = bPanelIsActive || (panel && panel->isActiveWindow());
    }
    return !isMinimized() && (bPanelIsActive || QWidget::isActiveWindow());
}

void Ui::VideoWindow::getCallStatus(bool& isAccepted)
{
    isAccepted = !outgoingNotAccepted_;
}

void Ui::VideoWindow::tryRunPanelsHideTimer()
{
    const bool haveSecurityWnd = secureCallWnd_ && secureCallWnd_->isVisible();

    // We hide panel if all are true:
    // 1. Security window is not opened.
    // 2. Mask panel is closed.
    // Removed:
    // 3. Has remove video or now is conference.
    // 4. Have at least one established connection.

    if (/*(hasRemoteVideoInCall() || currentContacts_.size() > 1) &&*/ !haveSecurityWnd && (maskPanel_ == nullptr || !maskPanel_->isOpened())
        /*&& Ui::GetDispatcher()->getVoipController().hasEstablishCall()*/)
    {
        showPanelTimer_.start();
    }
}

void Ui::VideoWindow::hideSecurityDialog()
{
    if (secureCallWnd_ && secureCallWnd_->isVisible())
    {
        secureCallWnd_->hide();
        onSecureCallWndClosed();
    }
}

void Ui::VideoWindow::onMaskListAnimationFinished(bool out)
{
    if (!out)
    {
        if (outgoingNotAccepted_)
        {
            maskPanel_->chooseFirstMask();
        }
    }
}

void Ui::VideoWindow::updateConferenceMode(voip_manager::VideoLayout layout)
{
    Ui::GetDispatcher()->getVoipController().setWindowVideoLayout(rootWidget_->frameId(), layout);
}

bool Ui::VideoWindow::hasRemoteVideoInCall()
{
    return hasRemoteVideo_.size() == 1 && hasRemoteVideo_.begin().value();
}

bool Ui::VideoWindow::hasRemoteVideoForConference()
{
    bool res = false;

    std::for_each(currentContacts_.begin(), currentContacts_.end(), [&res, this](const voip_manager::Contact& contact) {
        const auto contactString = QString::fromStdString(contact.contact);
        res = res || (hasRemoteVideo_.count(contactString) > 0 && hasRemoteVideo_[contactString]);
    });

    return res;
}

void Ui::VideoWindow::removeUnneededRemoteVideo()
{
    QHash <QString, bool> tempHasRemoteVideo;

    std::for_each(currentContacts_.begin(), currentContacts_.end(), [&tempHasRemoteVideo, this](const voip_manager::Contact& contact) {
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
        rootWidget_->startedTalk();

        emit onStartedTalk();
    }
}

void Ui::VideoWindow::onAddUserClicked()
{
    FullVideoWindowPanel dialogParent(this);

    // Correct attach to video window.
    panels_.push_back(&dialogParent);
    eventFilter_->addPanel(&dialogParent);
    updatePanels();

    dialogParent.updatePosition(*this);
    dialogParent.show();

    showAddUserToVideoConverenceDialogVideoWindow(this, &dialogParent);

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
        headerPanel_->changeConferenceMode(mainLayout.layout);
        checkPanelsVisibility();
    }
}

void Ui::VideoWindow::updateTopPanelSecureCall()
{
    if (!!headerPanel_)
    {
        headerPanel_->enableSecureCall(enableSecureCall_ && currentContacts_.size() <= 1);
    }
}

bool Ui::VideoWindow::isInFullscreen() const
{
#ifdef __linux__
    return isFullScreen() || isMaximized();
#else
    return isFullScreen();
#endif
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
        videoPanel_->fadeOut(kAnimationDefDuration);
        maskPanel_->fadeIn(kAnimationDefDuration);

        maskPanelState = SMP_ANIMATION;
        checkPanelsVisibility();

        QTimer::singleShot(kAnimationDefDuration, this, SLOT(setShowMaskPanelState()));

        onSetPreviewPrimary();

        offsetWindow(Utils::scale_value(kPreviewBorderOffset_with_mask_panel),
            headerPanel_->geometry().height() + Utils::scale_value(kPreviewBorderOffset_topBottom));
    }

    buttonStatistic_->masks = true;
}

void Ui::VideoWindow::onHideMaskPanel()
{
    if (maskPanel_->isVisible())
    {
        videoPanel_->fadeIn(kAnimationDefDuration);
        maskPanel_->fadeOut(kAnimationDefDuration);

        maskPanelState = SMP_ANIMATION;
        checkPanelsVisibility();

        QTimer::singleShot(kAnimationDefDuration, this, SLOT(setHideMaskPanelState()));

        onSetContactPrimary();

        videoPanel_->setSelectedMask(maskPanel_->getSelectedWidget() ? maskPanel_->getSelectedWidget() : maskPanel_->getFirstMask());

        offsetWindow(Utils::scale_value(kPreviewBorderOffset_with_video_panel),
            headerPanel_->geometry().height() + Utils::scale_value(kPreviewBorderOffset_topBottom));
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
                    emit finished();
            }
        }
    }

    QWidget::keyPressEvent(_e);
}

void Ui::VideoWindow::keyReleaseEvent(QKeyEvent* _e)
{
    QWidget::keyReleaseEvent(_e);
    if (_e->key() == Qt::Key_Escape)
    {
        emit onEscPressed();
    }
}

void Ui::VideoWindow::onScreenSharing()
{
    onSetPreviewPrimary();
    if (maskPanel_)
    {
        maskPanel_->selectMask(nullptr);
    }
}

void Ui::VideoWindow::createdNewCall()
{
    // Moved window to center of screen of main window.
    resizeToDefaultSize();

    // Reset statistic data.
    buttonStatistic_ = std::make_unique<ButtonStatisticData>();

    emit onCreateNewCall();
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
