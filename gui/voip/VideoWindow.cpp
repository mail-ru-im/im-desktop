#include "stdafx.h"
#include "VideoWindow.h"
#include "VideoWindowHeader.h"
#include "VideoPanel.h"
#include "VideoControls.h"
#include "VideoOverlay.h"
#include "DetachedVideoWnd.h"
#include "WindowController.h"
#include "MicroAlert.h"
#include "Utils.h"
#include "VoipController.h"
#include "VoipProxy.h"
#include "../core_dispatcher.h"
#include "../utils/gui_metrics.h"
#include "../utils/features.h"
#include "../utils/DrawUtils.h"
#include "../utils/macos/mac_support.h"
#include "../main_window/containers/FriendlyContainer.h"
#include "../main_window/MainPage.h"
#include "../main_window/MainWindow.h"
#include "../main_window/sounds/SoundsManager.h"
#include "../main_window/contact_list/RecentsTab.h"
#include "../main_window/contact_list/ContactListModel.h"
#include "../main_window/GroupChatOperations.h"
#include "../gui_settings.h"

#include "../controls/GeneralDialog.h"

#include "styles/ThemeParameters.h"
#include "../previewer/toast.h"
#include "../media/ptt/AudioRecorder2.h"
#include "../media/permissions/MediaCapturePermissions.h"

#ifdef __APPLE__
#include "macos/macosspec.h"
#include "../utils/macos/mac_support.h"
#endif

#ifdef __linux__
#include "linux/linuxspec.h"
#endif

#include "renders/OGLRender.h"


namespace
{
    constexpr int kMinimumW = 714;
    constexpr int kMinimumH = 510;

    constexpr int kShadowBorderWidth = 18;

    constexpr float kDefaultRelativeSizeW = 0.6f;
    constexpr float kDefaultRelativeSizeH = 0.7f;
    constexpr char videoWndWName[] = "video_window_w";
    constexpr char videoWndHName[] = "video_window_h";

    constexpr QMargins kShadowMargins { kShadowBorderWidth, kShadowBorderWidth, kShadowBorderWidth, kShadowBorderWidth };

    constexpr std::chrono::milliseconds kWindowHideTimeout(1500);
    constexpr std::chrono::milliseconds kMicMutedToastInterval(std::chrono::seconds(20));
    constexpr std::chrono::milliseconds kCheckSpacebarInterval(500);
    constexpr std::chrono::milliseconds kTypingInterval(500);
}


namespace Ui
{
    class VideoWindowPrivate
    {
    public:
        class DialogGuard
        {
            Q_DISABLE_COPY_MOVE(DialogGuard)
        public:
            DialogGuard(VideoWindowPrivate* _windowPrivate) : window_(_windowPrivate)
            {
                ToastManager::instance()->hideToast();
                panel_ = new FullVideoWindowPanel(window_->q);
                panel_->setAttribute(Qt::WA_DeleteOnClose);
                window_->stackLayout_->addWidget(panel_);
                panel_->raise();

                window_->controls_->fadeOut();
            }

            FullVideoWindowPanel* dialogParent() const
            {
                return panel_;
            }

            ~DialogGuard()
            {
                window_->controls_->fadeIn();
                panel_->close();
            }
        private:
            FullVideoWindowPanel* panel_;
            VideoWindowPrivate* window_;
        };


        ButtonStatistic::Data buttonStatistic_;

        struct CallDescription
        {
            std::string name;
            unsigned time;
        } callDescription_;

        mutable std::map<std::string, VideoPeer> videoPeers_;
        mutable std::mutex videoPeersLock_;
        mutable im::FramesCallback framesCallback_;

        QPixmap shadowPixmap_;
        // Remote video by users.
        QHash<QString, bool> hasRemoteVideo_;
        bool outgoingNotAccepted_ = false;

        // Current contacts
        std::vector<voip_manager::Contact> currentContacts_;
        voip_manager::MainVideoLayout currentLayout_;

        // All is connected and talking now.
        bool startTalking_ = false;
        bool miniWindowShown_ = false;
        bool isScreenSharingEnabled_ = false;

        // Micro Issues related
        MicroIssue microIssue_;
        QPointer<media::permissions::PermissonsChangeNotifier> permissonsChangeNotifier_;

        bool microphoneEnabled_ = false;
        bool microphoneEnabledBySpace_ = false;

        QTimer* toastMicMutedTimer_ = nullptr;
        QTimer* checkSpacebarTimer_ = nullptr;
        QTimer* overlapTimer_ = nullptr;
        QTimer* keyPressTimer_ = nullptr;

        bool isDialogActive_ = false;
        bool isCloseConfirmation_ = false;

        VideoWindow* q = nullptr;
        WindowController* windowController_ = nullptr;
        QVBoxLayout* rootLayout_ = nullptr;
        QStackedLayout* stackLayout_ = nullptr;
        IRender* rootWidget_ = nullptr;
        VideoControls* controls_ = nullptr;
        VideoOverlay* overlay_ = nullptr;
        VideoWindowHeader* header_ = nullptr;

        DetachedVideoWindow* detachedWnd_ = nullptr;

        Qt::WindowStates storedWState_;

#ifdef __APPLE__
        // We use fullscreen notification to fix a problem on macos
        platform_macos::FullScreenNotificaton fullscreenNotification_;
        bool exitFullScreen_ = false;
        bool exitMinimized_ = false;

        // We have own double click processing,
        // Because Qt has bug with double click and fullscreen
        QTime doubleClickTime_;
        QPoint doubleClickMousePos_;
#endif

        VideoWindowPrivate(VideoWindow* _q)
            : q(_q)
#ifdef __APPLE__
            , fullscreenNotification_(*_q)
#endif
        {}

        void setupWindowFlags()
        {
            if constexpr (platform::is_windows() || platform::is_linux())
                q->setWindowFlags(q->windowFlags() | Qt::FramelessWindowHint);
            else // macOS or unknown
                q->setWindowFlags((q->windowFlags() | Qt::Window | Qt::MaximizeUsingFullscreenGeometryHint) & ~Qt::WindowFullscreenButtonHint);
        }

