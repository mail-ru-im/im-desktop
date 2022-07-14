#include "stdafx.h"
#include "VideoControls.h"
#include "VideoPanel.h"
#include "PanelButtons.h"
#include "../utils/utils.h"
#include "../previewer/GalleryFrame.h"
#include "../core_dispatcher.h"
namespace
{
    constexpr std::chrono::milliseconds kAnimationDuration(500);
    constexpr std::chrono::milliseconds kPanelFadeDelay(1500);

    auto getMinimumMenuWidth() noexcept
    {
        return Utils::scale_value(196);
    }

    auto getPanelButtonIconSize() noexcept
    {
        return Utils::scale_value(QSize(16, 16));
    }

    auto getPanelBottomOffset() noexcept
    {
        return Utils::scale_value(12);
    }

    auto getButtonsLayoutMargins() noexcept
    {
        return Utils::scale_value(QMargins(8, 8, 8, 0));
    }

    auto screenIcon(Qt::WindowStates _states)
    {
        if (_states & Qt::WindowFullScreen)
            return ql1s(":/voip/normal_view");
        else
            return ql1s(":/voip/fullscreen_view");
    }

    auto viewIcon(Ui::IRender::ViewMode _mode)
    {
        switch (_mode)
        {
        case Ui::IRender::ViewMode::GridMode:
            return ql1s(":/voip/grid_view");
        case Ui::IRender::ViewMode::SpeakerMode:
            return ql1s(":/voip/speaker_view");
        default:
            break;
        }
        return QLatin1String();
    }
}

namespace Ui
{
    class VideoControlsPrivate
    {
    public:
        VideoControls* q = nullptr;
        VideoPanel* videoPanel_ = nullptr;
        QAction* gridAction_ = nullptr;
        QAction* speakerAction_ = nullptr;
        Previewer::CustomMenu* menu_ = nullptr;
        PanelToolButton* linkButton_ = nullptr;
        PanelToolButton* viewButton_ = nullptr;
        PanelToolButton* screenButton_ = nullptr;
        QTimeLine* timeLine_ = nullptr;
        QTimer* delayTimer_ = nullptr;
        std::vector<QGraphicsEffect*> effects_;
        double visualOpacity_ = 0.0;
        bool animationEnabled_ = true;

        VideoControlsPrivate(VideoControls* _q) : q(_q) {}

        void createDelayTimer()
        {
            delayTimer_ = new QTimer(q);
            delayTimer_->setSingleShot(true);
            delayTimer_->setInterval(kPanelFadeDelay);
            QObject::connect(delayTimer_, &QTimer::timeout, q, &VideoControls::onFadeTimeout);
        }

        void createTimeLine()
        {
            timeLine_ = new QTimeLine(kAnimationDuration.count(), q);
            timeLine_->setEasingCurve(QEasingCurve::InOutQuad);
            QObject::connect(timeLine_, &QTimeLine::valueChanged, q, &VideoControls::setOpacity);
        }

        void installEffect(QWidget* _widget)
        {
            QGraphicsOpacityEffect* effect = new QGraphicsOpacityEffect(_widget);
            effect->setOpacity(0.0);
            _widget->setGraphicsEffect(effect);
            effects_.emplace_back(effect);

            QObject::connect(timeLine_, &QTimeLine::valueChanged, effect, &QGraphicsOpacityEffect::setOpacity);
        }

        void createControlPanel()
        {
            videoPanel_ = new VideoPanel(q);

            installEffect(videoPanel_);
            QObject::connect(videoPanel_, &VideoPanel::lockAnimations, q, &VideoControls::animationsLocked);
            QObject::connect(videoPanel_, &VideoPanel::unlockAnimations, q, &VideoControls::animationsUnlocked);
        }

        QAction* createViewAction(QObject* _parent, IRender::ViewMode _mode, const QString& _title, bool _checked)
        {
            QAction* action = new QAction(qsl("Grid"), _parent);
            action->setCheckable(true);
            action->setChecked(_checked);
            action->setText(_title);
            action->setData(static_cast<int>(_mode));
            return action;
        }

