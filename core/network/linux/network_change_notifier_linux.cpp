// Copyright (c) 2012 The Chromium Authors. All rights reserved.

#include "network_change_notifier_linux.h"

#include "../../core.h"

namespace core
{
    network_change_notifier_linux::network_change_notifier_linux(std::unique_ptr<network_change_observer> _observer)
        : network_change_notifier(network_change_calculator_params_linux(), std::move(_observer))
    {

    }

    network_change_notifier_linux::~network_change_notifier_linux() = default;

    network_change_notifier::connection_type network_change_notifier_linux::get_current_connection_type() const
    {
        return address_tracker_->get_current_connection_type();
    }

    void network_change_notifier_linux::init()
    {
        assert(g_core->is_core_thread());
        network_change_notifier::init();
        auto address_change_callback = [wr_this = weak_from_this()]()
        {
            g_core->execute_core_context([wr_this]
            {
                auto ptr_this = wr_this.lock();
                if (!ptr_this)
                    return;
                auto ptr_linux = std::static_pointer_cast<network_change_notifier_linux>(ptr_this);
                ptr_linux->notify_of_ip_address_change();
            });

        };
        auto link_changed_callback = [wr_this = weak_from_this()]()
        {
            g_core->execute_core_context([wr_this]
            {
                auto ptr_this = wr_this.lock();
                if (!ptr_this)
                    return;
                auto ptr_linux = std::static_pointer_cast<network_change_notifier_linux>(ptr_this);
                ptr_linux->notify_of_connection_type_change();
            });
        };
        address_tracker_ = std::make_unique<internal::address_tracker_linux>(std::move(address_change_callback), std::move(link_changed_callback), []{}, std::unordered_set<std::string>());

        auto init_callback = [wr_this = weak_from_this()](connection_type type)
        {
            g_core->execute_core_context([wr_this, type]
            {
                auto ptr_this = wr_this.lock();
                if (!ptr_this)
                    return;
                auto ptr_linux = std::static_pointer_cast<network_change_notifier_linux>(ptr_this);
                ptr_linux->set_initial_connection_type(type);
            });
        };
        address_tracker_->init(std::move(init_callback));
    }

    network_change_notifier::network_change_calculator_params network_change_notifier_linux::network_change_calculator_params_linux()
    {
        network_change_calculator_params params;
        // Delay values arrived at by simple experimentation and adjusted so as to
        // produce a single signal when switching between network connections.
        params.ip_address_offline_delay_ = std::chrono::milliseconds(2000);
        params.ip_address_online_delay_ = std::chrono::milliseconds(2000);
        params.connection_type_offline_delay_ = std::chrono::milliseconds(1500);
        params.connection_type_online_delay_ = std::chrono::milliseconds(500);
        return params;
    }

}  // namespace core
