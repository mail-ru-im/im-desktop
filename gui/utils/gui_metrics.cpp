#include "stdafx.h"
#include "gui_metrics.h"
#include "../core_dispatcher.h"
#include "utils/utils.h"
#include "utils/memory_utils.h"
#include "log/log.h"

using namespace statistic;

namespace
{
core::stats::event_props_type get_duration_props(int32_t _milliseconds);
}

GuiMetrics::gui_time GuiMetrics::now() const
{
    return std::chrono::system_clock::now();
}

void GuiMetrics::eventStarted()
{
    start_time_ = now();
}

void GuiMetrics::eventUiReady(const bool _is_started_by_user)
{
    ui_show_time_ = std::chrono::system_clock::now();

    is_started_by_user_ = _is_started_by_user;
}

void GuiMetrics::eventCacheReady()
{
    if (event_cache_ready_called_)
        return;

    if (!is_started_by_user_)
        return;

    event_cache_ready_called_ = true;
    cache_load_time_ = now();

    core::stats::event_props_type props;

    const int32_t time_to_window = (ui_show_time_ - start_time_) / std::chrono::milliseconds(1);
    const int32_t time_to_cache = (cache_load_time_ - ui_show_time_) / std::chrono::milliseconds(1);
    const int32_t time_sum = (cache_load_time_ - start_time_) / std::chrono::milliseconds(1);

    props.emplace_back("From_Beginning_To_EmptyWindow", (time_to_window <= 20000) ? core::stats::round_value(time_to_window, 1000, 20000) : core::stats::round_value(time_to_window, 5000, 60000));
    props.emplace_back("From_EmptyWindow_To_Cache", (time_to_cache <= 20000) ? core::stats::round_value(time_to_cache, 1000, 20000) : core::stats::round_value(time_to_cache, 5000, 60000));
    props.emplace_back("App_Start_Summary", (time_sum <= 20000) ? core::stats::round_value(time_sum, 1000, 20000) : core::stats::round_value(time_sum, 5000, 60000));

    Ui::GetDispatcher()->post_stats_to_core(core::stats::stats_event_names::app_start, props);
}

void GuiMetrics::eventChatOpen(const QString& _contact)
{
    chat_open_times_[_contact] = now();
}

void GuiMetrics::eventChatLoaded(const QString& _contact)
{
    const auto citer = chat_open_times_.find(_contact);

    if (citer != chat_open_times_.cend())
    {
        const int32_t time_diff = (now() - citer->second) / std::chrono::milliseconds(1);

        core::stats::event_props_type props;

        props.emplace_back("From_Click_To_Cache", core::stats::round_value(time_diff, 50, 1000));

        chat_open_times_.erase(citer);

        Ui::GetDispatcher()->post_stats_to_core(core::stats::stats_event_names::chat_open, props);

        std::stringstream logData;
        logData << "archive/messages/get for " << _contact.toUtf8().constData() << " completed in " << time_diff << " ms";

        Log::write_network_log(logData.str());
    }
}

void GuiMetrics::eventStartForward()
{
    forward_start_time_ = now();
}

void GuiMetrics::eventForwardLoaded()
{
    const int32_t time_diff = (now() - forward_start_time_) / std::chrono::milliseconds(1);

    core::stats::event_props_type props;

    props.emplace_back("From_Click_To_View", core::stats::round_value(time_diff, 50, 1000));

    Ui::GetDispatcher()->post_stats_to_core(core::stats::stats_event_names::forward_list, props);
}

void GuiMetrics::eventOpenGallery()
{
    gallery_open_time_ = now();
}

void GuiMetrics::eventGalleryPhotoLoaded()
{
    const int32_t time_diff = (now() - gallery_open_time_) / std::chrono::milliseconds(1);

    core::stats::event_props_type props;

    props.emplace_back("From_Click_To_View", core::stats::round_value(time_diff, 50, 1000));

    Ui::GetDispatcher()->post_stats_to_core(core::stats::stats_event_names::photo_open, props);
}

void GuiMetrics::eventGalleryVideoLoaded()
{
    const int32_t time_diff = (now() - gallery_open_time_) / std::chrono::milliseconds(1);

    core::stats::event_props_type props;

    props.emplace_back("From_Click_To_View", core::stats::round_value(time_diff, 50, 1000));

    Ui::GetDispatcher()->post_stats_to_core(core::stats::stats_event_names::video_open, props);
}

