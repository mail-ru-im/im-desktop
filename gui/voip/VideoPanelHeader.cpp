#include "stdafx.h"
#include "VideoPanelHeader.h"

#include "VoipTools.h"

#include "../utils/utils.h"
#include "../core_dispatcher.h"
#include "../main_window/MainPage.h"
#include "../main_window/MainWindow.h"

#define DEFAULT_BORDER Utils::scale_value(12)
#define DEFAULT_WINDOW_ROUND_RECT Utils::scale_value(5)

const QString secureCallButton =
    qsl(" QPushButton { font-size: 15dip; text-align: center; border-style: none; background-color: transparent; } ");

const QString secureCallButtonClicked =
    qsl(" QPushButton { font-size: 15dip; text-align: center; border-style: none; background-color: #ffffff; } ");

#define SECURE_BTN_BORDER_W    Utils::scale_value(24)
#define SECURE_BTN_ICON_W      Utils::scale_value(16)
#define SECURE_BTN_ICON_H      SECURE_BTN_ICON_W
#define SECURE_BTN_TEXT_W      Utils::scale_value(50)
#define SECURE_BTN_ICON2TEXT_W Utils::scale_value(12)
#define SECURE_BTN_W           (2 * SECURE_BTN_BORDER_W + SECURE_BTN_ICON_W + SECURE_BTN_TEXT_W + SECURE_BTN_ICON2TEXT_W)


std::string Ui::getFotmatedTime(unsigned _ts)
{
    int hours = _ts / (60 * 60);
    int minutes = (_ts / 60) % 60;
    int sec = _ts % 60;

    std::stringstream timeString;
    if (hours > 0)
    {
        timeString << std::setfill('0') << std::setw(2) << hours << ":";
    }
    timeString << std::setfill('0') << std::setw(2) << minutes << ":";
    timeString << std::setfill('0') << std::setw(2) << sec;

    return timeString.str();
}



