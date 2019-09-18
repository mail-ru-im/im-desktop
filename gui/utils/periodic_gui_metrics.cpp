#include <QScreen>
#include <QTimer>
#include "periodic_gui_metrics.h"
#include "InterConnector.h"
#include "main_window/MainWindow.h"
#include "main_window/ContactDialog.h"
#include "main_window/MainPage.h"
#include "gui_settings.h"
#include "core_dispatcher.h"

namespace
{
    constexpr auto OneDay = std::chrono::hours(24);
    const std::map<core::stats::stats_event_names, std::chrono::milliseconds> StatPeriods = {
        { core::stats::stats_event_names::chatscr_dtpscreensize_event , OneDay },
        { core::stats::stats_event_names::chatscr_chatsize_event, OneDay },
        { core::stats::stats_event_names::chatscr_dtpframescount_event, OneDay },
        { core::stats::stats_event_names::chatscr_dtpscreentype_event , OneDay }
    };

    constexpr std::chrono::milliseconds PollInterval = std::chrono::hours(3);
}

namespace statistics
{

Periodic_Gui_Metrics& getPeriodicGuiMetrics()
{
    static Periodic_Gui_Metrics Periodic_Metrics;

    return Periodic_Metrics;
}

Periodic_Gui_Metrics::Periodic_Gui_Metrics(QObject *_parent)
    : QObject(_parent)
{
    QTimer *timer = new QTimer(this);

    connect(timer, &QTimer::timeout, this, &Periodic_Gui_Metrics::onPoll);

    timer->setInterval(PollInterval.count());
    timer->setSingleShot(false);
    timer->start();
}

void Periodic_Gui_Metrics::onPoll()
{
    poll();
    postStatsIfNeeded();
}

void Periodic_Gui_Metrics::poll()
{
    getScreenStats();
}

void Periodic_Gui_Metrics::postStatsIfNeeded()
{
    std::vector<core::stats::stats_event_names> sent_stats;
    sent_stats.reserve(stat_to_props.size());

    const auto now_ms = now();

    for (const auto& [stat, props] : stat_to_props)
    {
        const auto it = StatPeriods.find(stat);
        if (it == StatPeriods.end())
            continue;

        const auto [last_sent_time, has_been_sent] = getLastPostedTime(stat);
        const auto since_last_sent = now() - last_sent_time;

        if (!has_been_sent || since_last_sent > it->second)
        {
            Ui::GetDispatcher()->post_stats_to_core(stat, props);
            sent_stats.push_back(stat);
        }
    }

    if (!sent_stats.size())
        return;

    setLastPostedTime(sent_stats, now_ms);
}

void Periodic_Gui_Metrics::getScreenStats()
{
    const auto mw = Utils::InterConnector::instance().getMainWindow();
    const auto cd = Utils::InterConnector::instance().getContactDialog();
    const auto mp = Utils::InterConnector::instance().getMainPage();

    // 1. MW to Screen size in percents
    getMW_To_Screen(mw);

    // 2. Chat size
    getChatWidth(cd);

    // 3. DTP Frames count
    getDTPFramesCount(cd);

    // 4. DTP Screen type
    getDTPScreenType(mp);
}

void Periodic_Gui_Metrics::getMW_To_Screen(Ui::MainWindow *_mw)
{
    if (!_mw)
        return;

    const auto screen = _mw->getScreen();
    const auto rect = _mw->rect();
    const auto screenGeometry = QApplication::desktop()->availableGeometry(screen);

    const auto percents = (rect.width() * rect.height()) * 100. /  (screenGeometry.width() * screenGeometry.height());

    core::stats::event_prop_val_type val;

    if (percents <= 15)
        val = "0-15";
    else if (percents <= 30)
        val = "16-30";
    else if (percents <= 45)
        val = "31-45";
    else if (percents <= 60)
        val = "46-60";
    else if (percents <= 75)
        val = "61-75";
    else if (percents <= 80)
        val = "76-80";
    else if (percents < 100)
        val = "81-99";
    else
        val = "99+";

    saveStat(core::stats::stats_event_names::chatscr_dtpscreensize_event, { { "Size", std::move(val) } });
}

void Periodic_Gui_Metrics::getChatWidth(Ui::ContactDialog* _cd)
{
    if (!_cd)
        return;

    core::stats::event_prop_val_type val;
    const auto w = _cd->width();

    if (w <= 400)
        val = "0-400";
    else if (w < 460)
        val = "401-459";
    else if (w < 500)
        val = "460-499";
    else
        val = "500+";

    saveStat(core::stats::stats_event_names::chatscr_chatsize_event, { { "Size", std::move(val) } });
}

void Periodic_Gui_Metrics::getDTPFramesCount(Ui::ContactDialog *_cd)
{
    if (!_cd)
        return;

    core::stats::event_prop_val_type val;
    switch (_cd->getFrameCountMode())
    {
    case Ui::FrameCountMode::_1:
        val = "1";
        break;
    case Ui::FrameCountMode::_2:
        val = "2";
        break;
    case Ui::FrameCountMode::_3:
        val = "3";
        break;
    default:
        assert(!"handle new FrameCountMode value");
        break;
    }

    saveStat(core::stats::stats_event_names::chatscr_dtpframescount_event, { { "Counter" , std::move(val) } });
}

void Periodic_Gui_Metrics::getDTPScreenType(Ui::MainPage* _mp)
{
    if (!_mp)
        return;

    const auto lps = _mp->getLeftPanelState();
    const auto settings = Ui::get_gui_settings();
    const bool show_last_message = settings->get_value<bool>(settings_show_last_message, true);

    core::stats::event_prop_val_type val;

    switch (lps)
    {
    case Ui::LeftPanelState::picture_only:
        if (show_last_message)
            val = "CompactVert";
        else
            val = "CompactHorVert";
        break;
    case Ui::LeftPanelState::normal:
        if (show_last_message)
            val = "Base";
        else
            val = "CompactHorizont";
        break;
    case Ui::LeftPanelState::spreaded:
        break;
    default:
        break;
    }

    if (val.empty())
        return;

    saveStat(core::stats::stats_event_names::chatscr_dtpscreentype_event, { { "Type",  std::move(val) } });
}

void Periodic_Gui_Metrics::saveStat(core::stats::stats_event_names _stat_name, core::stats::event_props_type _props)
{
    stat_to_props[_stat_name] = std::move(_props);
}

std::tuple<std::chrono::milliseconds, bool> Periodic_Gui_Metrics::getLastPostedTime(core::stats::stats_event_names _stat_name) const
{
    const auto settings = Ui::get_gui_settings();
    const auto stat_to_time = settings->get_value<std::map<int64_t, int64_t>>(qsl(settings_stat_last_posted_times), {});

    if (const auto it = stat_to_time.find(static_cast<int64_t>(_stat_name)); it != stat_to_time.end())
        return { std::chrono::milliseconds(it->second), true };
    return { std::chrono::milliseconds(0), false };
}

void Periodic_Gui_Metrics::setLastPostedTime(const std::vector<core::stats::stats_event_names>& _stat_names, std::chrono::milliseconds _ms)
{
    const auto settings = Ui::get_gui_settings();
    auto stat_to_time = settings->get_value<std::map<int64_t, int64_t>>(qsl(settings_stat_last_posted_times), {});

    for (auto stat_name: _stat_names)
        stat_to_time[static_cast<int64_t>(stat_name)] = _ms.count();

    settings->set_value<std::map<int64_t, int64_t>>(settings_stat_last_posted_times, stat_to_time);
}

std::chrono::milliseconds Periodic_Gui_Metrics::now() const
{
    return std::chrono::time_point_cast<std::chrono::milliseconds>(std::chrono::system_clock::now()).time_since_epoch();
}

}
