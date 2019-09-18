#include "stdafx.h"
#include "VoipSysPanelHeader.h"

#include "VoipTools.h"
#include "../utils/utils.h"
#include "../../installer/utils/styles.h"

#ifdef __APPLE__
#   include <QtWidgets/qtoolbutton.h>
#else
#   include <QToolButton>
#   include <QStyleOptionToolButton>
#endif // __APPLE__

#define VOIP_VIDEO_PANEL_BTN_FULL_OFFSET (Utils::scale_value(32))
#define VOIP_VIDEO_PANEL_BTN_SIZE        (Utils::scale_value(36))

Ui::IncomingCallControls::IncomingCallControls(QWidget* _parent)
    : BaseBottomVideoPanel(_parent)
    , parent_(_parent)
    , rootWidget_(nullptr)
{
#ifndef __linux__
    setStyleSheet(Utils::LoadStyle(qsl(":/qss/incoming_call")));
#else
    setStyleSheet(Utils::LoadStyle(qsl(":/qss/incoming_call_linux")));
#endif

    setObjectName(qsl("incomingCallControls"));

#ifndef __linux__
    setAttribute(Qt::WA_NoSystemBackground, true);
    setAttribute(Qt::WA_TranslucentBackground, true);
#endif

    QVBoxLayout* rootLayout = Utils::emptyVLayout();
    rootLayout->setAlignment(Qt::AlignVCenter);
    setLayout(rootLayout);

    rootWidget_ = new QWidget(this);
    rootWidget_->setContentsMargins(0, 0, 0, 0);
    rootWidget_->setObjectName(qsl("incomingCallControls"));
    layout()->addWidget(rootWidget_);

    QHBoxLayout* layoutTarget = Utils::emptyHLayout();
    layoutTarget->setAlignment(Qt::AlignTop);
    rootWidget_->setLayout(layoutTarget);

    QWidget* rootWidget = rootWidget_;
    const int btnMinWidth = Utils::scale_value(40);

    auto getWidget = [rootWidget] (const bool _vertical)
    {
        QBoxLayout* layoutInWidget;
        if (_vertical)
        {
            layoutInWidget = Utils::emptyVLayout();
        }
        else
        {
            layoutInWidget = Utils::emptyHLayout();
        }

        layoutInWidget->setAlignment(Qt::AlignCenter);

        QWidget* widget = new voipTools::BoundBox<QWidget>(rootWidget);
        widget->setContentsMargins(0, 0, 0, 0);
        widget->setLayout(layoutInWidget);

        return widget;
    };

    auto getButton = [this, btnMinWidth] (QWidget* _parent, const char* _objectName, const char* /*text*/, const char* _slot, QPushButton** _btnOut)->int
    {
        QPushButton* btn = new voipTools::BoundBox<QPushButton>(_parent);
        btn->setObjectName(QString::fromUtf8(_objectName));
        btn->setSizePolicy(QSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred));
        btn->setCursor(QCursor(Qt::PointingHandCursor));
        connect(btn, SIGNAL(clicked()), this, _slot, Qt::QueuedConnection);
        *_btnOut = btn;

        btn->resize(btn->sizeHint().width(), btn->sizeHint().height());
        return (btn->width() - btnMinWidth) / 2;
    };

    auto getLabel = [btnMinWidth] (QWidget* _parent, const char* /*_objectName*/, const char* _text, QLabel** _labelOut)->int
    {
        QLabel* label = new voipTools::BoundBox<QLabel>(_parent);
        label->setSizePolicy(QSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred));
        label->setText(QString::fromUtf8(_text));
        label->setFont(Fonts::appFontScaled(9, Fonts::FontWeight::SemiBold));
        label->setAlignment(Qt::AlignCenter);
        label->setObjectName(qsl("voipPanelLabel"));

        *_labelOut = label;

        label->resize(label->sizeHint().width(), label->sizeHint().height());
        return (label->width() - btnMinWidth) * 0.5f;
    };

    QWidget* widget1 = getWidget(true);
    QWidget* widget2 = getWidget(true);
    QWidget* widget3 = getWidget(true);

    int tail1 = 0;
    int tail2 = 0;
    int tail3 = 0;

    const struct ButtonDesc
    {
        QWidget*    parent;
        std::string objectName;
        std::string text;
        std::string slot;
        int&        tail;

        ButtonDesc(QWidget*    parent,
            std::string objectName,
            std::string text,
            std::string slot,
            int&        tail) : parent(parent), objectName(objectName), text(text), slot(slot), tail(tail) {}
    }
    buttonsToCreate [] =
    {
        ButtonDesc(
            widget1,
            "incomingCall_hangup",
            QT_TRANSLATE_NOOP("voip_pages","DECLINE").toUtf8().data(),
            SLOT(_onDecline()),
            tail1
        ),
        ButtonDesc(
            widget2,
            "incomingCall_audio",
            QT_TRANSLATE_NOOP("voip_pages", "AUDIO").toUtf8().data(),
            SLOT(_onAudio()),
            tail2
        ),
        ButtonDesc(
            widget3,
            "incomingCall_video",
            QT_TRANSLATE_NOOP("voip_pages", "VIDEO").toUtf8().data(),
            SLOT(_onVideo()),
            tail3
        )
    };

    for (unsigned ix = 0; ix < sizeof buttonsToCreate / sizeof buttonsToCreate[0]; ++ix)
    {
        const ButtonDesc& buttonDesc = buttonsToCreate[ix];

        QPushButton* btn = NULL;
        const int btnTail = getButton(buttonDesc.parent, buttonDesc.objectName.c_str(), buttonDesc.text.c_str(), buttonDesc.slot.c_str(), &btn);

        QLabel* label = NULL;
        const int labelTail = getLabel(buttonDesc.parent, buttonDesc.objectName.c_str(), buttonDesc.text.c_str(), &label);

        assert(btn && label);
        if (!btn || !label)
        {
            continue;
        }
        buttonDesc.tail = btnTail;// std::max(btnTail, labelTail);

        QWidget* tmpWidget = getWidget(false);
        tmpWidget->layout()->addWidget(btn);
        buttonDesc.parent->layout()->addWidget(tmpWidget);
        qobject_cast<QVBoxLayout*>(buttonDesc.parent->layout())->addSpacing(Utils::scale_value(9));
        buttonDesc.parent->layout()->addWidget(label);
    }

    int w1Width = widget1->sizeHint().width();
    int w2Width = widget2->sizeHint().width();
    int w3Width = widget3->sizeHint().width();

    int leftSpace  = VOIP_VIDEO_PANEL_BTN_FULL_OFFSET + VOIP_VIDEO_PANEL_BTN_SIZE - (w1Width + w2Width + 1) / 2;
    int rightSpace = VOIP_VIDEO_PANEL_BTN_FULL_OFFSET + VOIP_VIDEO_PANEL_BTN_SIZE - (w2Width + w3Width + 1) / 2;

    int leftPart  = leftSpace + w1Width;
    int rightPart = rightSpace + w3Width;

    layoutTarget->addStretch(1);

    if (leftPart < rightPart)
    {
        layoutTarget->addSpacing(rightPart - leftPart);
    }

    layoutTarget->addWidget(widget1);
    layoutTarget->addSpacing(leftSpace);

    layoutTarget->addWidget(widget2);

    layoutTarget->addSpacing(rightSpace);
    layoutTarget->addWidget(widget3);

    if (leftPart > rightPart)
    {
        layoutTarget->addSpacing(leftPart - rightPart);
    }

    layoutTarget->addStretch(1);
}

