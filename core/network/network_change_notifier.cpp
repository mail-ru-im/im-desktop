// Copyright (c) 2012 The Chromium Authors. All rights reserved.

#include "stdafx.h"

#include "network_change_notifier.h"

#include <memory>

#include "../core.h"
#include "../tools/features.h"
#include "../../common.shared/string_utils.h"
#include "internal/network_interfaces.h"

#if defined(_WIN32)
#include "win32/network_change_notifier_win.h"
#elif defined(__linux__)
#include "linux/network_change_notifier_linux.h"
#elif defined(__APPLE__)
#include "macos/network_change_notifier_mac.h"
#endif

namespace core
{
    network_change_notifier::network_change_observer::network_change_observer() = default;
    network_change_notifier::network_change_observer::~network_change_observer() = default;

    network_change_notifier::network_change_calculator_params::network_change_calculator_params() = default;

    class network_change_notifier::network_change_calculator : public std::enable_shared_from_this<network_change_notifier::network_change_calculator>
    {
    public:
        explicit network_change_calculator(const network_change_calculator_params& _params)
            : params_(_params),
              have_announced_(false),
              last_announced_connection_type_(CONNECTION_NONE),
              pending_connection_type_(CONNECTION_NONE),
              timer_(-1)
        {

        }

        ~network_change_calculator()
        {
            if (timer_ != -1 && g_core)
                g_core->stop_timer(timer_);
        }

        void set_initial_connection_type(connection_type type);

        void set_notifier(std::weak_ptr<network_change_notifier> _notifier)
        {
            assert(g_core->is_core_thread());
            notifier_ = _notifier;
        }

        void on_ip_address_changed(connection_type type)
        {
            assert(g_core->is_core_thread());
            pending_connection_type_ = type;
            auto delay = last_announced_connection_type_ == CONNECTION_NONE ? params_.ip_address_offline_delay_ : params_.ip_address_online_delay_;
            // Cancels any previous timer.
            notify_after_delay(delay);
        }

        void on_connection_type_changed(connection_type type)
        {
            assert(g_core->is_core_thread());
            pending_connection_type_ = type;
            auto delay = last_announced_connection_type_ == CONNECTION_NONE ? params_.connection_type_offline_delay_ : params_.connection_type_online_delay_;
            // Cancels any previous timer.
            notify_after_delay(delay);
        }

        void notify_after_delay(std::chrono::milliseconds delay)
        {
            auto cb = [wr_this = weak_from_this()]
            {
                auto ptr_this = wr_this.lock();
                if (!ptr_this)
                    return;

                g_core->stop_timer(ptr_this->timer_);
                ptr_this->timer_ = -1;
                ptr_this->notify();
            };

            if (timer_ != -1)
                g_core->stop_timer(timer_);
            timer_ = g_core->add_timer({ cb }, delay);
        }

    private:
        void notify()
        {
            // Don't bother signaling about dead connections.
            if (have_announced_ && (last_announced_connection_type_ == CONNECTION_NONE) && (pending_connection_type_ == CONNECTION_NONE))
                return;

            auto notifier = notifier_.lock();
            if (notifier)
            {
                auto network_change_log = su::concat(connection_type_to_string(last_announced_connection_type_), " -> ", connection_type_to_string(pending_connection_type_));
                if (pending_connection_type_ == CONNECTION_NONE && last_announced_connection_type_ != CONNECTION_NONE)
                {
                    notifier->notify_network_down();
                    log_text(su::concat("Notify about network down: ", network_change_log));
                }
                else if (pending_connection_type_ != CONNECTION_NONE && last_announced_connection_type_ == CONNECTION_NONE)
                {
                    notifier->notify_network_up(pending_connection_type_);
                    log_text(su::concat("Notify about network up: ", network_change_log));
                }
                else
                {
                    notifier->notify_network_change(pending_connection_type_);
                    log_text(su::concat("Notify about change network connection type: ", network_change_log));
                }
            }

            have_announced_ = true;
            last_announced_connection_type_ = pending_connection_type_;
        }

        const network_change_calculator_params params_;

        std::weak_ptr<network_change_notifier> notifier_;

        // Indicates if notify_observers_of_network_change has been called yet.
        bool have_announced_;
        // Last value passed to notify_observers_of_network_change.
        connection_type last_announced_connection_type_;
        // Value to pass to notify_observers_of_network_change when notify is called.
        connection_type pending_connection_type_;
        // Used to delay notifications so duplicates can be combined.
        int timer_;

    };

    network_change_notifier::network_change_notifier(const network_change_calculator_params& _params, std::unique_ptr<network_change_observer> _observer)
        : network_change_calculator_(std::make_shared<network_change_calculator>(_params)),
          observer_(std::move(_observer))
    {

    }

