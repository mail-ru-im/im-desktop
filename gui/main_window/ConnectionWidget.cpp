#include "stdafx.h"

#include "ConnectionWidget.h"
#include "../fonts.h"
#include "../core_dispatcher.h"
#include "../utils/utils.h"
#include "../utils/InterConnector.h"
#include "history_control/MessageStyle.h"
#include "../utils/gui_metrics.h"
#include "controls/TextWidget.h"

using namespace Ui;

namespace
{
    constexpr auto PROGRESS_BAR_ANGLE_MAX = 360;
    constexpr auto QT_ANGLE_MULT = 16;
    constexpr auto PROGRESS_BAR_MIN_PERCENTAGE = 0.7;
    constexpr auto progressSpan = int(PROGRESS_BAR_MIN_PERCENTAGE * PROGRESS_BAR_ANGLE_MAX * QT_ANGLE_MULT);

    int getProgressWidgetSize()
    {
        return Utils::scale_value(28);
    }

    constexpr double animationDuration() noexcept
    {
        return 800;
    }

    int getProgressPenWidth()
    {
        return Utils::scale_value(2);
    }

    auto progressBarColorKey()
    {
        return Styling::ThemeColorKey{ Styling::StyleVariable::BASE_SECONDARY };
    }

    QPen getRotatingProgressBarPen(qreal _width)
    {
        return QPen(Styling::getColor(progressBarColorKey()), _width);
    }

    int getProgressWidth()
    {
        return Utils::scale_value(20);
    }

    Styling::ThemeColorKey getTextColor()
    {
        return Styling::ThemeColorKey{ Styling::StyleVariable::TEXT_SOLID };
    }

    std::string round_interval(const long long _min_val, const long long _value,
                               const long long _step, const long long _max_value)
    {
        im_assert(_value >= _min_val && _value <= _max_value && _step);
        if ((_value < _min_val) || (_value > _max_value) || !_step)
            return std::string();

        auto steps = (_value - _min_val) / _step;

        auto start = _min_val + steps * _step;
        auto end = start + _step - 1;

        return std::to_string(start) + '-' + std::to_string(end);
    }

    std::string duration_secs_interval(long long _s, long long _min_val,
                                       long long _step, long long _max_val)
    {
        std::string interval;

        if (_s < _min_val)
            interval = std::to_string(_s);
        else if (_s <= _max_val)
            interval = round_interval(_min_val, _s, _step, _max_val);
        else
            interval = std::to_string(_max_val + 1) + '+';

        return interval;
    }

    core::stats::event_props_type get_duration_secs_props(int32_t _seconds)
    {
        core::stats::event_props_type props;
        props.reserve(2);

        props.emplace_back("duration", std::to_string(_seconds));
        props.emplace_back("duration_interval", duration_secs_interval(_seconds, 1, 5, 60));

        return props;
    }

    static std::set<ConnectionState> WatchedConnectionStates = {
        ConnectionState::stateConnecting,
        ConnectionState::stateUpdating
    };

    static int ConnectionWidgetsCount = 0;
}

ProgressAnimation::ProgressAnimation(QWidget* _parent)
    : QWidget(_parent)
    , animation_(new QVariantAnimation(this))
    , color_{ progressBarColorKey() }
    , pen_(getRotatingProgressBarPen(getProgressPenWidth()))
    , progressWidth_(getProgressWidth())
{
    animation_->setStartValue(0.0);
    animation_->setEndValue(360.0);
    animation_->setDuration(animationDuration());
    animation_->setLoopCount(-1);
    connect(animation_, &QVariantAnimation::valueChanged, this, [this](const QVariant&)
    {
        if constexpr (platform::is_apple())
        {
            if (++rate_ <= 2)
                return;

            rate_ = 0;
        }

        update();
    });
}

ProgressAnimation::~ProgressAnimation()
{
    stopAnimation();
}

void ProgressAnimation::start()
{
    started_ = true;

    if (isVisible())
        startAnimation();
}

void ProgressAnimation::stop()
{
    started_ = false;
    if (isVisible())
    {
        stopAnimation();
        update();
    }
}