void GuiMetrics::eventChatCreate()
{
    chat_create_time_ = now();
}

void GuiMetrics::eventChatCreated()
{
    const int32_t time_diff = (now() - chat_create_time_) / std::chrono::milliseconds(1);

    core::stats::event_props_type props;

    props.emplace_back("From_Click_To_View", core::stats::round_value(time_diff, 50, 1000));

    Ui::GetDispatcher()->post_stats_to_core(core::stats::stats_event_names::groupchat_create, props);
}

void GuiMetrics::eventVideocallStart()
{
    call_start_time_ = now();

    if (call_type_ == voip_call_type::vct_unknown)
    {
        call_type_ = voip_call_type::vct_video;
    }
    else
    {
        call_type_ = voip_call_type::vct_not_first_call;
    }
}

void GuiMetrics::eventVideocallWindowCreated()
{
    if (call_type_ == voip_call_type::vct_video)
    {
        const int32_t time_diff = (now() - call_start_time_) / std::chrono::milliseconds(1);

        core::stats::event_props_type props;

        props.emplace_back("time", core::stats::round_value(time_diff, 100, 10000));

        Ui::GetDispatcher()->post_stats_to_core(core::stats::stats_event_names::videocall_from_click_to_window, props);
    }
}

void GuiMetrics::eventVoipStartCalling()
{
    static bool called = false;

    if (called)
        return;

    const int32_t time_diff = (now() - call_start_time_) / std::chrono::milliseconds(1);

    core::stats::event_props_type props;

    props.emplace_back("time", core::stats::round_value(time_diff, 100, 10000));

    if (call_type_ == voip_call_type::vct_video)
    {
        Ui::GetDispatcher()->post_stats_to_core(core::stats::stats_event_names::videocall_from_click_to_calling, props);
    }
    else if (call_type_ == voip_call_type::vct_audio)
    {
        Ui::GetDispatcher()->post_stats_to_core(core::stats::stats_event_names::audiocall_from_click_to_calling, props);
    }

    called = true;
}

void GuiMetrics::eventVideocallStartCapturing()
{
    static bool captured = false;

    if (!captured && call_type_ == voip_call_type::vct_video)
    {
        const int32_t time_diff = (now() - call_start_time_) / std::chrono::milliseconds(1);

        core::stats::event_props_type props;

        props.emplace_back("time", core::stats::round_value(time_diff, 100, 10000));

        Ui::GetDispatcher()->post_stats_to_core(core::stats::stats_event_names::videocall_from_click_to_camera, props);
    }

    captured = true;
}

void GuiMetrics::eventAudiocallStart()
{
    call_start_time_ = now();

    if (call_type_ == voip_call_type::vct_unknown)
    {
        call_type_ = voip_call_type::vct_audio;
    }
    else
    {
        call_type_ = voip_call_type::vct_not_first_call;
    }
}

void GuiMetrics::eventAudiocallWindowCreated()
{
    if (call_type_ == voip_call_type::vct_audio)
    {
        const int32_t time_diff = (now() - call_start_time_) / std::chrono::milliseconds(1);

        core::stats::event_props_type props;

        props.emplace_back("time", core::stats::round_value(time_diff, 100, 10000));

        Ui::GetDispatcher()->post_stats_to_core(core::stats::stats_event_names::audiocall_from_click_to_window, props);
    }
}

void GuiMetrics::eventConnectionDataReady()
{

}

void GuiMetrics::eventAppStartAutostart()
{
    auto autostart_time = (now() - start_time_) / std::chrono::milliseconds(1);
    updateMemorySize();

    auto props = durationAndMemoryProps(autostart_time);
    Ui::GetDispatcher()->post_stats_to_core(core::stats::stats_event_names::appstart_autostart_event, props);
}

void GuiMetrics::eventAppStartByIcon()
{
    auto byiconstart_time = (now() - start_time_) / std::chrono::milliseconds(1);
    updateMemorySize();

    auto props = durationAndMemoryProps(byiconstart_time);
    Ui::GetDispatcher()->post_stats_to_core(core::stats::stats_event_names::appstart_byicon_event, props);
}

void GuiMetrics::eventNotificationClicked()
{
    notification_click_time_ = now();
}

void GuiMetrics::eventForegroundByNotification()
{
    auto bynotifstart_time = (now() - notification_click_time_) / std::chrono::milliseconds(1);
    updateMemorySize();

    auto props = durationAndMemoryProps(bynotifstart_time);
    Ui::GetDispatcher()->post_stats_to_core(core::stats::stats_event_names::appforeground_bynotif_event, props);
}

