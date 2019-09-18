#include "stdafx.h"
#include "VideoPanel.h"

#include "DetachedVideoWnd.h"
#include "VoipTools.h"
#include "MaskPanel.h"
#include "../core_dispatcher.h"
#include "../main_window/MainPage.h"
#include "../main_window/MainWindow.h"
#include "../main_window/contact_list/ContactListModel.h"
#include "../utils/utils.h"
#include "../utils/gui_metrics.h"
#include "SelectionContactsForConference.h"
#include "../main_window/contact_list/ContactList.h"
#include "../main_window/contact_list/ChatMembersModel.h"
#include "../controls/ContextMenu.h"

#define internal_spacer_w  (Utils::scale_value(16))
#define internal_spacer_w4 (Utils::scale_value(16))
#define internal_spacer_w_small (Utils::scale_value(12))

#define DISPLAY_ADD_CHAT_BUTTON 0

enum { kFitSpacerWidth = 560, kNormalModeMinWidth = 560};

Ui::VideoPanel::VideoPanel(
    QWidget* _parent, QWidget* _container)
#ifdef __APPLE__
    : BaseBottomVideoPanel(_parent, Qt::Window | Qt::FramelessWindowHint | Qt::WindowDoesNotAcceptFocus | Qt::NoDropShadowWindowHint)
#elif defined(__linux__)
    : BaseBottomVideoPanel(_parent, Qt::Widget)
#else
    : BaseBottomVideoPanel(_parent)
