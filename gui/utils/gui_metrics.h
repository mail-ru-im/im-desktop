#pragma once

namespace statistic
{
    class GuiMetrics
    {
    public:

        using memory_size_t = uint64_t;

        void eventStarted();
        void eventUiReady(const bool _is_started_by_user);
        void eventCacheReady();

        void eventChatOpen(const QString& _contact);
        void eventChatLoaded(const QString& _contact);

        void eventStartForward();
        void eventForwardLoaded();

        void eventOpenGallery();
        void eventGalleryPhotoLoaded();
        void eventGalleryVideoLoaded();

        void eventChatCreate();
        void eventChatCreated();

        void eventVideocallStart();
        void eventVideocallWindowCreated();
        void eventVideocallStartCapturing();
        void eventAudiocallStart();
        void eventAudiocallWindowCreated();
        void eventVoipStartCalling();

        void eventConnectionDataReady();

        void eventAppStartAutostart();
        void eventAppStartByIcon();
        void eventNotificationClicked();
        void eventForegroundByNotification();
        void eventAppWakeFromSuspend();
        void eventAppSleep();
        void eventMainWindowActive();
        void eventAppContentUpdateBegin();
        void eventAppContentUpdateEnd();
        void eventAppConnectingBegin();
        void eventAppConnectingEnd();
        void eventAppIconClicked();

        void recordMemorySize(memory_size_t _bytes);

        enum voip_call_type
        {
            vct_unknown,
            vct_video,
            vct_audio,
            vct_not_first_call
        };

        struct memory_size
        {
            size_t memory_size_bytes_ = 0;

            std::string memory_size_string() const;
            core::stats::event_props_type memory_size_props() const;
        };

    private:
        enum class InternalState
        {
            NoState = 0,
            AfterSuspend = 1,
            AfterDockIconClick = 2
        };

        void setInternalState(InternalState _new_state);
        void dropState();
        InternalState state() const;

        void reportAfterSuspendStat();
        void reportForegroundByIconStat();

        void updateMemorySize();
        core::stats::event_props_type durationAndMemoryProps(int32_t _time) const;

    private:

        typedef std::chrono::system_clock::time_point gui_time;

        gui_time now() const;

        bool is_started_by_user_;

        gui_time start_time_;

        gui_time ui_show_time_;

        gui_time cache_load_time_;

        gui_time forward_start_time_;

        gui_time gallery_open_time_;

        gui_time chat_create_time_;

        voip_call_type call_type_ = voip_call_type::vct_unknown;

        gui_time call_start_time_;

        gui_time notification_click_time_;

        gui_time wake_from_suspend_time_;

        gui_time content_update_begin_time_;

        gui_time connecting_begin_time_;

        gui_time dock_app_icon_click_time_;

        std::map<QString, gui_time> chat_open_times_;

        memory_size memory_size_;

        bool event_cache_ready_called_ = false;

        InternalState internal_state_ = InternalState::NoState;
    };

    GuiMetrics& getGuiMetrics();
}