bool ProgressAnimation::isStarted() const
{
    return started_;
}

void Ui::ProgressAnimation::setProgressWidth(qreal _width)
{
    progressWidth_ = _width;
}

void Ui::ProgressAnimation::setProgressPenWidth(qreal _width)
{
    pen_.setWidthF(_width);
}

void Ui::ProgressAnimation::setProgressPenColorKey(const Styling::ThemeColorKey& _color)
{
    color_ = _color;
    pen_.setColor(color_.cachedColor());
}

void Ui::ProgressAnimation::adjust()
{
    const auto size = progressWidth_ + pen_.width() * 2;
    setFixedSize(size, size);
}

void ProgressAnimation::startAnimation()
{
    if (animation_->state() == QAbstractAnimation::State::Running)
        return;

    animation_->start();
}

void ProgressAnimation::stopAnimation()
{
    animation_->stop();
}

void ProgressAnimation::paintEvent(QPaintEvent*)
{
    if (animation_->state() != QAbstractAnimation::State::Running)
        return;

    QRectF progressRect(
        pen_.widthF() / 2.0,
        pen_.widthF() / 2.0,
        progressWidth_ - pen_.widthF(),
        progressWidth_ - pen_.widthF());

    progressRect.moveCenter(rect().center());

    if (color_.canUpdateColor())
        pen_.setColor(color_.actualColor());

    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    p.setPen(pen_);

    const auto angle = qRound(animation_->currentValue().toDouble() * QT_ANGLE_MULT);
    p.drawArc(progressRect, -angle, -progressSpan);
}

void ProgressAnimation::showEvent(QShowEvent*)
{
    if (started_)
        startAnimation();
}

void ProgressAnimation::hideEvent(QHideEvent*)
{
    stopAnimation();
}


ConnectionWidget::ConnectionWidget(QWidget* _parent, const Styling::ThemeColorKey& _textColor)
    : QWidget(_parent)
    , stateTextLabel_(new TextWidget(this, QString(), Data::MentionMap(), TextRendering::LinksVisible::DONT_SHOW_LINKS))
    , state_(ConnectionState::stateOnline)
    , animationWidget_(nullptr)
    , suspended_(false)
{
    auto rootLayout = Utils::emptyHLayout(this);

    animationWidget_ = new ProgressAnimation(this);
    animationWidget_->setFixedWidth(getProgressWidgetSize());

    rootLayout->addWidget(animationWidget_);

    stateTextLabel_->init({ Fonts::appFontScaled(16, Fonts::FontWeight::Normal), _textColor.isValid() ? _textColor : getTextColor() });

    rootLayout->addWidget(stateTextLabel_);
    rootLayout->addStretch();

    QObject::connect(GetDispatcher(), &core_dispatcher::connectionStateChanged, this, &ConnectionWidget::connectionStateChanged);

    setState(GetDispatcher()->getConnectionState());
    ++ConnectionWidgetsCount;
}

ConnectionWidget::~ConnectionWidget()
{
    --ConnectionWidgetsCount;
}

void ConnectionWidget::suspend()
{
    suspended_ = true;

    animationWidget_->stop();
}

void ConnectionWidget::resume()
{
    suspended_ = false;

    setState(GetDispatcher()->getConnectionState());
}

void ConnectionWidget::setState(const ConnectionState& _state)
{
    state_ = _state;

    QString stateText;

    switch (_state)
    {
        case ConnectionState::stateOnline:
        {
            stateText = QT_TRANSLATE_NOOP("connection", "Online");
            animationWidget_->stop();
            break;
        }
        case ConnectionState::stateConnecting:
        {
            if (!suspended_)
                animationWidget_->start();

            stateText = QT_TRANSLATE_NOOP("connection", "Connecting");
            break;
        }
        case ConnectionState::stateUpdating:
        {
            if (!suspended_)
                animationWidget_->start();

            stateText = QT_TRANSLATE_NOOP("connection", "Updating");
            break;
        }
        case ConnectionState::stateUnknown:
        {
            animationWidget_->stop();
            im_assert(false);
            break;
        }
    }

    stateTextLabel_->setText(stateText);
}