Ui::VideoPanelHeader::VideoPanelHeader(QWidget* _parent, int offset)
    : BaseTopVideoPanel(_parent)
    , lowWidget_(NULL)
    , secureCallEnabled_(false)
    , isVideoConference_(false)
    , startTalking_(false)
    , topOffset_(offset)
{
#ifdef __linux__
    setStyleSheet(Utils::LoadStyle(qsl(":/qss/video_panel_linux")));
#else
    setStyleSheet(Utils::LoadStyle(qsl(":/qss/video_panel")));

    setAttribute(Qt::WA_NoSystemBackground, true);
    setAttribute(Qt::WA_TranslucentBackground, true);
#endif
    setProperty("VideoPanelHeader", true);
    setContentsMargins(0, 0, 0, 0);

    QHBoxLayout* layout = Utils::emptyHLayout();
    lowWidget_ = new QWidget(this);
    { // low widget. it makes background panel coloured
        lowWidget_->setContentsMargins(0, 0, 0, 0);
        lowWidget_->setProperty("VideoPanelHeader", true);

        lowWidget_->setLayout(layout);

        QVBoxLayout* vLayoutParent = Utils::emptyVLayout();
        vLayoutParent->setAlignment(Qt::AlignBottom);
        vLayoutParent->addWidget(lowWidget_);
        setLayout(vLayoutParent);

        layout->setAlignment(Qt::AlignBottom);

        //layout->addSpacing(DEFAULT_BORDER);
    }

    auto addWidget = [] (QWidget* _parent)
    {
        QWidget* w = new QWidget(_parent);
        w->setContentsMargins(0, 0, 0, 0);
        w->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Expanding);
        return w;
    };

    auto addLayout = [] (QWidget* _w, Qt::Alignment _align)
    {
        assert(_w);
        if (_w)
        {
            QHBoxLayout* layout = Utils::emptyHLayout();
            layout->setAlignment(_align);
            _w->setLayout(layout);
        }
    };

    QWidget* leftWidg    = addWidget(lowWidget_);
    QWidget* centerWidg  = addWidget(lowWidget_);
    QWidget* rightWidg   = addWidget(lowWidget_);

    layout->addWidget(leftWidg, 1);
    layout->addWidget(centerWidg);
    layout->addWidget(rightWidg, 1);

    addLayout(leftWidg,   Qt::AlignLeft   | Qt::AlignBottom);
    addLayout(centerWidg, Qt::AlignCenter | Qt::AlignBottom);
    addLayout(rightWidg,  Qt::AlignRight  | Qt::AlignBottom);

    qobject_cast<QHBoxLayout*>(leftWidg->layout())->addSpacing(Utils::scale_value(16));

    QFont font = QApplication::font();
    font.setStyleStrategy(QFont::PreferAntialias);

    auto addButton = [this](const QString& _propertyName, const char* _slot, QWidget* parentWidget)->QPushButton*
    {
        QPushButton* btn = new voipTools::BoundBox<QPushButton>(parentWidget);

        //Utils::ApplyStyle(btn, _propertyName);
        btn->setProperty(_propertyName.toLatin1().data(), true);
        btn->setSizePolicy(QSizePolicy(QSizePolicy::Preferred, QSizePolicy::Expanding));
#ifdef _WIN32
        btn->setCursor(QCursor(Qt::PointingHandCursor));
#endif
        btn->setFlat(true);
        parentWidget->layout()->addWidget(btn);
        connect(btn, SIGNAL(clicked()), this, _slot, Qt::QueuedConnection);
        return btn;
    };

    conferenceMode_ = addButton(qsl("ConferenceAllTheSame"), SLOT(onChangeConferenceMode()), leftWidg);
    security_       = addButton(qsl("SecurityButton"), SLOT(_onSecureCallClicked()), centerWidg);

    QHBoxLayout* rightLayout = qobject_cast<QHBoxLayout*>(rightWidg->layout());

    rightLayout->addSpacing(Utils::scale_value(20));
    speaker_   = addButton(qsl("CallSoundOn"), SLOT(onPlaybackAudioOnOffClicked()), rightWidg);
    rightLayout->addSpacing(Utils::scale_value(20));
    addUsers_ = addButton(qsl("AddUser"), SLOT(onAddUserClicked()), rightWidg);
    rightLayout->addSpacing(Utils::scale_value(20));
    addButton(qsl("CallSettings"), SLOT(onClickSettings()), rightWidg);
    rightLayout->addSpacing(Utils::scale_value(16));

    QObject::connect(&Ui::GetDispatcher()->getVoipController(), SIGNAL(onVoipVolumeChanged(const std::string&, int)), this, SLOT(onVoipVolumeChanged(const std::string&, int)), Qt::DirectConnection);

    updateSecurityButtonState();
    //rootEffect_ = std::make_unique<UIEffects>(*lowWidget_, false, true);
}

Ui::VideoPanelHeader::~VideoPanelHeader()
{

}

void Ui::VideoPanelHeader::enterEvent(QEvent* _e)
{
    QWidget::enterEvent(_e);
    emit onMouseEnter();
}

void Ui::VideoPanelHeader::leaveEvent(QEvent* _e)
{
    QWidget::leaveEvent(_e);
    emit onMouseLeave();
}

void Ui::VideoPanelHeader::enableSecureCall(bool _enable)
{
    secureCallEnabled_ = _enable;
    updateSecurityButtonState();
}

void Ui::VideoPanelHeader::_onSecureCallClicked()
{
#ifdef __linux__
    auto rc = QRect(mapToGlobal(pos()), size());
    emit onSecureCallClicked(rc);
#else
    emit onSecureCallClicked(geometry());
#endif
}


void Ui::VideoPanelHeader::setSecureWndOpened(const bool _opened)
{
}


void Ui::VideoPanelHeader::updatePosition(const QWidget& parent)
{
    // We have code dublication here,
    // because Mac and Qt have different coords systems.
    // We can convert Mac coords to Qt, but we need to add
    // special cases for multi monitor systems.
#ifdef __APPLE__
    QRect parentRect = platform_macos::getWidgetRect(*parentWidget());
    platform_macos::setWindowPosition(*this,
        QRect(parentRect.left(),
            parentRect.top() + parentRect.height() - height() - topOffset_,
            parentRect.width(),
            height()));
    //    auto rc = platform_macos::getWindowRect(*parentWidget());
#elif defined(__linux__)
    auto rc = parentWidget()->geometry();
    move(0, 0);
    setFixedWidth(rc.width());
#else
    auto rc = parentWidget()->geometry();
    move(rc.x(), rc.y() + topOffset_);
    setFixedWidth(rc.width());
#endif
}

