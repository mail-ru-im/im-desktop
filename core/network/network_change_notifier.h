// Copyright (c) 2012 The Chromium Authors. All rights reserved.

#pragma once

#include <chrono>
#include <memory>

namespace core 
{

    struct network_interface;
    typedef std::vector<network_interface> network_interface_list;

    class network_change_notifier : public std::enable_shared_from_this<network_change_notifier>
    {
    public:
        enum connection_type
        {
            CONNECTION_UNKNOWN = 0,  // A connection exists, but its type is unknown.
                                     // Also used as a default value.
            CONNECTION_ETHERNET = 1,
            CONNECTION_WIFI = 2,
            CONNECTION_NONE = 3,  // No connection.
            CONNECTION_LAST = CONNECTION_NONE
        };

        static std::string_view connection_type_to_string(connection_type _type);

        class network_change_observer
        {
        public:
            network_change_observer();
            virtual ~network_change_observer();
            virtual void on_network_down() = 0;
            virtual void on_network_up() = 0;
            virtual void on_network_change() = 0;
        };

        network_change_notifier(const network_change_notifier&) = delete;
        virtual ~network_change_notifier();
        static bool is_enabled();
        static std::shared_ptr<network_change_notifier> create(std::unique_ptr<network_change_observer> _observer);

        static connection_type connection_type_from_interface_list(const network_interface_list& _interfaces);

        bool is_offline();

        static void log_text(std::string_view _text);

    protected:
        struct network_change_calculator_params 
        {
            network_change_calculator_params();
            // Controls delay after OnIPAddressChanged when transitioning from an
            // offline state.
            std::chrono::milliseconds ip_address_offline_delay_;
            // Controls delay after OnIPAddressChanged when transitioning from an
            // online state.
            std::chrono::milliseconds ip_address_online_delay_;
            // Controls delay after OnConnectionTypeChanged when transitioning from an
            // offline state.
            std::chrono::milliseconds connection_type_offline_delay_;
            // Controls delay after OnConnectionTypeChanged when transitioning from an
            // online state.
            std::chrono::milliseconds connection_type_online_delay_;
        };
        network_change_notifier(const network_change_calculator_params& _params, std::unique_ptr<network_change_observer> _observer);
        // These are the actual implementations of the static queryable APIs.
        // See the description of the corresponding functions named without "Current".
        // Implementations must be thread-safe. Implementations must also be
        // cheap as they are called often.
        virtual connection_type get_current_connection_type() const = 0;
#if !defined(__linux__)
        static connection_type connection_type_from_interfaces();
#endif

        virtual void init();
        void set_initial_connection_type(connection_type _type);

        void notify_of_ip_address_change();
        void notify_of_connection_type_change();

        void notify_network_down();
        void notify_network_up(connection_type _type);
        void notify_network_change(connection_type _type);


    private:
        class network_change_calculator;
        // Computes network_change signal from ip_Address and connection_type signals.
        std::shared_ptr<network_change_calculator> network_change_calculator_;
    
        std::unique_ptr<network_change_observer> observer_;

    };

}