void ConnectionWidget::connectionStateChanged(const ConnectionState& _state)
{
    setState(_state);
}

/* ConnectionStateWatcher */

ConnectionStateWatcher::ConnectionStateWatcher(QObject *_parent)
    : QObject(_parent),
      state_(ConnectionState::stateUnknown),
      isActive_(false)
{
    connect(&Utils::InterConnector::instance(), &Utils::InterConnector::activationChanged,
            this, &ConnectionStateWatcher::onActivationChanged);
}

void ConnectionStateWatcher::setState(ConnectionState _state)
{
    handleStateChanged(state_, _state);
    state_ = _state;
}

void ConnectionStateWatcher::onActivationChanged(bool _isActiveNow)
{
    isActive_ = _isActiveNow;

    if (WatchedConnectionStates.find(state_) == WatchedConnectionStates.end())
        return;

    if (_isActiveNow)
    {
        if (shouldStartAggregator())
            aggregator_.start();
    }
    else
    {
        aggregator_.pause();
    }
}

void ConnectionStateWatcher::handleStateChanged(ConnectionState _old, ConnectionState _new)
{
    if (_old == _new)
        return;

    auto aggregator_copy = aggregator_;

    aggregator_.reset();
    if (shouldStartAggregator(_new))
        aggregator_.start();

    switch (_new)
    {
    case ConnectionState::stateConnecting:
        statistic::getGuiMetrics().eventAppConnectingBegin();
        break;
    case ConnectionState::stateUpdating:
        statistic::getGuiMetrics().eventAppContentUpdateBegin();
        break;
    default:
        break;
    }

    if (WatchedConnectionStates.find(_old) == WatchedConnectionStates.end())
        return;

    auto dur = std::chrono::duration_cast<std::chrono::seconds>(aggregator_copy.finish());

    switch (_old)
    {
    case ConnectionState::stateConnecting:
    {
        Ui::GetDispatcher()->post_stats_to_core(core::stats::stats_event_names::chatscr_conmodeconnected_event,
                                                get_duration_secs_props(dur.count()));
        statistic::getGuiMetrics().eventAppConnectingEnd();
    }
        break;
    case ConnectionState::stateUpdating:
    {
        Ui::GetDispatcher()->post_stats_to_core(core::stats::stats_event_names::chatscr_conmodeupdated_event,
                                                get_duration_secs_props(dur.count()));
        statistic::getGuiMetrics().eventAppContentUpdateEnd();
    }
        break;
    default:
        break;
    }
}

bool ConnectionStateWatcher::shouldStartAggregator() const
{
    return shouldStartAggregator(state_);
}

bool ConnectionStateWatcher::shouldStartAggregator(ConnectionState _state) const
{
    return isActive_ && ConnectionWidgetsCount
            && (WatchedConnectionStates.find(_state) != WatchedConnectionStates.end());
}

/* ConnectionStateWatcher::Aggregator */

void ConnectionStateWatcher::Aggregator::start()
{
    im_assert(!isStarted());
    if (isStarted())
        return;

    start_time_ = std::chrono::system_clock::now();
}

void ConnectionStateWatcher::Aggregator::pause()
{
    if (!isStarted())
        return;

    total_duration_ += std::chrono::duration_cast<duration_t>(std::chrono::system_clock::now() - start_time_);
    resetStart();
}

ConnectionStateWatcher::Aggregator::duration_t ConnectionStateWatcher::Aggregator::finish()
{
    pause();
    auto res = total_duration_;
    reset();
    return res;
}

void ConnectionStateWatcher::Aggregator::reset()
{
    resetStart();
    total_duration_ = duration_t::zero();
}

void ConnectionStateWatcher::Aggregator::resetStart()
{
    start_time_ = time_point_t::min();
}

bool ConnectionStateWatcher::Aggregator::isStarted() const
{
    return start_time_ != time_point_t::min();
}
