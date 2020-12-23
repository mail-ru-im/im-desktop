// Copyright (c) 2012 The Chromium Authors. All rights reserved.

#include "network_change_notifier_mac.h"

#include <netinet/in.h>
#include <resolv.h>

#include "../../core.h"


namespace core
{

    static bool calculate_reachability(SCNetworkConnectionFlags _flags)
    {
        bool reachable = _flags & kSCNetworkFlagsReachable;
        bool connection_required = _flags & kSCNetworkFlagsConnectionRequired;
        return reachable && !connection_required;
    }

    network_change_notifier_mac::network_change_notifier_mac(std::unique_ptr<network_change_observer> _observer)
        : network_change_notifier(network_change_calculator_params_mac(), std::move(_observer)),
        connection_type_(CONNECTION_UNKNOWN),
        connection_type_initialized_(false),
        forwarder_(this)
    {
        assert(g_core->is_core_thread());
        config_watcher_ = std::make_shared<network_config_watcher_mac>(&forwarder_);
    }

    network_change_notifier_mac::~network_change_notifier_mac()
    {
        // Delete the ConfigWatcher to join the notifier thread, ensuring that
        // StartReachabilityNotifications() has an opportunity to run to completion.
        config_watcher_->stop_watch(run_loop_.get());
        config_watcher_.reset();

        // Now that StartReachabilityNotifications() has either run to completion or
        // never run at all, unschedule reachability_ if it was previously scheduled.
        if (reachability_.get() && run_loop_.get())
            SCNetworkReachabilityUnscheduleFromRunLoop(reachability_.get(), run_loop_.get(), kCFRunLoopCommonModes);

    }

    void network_change_notifier_mac::init()
    {
        network_change_notifier::init();
        config_watcher_->start_watch();
    }

    // static
    network_change_notifier::network_change_calculator_params network_change_notifier_mac::network_change_calculator_params_mac()
    {
        network_change_calculator_params params;
        // Delay values arrived at by simple experimentation and adjusted so as to
        // produce a single signal when switching between network connections.
        params.ip_address_offline_delay_ = std::chrono::milliseconds(500);
        params.ip_address_online_delay_ = std::chrono::milliseconds(1000);
        params.connection_type_offline_delay_ = std::chrono::milliseconds(1000);
        params.connection_type_online_delay_ = std::chrono::milliseconds(500);
        return params;
    }

    network_change_notifier::connection_type network_change_notifier_mac::get_current_connection_type() const
    {
        assert(g_core->is_core_thread());
        std::unique_lock<std::mutex> lock(connection_type_mutex_);
        // Make sure the initial connection type is set before returning.
        if (!connection_type_initialized_)
            initial_connection_type_cv_.wait(lock, [this] { return connection_type_initialized_; });

        return connection_type_;
    }

    void network_change_notifier_mac::forwarder::init()
    {
        net_config_watcher_->set_initial_connection_type_mac();
    }

    // static
    network_change_notifier::connection_type network_change_notifier_mac::calculate_connection_type(SCNetworkConnectionFlags _flags)
    {
        bool reachable = calculate_reachability(_flags);
        if (!reachable)
            return CONNECTION_NONE;

        return connection_type_from_interfaces();
    }

    void network_change_notifier_mac::forwarder::start_reachability_notifications()
    {
        net_config_watcher_->start_reachability_notifications();
    }

    void network_change_notifier_mac::forwarder::set_dynamic_store_notification_keys(SCDynamicStoreRef _store)
    {
        net_config_watcher_->set_dynamic_store_notification_keys(_store);
    }

    void network_change_notifier_mac::forwarder::on_network_config_change(CFArrayRef _changed_keys)
    {
        net_config_watcher_->on_network_config_change(_changed_keys);
    }

    void network_change_notifier_mac::set_initial_connection_type_mac()
    {
        // Called on notifier thread.
        assert(!g_core->is_core_thread());
        struct sockaddr_in addr = { 0 };
        addr.sin_len = sizeof(addr);
        addr.sin_family = AF_INET;
        reachability_.reset(SCNetworkReachabilityCreateWithAddress(kCFAllocatorDefault, reinterpret_cast<struct sockaddr*>(&addr)));

        SCNetworkConnectionFlags flags;
        connection_type conn_type = CONNECTION_UNKNOWN;
        if (SCNetworkReachabilityGetFlags(reachability_, &flags))
            conn_type = calculate_connection_type(flags);
        else 
            log_text("Could not get initial network connection type, assuming online.");
        
        {
            {
                std::scoped_lock lock(connection_type_mutex_);
                connection_type_ = conn_type;
                connection_type_initialized_ = true;
            }
            initial_connection_type_cv_.notify_all();

            auto initial_setter = [conn_type, wr_this = weak_from_this()]
            {
                auto ptr_this = wr_this.lock();
                if (!ptr_this)
                    return;
                auto ptr_this_mac = std::static_pointer_cast<network_change_notifier_mac>(ptr_this);
                ptr_this_mac->set_initial_connection_type(conn_type);

            };
            g_core->execute_core_context(initial_setter);

        }
    }