        void setupAttributes()
        {
            q->setAttribute(Qt::WA_Hover);
            q->setAttribute(Qt::WA_TranslucentBackground);
            q->setAutoFillBackground(false);

#ifdef _WIN32
            q->setAttribute(Qt::WA_NoSystemBackground);
            q->setAttribute(Qt::WA_StaticContents);
            // Following code decrease flickering on resize on Windows OS
            HWND hWnd = (HWND)q->effectiveWinId();
            // This line of code remove unnesesseary repaintings on resize
            ::SetClassLong(hWnd, GCL_STYLE, ::GetClassLong(hWnd, GCL_STYLE) & ~(CS_HREDRAW | CS_VREDRAW));
            // This line of code removes repaintings on overlap
            ::SetWindowLongPtr(hWnd, GWL_STYLE, ::GetWindowLongPtr(hWnd, GWL_STYLE) | WS_CLIPCHILDREN | WS_OVERLAPPED);
#elif defined(__APPLE__)
            // Disable zoom button - too much bugs in MacOS related to fullscreen mode
            Utils::disableZoomButton(q);
#endif
        }

        void createHeaderWidget()
        {
            if constexpr (platform::is_windows() || platform::is_linux())
            {
                header_ = new VideoWindowHeader(q);
                Testing::setAccessibleName(header_, qsl("AS VoipWindow header"));
                QObject::connect(header_, &VideoWindowHeader::onMinimized, q, &Ui::VideoWindow::showMinimized);
                QObject::connect(header_, &VideoWindowHeader::onFullscreen, q, &Ui::VideoWindow::showFullScreen);
                QObject::connect(header_, &VideoWindowHeader::requestHangup,q, &Ui::VideoWindow::onHangupRequested);

                windowController_ = new WindowController(q);
                windowController_->setWidget(q);
                windowController_->setBorderWidth(kShadowBorderWidth - 1);
                windowController_->setOptions(WindowController::Resizing);
                windowController_->setResizeMode(WindowController::ContinuousResize);
            }
        }

        void createRenderWidget()
        {
            OGLRenderWidget* renderWidget = new OGLRenderWidget(q);
            renderWidget->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
            rootWidget_ = renderWidget;
        }

        void createControlsWidget()
        {
            controls_ = new VideoControls(q);
            QObject::connect(q, &VideoWindow::windowStatesChanged, controls_, &VideoControls::onWindowStateChanged);
            QObject::connect(controls_, &VideoControls::toggleFullscreen, q, &VideoWindow::toggleFullscreen);
            QObject::connect(controls_, &VideoControls::requestGridView, q, &VideoWindow::setGridView);
            QObject::connect(controls_, &VideoControls::requestSpeakerView, q, &VideoWindow::setSpeakerView);
            QObject::connect(controls_, &VideoControls::requestLink, q, &VideoWindow::onLinkCopyRequest);
            QObject::connect(controls_->panel(), &VideoPanel::onShareScreenClick, q, &VideoWindow::onScreenSharing);
            QObject::connect(controls_->panel(), &VideoPanel::needShowScreenPermissionsPopup, q, &VideoWindow::showScreenPermissionsPopup);
            QObject::connect(controls_->panel(), &VideoPanel::addUserToConference, q, &VideoWindow::onAddUserRequest);
            QObject::connect(controls_->panel(), &VideoPanel::inviteVCSUrl, q, &VideoWindow::onInviteVCSUrlRequest);
            QObject::connect(controls_->panel(), &VideoPanel::requestHangup, q, &VideoWindow::onHangupRequested);

            QObject::connect(controls_->panel(), &VideoPanel::onMicrophoneClick, [this]() { buttonStatistic_.flags_ |= ButtonStatistic::ButtonMic; });
            QObject::connect(controls_->panel(), &VideoPanel::onGoToChatButton, [this]() { buttonStatistic_.flags_ |= ButtonStatistic::ButtonChat; });
            QObject::connect(controls_->panel(), &VideoPanel::onCameraClickOn, [this]() { buttonStatistic_.flags_ |= ButtonStatistic::ButtonCamera; });
            QObject::connect(controls_->panel(), &VideoPanel::onSpeakerClick, [this]() { buttonStatistic_.flags_ |= ButtonStatistic::ButtonSpeaker; });
            QObject::connect(controls_->panel(), &VideoPanel::addUserToConference, [this]() {buttonStatistic_.flags_ |= ButtonStatistic::ButtonAddToCall; });
        }

        void createOverlayWidget()
        {
            overlay_ = new VideoOverlay(q);
        }

        void createDetachedVideoWindow()
        {
            detachedWnd_ = new DetachedVideoWindow(q);
            detachedWnd_->installEventFilter(q);
            QObject::connect(detachedWnd_, &DetachedVideoWindow::needShowScreenPermissionsPopup, q, &VideoWindow::showScreenPermissionsPopup);
            QObject::connect(detachedWnd_, &DetachedVideoWindow::onMicrophoneClick, q, &VideoWindow::showMicroPermissionPopup);
            QObject::connect(detachedWnd_, &DetachedVideoWindow::onShareScreenClick, q, &VideoWindow::onScreenSharing);
            QObject::connect(detachedWnd_, &DetachedVideoWindow::activateMainVideoWindow, q, &VideoWindow::raiseWindow);
            QObject::connect(detachedWnd_, &DetachedVideoWindow::requestHangup, q, &VideoWindow::onHangupRequested);
            QObject::connect(q, &VideoWindow::windowTitleChanged, detachedWnd_, &DetachedVideoWindow::setWindowTitle);
        }

        void createTimers()
        {
            overlapTimer_ = new QTimer(q);
            QObject::connect(overlapTimer_, &QTimer::timeout, q, &VideoWindow::checkOverlap);

            keyPressTimer_ = new QTimer(q);
            keyPressTimer_->setInterval(kTypingInterval);
            keyPressTimer_->setSingleShot(true);
            QObject::connect(Utils::InterConnector::instance().getMainWindow(), &MainWindow::anyKeyPressed, keyPressTimer_, qOverload<>(&QTimer::start));

            checkSpacebarTimer_ = new QTimer(q);
            checkSpacebarTimer_->setInterval(kCheckSpacebarInterval);
            QObject::connect(checkSpacebarTimer_, &QTimer::timeout, q, &VideoWindow::onSpacebarTimeout);

            toastMicMutedTimer_ = new QTimer(q);
            toastMicMutedTimer_->setInterval(kMicMutedToastInterval);
            toastMicMutedTimer_->setSingleShot(true);
        }

