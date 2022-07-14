#include "stdafx.h"
#include "IncomingCallWindow.h"
#include "DetachedVideoWnd.h"
#include "VideoWindow.h"
#include "../core_dispatcher.h"
#include "../main_window/containers/FriendlyContainer.h"
#include "../utils/utils.h"
#include "../../core/Voip/VoipManagerDefines.h"
#include "src/CallStateInternal.h"
#include "VoipSysPanelHeader.h"
#include "../utils/InterConnector.h"

#include "WindowController.h"

enum
{
    kIncomingCallWndDefH = 300,
    kIncomingCallWndDefW = 400,
};
// default offset for next incoming window.
static const float defaultWindowOffset = 0.25f;

QList<Ui::IncomingCallWindow*> Ui::IncomingCallWindow::instances_;

void Ui::IncomingCallWindow::setFrameless()
{
    Qt::WindowFlags flags = windowFlags();
    flags |= Qt::FramelessWindowHint;
    flags &= ~(Qt::WindowTitleHint | Qt::WindowSystemMenuHint
               | Qt::WindowMinMaxButtonsHint | Qt::WindowCloseButtonHint);
    setWindowFlags(flags);
}

Ui::IncomingCallWindow::IncomingCallWindow(const std::string& call_id, const std::string& _contact, const std::string& call_type)
    : QWidget(nullptr, Qt::Window)
    , contact_(_contact)
    , call_id_(call_id)
    , call_type_(call_type)
    , transparentPanelOutgoingWidget_(nullptr)
    , shadow_(this)
    , incomingAlarmTimer_(new QTimer(this))
{
    setWindowFlags(Qt::WindowStaysOnTopHint | Qt::FramelessWindowHint | Qt::Window);
#ifdef _WIN32
    setAttribute(Qt::WA_ShowWithoutActivating);
#else
    setAttribute(Qt::WA_X11DoNotAcceptFocus);
#endif
    setAttribute(Qt::WA_NoSystemBackground, true);
    setAttribute(Qt::WA_TranslucentBackground, true);
    setFrameless();

    setStyleSheet(Utils::LoadStyle(qsl(":/qss/incoming_call")));

    QIcon icon(qsl(":/logo/ico"));
    setWindowIcon(icon);

    QVBoxLayout* rootLayout = Utils::emptyVLayout();
    rootLayout->setAlignment(Qt::AlignVCenter);
    setLayout(rootLayout);

#ifndef STRIP_VOIP
    rootWidget_ = new OGLRenderWidget(this);
    rootWidget_->setIncoming(true);
    rootWidget_->getWidget()->setSizePolicy(QSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding));
    rootLayout->addWidget(rootWidget_->getWidget());
#endif

    const QSize defaultSize(Utils::scale_value(kIncomingCallWndDefW), Utils::scale_value(kIncomingCallWndDefH));
    QWidget* darkPlate = new QWidget(this);
    darkPlate->setStyleSheet(qsl("background-color: rgba(0, 0, 0, 24%);"));
    darkPlate->setFixedSize(defaultSize);

    // create panels after main widget, so it can be visible without raise()
    header_ = std::unique_ptr<VoipSysPanelHeader>(new(std::nothrow) VoipSysPanelHeader(this, Utils::scale_value(kIncomingCallWndDefW)));
    controls_ = std::unique_ptr<IncomingCallControls>(new IncomingCallControls(this));
    if constexpr (platform::is_windows())
    {
        // Use it for Windows only, because macos Video Widnow resends mouse events to transparent panels.
        transparentPanelOutgoingWidget_ = std::unique_ptr<TransparentPanel>(new TransparentPanel(this, header_.get()));
    }

    std::vector<QPointer<BaseVideoPanel>> videoPanels;
    videoPanels.push_back(header_.get());
    videoPanels.push_back(controls_.get());
    if constexpr (platform::is_windows())
        videoPanels.push_back(transparentPanelOutgoingWidget_.get());

    eventFilter_ = new ResizeEventFilter(videoPanels, shadow_.getShadowWidget(), this);
    installEventFilter(eventFilter_);

    connect(controls_.get(), &IncomingCallControls::onDecline, this, &Ui::IncomingCallWindow::onDeclineButtonClicked, Qt::QueuedConnection);
    connect(controls_.get(), &IncomingCallControls::onVideo, this, &Ui::IncomingCallWindow::onAcceptVideoClicked, Qt::QueuedConnection);
    connect(controls_.get(), &IncomingCallControls::onAudio, this, &Ui::IncomingCallWindow::onAcceptAudioClicked, Qt::QueuedConnection);

    connect(&Ui::GetDispatcher()->getVoipController(), &voip_proxy::VoipController::onVoipCallNameChanged, this, &Ui::IncomingCallWindow::onVoipCallNameChanged);

    setFixedSize(defaultSize);
    resize(defaultSize);

    const auto screenRect = Utils::mainWindowScreen()->geometry();
    const auto wndSize = defaultSize;
    const auto center  = screenRect.center();

    QPoint windowPosition = QPoint(center.x() - wndSize.width() * 0.5f, center.y() - wndSize.height() * 0.5f);
    windowPosition = findBestPosition(windowPosition, QPoint(wndSize.width() * defaultWindowOffset, wndSize.height() * defaultWindowOffset));

    const QRect rc(windowPosition, wndSize);
    setGeometry(rc);

    connect(incomingAlarmTimer_, &QTimer::timeout, this, []() {Q_EMIT Utils::InterConnector::instance().incomingCallAlert();});
    incomingAlarmTimer_->setInterval(500);
    incomingAlarmTimer_->start();

    WindowController* windowController = new WindowController(this);
    windowController->setOptions(WindowController::Dragging);
    windowController->setBorderWidth(0);
    windowController->setWidget(this, this);
}

