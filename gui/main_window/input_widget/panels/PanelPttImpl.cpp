#include "stdafx.h"

#include "PanelPttImpl.h"
#include "HistogramPtt.h"
#include "../ButtonWithCircleHover.h"

#include "../InputWidgetUtils.h"
#include "../SubmitButton.h"

#include "utils/utils.h"
#include "styles/ThemeParameters.h"

#include "media/ptt/AudioUtils.h"
#include "media/ptt/AudioRecorder2.h"

#include "../../sounds/SoundsManager.h"
#include "../../../core_dispatcher.h"
#include "../../../previewer/toast.h"

namespace
{
    QSize deleteButtonSize()
    {
        return QSize(32, 32);
    }

    QString getErrorText(ptt::Error2 _error)
    {
        switch (_error)
        {
        case ptt::Error2::Convert:
            return QT_TRANSLATE_NOOP("input_widget", "Convert error");
        case ptt::Error2::DeviceInit:
            return QT_TRANSLATE_NOOP("input_widget", "DeviceInit error");
        case ptt::Error2::BufferOOM:
            return QT_TRANSLATE_NOOP("input_widget", "BufferOOM error");
        default:
            Q_UNREACHABLE();
        }
        return QString();
    }

    QMargins getRootMargins() noexcept
    {
        return { 0, Ui::getVerMargin(), Ui::getHorMargin(), Ui::getVerMargin() - Utils::getShadowMargin() };
    }
}


namespace Ui
{
    class DeleteButton : public ButtonWithCircleHover
    {
    public:
        DeleteButton(QWidget* _parent, SubmitButton* _submit)
            : ButtonWithCircleHover(_parent, qsl(":/ptt/delete_record"), deleteButtonSize(), Styling::getParameters().getColor(Styling::StyleVariable::SECONDARY_ATTENTION))
            , buttonSubmit_(_submit)
        {
            assert(_submit);

            setHoverColor(Styling::getParameters().getColor(Styling::StyleVariable::SECONDARY_ATTENTION_HOVER));
            setActiveColor(Styling::getParameters().getColor(Styling::StyleVariable::SECONDARY_ATTENTION_ACTIVE));
            setFixedSize(Utils::scale_value(deleteButtonSize()));
            setCustomToolTip(QT_TRANSLATE_NOOP("input_widget", "Delete"));
            setRectExtention(QMargins(0, 0, Utils::scale_value(8), 0));
            setFocusPolicy(Qt::TabFocus);
            setFocusColor(focusColorAttention());

            auto circleHover = std::make_unique<CircleHover>();
            auto c = Styling::getParameters().getColor(Styling::StyleVariable::SECONDARY_ATTENTION);
            c.setAlphaF(0.22);
            circleHover->setColor(c);
            setCircleHover(std::move(circleHover));
        }

        QWidget* previousInFocusChain() const
        {
            return buttonSubmit_;
        }

    protected:
        bool focusNextPrevChild(bool _next) override
        {
            if (!_next)
            {
                buttonSubmit_->setFocus(Qt::BacktabFocusReason);
                return true;
            }
            return QWidget::focusNextPrevChild(_next);
        }

    private:
        SubmitButton* buttonSubmit_;
    };