        void createViewMenu()
        {
            menu_ = new Previewer::CustomMenu(q);
            menu_->setAttribute(Qt::WA_DeleteOnClose, false);
            menu_->setFeatures(Previewer::CustomMenu::DefaultPopup);
            menu_->setArrowSize({});
            menu_->setMinimumWidth(getMinimumMenuWidth());
            menu_->setCheckMark(Utils::renderSvg(ql1s(":/voip/check_mark")));
            QObject::connect(menu_, &QMenu::aboutToShow, q, &VideoControls::animationsLocked);
            QObject::connect(menu_, &QMenu::aboutToHide, q, &VideoControls::animationsUnlocked);

            QActionGroup* viewActions = new QActionGroup(menu_);
            viewActions->setExclusive(true);

            gridAction_ = createViewAction(viewActions, IRender::ViewMode::GridMode, QT_TRANSLATE_NOOP("voip_pages", "Grid"), false);
            QObject::connect(gridAction_, &QAction::triggered, q, &VideoControls::requestGridView);

            speakerAction_ = createViewAction(viewActions, IRender::ViewMode::SpeakerMode, QT_TRANSLATE_NOOP("voip_pages", "Speaker"), true);
            QObject::connect(speakerAction_, &QAction::triggered, q, &VideoControls::requestSpeakerView);

            const auto& actions = viewActions->actions();
            for (auto action : actions)
            {
                const auto iconPath = viewIcon(static_cast<IRender::ViewMode>(action->data().toInt()));
                menu_->addAction(action, Utils::renderSvg(iconPath));
            }
        }

        PanelToolButton* createButton(const QString& _title, const QString& _iconPath, Qt::ToolButtonStyle _style)
        {
            auto button = new PanelToolButton(_title, q);
            button->setIcon(_iconPath);
            button->setIconSize(getPanelButtonIconSize());
            button->setIconStyle(PanelToolButton::Rounded);
            button->setButtonStyle(_style);
            button->setCheckable(false);
            button->setChecked(false);
            button->setCursor(Qt::PointingHandCursor);
            button->setTooltipsEnabled(false);
            button->setAutoRaise(true);
            installEffect(button);
            return button;
        }

        void createButtons()
        {
            linkButton_ = createButton(QT_TRANSLATE_NOOP("video_pages", "Link"), ql1s(":/voip/link"), Qt::ToolButtonTextBesideIcon);
            linkButton_->setVisible(false);
            QObject::connect(linkButton_, &PanelToolButton::clicked, q, &VideoControls::requestLink);

            viewButton_ = createButton(QT_TRANSLATE_NOOP("video_pages", "View"), viewIcon(IRender::ViewMode::GridMode), Qt::ToolButtonTextBesideIcon);
            viewButton_->setMenu(menu_);
            viewButton_->setPopupMode(PanelToolButton::InstantPopup);
            viewButton_->setMenuAlignment(Qt::AlignBottom | Qt::AlignRight);
            viewButton_->setVisible(false);

            screenButton_ = createButton(QString(), screenIcon(Qt::WindowNoState), Qt::ToolButtonIconOnly);
            QObject::connect(screenButton_, &PanelToolButton::clicked, q, &VideoControls::toggleFullscreen);
        }

        void createLayout()
        {
            QHBoxLayout* buttonsLayout = new QHBoxLayout;
            buttonsLayout->setContentsMargins(getButtonsLayoutMargins());
            buttonsLayout->addWidget(linkButton_, 0, Qt::AlignTop);
            buttonsLayout->addStretch();
            buttonsLayout->addWidget(viewButton_, 0, Qt::AlignTop);
            buttonsLayout->addWidget(screenButton_, 0, Qt::AlignTop);

            QVBoxLayout* layout = new QVBoxLayout(q);
            layout->setContentsMargins(0, 0, 0, getPanelBottomOffset());
            layout->addLayout(buttonsLayout);
            layout->addStretch();
            layout->addWidget(videoPanel_, 1, Qt::AlignHCenter | Qt::AlignBottom);
        }