    void network_change_notifier_mac::start_reachability_notifications()
    {
        // Called on notifier thread.
        run_loop_.reset(CFRunLoopGetCurrent());
        CFRetain(run_loop_.get());

        assert(reachability_);
        SCNetworkReachabilityContext reachability_context = {
            0,     // version
            this,  // user data
            NULL,  // retain
            NULL,  // release
            NULL   // description
        };
        if (!SCNetworkReachabilitySetCallback(
            reachability_, &network_change_notifier_mac::reachability_callback,
            &reachability_context))
        {
            log_text("Could not set network reachability callback");
            reachability_.reset();
        }
        else if (!SCNetworkReachabilityScheduleWithRunLoop(reachability_, run_loop_,
            kCFRunLoopCommonModes))
        {
            log_text("Could not schedule network reachability on run loop");
            reachability_.reset();
        }
    }

    void network_change_notifier_mac::set_dynamic_store_notification_keys(SCDynamicStoreRef _store)
    {
        internal::scoped_cftype_ref<CFMutableArrayRef> notification_keys(CFArrayCreateMutable(kCFAllocatorDefault, 0, &kCFTypeArrayCallBacks));
        internal::scoped_cftype_ref<CFStringRef> key(SCDynamicStoreKeyCreateNetworkGlobalEntity(NULL, kSCDynamicStoreDomainState, kSCEntNetInterface));
        CFArrayAppendValue(notification_keys.get(), key.get());
        key.reset(SCDynamicStoreKeyCreateNetworkGlobalEntity(NULL, kSCDynamicStoreDomainState, kSCEntNetIPv4));
        CFArrayAppendValue(notification_keys.get(), key.get());
        key.reset(SCDynamicStoreKeyCreateNetworkGlobalEntity(NULL, kSCDynamicStoreDomainState, kSCEntNetIPv6));
        CFArrayAppendValue(notification_keys.get(), key.get());

        // Set the notification keys.  This starts us receiving notifications.
        bool ret = SCDynamicStoreSetNotificationKeys(_store, notification_keys.get(), NULL);

        assert(ret);
    }

    void network_change_notifier_mac::on_network_config_change(CFArrayRef _changed_keys)
    {
        assert(run_loop_.get() == CFRunLoopGetCurrent());

        for (CFIndex i = 0; i < CFArrayGetCount(_changed_keys); ++i)
        {
            CFStringRef key = static_cast<CFStringRef>(CFArrayGetValueAtIndex(_changed_keys, i));
            if (CFStringHasSuffix(key, kSCEntNetIPv4) || CFStringHasSuffix(key, kSCEntNetIPv6))
            {
                auto notify_task = [wr_this = weak_from_this()]
                {
                    auto ptr_this = wr_this.lock();
                    if (!ptr_this)
                        return;
                    auto ptr_this_mac = std::static_pointer_cast<network_change_notifier_mac>(ptr_this);
                    ptr_this_mac->notify_of_ip_address_change();
                };
                g_core->execute_core_context(notify_task);
                return;
            }
        }
    }

    // static
    void network_change_notifier_mac::reachability_callback(SCNetworkReachabilityRef _target, SCNetworkConnectionFlags _flags, void* _notifier)
    {
        assert(!g_core->is_core_thread());
        network_change_notifier_mac* notifier_mac = static_cast<network_change_notifier_mac*>(_notifier);

        assert(notifier_mac->run_loop_.get() == CFRunLoopGetCurrent());
        connection_type new_type = calculate_connection_type(_flags);
        connection_type old_type;
        {
            std::scoped_lock<std::mutex> lock(notifier_mac->connection_type_mutex_);
            old_type = std::exchange(notifier_mac->connection_type_, new_type);
        }
        if (old_type != new_type)
        {
            auto notify_task = [notifier_mac]
            {
                notifier_mac->notify_of_connection_type_change();
            };
            g_core->execute_core_context(notify_task);
        }

    }

}  // namespace net