        void createLayout(QWidget* _centralWidget)
        {
            rootLayout_ = Utils::emptyVLayout(q);
            if (header_)
                rootLayout_->addWidget(header_);
            rootLayout_->addWidget(_centralWidget);
            if constexpr (platform::is_linux() || platform::is_windows())
                rootLayout_->setContentsMargins(kShadowMargins);
            stackLayout_ = new QStackedLayout(_centralWidget);
            stackLayout_->setContentsMargins(QMargins{});
            stackLayout_->setSpacing(0);
            stackLayout_->setStackingMode(QStackedLayout::StackAll);
            stackLayout_->addWidget(overlay_);
            stackLayout_->addWidget(controls_);
            stackLayout_->addWidget(rootWidget_->getWidget());
        }

        void createShortcuts()
        {
            if constexpr (!platform::is_apple())
            {
                auto closeShortcut = new QShortcut(Qt::ControlModifier + Qt::Key_Q, q, nullptr, nullptr, Qt::ApplicationShortcut);
                QObject::connect(closeShortcut, &QShortcut::activated, Utils::InterConnector::instance().getMainWindow(), &MainWindow::exit);
            }
            auto switchMicrophoneShortcut = new QShortcut(Qt::ControlModifier + Qt::SHIFT + Qt::Key_A, q, nullptr, nullptr, Qt::ApplicationShortcut);
            switchMicrophoneShortcut->setAutoRepeat(false);
            QObject::connect(switchMicrophoneShortcut, &QShortcut::activated, q, &VideoWindow::switchMicrophoneEnableByShortcut);
        }

        void initializeVoip()
        {
            auto voipCtrl = &Ui::GetDispatcher()->getVoipController();
            QObject::connect(voipCtrl, &voip_proxy::VoipController::onVoipCallTimeChanged, q, &VideoWindow::onVoipCallTimeChanged);
            QObject::connect(voipCtrl, &voip_proxy::VoipController::onVoipCallNameChanged, q, &VideoWindow::onVoipCallNameChanged);
            QObject::connect(voipCtrl, &voip_proxy::VoipController::onVoipCallDestroyed, q, &VideoWindow::onVoipCallDestroyed);
            QObject::connect(voipCtrl, &voip_proxy::VoipController::onVoipMediaRemoteVideo, q, &VideoWindow::onVoipMediaRemoteVideo);
            QObject::connect(voipCtrl, &voip_proxy::VoipController::onVoipWindowRemoveComplete, q, &VideoWindow::onVoipWindowRemoveComplete);
            QObject::connect(voipCtrl, &voip_proxy::VoipController::onVoipWindowAddComplete, q, &VideoWindow::onVoipWindowAddComplete);
            QObject::connect(voipCtrl, &voip_proxy::VoipController::onVoipCallCreated, q, &VideoWindow::onVoipCallCreated);
            QObject::connect(voipCtrl, &voip_proxy::VoipController::onVoipCallConnected, q, &VideoWindow::onVoipCallConnected);

            QObject::connect(voipCtrl, &voip_proxy::VoipController::onVoipVoiceEnable, q, &VideoWindow::onVoipVoiceEnable);
            QObject::connect(voipCtrl, &voip_proxy::VoipController::onVoipMediaLocalAudio, q, &VideoWindow::onVoipMediaLocalAudio);
            QObject::connect(voipCtrl, &voip_proxy::VoipController::onVoipMediaLocalVideo, q, &VideoWindow::onVoipMediaLocalVideo);
            QObject::connect(voipCtrl, &voip_proxy::VoipController::onVoipVideoDeviceSelected, q, &VideoWindow::onVoipVideoDeviceSelected);

            QObject::connect(q, &VideoWindow::drawFrames, q, &VideoWindow::onDrawFrames);

            QObject::connect(GetSoundsManager(), &SoundsManager::deviceListChanged, q, &VideoWindow::onAudioDeviceListChanged);
        }

        void initUi()
        {
            QWidget* centralWidget = new QWidget(q);
            createTimers();
            createHeaderWidget();
            createRenderWidget();
            createControlsWidget();
            createOverlayWidget();
            createLayout(centralWidget);
            createDetachedVideoWindow();
            createShortcuts();
            initializeVoip();
            overlay_->hide();
            overlapTimer_->start(500);
#ifdef __APPLE__
            QObject::connect(&fullscreenNotification_, &platform_macos::FullScreenNotificaton::fullscreenAnimationFinish, q, &VideoWindow::fullscreenAnimationFinish);
            QObject::connect(&fullscreenNotification_, &platform_macos::FullScreenNotificaton::minimizeAnimationFinish, q, &VideoWindow::minimizeAnimationFinish);
#endif
        }

        void hideDetachedWindow()
        {
            if (!miniWindowShown_)
                return;
            miniWindowShown_ = false;
            detachedWnd_->hideFrame();
        }

        quintptr getContentWinId() const
        {
            auto framesCallback = [this](std::unique_ptr<const im::IVideoFrame> frame, const voip::PeerId& peerId)
            {   // called not from main thread
                std::unique_lock<std::mutex> lock(videoPeersLock_);
                VideoPeer& peer = videoPeers_[peerId];
                peer.lastFrame_ = std::move(frame);
                peer.lastFrameDrawed_ = false;
                Q_EMIT q->drawFrames();
            };
            if (!framesCallback_)
                framesCallback_ = framesCallback;
            return (quintptr)&framesCallback_;
        }

        bool verifyWinId(quintptr _winId) const
        {
            return _winId == getContentWinId();
        }

        void updateUserName()
        {
            if (currentContacts_.empty())
                return;

            if (!isCallVCS())
            {
                std::vector<std::string> friendlyNames;
                friendlyNames.reserve(currentContacts_.size());
                for (const auto& x : currentContacts_)
                    friendlyNames.push_back(Logic::GetFriendlyContainer()->getFriendly(QString::fromStdString(x.contact)).toStdString());
                callDescription_.name = voip_proxy::VoipController::formatCallName(friendlyNames, QT_TRANSLATE_NOOP("voip_pages", "and").toUtf8().constData());
                im_assert(!callDescription_.name.empty());
            }

            controls_->panel()->setContacts(currentContacts_, startTalking_);
        }

        static bool isCallVCS()
        {
            return Ui::GetDispatcher()->getVoipController().isCallVCS();
        }

        static bool isCallPinnedRoom()
        {
            return Ui::GetDispatcher()->getVoipController().isCallPinnedRoom();
        }

