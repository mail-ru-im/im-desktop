#include "stdafx.h"
#include "IncomingCallWindow.h"

#include "DetachedVideoWnd.h"
#include "VideoWindow.h"
#include "VoipTools.h"
#include "../core_dispatcher.h"
#include "../cache/avatars/AvatarStorage.h"
#include "../main_window/friendly/FriendlyContainer.h"
#include "../utils/utils.h"
#include "../../core/Voip/VoipManagerDefines.h"
#include "VoipSysPanelHeader.h"

namespace
{
    enum
    {
#ifdef __linux__
        kIncomingCallWndDefH = 400,
#else
        kIncomingCallWndDefH = 300,
#endif
        kIncomingCallWndDefW = 400,
    };

    // default offset for next incoming window.
    const float defaultWindowOffset = 0.25f;

#ifdef _DEBUG
    const int ht[kIncomingCallWndDefH > 0 ? 1 : -1] = { 0 }; // kIncomingCallWndDefH must be not null
    const int wt[kIncomingCallWndDefW > 0 ? 1 : -1] = { 0 }; // kIncomingCallWndDefW must be not null
#endif
}

QList<Ui::IncomingCallWindow*> Ui::IncomingCallWindow::instances_;

Ui::IncomingCallWindow::IncomingCallWindow(const std::string& _account, const std::string& _contact)
    : QWidget(nullptr, Qt::Window)
    , contact_(_contact)
    , account_(_account)
    , header_(new(std::nothrow) voipTools::BoundBox<VoipSysPanelHeader>(this, Utils::scale_value(kIncomingCallWndDefW)))
    , controls_(new voipTools::BoundBox<IncomingCallControls>(this))
    , transparentPanelOutgoingWidget_(nullptr)
    , shadow_(this)
{
#ifndef __linux__
    setStyleSheet(Utils::LoadStyle(qsl(":/qss/incoming_call")));
#else
    setStyleSheet(Utils::LoadStyle(qsl(":/qss/incoming_call_linux")));
#endif

    QIcon icon(build::GetProductVariant(qsl(":/logo/ico_icq"), qsl(":/logo/ico_agent"), qsl(":/logo/ico_biz"), qsl(":/logo/ico_dit")));
    setWindowIcon(icon);

#ifdef _WIN32
    setWindowFlags(Qt::WindowStaysOnTopHint | Qt::FramelessWindowHint | Qt::Window);
    setAttribute(Qt::WA_ShowWithoutActivating);
#else
    // We have a problem on Mac with WA_ShowWithoutActivating and WindowDoesNotAcceptFocus.
    // If video window is showing with these flags, we can not activate ICQ mainwindow
    // We use Qt::Dialog here, because it works correctly, if main window is in fullscreen mode
    setWindowFlags(Qt::WindowStaysOnTopHint | Qt::Dialog);
    //setAttribute(Qt::WA_ShowWithoutActivating);
    setAttribute(Qt::WA_X11DoNotAcceptFocus);
#endif

#ifndef __linux__
    setAttribute(Qt::WA_UpdatesDisabled);
#endif

    QVBoxLayout* rootLayout = Utils::emptyVLayout();
    rootLayout->setAlignment(Qt::AlignVCenter);
    setLayout(rootLayout);

    header_->setWindowFlags(header_->windowFlags() | Qt::WindowStaysOnTopHint);
    controls_->setWindowFlags(controls_->windowFlags() | Qt::WindowStaysOnTopHint);

    std::vector<QPointer<Ui::BaseVideoPanel>> panels;
    panels.push_back(header_.get());
    panels.push_back(controls_.get());

#ifdef _WIN32
    // Use it for Windows only, because macos Video Widnow resends mouse events to transparent panels.
    transparentPanelOutgoingWidget_ = std::unique_ptr<TransparentPanel>(new TransparentPanel(this, header_.get()));
    panels.push_back(transparentPanelOutgoingWidget_.get());
#endif

#ifdef __linux__
    rootLayout->addWidget(header_.get());
#endif

#ifndef STRIP_VOIP
    rootWidget_ = platform_specific::GraphicsPanel::create(this, panels, false, false);
    rootWidget_->setContentsMargins(0, 0, 0, 0);
    rootWidget_->setAttribute(Qt::WA_UpdatesDisabled);
    rootWidget_->setSizePolicy(QSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding));
#ifdef __linux__
    rootLayout->addWidget(rootWidget_, 1);
#else
    layout()->addWidget(rootWidget_);
#endif

#endif //__linux__

#ifdef __linux__
    rootLayout->addWidget(controls_.get());