    InputPanelPttImpl::InputPanelPttImpl(QWidget* _parent, const QString& _contact, QThread* _pttThread, SubmitButton* _submit)
        : QWidget(_parent)
        , buttonSubmit_(_submit)
    {
        setFixedHeight(getDefaultInputHeight());

        setMouseTracking(true);
        setAttribute(Qt::WA_Hover, true);

        auto rootLayot = Utils::emptyHLayout(this);
        rootLayot->setContentsMargins(getRootMargins());

        deleteButton_ = new DeleteButton(this, buttonSubmit_);

        rootLayot->addSpacing(Utils::scale_value(12));
        rootLayot->addWidget(deleteButton_);
        rootLayot->addSpacing(Utils::scale_value(8));
        histogram_ = new HistogramPtt(this, [wThis = QPointer(this)](int _sample, double _resampleCoeff)
            {
                if (wThis && wThis->player_)
                {
                    const auto sample = size_t(_sample / _resampleCoeff * ptt::sampleBlockSizeForHist());
                    if (const auto time = wThis->player_->timePosition(sample); time)
                        return *time;
                }

                return std::chrono::milliseconds::zero();
            });
        rootLayot->addWidget(histogram_);

        setButtonsTabOrder();

        recorder_ = new ptt::AudioRecorder2(nullptr, _contact, ptt::maxDuration(), ptt::minDuration());

        connect(&Ui::GetDispatcher()->getVoipController(), &voip_proxy::VoipController::onVoipDeviceListUpdated, recorder_, &ptt::AudioRecorder2::deviceListChanged);
        recorder_->moveToThread(_pttThread);

        player_ = new ptt::AudioPlayer(this);

        QObject::connect(recorder_, &ptt::AudioRecorder2::aacReady, this, [this](const QString& _contact, const QString& _file, std::chrono::seconds _duration, const ptt::StatInfo& _statInfo)
        {
            emit pttReady(_file, _duration, _statInfo, QPrivateSignal());
        });

        QObject::connect(recorder_, &ptt::AudioRecorder2::spectrum, this, [this](const QVector<double>& _v)
        {
            histogram_->onSpectrum(_v);
        });

        QObject::connect(recorder_, &ptt::AudioRecorder2::stateChanged, this, &InputPanelPttImpl::onStateChanged);

        QObject::connect(recorder_, &ptt::AudioRecorder2::durationChanged, this, [this](std::chrono::milliseconds _duration, const QString&)
        {
            histogram_->setDuration(_duration);
        });

        QObject::connect(recorder_, &ptt::AudioRecorder2::pcmDataReady, this, &InputPanelPttImpl::onPcmDataReady);

        QObject::connect(recorder_, &ptt::AudioRecorder2::limitReached, this, []()
        {
            GetSoundsManager()->playSound(SoundsManager::Sound::PttLimit);
        });

        QObject::connect(recorder_, &ptt::AudioRecorder2::tooShortRecord, this, &InputPanelPttImpl::removeRecord);
        QObject::connect(recorder_, &ptt::AudioRecorder2::error, this, &InputPanelPttImpl::onError);

        QObject::connect(deleteButton_, &CustomButton::clicked, this, &InputPanelPttImpl::onDeleteClicked);
        QObject::connect(histogram_, &HistogramPtt::clicked, this, &InputPanelPttImpl::stop);
        QObject::connect(histogram_, &HistogramPtt::clickOnSample, this, &InputPanelPttImpl::onSampleClicked);
        QObject::connect(histogram_, &HistogramPtt::playButtonClicked, this, &InputPanelPttImpl::play);
        QObject::connect(histogram_, &HistogramPtt::pauseButtonClicked, this, &InputPanelPttImpl::pausePlay);
        QObject::connect(histogram_, &HistogramPtt::stopButtonClicked, this, &InputPanelPttImpl::stop);
        QObject::connect(histogram_, &HistogramPtt::underMouseChanged, this, [this](bool _underMouse)
        {
            emit histogramUnderMouseChanged(_underMouse, QPrivateSignal());
        });

        QObject::connect(player_, &ptt::AudioPlayer::stateChanged, this, &InputPanelPttImpl::onPlayerStateChanged);
        QObject::connect(player_, &ptt::AudioPlayer::samplePositionChanged, this, [this](int _pos)
        {
            if (_pos > 0)
                _pos /= ptt::sampleBlockSizeForHist();
            histogram_->setCurrentSample(_pos);
        });

        QObject::connect(player_, &ptt::AudioPlayer::timePositionChanged, this, [this](std::chrono::milliseconds _duration)
        {
            histogram_->setDuration(_duration);
        });
    }

    InputPanelPttImpl::~InputPanelPttImpl()
    {
        if (recorder_)
        {
            recordScheduled_ = false;
            recorder_->stop();
            recorder_->deleteLater();
        }
    }

    void InputPanelPttImpl::record()
    {
        if (recorder_)
        {
            auto duration = std::chrono::milliseconds(GetSoundsManager()->playSound(SoundsManager::Sound::StartPtt));
            recordScheduled_ = true;
            histogram_->switchToInit();

            auto startRecord = [this]()
            {
                if (recorder_)
                {
                    if (std::exchange(recordScheduled_, false))
                        recorder_->record();
                }
            };

            if (duration.count() > 0)
                QTimer::singleShot(duration.count(), this, startRecord);
            else
                startRecord();
        }

        playButtonWasClicked_ = false;
    }

    void InputPanelPttImpl::stop()
    {
        if (recorder_)
        {
            if (std::exchange(recordScheduled_, false))
                removeRecord();
            else
                recorder_->stop();
        }

        if (player_ && recorder_ && !player_->hasBuffer())
            recorder_->getPcmData();
    }

    void InputPanelPttImpl::pauseRecord()
    {
        if (recorder_)
            recorder_->pauseRecord();
    }

    void InputPanelPttImpl::play()
    {
        if (player_)
            player_->play();

        playButtonWasClicked_ = true;
    }