        bool hasRemoteVideoInCall()
        {
            return hasRemoteVideo_.size() == 1 && hasRemoteVideo_.begin().value();
        }

        void resetWindowSize()
        {
            Ui::MainWindow* wnd = static_cast<Ui::MainWindow*>(MainPage::instance()->window());
            if (!wnd)
                return;

            QRect screenRect;
            if constexpr (platform::is_windows())
                screenRect = QApplication::desktop()->availableGeometry(wnd->getScreen());
            else
                screenRect = QApplication::desktop()->availableGeometry(wnd);

            q->setGeometry(
                QStyle::alignedRect(
                    Qt::LeftToRight,
                    Qt::AlignCenter,
                    QSize(screenRect.size().width() * kDefaultRelativeSizeW, screenRect.size().height() * kDefaultRelativeSizeH),
                    screenRect
                )
            );
        }

        QRect toastRect() const
        {
            QRect rect = q->rect();
            rect.setBottom(controls_->panel()->geometry().top());
            return rect;
        }
    };

    VideoWindow::VideoWindow()
        : QWidget()
        , d(std::make_unique<VideoWindowPrivate>(this))
    {
        setWindowIcon(QIcon(qsl(":/logo/ico")));
        setMinimumSize(Utils::scale_value(kMinimumW), Utils::scale_value(kMinimumH));
        d->setupWindowFlags();
        d->setupAttributes();
        d->initUi();
        d->resetWindowSize();
    }

    VideoWindow::~VideoWindow()
    {
        ToastManager::instance()->hideToast();
    }

    QRect VideoWindow::toastRect() const
    {
        return d->toastRect();
    }

    void VideoWindow::raiseWindow()
    {
        platform_specific::showInCurrentWorkspace(this, d->detachedWnd_);

        const auto scrP = windowHandle()->screen();
        const auto scrC = d->detachedWnd_->windowHandle()->screen();
        if (scrP != scrC)
        {
            QRect newGeo = geometry();
            newGeo.moveCenter(scrC->geometry().center());
            setGeometry(newGeo);
        }

        if (isMinimized())
            showNormal();
        raise();
        activateWindow();
    }

    void VideoWindow::hideFrame()
    {
        GetDispatcher()->getVoipController().setWindowRemove((quintptr)d->getContentWinId());
        hide();
    }

    void VideoWindow::showFrame()
    {
        GetDispatcher()->getVoipController().setWindowAdd((quintptr)d->getContentWinId(), "", true, false, 0);

        d->rootWidget_->setVCS(d->isCallVCS());
        d->rootWidget_->setPinnedRoom(d->isCallPinnedRoom());
        d->detachedWnd_->getRender()->setVCS(d->isCallVCS());
        d->detachedWnd_->getRender()->setPinnedRoom(d->isCallPinnedRoom());

        show();
    }

    void VideoWindow::toggleFullscreen()
    {
        setWindowState(windowState() ^ Qt::WindowFullScreen);
    }

    void VideoWindow::setGridView()
    {
        d->rootWidget_->setViewMode(IRender::ViewMode::GridMode);
        d->controls_->updateControls(IRender::ViewMode::GridMode);
    }

    void VideoWindow::setSpeakerView()
    {
        d->rootWidget_->setViewMode(IRender::ViewMode::SpeakerMode);
        d->controls_->updateControls(IRender::ViewMode::SpeakerMode);
    }

    void VideoWindow::showMicroPermissionPopup()
    {
        raiseWindow();
        d->controls_->panel()->showPermissionPopup();
    }

    void VideoWindow::switchVideoEnableByShortcut()
    {
        if (isMinimized() || d->detachedWnd_->isVisible())
            raiseWindow();

        // Ctrl+Shift+V  - enable/disable video
        d->controls_->panel()->onVideoOnOffClicked();
    }

    void VideoWindow::switchScreenSharingEnableByShortcut()
    {
        if (isMinimized() || d->detachedWnd_->isVisible())
            raiseWindow();

        // Ctrl+Shift+S  - enable/disable screen sharing
        d->controls_->panel()->onShareScreen();
    }

    void VideoWindow::switchSoundEnableByShortcut()
    {
        if (isMinimized() || d->detachedWnd_->isVisible())
            raiseWindow();

        // Ctrl+Shift+D  - enable/disable sound
        d->controls_->panel()->onSpeakerOnOffClicked();
    }

    void VideoWindow::switchMicrophoneEnableByShortcut()
    {
        if (isMinimized() || d->detachedWnd_->isVisible())
            raiseWindow();

        if (!GetDispatcher()->getVoipController().isLocalAudAllowed())
        {
            VideoWindowToastProvider::instance().show(VideoWindowToastProvider::Type::MicNotAllowed, this, d->toastRect());
            return;
        }

        // Ctrl+Shift+A  - enable/disable microphone
        VideoWindowToastProvider::Options options = VideoWindowToastProvider::instance().defaultOptions();
        options.text_ = d->microphoneEnabled_ ? QT_TRANSLATE_NOOP("voip_pages", "Microphone disabled") : QT_TRANSLATE_NOOP("voip_pages", "Microphone enabled");
        VideoWindowToastProvider::instance().showToast(options, this, d->toastRect());
        GetDispatcher()->getVoipController().setSwitchACaptureMute();
    }

    void VideoWindow::onDrawFrames()
    {
        std::unique_lock<std::mutex> lock(d->videoPeersLock_);
        for (auto& p : d->videoPeers_)
        {
            const std::string& streamId = p.first;
            VideoPeer& peer = p.second;
            if (!peer.lastFrame_)
                continue;
            if (!peer.lastFrameDrawed_)
            {
                peer.lastFrameDrawed_ = true;
                d->rootWidget_->setFrame(peer.lastFrame_.get(), streamId);
                if (d->detachedWnd_ && d->miniWindowShown_)
                    d->detachedWnd_->getRender()->setFrame(peer.lastFrame_.get(), streamId);
            }
        }
    }

    void VideoWindow::removeUnneededRemoteVideo()
    {
        QHash <QString, bool> tempHasRemoteVideo;

        std::for_each(d->currentContacts_.begin(), d->currentContacts_.end(), [&tempHasRemoteVideo, this](const voip_manager::Contact& contact)
        {
            const auto contactString = QString::fromStdString(contact.contact);
            tempHasRemoteVideo[contactString] = (d->hasRemoteVideo_.count(contactString) > 0 && d->hasRemoteVideo_[contactString]);
        });
        d->hasRemoteVideo_.swap(tempHasRemoteVideo);
    }