#endif

    std::vector<QPointer<BaseVideoPanel>> videoPanels;
    videoPanels.push_back(header_.get());
    videoPanels.push_back(controls_.get());
    videoPanels.push_back(transparentPanelOutgoingWidget_.get());

    eventFilter_ = new ResizeEventFilter(videoPanels, shadow_.getShadowWidget(), this);
    installEventFilter(eventFilter_);

    connect(controls_.get(), &IncomingCallControls::onDecline, this, &Ui::IncomingCallWindow::onDeclineButtonClicked, Qt::QueuedConnection);
    connect(controls_.get(), &IncomingCallControls::onVideo, this, &Ui::IncomingCallWindow::onAcceptVideoClicked, Qt::QueuedConnection);
    connect(controls_.get(), &IncomingCallControls::onAudio, this, &Ui::IncomingCallWindow::onAcceptAudioClicked, Qt::QueuedConnection);

    connect(&Ui::GetDispatcher()->getVoipController(), &voip_proxy::VoipController::onVoipWindowRemoveComplete, this, &Ui::IncomingCallWindow::onVoipWindowRemoveComplete);
    connect(&Ui::GetDispatcher()->getVoipController(), &voip_proxy::VoipController::onVoipWindowAddComplete, this, &Ui::IncomingCallWindow::onVoipWindowAddComplete);
    connect(&Ui::GetDispatcher()->getVoipController(), &voip_proxy::VoipController::onVoipCallNameChanged, this, &Ui::IncomingCallWindow::onVoipCallNameChanged);
    //connect(&Ui::GetDispatcher()->getVoipController(), SIGNAL(onVoipMediaLocalVideo(bool)), header_.get(), SLOT(setVideoStatus(bool)), Qt::DirectConnection);
    //connect(&Ui::GetDispatcher()->getVoipController(), SIGNAL(onVoipMediaLocalVideo(bool)), controls_.get(), SLOT(setVideoStatus(bool)), Qt::DirectConnection);

    const QSize defaultSize(Utils::scale_value(kIncomingCallWndDefW), Utils::scale_value(kIncomingCallWndDefH));
    setMinimumSize(defaultSize);
    setMaximumSize(defaultSize);
    resize(defaultSize);

    QDesktopWidget dw;
    const auto screenRect = dw.screenGeometry(dw.primaryScreen());
    const auto wndSize = defaultSize;
    const auto center  = screenRect.center();

    QPoint windowPosition = QPoint(center.x() - wndSize.width() * 0.5f, center.y() - wndSize.height() * 0.5f);
    windowPosition = findBestPosition(windowPosition, QPoint(wndSize.width() * defaultWindowOffset, wndSize.height() * defaultWindowOffset));

    const QRect rc(windowPosition, wndSize);
    setGeometry(rc);

    // I hope constructor and destructor are called from the same thread.
    instances_.push_back(this);

    // Round rect.
    //QPainterPath path(QPointF(0, 0));
    //path.addRoundRect(rect(), Utils::scale_value(8));
    //QRegion region(path.toFillPolygon().toPolygon());
    //setMask(region);
}

Ui::IncomingCallWindow::~IncomingCallWindow()
{
#ifndef STRIP_VOIP
    delete rootWidget_;
#endif
    removeEventFilter(eventFilter_);
    delete eventFilter_;

    // I hope constructor and destructor are called from the same thread.
    instances_.removeAll(this);
}

void Ui::IncomingCallWindow::onVoipWindowRemoveComplete(quintptr _winId)
{
#ifndef STRIP_VOIP
    if (_winId == rootWidget_->frameId())
    {
        hide();
    }
#endif
}

void Ui::IncomingCallWindow::onVoipWindowAddComplete(quintptr _winId)
{
#ifndef STRIP_VOIP
    if (_winId == rootWidget_->frameId())
    {
        showNormal();
    }
#endif
}

void Ui::IncomingCallWindow::showFrame()
{
#ifndef STRIP_VOIP
    assert(rootWidget_->frameId());
    if (rootWidget_->frameId())
    {
        Ui::GetDispatcher()->getVoipController().setWindowAdd((quintptr)rootWidget_->frameId(), false, true, 0);
    }
#endif
}

void Ui::IncomingCallWindow::hideFrame()
{
#ifndef STRIP_VOIP
    assert(rootWidget_->frameId());
    if (rootWidget_->frameId())
    {
        Ui::GetDispatcher()->getVoipController().setWindowRemove((quintptr)rootWidget_->frameId());
    }
#endif
    hide();
}

void Ui::IncomingCallWindow::showEvent(QShowEvent* _e)
{
    updateTitle();

    header_->show();
    controls_->show();
    if (transparentPanelOutgoingWidget_)
    {
        transparentPanelOutgoingWidget_->show();
    }

    //shadow_.showShadow();

    QWidget::showEvent(_e);
}