Ui::IncomingCallWindow::~IncomingCallWindow()
{
    removeEventFilter(eventFilter_);
    delete eventFilter_;

    instances_.removeAll(this);
}

void Ui::IncomingCallWindow::showFrame()
{
#ifndef STRIP_VOIP
    instances_.push_back(this);
#endif
    showNormal();
}

void Ui::IncomingCallWindow::hideFrame()
{
    incomingAlarmTimer_->stop();
#ifndef STRIP_VOIP
    instances_.removeAll(this);
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
    QWidget::hideEvent(_e);
}

void Ui::IncomingCallWindow::changeEvent(QEvent* _e)
{
    QWidget::changeEvent(_e);
}

void Ui::IncomingCallWindow::onVoipCallNameChanged(const voip_manager::ContactsList& _contacts)
{
#ifndef STRIP_VOIP
    rootWidget_->updatePeers(_contacts);
    if (_contacts.contacts.empty())
        return;

    contacts_ = _contacts.contacts;
    std::vector<std::string> users;
    users.reserve(contacts_.size());
    for (const auto& c : contacts_)
        users.push_back(c.contact);
    vcs_conference_name_ = _contacts.conference_name;

    updateTitle();

    header_->setStatus(QT_TRANSLATE_NOOP("voip_pages", "Incoming call").toUtf8().constData());
#endif //STRIP_VOIP
}

void Ui::IncomingCallWindow::onAcceptVideoClicked()
{
    im_assert(!contact_.empty());
    hideFrame();
    if (!contact_.empty())
    {
        Ui::GetDispatcher()->getVoipController().setAcceptCall(call_id_.c_str(), true);
    }
    close();
}

void Ui::IncomingCallWindow::onAcceptAudioClicked()
{
    im_assert(!contact_.empty());
    hideFrame();
    if (!contact_.empty())
    {
        Ui::GetDispatcher()->getVoipController().setAcceptCall(call_id_.c_str(), false);
    }
    close();
}


void Ui::IncomingCallWindow::onDeclineButtonClicked()
{
    im_assert(!contact_.empty());
    hideFrame();
    if (!contact_.empty())
    {
        Ui::GetDispatcher()->getVoipController().setDecline(call_id_.c_str(), contact_.c_str(), false);
    }
    close();
}

void Ui::IncomingCallWindow::updateTitle()
{
    if (voip::kCallType_VCS == call_type_)
    {
        header_->setTitle(vcs_conference_name_.empty() ? QT_TRANSLATE_NOOP("voip_pages", "VCS call").toUtf8().constData() : vcs_conference_name_.c_str());
        return;
    }
    if (contacts_.empty())
        return;

    auto res = std::find(contacts_.begin(), contacts_.end(), voip_manager::Contact(call_id_, contact_));
    if (res != contacts_.end())
    {
        std::vector<std::string> friendlyNames;
        friendlyNames.reserve(contacts_.size());
        for(unsigned ix = 0; ix < contacts_.size(); ix++)
            friendlyNames.push_back(Logic::GetFriendlyContainer()->getFriendly(QString::fromStdString(contacts_[ix].contact)).toStdString());

        auto name = voip_proxy::VoipController::formatCallName(friendlyNames, QT_TRANSLATE_NOOP("voip_pages", "and").toUtf8().constData());
        im_assert(!name.empty());

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
        if (window && !window->isHidden())
            avaliblePositions.removeAll(window->pos());
    }
    if (!avaliblePositions.empty())
        res = avaliblePositions.first();
    return res;
}

