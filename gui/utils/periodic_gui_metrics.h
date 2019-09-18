#pragma once

#include "../../corelib/enumerations.h"
#include "utils.h"

namespace Ui
{
class MainWindow;
class ContactDialog;
class MainPage;
}

namespace statistics
{

/* Deals with stats that are meant to be sent periodically, e.g. once a day */
/* No support for saving/retrieving from disk on launch */

class Periodic_Gui_Metrics: public QObject
{
    Q_OBJECT

public:
    Periodic_Gui_Metrics(QObject* _parent = nullptr);

public Q_SLOTS:
    void onPoll();

public:
    void poll();
    void postStatsIfNeeded();

private:
    void saveStat(core::stats::stats_event_names _stat_name, core::stats::event_props_type _props);

    std::tuple<std::chrono::milliseconds /* last posted time */, bool /* has been posted */> getLastPostedTime(core::stats::stats_event_names _stat_name) const;
    void setLastPostedTime(const std::vector<core::stats::stats_event_names>& _stat_names, std::chrono::milliseconds _ms);

    std::chrono::milliseconds now() const;

    void getScreenStats();

    void getMW_To_Screen(Ui::MainWindow* _mw);
    void getChatWidth(Ui::ContactDialog* _cd);
    void getDTPFramesCount(Ui::ContactDialog* _cd);
    void getDTPScreenType(Ui::MainPage* _mp);

private:
    std::map<core::stats::stats_event_names, core::stats::event_props_type> stat_to_props;
};

Periodic_Gui_Metrics& getPeriodicGuiMetrics();

}