        void initUi()
        {
            createDelayTimer();
            createTimeLine();
            createControlPanel();
            createViewMenu();
            createButtons();
            createLayout();
        }

        void fadeIn()
        {
            if (visualOpacity_ == 1.0 ||
                (timeLine_->state() == QTimeLine::Running &&
                 timeLine_->direction() == QTimeLine::Forward))
                return;

            timeLine_->setDirection(QTimeLine::Forward);
            if (timeLine_->state() != QTimeLine::Running)
                timeLine_->start();
        }

        void fadeOut()
        {
            if (visualOpacity_ == 0.0 ||
                (timeLine_->state() == QTimeLine::Running &&
                 timeLine_->direction() == QTimeLine::Backward))
                return;

            timeLine_->setDirection(QTimeLine::Backward);
            if (timeLine_->state() != QTimeLine::Running)
                timeLine_->start();
        }
    };

    VideoControls::VideoControls(QWidget* _parent)
        : ShapedWidget(_parent)
        , d(std::make_unique<VideoControlsPrivate>(this))
    {
        d->initUi();
        setFrameShape(QFrame::NoFrame);
        setMidLineWidth(0);
        setLineWidth(0);
    }

    VideoControls::~VideoControls()
    {
        d->menu_->close();
    }

    VideoPanel* VideoControls::panel() const
    {
        return d->videoPanel_;
    }

    void VideoControls::updateControls(const voip_manager::ContactsList& _contacts)
    {
        const bool isPlainVCS = Ui::GetDispatcher()->getVoipController().isCallVCS();
        const bool hasLink = Ui::GetDispatcher()->getVoipController().isCallVCSOrLink();
        d->viewButton_->setVisible(_contacts.isActive && _contacts.contacts.size() > 0 && !isPlainVCS);
        d->linkButton_->setVisible(hasLink);
    }

    void VideoControls::updateControls(IRender::ViewMode _mode)
    {
        d->videoPanel_->setConferenceMode(_mode != IRender::ViewMode::SpeakerMode);
        d->viewButton_->setIcon(viewIcon(_mode));
        QAction* action = nullptr;
        switch (_mode)
        {
        case IRender::ViewMode::GridMode:
            action = d->gridAction_;
            break;
        case IRender::ViewMode::SpeakerMode:
            action = d->speakerAction_;
            break;
        default:
            break;
        }

        QSignalBlocker blocker(this);
        if (action)
            action->setChecked(true);
    }

    void VideoControls::fadeIn()
    {
        d->fadeIn();
        d->delayTimer_->start();
    }

    void VideoControls::fadeOut()
    {
        d->fadeOut();
    }

    void VideoControls::onFadeTimeout()
    {
        if (!d->animationEnabled_ || d->videoPanel_->rect().contains(d->videoPanel_->mapFromGlobal(QCursor::pos())))
            delayFadeOut();
        else
            d->fadeOut();
    }

    void VideoControls::delayFadeOut()
    {
        d->delayTimer_->start();
    }

    void VideoControls::setOpacity(double _value)
    {
        d->visualOpacity_ = _value;
    }

    void VideoControls::animationsLocked()
    {
        d->animationEnabled_ = false;
        for (auto e : d->effects_)
            e->setEnabled(d->animationEnabled_);

        d->visualOpacity_ = 1.0;
    }

    void VideoControls::animationsUnlocked()
    {
        d->animationEnabled_ = true;
        for (auto e : d->effects_)
            e->setEnabled(d->animationEnabled_);
    }

    void VideoControls::onWindowStateChanged(Qt::WindowStates _states)
    {
        d->screenButton_->setIcon(screenIcon(_states));
    }
}