void GuiMetrics::reportAfterSuspendStat()
{
    auto aftersuspend_time = (now() - wake_from_suspend_time_) / std::chrono::milliseconds(1);
    updateMemorySize();

    auto props = durationAndMemoryProps(aftersuspend_time);
    Ui::GetDispatcher()->post_stats_to_core(core::stats::stats_event_names::appstart_fromsuspend_event, props);
}

void GuiMetrics::reportForegroundByIconStat()
{
    auto after_dock_icon_click = (now() - dock_app_icon_click_time_) / std::chrono::milliseconds(1);
    updateMemorySize();

    auto props = durationAndMemoryProps(after_dock_icon_click);
    Ui::GetDispatcher()->post_stats_to_core(core::stats::stats_event_names::appforeground_byicon_event, props);
}

void GuiMetrics::eventAppWakeFromSuspend()
{
    wake_from_suspend_time_ = now();
    setInternalState(InternalState::AfterSuspend);
}

void GuiMetrics::eventAppSleep()
{
    dropState();
}

void GuiMetrics::eventMainWindowActive()
{
    if (internal_state_ == InternalState::NoState)
        return;

    switch (internal_state_)
    {
    case InternalState::AfterSuspend:
        reportAfterSuspendStat();
        break;
    case InternalState::AfterDockIconClick:
        reportForegroundByIconStat();
        break;
    default:
        assert(!"handle internal state");
    }

    dropState();
}

void GuiMetrics::eventAppContentUpdateBegin()
{
    content_update_begin_time_ = now();
}

void GuiMetrics::eventAppContentUpdateEnd()
{
    auto update_time = (now() - content_update_begin_time_) / std::chrono::milliseconds(1);
    auto props = get_duration_props(update_time);

    Ui::GetDispatcher()->post_stats_to_core(core::stats::stats_event_names::appcontentupdated_event, props);
}

void GuiMetrics::eventAppConnectingBegin()
{
    connecting_begin_time_ = now();
}

void GuiMetrics::eventAppConnectingEnd()
{
    auto connecting_time = (now() - connecting_begin_time_) / std::chrono::milliseconds(1);
    auto props = get_duration_props(connecting_time);

    Ui::GetDispatcher()->post_stats_to_core(core::stats::stats_event_names::appconnected_event, props);
}

void GuiMetrics::eventAppIconClicked()
{
    dock_app_icon_click_time_ = now();
    setInternalState(InternalState::AfterDockIconClick);
}

void GuiMetrics::updateMemorySize()
{
    recordMemorySize(Utils::getCurrentProcessRamUsage());
}

core::stats::event_props_type GuiMetrics::durationAndMemoryProps(int32_t _time) const
{
    auto props = get_duration_props(_time);
    auto mem_size_props = memory_size_.memory_size_props();
    std::copy(mem_size_props.begin(), mem_size_props.end(), std::back_inserter(props));

    return props;
}

void GuiMetrics::recordMemorySize(GuiMetrics::memory_size_t _bytes)
{
    memory_size_.memory_size_bytes_ = _bytes;
}

void GuiMetrics::setInternalState(GuiMetrics::InternalState _new_state)
{
    internal_state_ = _new_state;
}

void GuiMetrics::dropState()
{
    internal_state_ = GuiMetrics::InternalState::NoState;
}

GuiMetrics::InternalState GuiMetrics::state() const
{
    return internal_state_;
}

GuiMetrics& statistic::getGuiMetrics()
{
    static GuiMetrics metrics;

    return metrics;
}

std::string GuiMetrics::memory_size::memory_size_string() const
{
    return std::to_string(memory_size_bytes_);
}

core::stats::event_props_type GuiMetrics::memory_size::memory_size_props() const
{
    core::stats::event_props_type props;
    props.reserve(2);

    props.push_back({ "mem_size", memory_size_string() });
    props.push_back({ "mem_size_interval", core::stats::memory_size_interval(memory_size_bytes_) });

    return props;
}

namespace
{

core::stats::event_props_type get_duration_props(int32_t _milliseconds)
{
    core::stats::event_props_type props;
    props.reserve(2);

    props.emplace_back("duration", std::to_string(_milliseconds));
    props.emplace_back("duration_interval", core::stats::duration_interval(_milliseconds));

    return props;
}

}