#endif
    , mouseUnderPanel_(false)
    , container_(_container)
    , parent_(_parent)
    , rootWidget_(nullptr)
    , addChatButton_(nullptr)
    , fullScreenButton_(nullptr)
    , stopCallButton_(nullptr)
    , videoButton_(nullptr)
    , isTakling(false)
    , isFadedVisible(false)
    , localVideoEnabled_(false)
    , isScreenSharingEnabled_(false)
    , isCameraEnabled_(true)
{
#ifndef __linux__
    setStyleSheet(Utils::LoadStyle(qsl(":/qss/video_panel")));
    setProperty("VideoPanelMain", true);
    setAttribute(Qt::WA_NoSystemBackground, true);
    setAttribute(Qt::WA_TranslucentBackground, true);
#else
    setStyleSheet(Utils::LoadStyle(qsl(":/qss/video_panel_linux")));
    setProperty("VideoPanelMain", true);
#endif


    rootWidget_ = new QWidget(this);
    rootWidget_->setContentsMargins(0, 0, 0, 0);
    rootWidget_->setProperty("VideoPanel", true);

    QVBoxLayout* mainLayout   = Utils::emptyVLayout();
    setLayout(mainLayout);

    QHBoxLayout* layoutTarget = Utils::emptyHLayout();
    layoutTarget->setAlignment(Qt::AlignTop);
    mainLayout->addWidget(rootWidget_);
    rootWidget_->setLayout(layoutTarget);

    QWidget* parentWidget = rootWidget_;

    auto addButton = [this, layoutTarget](QPushButton* btn, const char* _propertyName, const char* _slot, QHBoxLayout* layout = nullptr)
    {
        if (_propertyName != NULL)
        {
            btn->setProperty(_propertyName, true);
        }
        btn->setSizePolicy(QSizePolicy(QSizePolicy::Preferred, QSizePolicy::Expanding));
        btn->setCursor(QCursor(Qt::PointingHandCursor));
        btn->setFlat(true);
        (layout ? layout : layoutTarget)->addWidget(btn);

#ifdef __APPLE__
        connect(btn, &QPushButton::clicked, this, &VideoPanel::activateWindow, Qt::QueuedConnection);
#endif
        connect(btn, SIGNAL(clicked()), this, _slot, Qt::QueuedConnection);
    };
    auto addAndCreateButton = [parentWidget, addButton] (const char* _propertyName, const char* _slot, QHBoxLayout* layout = nullptr)->QPushButton*
    {
        QPushButton* btn = new voipTools::BoundBox<QPushButton>(parentWidget);
        addButton(btn, _propertyName, _slot, layout);
        return btn;
    };


    layoutTarget->addSpacing(internal_spacer_w_small);

    /**
     * We have this layout:
     * | Left Layout | Center Controls | Right Layout|
     */
    QHBoxLayout* leftLayout  = Utils::emptyHLayout();
    QHBoxLayout* rightLayout = Utils::emptyHLayout();

    goToChat_ = addAndCreateButton("CallGoChat", SLOT(onClickGoChat()), leftLayout);
    goToChat_->setToolTip(QT_TRANSLATE_NOOP("tooltips", "Open chat page"));
    leftLayout->addStretch(1);
    layoutTarget->addLayout(leftLayout, 1);

    openMasks_ = new MaskWidget(nullptr);
    addButton(openMasks_, "OpenMasks", SIGNAL(onShowMaskPanel()));
    openMasks_->setMaskEngineReady(true);

    layoutTarget->addSpacing(internal_spacer_w);

    microfone_ = addAndCreateButton("CallEnableMic", SLOT(onCaptureAudioOnOffClicked()));

    layoutTarget->addSpacing(internal_spacer_w);

    stopCallButton_ = addAndCreateButton("StopCallButton", SLOT(onHangUpButtonClicked()));
    stopCallButton_->setToolTip(QT_TRANSLATE_NOOP("tooltips", "End call"));
    stopCallButton_->setProperty("BigButton", true);
    layoutTarget->addSpacing(internal_spacer_w);

    shareScreenButton_ = addAndCreateButton("ShareScreenButtonDisable", SLOT(onShareScreen()));
    shareScreenButton_->setProperty("BigButton", true);
#ifdef __linux__
    shareScreenButton_->hide();
#else
    layoutTarget->addSpacing(internal_spacer_w);
#endif

    videoButton_ = addAndCreateButton(NULL, SLOT(onVideoOnOffClicked()));
    videoButton_->setProperty("BigButton", true);
    //layoutTarget->addSpacing(internal_spacer_w);

    //rightSpacer_ = new QSpacerItem(1, 1, QSizePolicy::MinimumExpanding);
    //layoutTarget->addSpacerItem(rightSpacer_);

    rightLayout->addStretch(1);

    rightLayout->addSpacing(internal_spacer_w);
    fullScreenButton_ = addAndCreateButton("CallFSOff", SLOT(_onFullscreenClicked()), rightLayout);

    layoutTarget->addLayout(rightLayout, 1);

    layoutTarget->addSpacing(internal_spacer_w_small);

    resetHangupText();

    QObject::connect(&Ui::GetDispatcher()->getVoipController(), SIGNAL(onVoipMediaLocalVideo(bool)), this, SLOT(onVoipMediaLocalVideo(bool)), Qt::DirectConnection);
    //QObject::connect(&Ui::GetDispatcher()->getVoipController(), SIGNAL(onVoipCallNameChanged(const voip_manager::ContactsList&)), this, SLOT(onVoipCallNameChanged(const voip_manager::ContactsList&)), Qt::DirectConnection);
    //QObject::connect(&Ui::GetDispatcher()->getVoipController(), SIGNAL(onVoipMinimalBandwidthChanged(bool)), this, SLOT(onVoipMinimalBandwidthChanged(bool)), Qt::DirectConnection);
    QObject::connect(&Ui::GetDispatcher()->getVoipController(), SIGNAL(onVoipVideoDeviceSelected(const voip_proxy::device_desc&)), this, SLOT(onVoipVideoDeviceSelected(const voip_proxy::device_desc&)), Qt::DirectConnection);
    QObject::connect(&Ui::GetDispatcher()->getVoipController(), SIGNAL(onVoipMediaLocalAudio(bool)), this, SLOT(onVoipMediaLocalAudio(bool)), Qt::DirectConnection);

    //rootEffect_ = std::make_unique<UIEffects>(*rootWidget_, false, true);
}

Ui::VideoPanel::~VideoPanel()
{

}

void Ui::VideoPanel::keyReleaseEvent(QKeyEvent* _e)
{
    QWidget::keyReleaseEvent(_e);
    if (_e->key() == Qt::Key_Escape)
    {
        emit onkeyEscPressed();
    }
}

