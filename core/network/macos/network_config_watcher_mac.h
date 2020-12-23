// Copyright (c) 2012 The Chromium Authors. All rights reserved.

#pragma once

#include <SystemConfiguration/SystemConfiguration.h>

#include <memory>

#include "scoped_cftyperef.h"
#include "../../async_task.h"

namespace core
{

    // Helper class for watching the Mac OS system network settings.
    class network_config_watcher_mac : public std::enable_shared_from_this<network_config_watcher_mac>
    {
    public:
        // NOTE: The lifetime of Delegate is expected to exceed the lifetime of
        // NetworkConfigWatcherMac.
        class delegate
        {
        public:
            virtual ~delegate() {}

            // Called to let the delegate do any setup work the must be run on the
            // notifier thread immediately after it starts.
            virtual void init() {}

            // Called to start receiving notifications from the SCNetworkReachability
            // API.
            // Will be called on the notifier thread.
            virtual void start_reachability_notifications() = 0;

            // Called to register the notification keys on |store|.
            // Implementors are expected to call SCDynamicStoreSetNotificationKeys().
            // Will be called on the notifier thread.
            virtual void set_dynamic_store_notification_keys(SCDynamicStoreRef _store) = 0;

            // Called when one of the notification keys has changed.
            // Will be called on the notifier thread.
            virtual void on_network_config_change(CFArrayRef _changed_keys) = 0;
        };

        explicit network_config_watcher_mac(delegate* _delegate);
        ~network_config_watcher_mac();

        void start_watch();
        void stop_watch(CFRunLoopRef _run_loop);

    private:
        bool init();
        void clean_up();

        // Returns whether initializing notifications has succeeded.
        bool init_notifications();


        delegate* const delegate_;
        internal::scoped_cftype_ref<CFRunLoopSourceRef> run_loop_source_;

        std::atomic_bool notifications_initialized_;
        bool need_abort_;
        bool timer_ready_;
        std::mutex wakeup_mutex_;
        std::condition_variable timer_cv_;

        // The thread used to listen for notifications.  This relays the notification
        // to the registered observers without posting back to the thread the object
        // was created on.
        std::thread watch_thread_;

    };

}  // namespace core