void Ui::IncomingCallWindow::hideEvent(QHideEvent* _e)
{
    header_->hide();
    controls_->hide();
    if (transparentPanelOutgoingWidget_)
    {
        transparentPanelOutgoingWidget_->hide();
    }

    //shadow_.hideShadow();

    QWidget::hideEvent(_e);
}

void Ui::IncomingCallWindow::changeEvent(QEvent* _e)
{
    QWidget::changeEvent(_e);
}

void Ui::IncomingCallWindow::onVoipCallNameChanged(const voip_manager::ContactsList& _contacts)
{
#ifndef STRIP_VOIP
    if(_contacts.contacts.empty()) {
        return;
    }

    auto res = std::find(_contacts.windows.begin(), _contacts.windows.end(), (void*)rootWidget_->frameId());

    if (res != _contacts.windows.end())
    {
        contacts_ = _contacts.contacts;
        std::vector<std::string> users;
        users.reserve(contacts_.size());
        for (const auto& c : contacts_)
            users.push_back(c.contact);

        updateTitle();

        header_->setStatus(QT_TRANSLATE_NOOP("voip_pages", "Incoming call").toUtf8().constData());
    }
#endif //STRIP_VOIP
}

void Ui::IncomingCallWindow::onAcceptVideoClicked()
{
    assert(!contact_.empty());
    if (!contact_.empty())
    {
        Ui::GetDispatcher()->getVoipController().setAcceptV(contact_.c_str());
    }
    hide();
}

void Ui::IncomingCallWindow::onAcceptAudioClicked()
{
    assert(!contact_.empty());
    if (!contact_.empty())
    {
        Ui::GetDispatcher()->getVoipController().setAcceptA(contact_.c_str());
    }
    hide();
}


void Ui::IncomingCallWindow::onDeclineButtonClicked()
{
    assert(!contact_.empty());
    if (!contact_.empty())
    {
        Ui::GetDispatcher()->getVoipController().setDecline(contact_.c_str(), false);
    }
    hide();
}


void Ui::IncomingCallWindow::updateTitle()
{
    if(contacts_.empty())
    {
        return;
    }

    auto res = std::find(contacts_.begin(), contacts_.end(), voip_manager::Contact(account_, contact_));

    if (res != contacts_.end())
    {
        std::vector<std::string> friendlyNames;
        friendlyNames.reserve(contacts_.size());
        for(unsigned ix = 0; ix < contacts_.size(); ix++)
            friendlyNames.push_back(Logic::GetFriendlyContainer()->getFriendly(QString::fromStdString(contacts_[ix].contact)).toStdString());

        auto name = voip_proxy::VoipController::formatCallName(friendlyNames, QT_TRANSLATE_NOOP("voip_pages", "and").toUtf8().constData());
        assert(!name.empty());

        header_->setTitle(name.c_str());
    }
}

QPoint Ui::IncomingCallWindow::findBestPosition(const QPoint& _windowPosition, const QPoint& _offset)
{
    QPoint res = _windowPosition;

    QList<QPoint> avaliblePositions;

    // Create list with all avalible positions.
    for (int i = 0; i < instances_.size() + 1; i ++)
    {
        avaliblePositions.push_back(_windowPosition + _offset * i);
    }

    // Search position for next window.
    // For several incomming window we place them into cascade.
    for (IncomingCallWindow* window : instances_)
    {
        if (window)
        {
            avaliblePositions.removeAll(window->pos());
        }
    }

    if (!avaliblePositions.empty())
    {
        res = avaliblePositions.first();
    }

    return res;
}

#ifdef _WIN32

void Ui::IncomingCallWindow::mousePressEvent(QMouseEvent* _e)
{
    posDragBegin_ = _e->pos();
}

void Ui::IncomingCallWindow::mouseMoveEvent(QMouseEvent* _e)
{
    if (_e->buttons() & Qt::LeftButton)
    {
        QPoint diff = _e->pos() - posDragBegin_;
        QPoint newpos = this->pos() + diff;

        this->move(newpos);
    }
}

#endif

#ifndef _WIN32

// Resend messages to header to fix drag&drop for transparent panels.
void Ui::IncomingCallWindow::mouseMoveEvent(QMouseEvent* event)
{
    resendMouseEventToPanel(event);
}

void Ui::IncomingCallWindow::mouseReleaseEvent(QMouseEvent * event)
{
    resendMouseEventToPanel(event);
}

void Ui::IncomingCallWindow::mousePressEvent(QMouseEvent * event)
{
    resendMouseEventToPanel(event);
}

template <typename E> void Ui::IncomingCallWindow::resendMouseEventToPanel(E* event_)
{
    if (header_->isVisible() && (header_->rect().contains(event_->pos()) || header_->isGrabMouse()))
    {
        QApplication::sendEvent(header_.get(), event_);
    }
}

#endif
