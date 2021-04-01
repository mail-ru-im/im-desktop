// Copyright (c) 2012 The Chromium Authors. All rights reserved.

#include "network_config_watcher_mac.h"

#include <algorithm>
#include <thread>

#include "network_change_notifier_mac.h"
#include "scoped_typeref.h"

#include "../../utils.h"
#include "../../../common.shared/string_utils.h"

namespace core
{

    namespace
    {
        enum class sc_status_code
        {
            // Unmapped error codes.
            SC_UNKNOWN = 0,

            // These map to the corresponding SCError.
            SC_OK = 1,
            SC_FAILED = 2,
            SC_INVALID_ARGUMENT = 3,
            SC_ACCESS_ERROR = 4,
            SC_NO_KEY = 5,
            SC_KEY_EXISTS = 6,
            SC_LOCKED = 7,
            SC_NEED_LOCK = 8,
            SC_NO_STORE_SESSION = 9,
            SC_NO_STORE_SERVER = 10,
            SC_NOTIFIER_ACTIVE = 11,
            SC_NO_PREFS_SESSION = 12,
            SC_PREFS_BUSY = 13,
            SC_NO_CONFIG_FILE = 14,
            SC_NO_LINK = 15,
            SC_STALE = 16,
            SC_MAX_LINK = 17,
            SC_REACHABILITY_UNKNOWN = 18,
            SC_CONNECTION_NO_SERVICE = 19,
            SC_CONNECTION_IGNORE = 20,

            // Maximum value
            SC_COUNT,
        };

        sc_status_code convert_to_sc_status_code(int sc_error)
        {
            switch (sc_error)
            {
            case kSCStatusOK:
                return sc_status_code::SC_OK;
            case kSCStatusFailed:
                return sc_status_code::SC_FAILED;
            case kSCStatusInvalidArgument:
                return sc_status_code::SC_INVALID_ARGUMENT;
            case kSCStatusAccessError:
                return sc_status_code::SC_ACCESS_ERROR;
            case kSCStatusNoKey:
                return sc_status_code::SC_NO_KEY;
            case kSCStatusKeyExists:
                return sc_status_code::SC_KEY_EXISTS;
            case kSCStatusLocked:
                return sc_status_code::SC_LOCKED;
            case kSCStatusNeedLock:
                return sc_status_code::SC_NEED_LOCK;
            case kSCStatusNoStoreSession:
                return sc_status_code::SC_NO_STORE_SESSION;
            case kSCStatusNoStoreServer:
                return sc_status_code::SC_NO_STORE_SERVER;
            case kSCStatusNotifierActive:
                return sc_status_code::SC_NOTIFIER_ACTIVE;
            case kSCStatusNoPrefsSession:
                return sc_status_code::SC_NO_PREFS_SESSION;
            case kSCStatusPrefsBusy:
                return sc_status_code::SC_PREFS_BUSY;
            case kSCStatusNoConfigFile:
                return sc_status_code::SC_NO_CONFIG_FILE;
            case kSCStatusNoLink:
                return sc_status_code::SC_NO_LINK;
            case kSCStatusStale:
                return sc_status_code::SC_STALE;
            case kSCStatusMaxLink:
                return sc_status_code::SC_MAX_LINK;
            case kSCStatusReachabilityUnknown:
                return sc_status_code::SC_REACHABILITY_UNKNOWN;
            case kSCStatusConnectionNoService:
                return sc_status_code::SC_CONNECTION_NO_SERVICE;
            case kSCStatusConnectionIgnore:
                return sc_status_code::SC_CONNECTION_IGNORE;
            default:
                return sc_status_code::SC_UNKNOWN;
            }
        }

        // Called back by OS.  Calls on_network_config_change().
        void dynamic_store_callback(SCDynamicStoreRef /* _store */, CFArrayRef _changed_keys, void* _config_delegate)
        {
            network_config_watcher_mac::delegate* net_config_delegate = static_cast<network_config_watcher_mac::delegate*>(_config_delegate);
            net_config_delegate->on_network_config_change(_changed_keys);
        }

    }  // namespace