    void VideoWindow::onVoipCallTimeChanged(unsigned _secElapsed, bool _hasCall)
    {
        d->callDescription_.time = _hasCall ? _secElapsed : 0;
        if (_hasCall)
            d->buttonStatistic_.currentTime_ = _secElapsed;
    }

    void VideoWindow::onVoipCallNameChanged(const voip_manager::ContactsList& _contacts)
    {
        d->controls_->updateControls(_contacts);

        if (!_contacts.isActive)
            return;

        d->rootWidget_->updatePeers(_contacts);
        d->controls_->updateControls(d->rootWidget_->getViewMode());

        if (!d->isCallVCS())
            setWindowTitle(QString(qsl("%1: %2")).arg(QT_TRANSLATE_NOOP("voip_pages", "In call")).arg(_contacts.contacts.size() + 1));

        if (_contacts.contacts.empty())
            return;

        const auto res = std::find(_contacts.windows.begin(), _contacts.windows.end(), (void*)d->getContentWinId());
        if (res != _contacts.windows.end())
            d->currentContacts_ = _contacts.contacts;

        d->callDescription_.name = _contacts.conference_name;
        if (d->isCallVCS())
            setWindowTitle(QString::fromStdString(d->callDescription_.name));

        d->updateUserName();
        removeUnneededRemoteVideo();
    }

    void VideoWindow::onVoipCallDestroyed(const voip_manager::ContactEx& _contactEx)
    {
        if (!_contactEx.current_call)
            return;

        d->rootWidget_->clear();
        d->detachedWnd_->getRender()->clear();
        d->hasRemoteVideo_.clear();
        d->outgoingNotAccepted_ = false;
        d->startTalking_ = false;

        d->toastMicMutedTimer_->stop();
        ToastManager::instance()->hideToast();

        if constexpr (!platform::is_windows())
            releaseKeyboard();

        d->controls_->panel()->callDestroyed();
        d->controls_->updateControls(d->rootWidget_->getViewMode());

        d->currentContacts_.clear();
        d->buttonStatistic_.send();
    }

    void VideoWindow::onVoipMediaRemoteVideo(const voip_manager::VideoEnable& videoEnable)
    {
        d->hasRemoteVideo_[QString::fromStdString(videoEnable.contact.contact)] = videoEnable.enable;
        if (d->currentContacts_.size() <= 1)
        {
            if (d->hasRemoteVideoInCall())
                d->controls_->delayFadeOut();
            else
                d->controls_->fadeIn();
        }
    }

    void VideoWindow::onVoipWindowRemoveComplete(quintptr _winId)
    {
        if (!d->verifyWinId(_winId))
            return;

        // breaks mousePressEvent/mouseReleaseEvent events in VideoFrameMacos.mm on 2nd call
        if (d->permissonsChangeNotifier_)
        {
            d->permissonsChangeNotifier_->stop();
            d->permissonsChangeNotifier_->deleteLater();
        }
        Ui::GetDispatcher()->getVoipController().setRequestSettings(); // Trigger pending call accepts if any

        if (isFullScreen() || isMinimized())
        {
#ifdef __APPLE__
            // on macos, we have to wait for the showMinimized->showNormal animation to complete before hide()
            d->exitMinimized_ = isMinimized();
            if (d->exitMinimized_)
            {
                showNormal();
                hide();
                qApp->processEvents();
                return;
            }
            // on macos, we have to wait for the showFullScreen->showNormal animation to complete before hide()
            d->exitFullScreen_ = isFullScreen();

            showNormal();
            return;
#endif
            showNormal();
            d->hideDetachedWindow();
        }
        raise();
        hide();
    }

    void VideoWindow::onVoipWindowAddComplete(quintptr _winId)
    {
        if (!d->verifyWinId(_winId))
            return;

        // dismiss exiting on MacOS if we have overlapped call
#ifdef __APPLE__
        d->exitFullScreen_ = false;
#endif
        if (isMinimized())
            showNormal();
        else
            show();

        d->controls_->panel()->updatePanelButtons();
        checkMicroPermission();
        if constexpr (platform::is_windows())
        {
            d->permissonsChangeNotifier_ = new media::permissions::PermissonsChangeNotifier(this);
            connect(d->permissonsChangeNotifier_, &media::permissions::PermissonsChangeNotifier::deviceChanged, this, [this](media::permissions::DeviceType _devType)
            {
                if (_devType == media::permissions::DeviceType::Microphone)
                    checkMicroPermission();
            });
            d->permissonsChangeNotifier_->start();
        }
        d->rootWidget_->getWidget()->update();
    }

    void VideoWindow::onVoipCallCreated(const voip_manager::ContactEx& _contactEx)
    {
        statistic::getGuiMetrics().eventVoipStartCalling();
        if (!_contactEx.incoming)
        {
            d->outgoingNotAccepted_ = true;
            d->startTalking_ = false;
        }
        d->controls_->panel()->updatePanelButtons();
    }

    void VideoWindow::onVoipCallConnected(const voip_manager::ContactEx& _contactEx)
    {
        if (!d->startTalking_)
        {
            d->startTalking_ = true;
            d->updateUserName();
        }
        // Reset statistic data
        d->buttonStatistic_.reset();
    }

    void VideoWindow::onVoipVoiceEnable(bool _enabled)
    {
        if (_enabled && !d->microphoneEnabled_ && !d->toastMicMutedTimer_->isActive() && GetDispatcher()->getVoipController().isLocalAudAllowed() && !d->keyPressTimer_->isActive())
        {
            if (get_gui_settings()->get_value<bool>(settings_warn_about_disabled_microphone, true))
            {
                VideoWindowToastProvider::instance().show(VideoWindowToastProvider::Type::NotifyMicMuted, this, rect());

                d->toastMicMutedTimer_->start();
            }
        }
    }

