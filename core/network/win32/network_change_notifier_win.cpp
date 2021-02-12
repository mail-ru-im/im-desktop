// Copyright (c) 2012 The Chromium Authors. All rights reserved.

#include "stdafx.h"

#include "network_change_notifier_win.h"

#include <iphlpapi.h>
#pragma comment(lib, "IPHLPAPI.lib")

#include <winsock2.h>

#include "winsock_util.h"
#include "../../core.h"
#include "../../../common.shared/string_utils.h"


namespace core
{
    // static
    network_change_notifier::network_change_calculator_params network_change_notifier_win::network_change_calculator_params_win()
    {
        network_change_notifier::network_change_calculator_params params;
        // Delay values arrived at by simple experimentation and adjusted so as to
        // produce a single signal when switching between network connections.
        params.ip_address_offline_delay_ = std::chrono::milliseconds(1500);
        params.ip_address_online_delay_ = std::chrono::milliseconds(1500);
        params.connection_type_offline_delay_ = std::chrono::milliseconds(1500);
        params.connection_type_online_delay_ = std::chrono::milliseconds(1000);
        return params;
    }

    network_change_notifier_win::network_change_notifier_win(std::unique_ptr<network_change_observer> _observer)
        : network_change_notifier(network_change_calculator_params_win(), std::move(_observer)),
        timer_(0),
        sequential_failures_{ 0 },
        last_computed_connection_type_(recompute_current_connection_type()),
        offline_polls_(0),
        delay_connection_type_timer_(0),
        last_announced_offline_(last_computed_connection_type_ == CONNECTION_NONE)
    {
        memset(&addr_overlapped_, 0, sizeof addr_overlapped_);
        addr_overlapped_.hEvent = WSACreateEvent();

        set_initial_connection_type(last_computed_connection_type_);

        watch_for_address_change();
    }

    network_change_notifier_win::~network_change_notifier_win()
    {
        if (is_watching())
        {
            CancelIPChangeNotify(&addr_overlapped_);

            if (!UnregisterWaitEx(wait_object_, INVALID_HANDLE_VALUE))
                log_text("UnregisterWaitEx failed");

        }
        WSACloseEvent(addr_overlapped_.hEvent);
        if (timer_ > 0 && g_core)
            g_core->stop_timer(timer_);
    }

    void network_change_notifier_win::watch_for_address_change()
    {
        assert(g_core->is_core_thread());
        if (!watch_for_address_change_internal())
        {
            ++sequential_failures_;
            // watch again ofter 500ms
            timer_ = g_core->add_timer([wr_this = weak_from_this()]
            {
                auto ptr_this = wr_this.lock();
                if (!ptr_this)
                    return;
                auto ptr_this_win = std::static_pointer_cast<network_change_notifier_win>(ptr_this);
                g_core->stop_timer(ptr_this_win->timer_);
                ptr_this_win->timer_ = 0;
                ptr_this_win->watch_for_address_change();
            }, std::chrono::milliseconds(500));
            return;
        }

        if (sequential_failures_ > 0)
        {
            g_core->execute_core_context([wr_this = weak_from_this()]
            {
                auto ptr_this = wr_this.lock();
                if (!ptr_this)
                    return;
                auto ptr_this_win = std::static_pointer_cast<network_change_notifier_win>(ptr_this);
                ptr_this_win->notify();
            });
        }

        sequential_failures_ = 0;
    }

    network_change_notifier::connection_type network_change_notifier_win::get_current_connection_type() const
    {
        return last_computed_connection_type_;
    }

    bool network_change_notifier_win::watch_for_address_change_internal()
    {
        assert(g_core->is_core_thread());
        internal::reset_event_if_signaled(addr_overlapped_.hEvent);
        HANDLE handle = nullptr;
        DWORD ret = NotifyAddrChange(&handle, &addr_overlapped_);
        if (ret != ERROR_IO_PENDING)
            return false;

        start_watch();

        return true;
    }

    void CALLBACK network_change_notifier_win::wait_done(void* param, BOOLEAN timed_out)
    {
        if (!g_core)
            return;
        assert(!g_core->is_core_thread());
        g_core->execute_core_context([param]
        {
            // ignore event when destroy network_change_notifier by omicron config
            if (!g_core->is_network_change_notifier_valid())
                return;
            network_change_notifier_win* that = static_cast<network_change_notifier_win*>(param);
            that->on_signal();
        });
    }

    bool network_change_notifier_win::start_watch()
    {
        assert(g_core->is_core_thread());
        DWORD wait_flags = WT_EXECUTEINWAITTHREAD | WT_EXECUTEONLYONCE;
        if (!RegisterWaitForSingleObject(&wait_object_, addr_overlapped_.hEvent, wait_done, this, INFINITE, wait_flags))
        {
            wait_object_ = nullptr;
            return false;
        }

        return true;
    }