    bool network_config_watcher_mac::init()
    {
        delegate_->init();
        return true;
    }

    void network_config_watcher_mac::clean_up()
    {
        if (!run_loop_source_.get())
            return;

        CFRunLoopRemoveSource(CFRunLoopGetCurrent(), run_loop_source_.get(), kCFRunLoopCommonModes);
        run_loop_source_.reset();
    }


    bool network_config_watcher_mac::init_notifications()
    {
        // SCDynamicStore API does not exist on iOS.
        // Add a run loop source for a dynamic store to the current run loop.
        SCDynamicStoreContext context = {
            0,          // Version 0.
            delegate_,  // User data.
            NULL,       // This is not reference counted.  No retain function.
            NULL,       // This is not reference counted.  No release function.
            NULL,       // No description for this.
        };
        internal::scoped_cftype_ref<SCDynamicStoreRef> store(SCDynamicStoreCreate(NULL, CFSTR("icq"), dynamic_store_callback, &context));
        if (!store)
        {
            int error = SCError();
            network_change_notifier::log_text(su::concat("SCDynamicStoreCreate failed with Error: ", std::to_string(error), " - ", SCErrorString(error)));
            return false;
        }
        run_loop_source_.reset(SCDynamicStoreCreateRunLoopSource(NULL, store.get(), 0));
        if (!run_loop_source_)
        {
            int error = SCError();
            network_change_notifier::log_text(su::concat("SCDynamicStoreCreateRunLoopSource failed with Error: ", std::to_string(error), " - ", SCErrorString(error)));
            return false;
        }
        CFRunLoopAddSource(CFRunLoopGetCurrent(), run_loop_source_.get(), kCFRunLoopCommonModes);

        // Set up notifications for interface and IP address changes.
        delegate_->start_reachability_notifications();

        delegate_->set_dynamic_store_notification_keys(store.get());

        return true;
    }

    network_config_watcher_mac::network_config_watcher_mac(delegate* _delegate)
        : delegate_(_delegate),
          notifications_initialized_(false)
    {

    }

    network_config_watcher_mac::~network_config_watcher_mac()
    {
        watch_thread_.join();
    }

    void network_config_watcher_mac::start_watch()
    {
        // We create this notifier thread because the notification implementation
        // needs a thread with a CFRunLoop, and there's no guarantee that
        // current_thread meets that criterion.
        auto init_task = [this]()
        {
            utils::set_this_thread_name("Network config watcher");
            init();
            // continue init notifications
            auto wakeup_task = [wr_this = weak_from_this()]
            {
                auto ptr_this = wr_this.lock();
                if (!ptr_this)
                    return;
                {
                    std::scoped_lock<std::mutex> locker(ptr_this->wakeup_mutex_);
                    ptr_this->timer_ready_ = true;
                }
                ptr_this->timer_cv_.notify_one();
            };
            int timer = -1;
            bool success = init_notifications();
            while (!success)
            {
                network_change_notifier::log_text("Retrying SystemConfiguration registration in 1 second.");
                // repeat if initialization fails after 1 second
                std::unique_lock<std::mutex> locker(wakeup_mutex_);
                timer_ready_ = false;
                if (timer == -1)
                    g_core->add_timer({wakeup_task}, std::chrono::seconds(1));
                if (!timer_ready_ && !need_abort_)
                    timer_cv_.wait(locker, [this] { return timer_ready_ || need_abort_; });

                timer_ready_ = false;
                if (need_abort_)
                {
                    g_core->stop_timer(timer);
                    return;
                }
                locker.unlock();
                success = init_notifications();
            }
            g_core->stop_timer(timer);
            notifications_initialized_ = true;
            CFRunLoopRun();
            clean_up();
        };
        watch_thread_ = std::thread(init_task);
    }

    void network_config_watcher_mac::stop_watch(CFRunLoopRef _run_loop)
    {
        if (notifications_initialized_)
        {
            CFRunLoopStop(_run_loop);
            return;
        }

        {
            std::scoped_lock<std::mutex> locker(wakeup_mutex_);
            need_abort_ = true;
        }
        timer_cv_.notify_one();

    }

}  // namespace core