    void InputPanelPttImpl::pausePlay()
    {
        if (player_)
            player_->pause();
    }

    void InputPanelPttImpl::getData(ptt::StatInfo&& _statInfo)
    {
        if (player_)
        {
            player_->stop();
            player_->clear();
        }
        _statInfo.playBeforeSent = playButtonWasClicked_;

        if (recorder_)
        {
            recorder_->getData(std::move(_statInfo));
            recorder_->clear();
        }

        histogram_->switchToInit();
    }

    bool InputPanelPttImpl::removeIfUnderMouse()
    {
        if (deleteButton_->containsCursorUnder())
        {
            onDeleteClicked();
            return true;
        }
        return false;
    }

    void InputPanelPttImpl::pressedMouseMove()
    {
        deleteButton_->updateHover();
        histogram_->updateHover();
    }

    void InputPanelPttImpl::enableCircleHover(bool _val)
    {
        deleteButton_->enableCircleHover(_val);
        histogram_->enableCircleHover(_val);
    }

    bool InputPanelPttImpl::canLock() const
    {
        const auto p = QCursor::pos();
        return !deleteButton_->containsCursorUnder() && !histogram_->rect().contains(histogram_->mapFromGlobal(p));
    }

    void InputPanelPttImpl::showEvent(QShowEvent* _event)
    {
        buttonSubmit_->overrideNextInFocusChain(deleteButton_);
    }

    void InputPanelPttImpl::hideEvent(QHideEvent* _event)
    {
        buttonSubmit_->overrideNextInFocusChain(nullptr);
    }

    void InputPanelPttImpl::onError(ptt::Error2 _error)
    {
        Utils::showToastOverMainWindow(getErrorText(_error), Utils::scale_value(10), 2);
    }

    void InputPanelPttImpl::setUnderQuote(const bool _underQuote)
    {
        const auto topMargin = (_underQuote ? getVerMargin() : 0);
        const auto h = getDefaultInputHeight() - topMargin;
        setFixedHeight(h);

        auto m = getRootMargins();
        m.setTop(m.top() - topMargin);
        layout()->setContentsMargins(m);
        layout()->activate();
    }

    void InputPanelPttImpl::setFocusOnDelete()
    {
        deleteButton_->setFocus(Qt::TabFocusReason);
    }

    void InputPanelPttImpl::setButtonsTabOrder()
    {
        const std::vector<QWidget*> widgets = { deleteButton_, histogram_->getButtonWidget(), buttonSubmit_ };
        for (int i = 0; i < int(widgets.size()) - 1; ++i)
        {
            assert(widgets[i]);
            assert(widgets[i + 1]);
            setTabOrder(widgets[i], widgets[i + 1]);
        }
    }

    void InputPanelPttImpl::onStateChanged(ptt::State2 _state)
    {
        if (_state == ptt::State2::Recording)
            histogram_->start();
        else if (_state == ptt::State2::Paused || _state == ptt::State2::Stopped)
            histogram_->stop();

        emit stateChanged(_state, QPrivateSignal());
    }

    void InputPanelPttImpl::onPcmDataReady(const ptt::Buffer& _buffer)
    {
        if (player_)
            player_->setBuffer(_buffer);
    }

    void InputPanelPttImpl::onSampleClicked(int _sample, double _resampleCoeff)
    {
        if (player_)
            player_->setSampleOffset(size_t(_sample /  _resampleCoeff * ptt::sampleBlockSizeForHist()));
    }

    void InputPanelPttImpl::removeRecord()
    {
        recordScheduled_ = false;
        if (player_)
        {
            player_->stop();
            player_->clear();
        }
        if (recorder_)
        {
            recorder_->stop();
            recorder_->clear();
        }

        emit pttRemoved(QPrivateSignal());

        GetSoundsManager()->playSound(SoundsManager::Sound::RemovePtt);
        histogram_->switchToInit();
    }

    void InputPanelPttImpl::onDeleteClicked()
    {
        removeRecord();
        Ui::GetDispatcher()->post_stats_to_core(core::stats::stats_event_names::chatscr_recordptt_action, { { "do", "del" } });
    }

    void InputPanelPttImpl::onPlayerStateChanged(ptt::AudioPlayer::State _state)
    {
        switch (_state)
        {
        case ptt::AudioPlayer::State::Stopped:
            histogram_->setPlayingButton();
            break;
        case ptt::AudioPlayer::State::Paused:
            histogram_->setPlayingButton();
            break;
        case ptt::AudioPlayer::State::Playing:
            histogram_->setPausedButton();
            break;
        default:
            break;
        }
    }
}
