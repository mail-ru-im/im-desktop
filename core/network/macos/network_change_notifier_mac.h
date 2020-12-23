// Copyright (c) 2012 The Chromium Authors. All rights reserved.

#pragma once

#include <SystemConfiguration/SystemConfiguration.h>

#include <memory>

#include "../network_change_notifier.h"
#include "network_config_watcher_mac.h"
#include "scoped_typeref.h"

namespace core
{

    class network_change_notifier_mac : public network_change_notifier
    {
    public:
        network_change_notifier_mac(std::unique_ptr<network_change_observer> _observer);
        ~network_change_notifier_mac() override;

        // NetworkChangeNotifier implementation:
        connection_type get_current_connection_type() const override;

        // Forwarder just exists to keep the NetworkConfigWatcherMac API out of
        // NetworkChangeNotifierMac's public API.
        class forwarder : public network_config_watcher_mac::delegate 
        {
        public:
            explicit forwarder(network_change_notifier_mac* _net_config_watcher)
                : net_config_watcher_(_net_config_watcher)
            {

            }

            // network_config_watcher_mac::delegate  implementation:
            void init() override;
            void start_reachability_notifications() override;
            void set_dynamic_store_notification_keys(SCDynamicStoreRef _store) override;
            void on_network_config_change(CFArrayRef _changed_keys) override;

        private:
            network_change_notifier_mac* const net_config_watcher_;
        };
    protected:
        virtual void init() override;

    private:
        // Called on the main thread on startup, afterwards on the notifier thread.

        static connection_type calculate_connection_type(SCNetworkConnectionFlags _flags);

        // Methods directly called by the network_config_watcher_mac::delegate :
        void start_reachability_notifications();
        void set_dynamic_store_notification_keys(SCDynamicStoreRef store);
        void on_network_config_change(CFArrayRef changed_keys);

        void set_initial_connection_type_mac();

        static void reachability_callback(SCNetworkReachabilityRef _target, SCNetworkConnectionFlags _flags, void* _notifier);

        static network_change_calculator_params network_change_calculator_params_mac();

        connection_type connection_type_;
        bool connection_type_initialized_;

        mutable std::mutex connection_type_mutex_;
        mutable std::condition_variable initial_connection_type_cv_;
        internal::scoped_cftype_ref<SCNetworkReachabilityRef> reachability_;
        internal::scoped_cftype_ref<CFRunLoopRef> run_loop_;

        forwarder forwarder_;
        std::shared_ptr<network_config_watcher_mac> config_watcher_;
    };

}  // namespace core