    void VideoWindow::onVoipVideoDeviceSelected(const voip_proxy::device_desc& _desc)
    {
#ifdef __APPLE__
        if (d->exitMinimized_)
        {
            hide();
            return;
        }
#endif
        const bool isScreenSharingEnabled_ = (_desc.dev_type == voip_proxy::kvoipDevTypeVideoCapture &&
                                              _desc.video_dev_type == voip_proxy::kvoipDeviceDesktop);
        if (isScreenSharingEnabled_)
        {
            if (isVisible() && !isMinimized())
                showMinimized();
        }
        else
        {
            if (!isMinimized())
                return;

            if constexpr (platform::is_windows())
                showNormal();

            if ((d->storedWState_ & Qt::WindowFullScreen))
                showFullScreen();
            else if (d->storedWState_ & Qt::WindowMaximized)
                showMaximized();
            else
                showNormal();

            raise();
            activateWindow();
        }
    }

    void VideoWindow::onVoipMediaLocalAudio(bool _enabled)
    {
        d->microphoneEnabled_ = _enabled;
        if (!_enabled)
        {
            d->microphoneEnabledBySpace_ = false;
            releaseKeyboard();
        }
        else
        {
            d->toastMicMutedTimer_->stop();
        }

        updateMicroAlertState();
    }

    void VideoWindow::onVoipMediaLocalVideo(bool _enabled)
    {
        d->rootWidget_->localMediaChanged(d->microphoneEnabled_, _enabled);
    }

    void VideoWindow::onAudioDeviceListChanged()
    {
        if (isActiveWindow() || Ui::GetDispatcher()->getVoipController().hasEstablishCall())
            checkMicroPermission();
    }

    void VideoWindow::onHangupRequested()
    {
        if (GetDispatcher()->getVoipController().isResetCallDialogActive())
        {
            Utils::InterConnector::instance().getMainWindow()->activate();
            return;
        }

#ifdef __APPLE__
        Utils::enableCloseButton(this, false);
#endif

        static bool isShow = false;
        if (isShow)
            return;

        QScopedValueRollback guard(isShow, true);
        QScopedValueRollback scopedValue(d->isDialogActive_, true);
        QScopedValueRollback closeGuard(d->isCloseConfirmation_, true);

        VideoWindowPrivate::DialogGuard dialogGuard(d.get());

        d->controls_->fadeOut();
        const auto confirm = Utils::GetConfirmationWithTwoButtons(
            QT_TRANSLATE_NOOP("voip_video_panel", "Cancel"),
            QT_TRANSLATE_NOOP("voip_video_panel", "Yes"),
            QT_TRANSLATE_NOOP("voip_video_panel", "Are you sure you want to end the meeting?"),
            QT_TRANSLATE_NOOP("voip_video_panel", "End meeting"),
            dialogGuard.dialogParent()
        );

        if (confirm)
        {
            GetDispatcher()->getVoipController().setHangup();
            return;
        }
#ifdef __APPLE__
        Utils::enableCloseButton(this, true);
#endif
    }

    void Ui::VideoWindow::onSpacebarTimeout()
    {
        if (!platform_specific::isSpacebarDown())
        {
            d->checkSpacebarTimer_->stop();
            QApplication::postEvent(this, new QKeyEvent(QEvent::KeyRelease, Qt::Key_Space, Qt::NoModifier));
        }
    }

    void VideoWindow::checkOverlap()
    {
        if (!isVisible() || (windowState() & Qt::WindowFullScreen))
        {
            d->hideDetachedWindow();
            return;
        }
        else
        {
            if (platform_specific::windowIsOverlapped(this) || isMinimized())
            {
                if (!d->detachedWnd_->isVisible())
                    d->detachedWnd_->showFrame();
            }
            else
            {
                if (d->detachedWnd_->isVisible())
                    d->detachedWnd_->hideFrame();
            }
        }
        if (!isVisible() || isFullScreen())
        {
            d->hideDetachedWindow();
            return;
        }

        // We do not change normal video window to small video window, if
        // there is active modal window, because it can lost focus and
        // close automatically.
        if (!d->detachedWnd_->isMousePressed() && !QApplication::activeModalWidget())
        {
            // Additional to overlapping we show small video window, if VideoWindow was minimized.
            if (!d->detachedWnd_->closedManualy() && (platform_specific::windowIsOverlapped(this) || isMinimized()))
            {
                if (!d->miniWindowShown_)
                {
                    d->miniWindowShown_ = true;
                    if (d->isScreenSharingEnabled_)
                    {
                        d->detachedWnd_->moveToCorner();
                        d->detachedWnd_->showFrame(DetachedVideoWindow::WindowMode::Compact);
                    }
                    else
                    {
                        d->detachedWnd_->showFrame();
                    }
                }
            }
            else
            {
                d->hideDetachedWindow();
            }
        }
    }

    void VideoWindow::checkMicroPermission()
    {
        Ui::GetDispatcher()->getVoipController().checkPermissions(true, false, false);
        updateMicroAlertState();
    }

    void VideoWindow::updateMicroAlertState()
    {
        if (Ui::GetDispatcher()->getVoipController().isAudPermissionGranted())
            d->microIssue_ = !ptt::AudioRecorder2::hasDevice() ? MicroIssue::NotFound : MicroIssue::None;
        else
            d->microIssue_ = MicroIssue::NoPermission;

        d->overlay_->showMicroAlert(d->microIssue_);
    }

    void VideoWindow::onSetPreviewPrimary()
    {
        // Don't switch view for video conference.
        onSetContactPrimary(PREVIEW_RENDER_NAME);
    }

    void VideoWindow::onSetContactPrimary()
    {
        if (!d->currentContacts_.empty())
            onSetContactPrimary(d->currentContacts_.front().contact);
    }

    void VideoWindow::onSetContactPrimary(const std::string& _contact)
    {
        if (d->currentLayout_.layout != voip_manager::OneIsBig || d->isCallVCS())
            return;
    }

    void VideoWindow::onScreenSharing(bool _on)
    {
        d->isScreenSharingEnabled_ = _on;
        onSetContactPrimary();
    }

    void VideoWindow::onLinkCopyRequest()
    {
        d->controls_->panel()->onClickCopyVCSLink();
    }

    void VideoWindow::onAddUserRequest()
    {
        QScopedValueRollback scopedValue(d->isDialogActive_, true);

        VideoWindowPrivate::DialogGuard dialogGuard(d.get());

        d->controls_->fadeOut();
        Utils::hideVideoWindowToast();
        showAddUserToVideoConverenceDialogVideoWindow(this, dialogGuard.dialogParent());
    }