Ui::IncomingCallControls::~IncomingCallControls()
{
}

void Ui::IncomingCallControls::_onDecline()
{
    emit onDecline();
}

void Ui::IncomingCallControls::_onAudio()
{
    emit onAudio();
}

void Ui::IncomingCallControls::_onVideo()
{
    emit onVideo();
}

void Ui::IncomingCallControls::changeEvent(QEvent* _e)
{
    QWidget::changeEvent(_e);
    if (_e->type() == QEvent::ActivationChange)
    {
        if (isActiveWindow() || rootWidget_->isActiveWindow())
        {
            if (parent_)
            {
                //QSignalBlocker sb(parent_);
                parent_->raise();
                raise();
            }
        }
    }
}


Ui::VoipSysPanelHeader::VoipSysPanelHeader(QWidget* _parent, int maxAvalibleWidth)
    : MoveablePanel(_parent)
    , nameAndStatusContainer_(nullptr)
    , rootWidget_(nullptr)
{
#ifdef __linux__
    setStyleSheet(Utils::LoadStyle(qsl(":/qss/video_panel_linux")));
#else
    setStyleSheet(Utils::LoadStyle(qsl(":/qss/video_panel")));
#endif

#ifdef __APPLE__
    setProperty("VoipPanelHeaderMac", true);
#else
    setProperty("VoipPanelHeader", true);
#endif

    setAttribute(Qt::WA_NoSystemBackground, true);
    setAttribute(Qt::WA_TranslucentBackground, true);

    QVBoxLayout* rootLayout = Utils::emptyVLayout();
    rootLayout->setAlignment(Qt::AlignVCenter);
    setLayout(rootLayout);

    rootWidget_ = new QWidget(this);
    rootWidget_->setContentsMargins(0, 0, 0, 0);
    rootWidget_->setObjectName(qsl("VoipPanelHeader"));
    rootWidget_->setProperty("VoipPanelHeaderText_Has_Video", false);
    layout()->addWidget(rootWidget_);

    QVBoxLayout* vLayout = Utils::emptyVLayout();

    QHBoxLayout* layout = Utils::emptyHLayout();
    layout->setAlignment(Qt::AlignVCenter);
    rootWidget_->setLayout(vLayout);

    vLayout->addStretch(1);
    vLayout->addLayout(layout);


    nameAndStatusContainer_ = new NameAndStatusWidget(rootWidget_, Utils::scale_value(18),
        Utils::scale_value(15), Utils::scale_value(maxAvalibleWidth));
    nameAndStatusContainer_->setNameProperty("VoipPanelHeaderText_Name", true);
    nameAndStatusContainer_->setNameProperty("VoipPanelHeaderText_Has_Video", false);
    nameAndStatusContainer_->setStatusProperty("VoipPanelHeaderText_Status", true);
    nameAndStatusContainer_->setStatusProperty("VoipPanelHeaderText_Has_Video", true);
    nameAndStatusContainer_->setSizePolicy(QSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred));

    layout->addSpacing(Utils::scale_value(4));
    layout->addWidget(nameAndStatusContainer_, 0, Qt::AlignHCenter);
    layout->addSpacing(Utils::scale_value(4));
}

Ui::VoipSysPanelHeader::~VoipSysPanelHeader()
{

}

void Ui::VoipSysPanelHeader::enterEvent(QEvent* _e)
{
    QWidget::enterEvent(_e);
    emit onMouseEnter();
}

void Ui::VoipSysPanelHeader::leaveEvent(QEvent* _e)
{
    QWidget::leaveEvent(_e);
    emit onMouseLeave();
}

void Ui::VoipSysPanelHeader::setTitle(const char* _s)
{
    if (nameAndStatusContainer_)
    {
        nameAndStatusContainer_->setName(_s);
    }
}

void Ui::VoipSysPanelHeader::setStatus(const char* _s)
{
    if (nameAndStatusContainer_)
    {
        nameAndStatusContainer_->setStatus(_s);
    }
}

bool Ui::VoipSysPanelHeader::uiWidgetIsActive() const
{
    if (rootWidget_)
    {
        return rootWidget_->isActiveWindow();
    }
    return false;
}


void Ui::VoipSysPanelHeader::fadeIn(unsigned int duration) {}

void Ui::VoipSysPanelHeader::fadeOut(unsigned int duration) {}