    void core::network_change_notifier_win::set_current_connection_type(connection_type _connection_type)
    {
        assert(g_core->is_core_thread());
        last_computed_connection_type_ = _connection_type;
    }

    network_change_notifier::connection_type network_change_notifier_win::recompute_current_connection_type()
    {
        assert(g_core->is_core_thread());
        internal::ensure_winsock_init();

        HANDLE ws_handle;
        WSAQUERYSET query_set = { 0 };
        query_set.dwSize = sizeof(WSAQUERYSET);
        query_set.dwNameSpace = NS_NLA;
        // Initiate a client query to iterate through the
        // currently connected networks.
        if (0 != WSALookupServiceBegin(&query_set, LUP_RETURN_ALL, &ws_handle))
        {
            log_text(su::concat("WSALookupServiceBegin failed with: ", std::to_string(WSAGetLastError())));
            return network_change_notifier::CONNECTION_UNKNOWN;
        }

        bool found_connection = false;

        // Retrieve the first available network. In this function, we only
        // need to know whether or not there is network connection.
        // Allocate 256 bytes for name, it should be enough for most cases.
        // If the name is longer, it is OK as we will check the code returned and
        // set correct network status.
        char result_buffer[sizeof(WSAQUERYSET) + 256] = { 0 };
        DWORD length = sizeof(result_buffer);
        reinterpret_cast<WSAQUERYSET*>(&result_buffer[0])->dwSize = sizeof(WSAQUERYSET);
        int result = WSALookupServiceNext(ws_handle, LUP_RETURN_NAME, &length, reinterpret_cast<WSAQUERYSET*>(&result_buffer[0]));

        if (result == 0)
        {
            // Found a connection!
            found_connection = true;
        }
        else
        {
            assert(SOCKET_ERROR == result);
            result = WSAGetLastError();

            // Error code WSAEFAULT means there is a network connection but the
            // result_buffer size is too small to contain the results. The
            // variable "length" returned from WSALookupServiceNext is the minimum
            // number of bytes required. We do not need to retrieve detail info,
            // it is enough knowing there was a connection.
            if (result == WSAEFAULT)
            {
                found_connection = true;
            }
            else if (result == WSA_E_NO_MORE || result == WSAENOMORE)
            {
                // There was nothing to iterate over!
            }
            else
            {
                log_text(su::concat("WSALookupServiceNext() failed with: ", std::to_string(result)));
            }
        }

        result = WSALookupServiceEnd(ws_handle);
        if (result != 0)
            log_text(su::concat("WSALookupServiceEnd() failed with: ", std::to_string(result)));

        return found_connection ? connection_type_from_interfaces() : network_change_notifier::CONNECTION_NONE;
    }

    void network_change_notifier_win::on_signal()
    {
        assert(g_core->is_core_thread());
        log_text("internal: interface changed");
        watch_for_address_change();

        notify();
    }

    bool core::network_change_notifier_win::is_watching() const
    {
        return wait_object_ != nullptr;
    }

    void core::network_change_notifier_win::notify()
    {
        assert(g_core->is_core_thread());
        auto current_connection_type = recompute_current_connection_type();
        set_current_connection_type(current_connection_type);

        notify_of_ip_address_change();

        delay_notify_parent_of_connection_type_change();
    }

    void network_change_notifier_win::notify_parent_of_connection_type_change()
    {
        auto current_connection_type = recompute_current_connection_type();
        set_current_connection_type(current_connection_type);
        bool current_offline = is_offline();
        offline_polls_++;
        // If we continue to appear offline, delay sending out the notification in
        // case we appear to go online within 20 seconds.  UMA histogram data shows
        // we may not detect the transition to online state after 1 second but within
        // 20 seconds we generally do.
        if (last_announced_offline_ && current_offline && offline_polls_ <= 20)
        {
            delay_notify_parent_of_connection_type_change();
            return;
        }
        last_announced_offline_ = current_offline;

        notify_of_connection_type_change();
    }

    void network_change_notifier_win::delay_notify_parent_of_connection_type_change()
    {
        auto type_change_task = [wr_this = weak_from_this()]()
        {
            auto ptr_this = wr_this.lock();
            if (!ptr_this)
                return;
            auto ptr_this_win = std::static_pointer_cast<network_change_notifier_win>(ptr_this);
            g_core->stop_timer(ptr_this_win->delay_connection_type_timer_);
            ptr_this_win->notify_parent_of_connection_type_change();
        };
        if (delay_connection_type_timer_ != -1)
            g_core->stop_timer(delay_connection_type_timer_);
        delay_connection_type_timer_ = g_core->add_timer(type_change_task, std::chrono::seconds(1));
    }

}