    void VideoWindow::onInviteVCSUrlRequest(const QString& _url)
    {
        QScopedValueRollback scopedValue(d->isDialogActive_, true);

        VideoWindowPrivate::DialogGuard dialogGuard(d.get());

        Utils::hideVideoWindowToast();
        showInviteVCSDialogVideoWindow(dialogGuard.dialogParent(), _url);
    }

    void VideoWindow::showScreenPermissionsPopup(media::permissions::DeviceType _type)
    {
        static bool isShow = false;
        if (isShow)
            return;

        QScopedValueRollback guard(isShow, true);
        QScopedValueRollback scopedValue(d->isDialogActive_, true);

        VideoWindowPrivate::DialogGuard dialogGuard(d.get());

        bool res = false;
        if (media::permissions::DeviceType::Screen == _type)
        {
            res = Utils::GetConfirmationWithTwoButtons(QT_TRANSLATE_NOOP("input_widget", "Cancel"), QT_TRANSLATE_NOOP("input_widget", "Open settings"),
                QT_TRANSLATE_NOOP("voip_pages", "To share screen you need to allow access to the screen recording in the system settings"),
                QT_TRANSLATE_NOOP("voip_pages", "Screen recording permissions"), dialogGuard.dialogParent());
        }
        else if (media::permissions::DeviceType::Microphone == _type)
        {
            res = Utils::GetConfirmationWithTwoButtons(QT_TRANSLATE_NOOP("input_widget", "Cancel"), QT_TRANSLATE_NOOP("input_widget", "Open settings"),
                QT_TRANSLATE_NOOP("voip_video_panel", "Allow access to microphone. Click \"Settings\" and allow application access to microphone"),
                QT_TRANSLATE_NOOP("popup_window", "Allow microphone access, so that interlocutor could hear you"), dialogGuard.dialogParent());
        }
        else if (media::permissions::DeviceType::Camera == _type)
        {
            res = Utils::GetConfirmationWithTwoButtons(QT_TRANSLATE_NOOP("input_widget", "Cancel"), QT_TRANSLATE_NOOP("input_widget", "Open settings"),
                QT_TRANSLATE_NOOP("voip_pages", "To use camera you need to allow access to the camera in the system settings"),
                QT_TRANSLATE_NOOP("voip_pages", "Camera permissions"), dialogGuard.dialogParent());
        }
        if (res)
            media::permissions::openPermissionSettings(_type);
    }

    void VideoWindow::minimizeAnimationFinish()
    {
#ifdef __APPLE__
        Utils::disableZoomButton(this);
        d->exitMinimized_ = false;
#endif
    }

    void VideoWindow::fullscreenAnimationFinish()
    {
#ifdef __APPLE__
        Utils::disableZoomButton(this);
        if (!d->exitFullScreen_)
            return;
        d->exitFullScreen_ = false;
        hide();
#endif
    }

    bool VideoWindow::event(QEvent* _event)
    {
        switch (_event->type())
        {
        case QEvent::HoverEnter:
            hoverEnter(static_cast<QHoverEvent*>(_event));
            return true;
            break;
        case QEvent::HoverLeave:
            hoverLeave(static_cast<QHoverEvent*>(_event));
            return true;
            break;
        case QEvent::HoverMove:
            hoverMove(static_cast<QHoverEvent*>(_event));
            return true;
            break;
        case QEvent::WindowStateChange:
            windowStateChanged(static_cast<QWindowStateChangeEvent*>(_event));
            break;
        case QEvent::ActivationChange:
            if constexpr (platform::is_apple())
            {
                // workaround to release keyboard after switching between windows by cmd+tab
                // cause the keyboard focus remains but the keyboard events are not received
                if (keyboardGrabber() == this && !platform_specific::isSpacebarDown())
                    QApplication::postEvent(this, new QKeyEvent(QEvent::KeyRelease, Qt::Key_Space, Qt::NoModifier));
            }
            else
            {
                if (GetDispatcher()->getVoipController().isResetCallDialogActive() && d->isCloseConfirmation_)
                    Utils::InterConnector::instance().getMainWindow()->activate();
            }
            break;
        default:
            break;
        }
        return QWidget::event(_event);
    }

    bool VideoWindow::eventFilter(QObject* _watched, QEvent* _event)
    {
        if (_watched == d->detachedWnd_ &&
            (_event->type() == QEvent::KeyPress ||
            _event->type() == QEvent::KeyRelease))
        {
            QKeyEvent* keyEvent = static_cast<QKeyEvent*>(_event);
            if (keyEvent->key() == Qt::Key_Space && keyEvent->modifiers() == Qt::NoModifier)
            {
                qApp->sendEvent(this, _event);
                return true;
            }
        }
        return QWidget::eventFilter(_watched, _event);
    }

    void VideoWindow::hoverEnter(QHoverEvent* _event)
    {
        if (d->isDialogActive_)
            return;

        d->controls_->fadeIn();
    }

    void VideoWindow::hoverMove(QHoverEvent* _event)
    {
        if (d->isDialogActive_)
            return;

        d->controls_->fadeIn();
        d->controls_->delayFadeOut();
    }

    void VideoWindow::hoverLeave(QHoverEvent* _event)
    {
        if (d->isDialogActive_)
            return;

        d->controls_->delayFadeOut();
    }

    void VideoWindow::windowStateChanged(QWindowStateChangeEvent* _event)
    {
        d->storedWState_ = _event->oldState();
        const bool fullscreen = windowState() & Qt::WindowFullScreen;
        if constexpr (platform::is_linux() || platform::is_windows())
        {
            d->header_->setVisible(!fullscreen);
            d->rootLayout_->setContentsMargins(fullscreen ? QMargins{} : kShadowMargins);
            d->windowController_->setEnabled(!fullscreen);
        }

        // This fixes Windows DWM bug related to OpenGL window in fullscreen
        // see https://doc.qt.io/qt-5/windows-issues.html and/or
        // https://doc.qt.io/qt-6/windows-issues.html for details
        // It's critical to use windowHandle() to set geometry since
        // otherwise we will trigger yet another window state change event
        if constexpr (platform::is_windows())
        {
            if (fullscreen)
                windowHandle()->setGeometry(screen()->geometry().adjusted(-1, -1, 1, 1));
        }

        Q_EMIT windowStatesChanged(windowState());
    }