void Ui::VideoPanel::controlActivated(bool _activated)
{
    mouseUnderPanel_ = _activated;

    if (_activated)
    {
        emit onMouseEnter();
    }
    else
    {
        emit onMouseLeave();
    }
}

void Ui::VideoPanel::onClickGoChat()
{
    if (!activeContact_.empty())
    {
        Ui::GetDispatcher()->getVoipController().openChat(QString::fromStdString(activeContact_[0].contact));

        emit onGoToChatButton();
    }
}

void Ui::VideoPanel::setContacts(const std::vector<voip_manager::Contact>& contacts)
{
    activeContact_ = contacts;
    // Update button visibility.
    goToChat_->setVisible(contacts.size() == 1);
}

//void Ui::VideoPanel::onClickAddChat()
//{
//    assert(false);
//}

void Ui::VideoPanel::setFullscreenMode(bool _en)
{
    if (!fullScreenButton_)
    {
        return;
    }

    fullScreenButton_->setProperty("CallFSOff", _en);
    fullScreenButton_->setProperty("CallFSOn", !_en);
    fullScreenButton_->setToolTip(_en ? QT_TRANSLATE_NOOP("tooltips", "Exit full screen") : QT_TRANSLATE_NOOP("tooltips", "Full screen"));

    fullScreenButton_->setStyle(QApplication::style());
}

void Ui::VideoPanel::_onFullscreenClicked()
{
    emit onFullscreenClicked();
}

void Ui::VideoPanel::enterEvent(QEvent* _e)
{
    QWidget::enterEvent(_e);
    if (!mouseUnderPanel_)
    {
        emit onMouseEnter();
    }
}

void Ui::VideoPanel::leaveEvent(QEvent* _e)
{
    QWidget::leaveEvent(_e);

    mouseUnderPanel_ = false;
    emit onMouseLeave();
}

void Ui::VideoPanel::resizeEvent(QResizeEvent* _e)
{
    QWidget::resizeEvent(_e);

#ifdef __APPLE__
    // Forced set fixed size, because under mac we use cocoa to change panel size.
    setFixedSize(size());
#endif

#ifdef __APPLE__
    assert(parent_);
    if (parent_ && !parent_->isFullScreen())
    {
        auto rc = rect();
        QPainterPath path(QPointF(0, 0));
        path.addRoundedRect(rc.x(), rc.y(), rc.width(), rc.height(), Utils::scale_value(5), Utils::scale_value(5));

        QRegion region(path.toFillPolygon().toPolygon());
        region = region + QRect(0, 0, rc.width(), Utils::scale_value(5));

        setMask(region);
    }
    else
    {
        clearMask();
    }
#endif
}

void Ui::VideoPanel::resetHangupText()
{
    if (stopCallButton_)
    {
        stopCallButton_->setText(QString());
        stopCallButton_->repaint();
    }
}


void Ui::VideoPanel::onHangUpButtonClicked()
{
    Ui::GetDispatcher()->getVoipController().setHangup();
}

