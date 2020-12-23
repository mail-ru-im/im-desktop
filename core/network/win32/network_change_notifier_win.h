// Copyright (c) 2012 The Chromium Authors. All rights reserved.

#pragma once

#include <minwinbase.h>

#include "../network_change_notifier.h"

using OVERLAPPED = _OVERLAPPED;

namespace core 
{
    class network_change_notifier_win : public network_change_notifier
    {
    public:
        explicit network_change_notifier_win(std::unique_ptr<network_change_observer> _observer);
        network_change_notifier_win(const network_change_notifier_win&) = delete;
        ~network_change_notifier_win() override;
        void watch_for_address_change();

    protected:
        virtual connection_type get_current_connection_type() const override;

    private:
        bool watch_for_address_change_internal();
        static void CALLBACK wait_done(void* param, BOOLEAN timed_out);
        bool start_watch();
        void set_current_connection_type(connection_type _connection_type);
    
        // Does the actual work to determine the current connection type.
        // It is not thread safe, see crbug.com/324913.
        static connection_type recompute_current_connection_type();
        static network_change_calculator_params network_change_calculator_params_win();

        void on_signal();

        bool is_watching() const;

        void notify();

        void notify_parent_of_connection_type_change();
        void delay_notify_parent_of_connection_type_change();

        OVERLAPPED addr_overlapped_;

        // The wait handle returned by RegisterWaitForSingleObject.
        HANDLE wait_object_ = nullptr;

        uint32_t timer_;
        int sequential_failures_;

        connection_type last_computed_connection_type_;

        int offline_polls_;

        int delay_connection_type_timer_;

        // Result of is_offline() when notify_of_connection_type_change()
        // was last called.
        bool last_announced_offline_;
    };

} // namespace core