    void VideoWindow::showEvent(QShowEvent* _event)
    {
#ifdef __APPLE__
        Utils::enableCloseButton(this, true);
#endif
        d->hideDetachedWindow();

        d->updateUserName();
        checkMicroPermission();

        Utils::InterConnector::instance().getMainWindow()->updateMainMenu();
        d->storedWState_ = windowState();
        QWidget::showEvent(_event);

        if constexpr (platform::is_apple())
            activateWindow();
    }

    void VideoWindow::hideEvent(QHideEvent* _event)
    {
        d->detachedWnd_->hideFrame();
        QWidget::hideEvent(_event);
    }

    void VideoWindow::resizeEvent(QResizeEvent* _event)
    {
        VideoWindowToastProvider::instance().hide();
        if (windowState() & Qt::WindowFullScreen)
            VideoWindowToastProvider::instance().show(VideoWindowToastProvider::Type::NotifyFullScreen, this, rect());

        if constexpr (platform::is_windows() || platform::is_linux())
        {
            if (windowState() & Qt::WindowFullScreen)
                return;

            QStyleOption opt;
            opt.initFrom(this);
            d->shadowPixmap_ = QPixmap(_event->size());
            d->shadowPixmap_.fill(Qt::transparent);
            Utils::drawRoundedShadow(&d->shadowPixmap_, d->shadowPixmap_.rect(), kShadowBorderWidth, opt.state);
        }
        QWidget::resizeEvent(_event);

    }

    void VideoWindow::paintEvent(QPaintEvent* _event)
    {
        if constexpr (platform::is_apple())
        {
            QWidget::paintEvent(_event);
            return;
        }

        if (windowState() & Qt::WindowFullScreen)
            return;

        QPainter painter(this);
        if (!d->shadowPixmap_.isNull())
            painter.drawPixmap(_event->rect(), d->shadowPixmap_, _event->rect());
    }

    void VideoWindow::closeEvent(QCloseEvent* _e)
    {
        _e->ignore();
        onHangupRequested();
    }

    void VideoWindow::keyPressEvent(QKeyEvent* _event)
    {
        const auto key = _event->key();
        const auto autorepeat = _event->isAutoRepeat();

        if (key != Qt::Key_Space || autorepeat || d->microphoneEnabled_)
        {
            QWidget::keyPressEvent(_event);
            return;
        }

        const bool audioPermissionGranted = GetDispatcher()->getVoipController().isAudPermissionGranted();
        const bool localAudioAllowed = GetDispatcher()->getVoipController().isLocalAudAllowed();
        if (!audioPermissionGranted)
        {
            QWidget::keyPressEvent(_event);
            return;
        }

        if (!localAudioAllowed)
        {
            VideoWindowToastProvider::instance().show(VideoWindowToastProvider::Type::MicNotAllowed, this, d->toastRect());
            return;
        }

        // temporarily enable microphone
        d->microphoneEnabledBySpace_ = true;
        grabKeyboard();
        if constexpr (platform::is_windows())
            d->checkSpacebarTimer_->start();
        GetDispatcher()->getVoipController().setSwitchACaptureMute();

        VideoWindowToastProvider::Options options = VideoWindowToastProvider::instance().defaultOptions();
        options.text_ = QT_TRANSLATE_NOOP("voip_pages", "Hold Space key for temporary unmute");
        VideoWindowToastProvider::instance().showToast(options, this, d->toastRect());
    }

    void VideoWindow::keyReleaseEvent(QKeyEvent* _event)
    {
        const auto key = _event->key();
        if (key == Qt::Key_Escape && (windowState() & Qt::WindowFullScreen))
        {
            setWindowState(Qt::WindowNoState | Qt::WindowActive);
            return;
        }

        if(key == Qt::Key_Space && !_event->isAutoRepeat() && d->microphoneEnabledBySpace_)
        {
            // disable temporarily enabled microphone
            d->microphoneEnabledBySpace_ = false;
            releaseKeyboard();
            GetDispatcher()->getVoipController().setSwitchACaptureMute();
            VideoWindowToastProvider::instance().hide();
            return;
        }

        QWidget::keyReleaseEvent(_event);
    }

    void VideoWindow::mouseDoubleClickEvent(QMouseEvent* _e)
    {
        QWidget::mouseDoubleClickEvent(_e);
        if (d->isCallVCS())
            return;

        d->rootWidget_->toggleRenderLayout(d->rootWidget_->getWidget()->mapFrom(this, _e->pos()));
        d->controls_->updateControls(d->rootWidget_->getViewMode());
    }

    void VideoWindow::contextMenuEvent(QContextMenuEvent*)
    {
        // If user press "menu" key on keyboard then QContextMenuEvent in
        // globalPos() method will return the center of widget, which
        // is inacceptable. Since we can't distinguish from key presses and mouse
        // presses within the QContextMenuEvent we have to take a cursor position
        // to retrieve actual mouse position to show menu there
        const QPoint globalPos = QCursor::pos();
        const QPoint localPos = d->rootWidget_->getWidget()->mapFromGlobal(globalPos);
        if (QMenu* menu = d->rootWidget_->createContextMenu(localPos))
            menu->exec(globalPos);
    }

    bool VideoWindow::nativeEvent(const QByteArray& _data, void* _message, long* _result)
    {
#ifdef _WIN32
        // Following code decrease flickering on resize on Windows OS
        MSG* msg = static_cast<MSG*>(_message);
        switch (msg->message)
        {
        case WM_SYSCOMMAND:
            if (msg->wParam == SC_CLOSE && d->isCloseConfirmation_)
            {
                msg->lParam = 0;
                return true;
            }
            break;
        case WM_WINDOWPOSCHANGING:
            {
                WINDOWPOS* wpos = (WINDOWPOS*)msg->lParam;
                wpos->flags |= SWP_NOCOPYBITS;
                wpos->flags |= SWP_NOREDRAW;
            }
            break;
        case WM_SIZING:
        case WM_SIZE:
            ::SetWindowPos(msg->hwnd, 0, 0, 0, LOWORD(msg->lParam), HIWORD(msg->lParam), SWP_NOREPOSITION | SWP_NOMOVE | SWP_NOREDRAW | SWP_NOZORDER | SWP_NOACTIVATE | SWP_NOCOPYBITS);
            break;
        }
#endif
#ifdef __linux__
        if (_data != "xcb_generic_event_t")
            return false;

        if (platform_linux::isNetWmDeleteWindowEvent(_message) && d->isCloseConfirmation_)
            return true;
#endif
        return QWidget::nativeEvent(_data, _message, _result);
    }
}