    std::string_view network_change_notifier::connection_type_to_string(connection_type _type)
    {
        switch (_type)
        {
        case CONNECTION_UNKNOWN:
            return "CONNECTION_UNKNOWN";
        case CONNECTION_ETHERNET:
            return "CONNECTION_ETHERNET";
        case CONNECTION_WIFI:
            return "CONNECTION_WIFI";
        case CONNECTION_NONE:
            return "CONNECTION_NONE";
        default:
            return "[Error when detect connection type]";
        }
    }

    network_change_notifier::~network_change_notifier()
    {
        log_text("Stop service");
    }

    // static
    bool network_change_notifier::is_enabled()
    {
        return ::features::is_notify_network_change_enabled();
    }

    std::shared_ptr<network_change_notifier> network_change_notifier::create(std::unique_ptr<network_change_observer> _observer)
    {
        assert(g_core->is_core_thread());
#if defined(_WIN32)
        std::shared_ptr<network_change_notifier> notifier = std::make_shared<network_change_notifier_win>(std::move(_observer));
#elif defined(__linux__)
        std::shared_ptr<network_change_notifier> notifier = std::make_shared<network_change_notifier_linux>(std::move(_observer));
#elif defined(__APPLE__)
        std::shared_ptr<network_change_notifier> notifier = std::make_shared<network_change_notifier_mac>(std::move(_observer));
#else
        assert(false);
#endif
        log_text("Startup service");
        notifier->init();
        return notifier;
    }

    void network_change_notifier::init()
    {
        assert(g_core->is_core_thread());
        network_change_calculator_->set_notifier(weak_from_this());
    }

    network_change_notifier::connection_type network_change_notifier::connection_type_from_interface_list(const network_interface_list& _interfaces)
    {
        bool first = true;
        connection_type result = CONNECTION_NONE;
        for (size_t i = 0; i < _interfaces.size(); ++i)
        {
#if defined(_WIN32)
            if (_interfaces[i].friendly_name_ == "Teredo Tunneling Pseudo-Interface")
                continue;
#endif
#if defined(__APPLE__)
            // Ignore link-local addresses as they aren't globally routable.
            // Mac assigns these to disconnected interfaces like tunnel interfaces
            // ("utun"), airdrop interfaces ("awdl"), and ethernet ports ("en").
            if (_interfaces[i].address_.is_link_local())
                continue;
#endif

            // Remove VMware & VirtualBox network interfaces as they're internal and should not be
            // used to determine the network connection type.
            auto name_copy = _interfaces[i].friendly_name_;
            std::transform(name_copy.begin(), name_copy.end(), name_copy.begin(), [](auto c) { return std::tolower(c); });
            if ((name_copy.find("vmnet") != std::string::npos) || (name_copy.find("virtualbox host") != std::string::npos))
                continue;

            if (first)
            {
                first = false;
                result = _interfaces[i].type_;
            }
            else if (result != _interfaces[i].type_)
                return CONNECTION_UNKNOWN;
        }
        return result;
    }

    bool core::network_change_notifier::is_offline()
    {
        assert(g_core->is_core_thread());
        return get_current_connection_type() == CONNECTION_NONE;
    }

#if !defined(__linux__)
    network_change_notifier::connection_type network_change_notifier::connection_type_from_interfaces()
    {
        network_interface_list interfaces;
        if (!get_network_list(&interfaces, EXCLUDE_HOST_SCOPE_VIRTUAL_INTERFACES))
            return CONNECTION_UNKNOWN;
        return connection_type_from_interface_list(interfaces);
    }
#endif
    void network_change_notifier::set_initial_connection_type(connection_type _type)
    {
        assert(g_core->is_core_thread());
        network_change_calculator_->set_initial_connection_type(_type);
    }

    void core::network_change_notifier::notify_of_ip_address_change()
    {
        assert(g_core->is_core_thread());
        auto type = get_current_connection_type();
        network_change_calculator_->on_ip_address_changed(type);
    }

    void core::network_change_notifier::notify_of_connection_type_change()
    {
        assert(g_core->is_core_thread());
        auto type = get_current_connection_type();
        network_change_calculator_->on_connection_type_changed(type);
    }

    void network_change_notifier::notify_network_down()
    {
        assert(g_core->is_core_thread());
        observer_->on_network_down();
    }

    void network_change_notifier::notify_network_up(connection_type _type)
    {
        assert(g_core->is_core_thread());
        observer_->on_network_up();
    }

    void network_change_notifier::notify_network_change(connection_type _type)
    {
        assert(g_core->is_core_thread());
        observer_->on_network_change();
    }

    void network_change_notifier::log_text(std::string_view _text)
    {
        // g_core don't exist in ~core_dispather() call
        if (g_core)
            g_core->write_string_to_network_log(su::concat("[network_change_notifier]: ", _text));
    }

    void network_change_notifier::network_change_calculator::set_initial_connection_type(connection_type _type)
    {
        last_announced_connection_type_ = _type;
    }

} // namespace core;