void Ui::VideoPanel::onShareScreen()
{
    const QList<voip_proxy::device_desc>& screens = Ui::GetDispatcher()->getVoipController().screenList();
    int screenIndex = 0;

    if (!isScreenSharingEnabled_ && screens.size() > 1)
    {
        ContextMenu menu(this);
        ContextMenu::applyStyle(&menu, false, Utils::scale_value(15), Utils::scale_value(36));
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

    if (isScreenSharingEnabled_)
    {
        emit onShareScreenClickOn();
    }
}

void Ui::VideoPanel::onVoipMediaLocalVideo(bool _enabled)
{
    if (!videoButton_)
    {
        return;
    }

    localVideoEnabled_ = _enabled;

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
    if (!(isCameraEnabled_ && localVideoEnabled_))
    {
        emit onCameraClickOn();
    }

    Ui::GetDispatcher()->getVoipController().setSwitchVCaptureMute();
}

void Ui::VideoPanel::fadeIn(unsigned int _kAnimationDefDuration)
{
    if (!isFadedVisible)
    {
        isFadedVisible = true;
#ifndef __linux__
        BaseVideoPanel::fadeIn(_kAnimationDefDuration);
        //rootEffect_->geometryTo(QRect(0, 0, width(), height()), _kAnimationDefDuration);
#endif
    }
}

void Ui::VideoPanel::fadeOut(unsigned int _kAnimationDefDuration)
{
    if (isFadedVisible)
    {
        isFadedVisible = false;
#ifndef __linux__
        BaseVideoPanel::fadeOut(_kAnimationDefDuration);
        //rootEffect_->geometryTo(QRect(0, height(), width(), 1), _kAnimationDefDuration);
#endif
    }
}

void Ui::VideoPanel::forceFinishFade()
{
    BaseVideoPanel::forceFinishFade();

    //if (rootEffect_)
    //{
    //    rootEffect_->forceFinish();
    //    rootWidget_->update();
    //    rootWidget_->setFixedWidth(width());
    //    rootWidget_->setFixedWidth(QWIDGETSIZE_MAX);
    //}
}

bool Ui::VideoPanel::isFadedIn()
{
    return isFadedVisible;
}

bool Ui::VideoPanel::isActiveWindow()
{
    return QWidget::isActiveWindow();
}

bool Ui::VideoPanel::isNormalPanelMode()
{
    auto rc = rect();

    return (rc.width() >= Utils::scale_value(kNormalModeMinWidth));
}

bool Ui::VideoPanel::isFitSpacersPanelMode()
{
    auto rc = rect();
    return (rc.width() < Utils::scale_value(kFitSpacerWidth));
}

void Ui::VideoPanel::activateVideoWindow()
{
    parentWidget()->activateWindow();
    parentWidget()->raise();
}

int Ui::VideoPanel::heightOfCommonPanel()
{
    return rootWidget_->height();
}

void Ui::VideoPanel::onVoipVideoDeviceSelected(const voip_proxy::device_desc& desc)
{
    isScreenSharingEnabled_ = (desc.dev_type == voip_proxy::kvoipDevTypeVideoCapture && desc.video_dev_type == voip_proxy::kvoipDeviceDesktop);
    isCameraEnabled_        = (desc.dev_type == voip_proxy::kvoipDevTypeVideoCapture && desc.video_dev_type == voip_proxy::kvoipDeviceCamera);

    updateVideoDeviceButtonsState();
}

void Ui::VideoPanel::updateVideoDeviceButtonsState()
{
    bool enableCameraButton = localVideoEnabled_ && isCameraEnabled_;
    bool enableScreenButton = localVideoEnabled_ && isScreenSharingEnabled_;

    videoButton_->setProperty("CallEnableCam", enableCameraButton);
    videoButton_->setProperty("CallDisableCam", !enableCameraButton);
    videoButton_->setToolTip(enableCameraButton ? QT_TRANSLATE_NOOP("tooltips", "Turn off camera") : QT_TRANSLATE_NOOP("tooltips", "Turn on camera"));

    shareScreenButton_->setProperty("ShareScreenButtonDisable", !enableScreenButton);
    shareScreenButton_->setProperty("ShareScreenButtonEnable",  enableScreenButton);
    shareScreenButton_->setToolTip(enableScreenButton ? QT_TRANSLATE_NOOP("tooltips", "Turn off screen sharing") : QT_TRANSLATE_NOOP("tooltips", "Turn on screen sharing"));

    videoButton_->setStyle(QApplication::style());
    shareScreenButton_->setStyle(QApplication::style());
}

void Ui::VideoPanel::onCaptureAudioOnOffClicked()
{
    Ui::GetDispatcher()->getVoipController().setSwitchACaptureMute();

    emit onMicrophoneClick();
}

void Ui::VideoPanel::onVoipMediaLocalAudio(bool _enabled)
{
    if (microfone_)
    {
        microfone_->setProperty("CallEnableMic", _enabled);
        microfone_->setProperty("CallDisableMic", !_enabled);
        microfone_->setToolTip(_enabled ? QT_TRANSLATE_NOOP("tooltips", "Turn off microphone") : QT_TRANSLATE_NOOP("tooltips", "Turn on microphone"));
        microfone_->setStyle(QApplication::style());
    }
}


void Ui::VideoPanel::setSelectedMask(MaskWidget* maskWidget)
{
    if (maskWidget)
    {
        openMasks_->setPixmap(maskWidget->pixmap());
    }
}