void Ui::VideoPanelHeader::onChangeConferenceMode()
{
    bool isAllTheSame = conferenceMode_->property("ConferenceAllTheSame").toBool();
    isAllTheSame = !isAllTheSame;

    Utils::ApplyPropertyParameter(conferenceMode_, "ConferenceAllTheSame", isAllTheSame);
    Utils::ApplyPropertyParameter(conferenceMode_, "ConferenceOneIsBig", !isAllTheSame);

    emit updateConferenceMode(isAllTheSame ? voip_manager::AllTheSame : voip_manager::OneIsBig);
}

void Ui::VideoPanelHeader::changeConferenceMode(voip_manager::VideoLayout layout)
{
    Utils::ApplyPropertyParameter(conferenceMode_, "ConferenceAllTheSame", layout == voip_manager::AllTheSame);
    Utils::ApplyPropertyParameter(conferenceMode_, "ConferenceOneIsBig", layout == voip_manager::OneIsBig);
}

void Ui::VideoPanelHeader::onPlaybackAudioOnOffClicked()
{
    Ui::GetDispatcher()->getVoipController().setSwitchAPlaybackMute();

    emit onPlaybackClick();
}

void Ui::VideoPanelHeader::onClickSettings()
{
    if (MainPage* mainPage = MainPage::instance())
    {
        if (MainWindow* wnd = static_cast<MainWindow*>(mainPage->window()))
        {
            wnd->raise();
            wnd->activate();
        }
        mainPage->settingsTabActivate(Utils::CommonSettingsType::CommonSettingsType_VoiceVideo);
    }
}

void Ui::VideoPanelHeader::onVoipVolumeChanged(const std::string& _deviceType, int _vol)
{
    if (_deviceType == "audio_playback")
    {
        speaker_->setProperty("CallSoundOn",     _vol > 0);
        speaker_->setProperty("CallSoundOff",    _vol <= 0);
        speaker_->setStyle(QApplication::style());
    }
}

void Ui::VideoPanelHeader::fadeIn(unsigned int duration)
{
#ifndef __linux__
    BaseVideoPanel::fadeIn(duration);
    //rootEffect_->geometryTo(QRect(0, 0, width(), height()), duration);
#endif
}

void Ui::VideoPanelHeader::fadeOut(unsigned int duration)
{
#ifndef __linux__
    BaseVideoPanel::fadeOut(duration);
    //rootEffect_->geometryTo(QRect(0, -height(), width(), height()), duration);
#endif
}

void Ui::VideoPanelHeader::onAddUserClicked()
{
    emit addUserToConference();
}

void Ui::VideoPanelHeader::setContacts(const std::vector<voip_manager::Contact>& contacts)
{
    // Update button visibility.
    addUsers_->setVisible((int32_t)contacts.size() < Ui::GetDispatcher()->getVoipController().maxVideoConferenceMembers() - 1);
    isVideoConference_ = contacts.size() > 2;
    updateSecurityButtonState();
}

void Ui::VideoPanelHeader::setTopOffset(int offset)
{
    topOffset_ = offset;
}

void Ui::VideoPanelHeader::switchConferenceMode()
{
    onChangeConferenceMode();
}

void Ui::VideoPanelHeader::updateSecurityButtonState()
{
    security_->setVisible(!isVideoConference_ && secureCallEnabled_ && startTalking_);
}

void Ui::VideoPanelHeader::onCreateNewCall()
{
    isVideoConference_ = false;
    secureCallEnabled_ = false;
    startTalking_ = false;

    updateSecurityButtonState();
}

void Ui::VideoPanelHeader::onStartedTalk()
{
    startTalking_ = true;
    updateSecurityButtonState();
}