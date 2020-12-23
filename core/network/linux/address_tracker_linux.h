// Copyright (c) 2012 The Chromium Authors. All rights reserved.

#pragma once

#include <sys/socket.h>  // Needed to include netlink.
// Mask superfluous definition of |struct net|. This is fixed in Linux 2.6.38.
#define net net_kernel
#include <linux/rtnetlink.h>
#undef net
#include <stddef.h>

#include <map>
#include <memory>
#include <string>
#include <unordered_set>
#include <thread>
#include <functional>

#include "../network_change_notifier.h"
#include "../internal/ip_address.h"

namespace core
{
    namespace internal
    {

        // Keeps track of network interface addresses using rtnetlink. Used by
        // network_change_notifier to provide signals to registered IPAddressObservers.
        class address_tracker_linux
        {
        public:
            typedef std::map<ip_address, struct ifaddrmsg> address_map;

            // Tracking version constructor: it will run |address_callback| when
            // the AddressMap changes, |link_callback| when the list of online
            // links changes, and |tunnel_callback| when the list of online
            // tunnels changes.
            // |ignored_interfaces| is the list of interfaces to ignore.  Changes to an
            // ignored interface will not cause any callback to be run. An ignored
            // interface will not have entries in GetAddressMap() and GetOnlineLinks().
            // NOTE: Only ignore interfaces not used to connect to the internet. Adding
            // interfaces used to connect to the internet can cause critical network
            // changed signals to be lost allowing incorrect stale state to persist.

            address_tracker_linux(std::function<void()> _address_callback, std::function<void()> _link_callback, std::function<void()> _tunnel_callback, std::unordered_set<std::string> _ignored_interfaces);

            virtual ~address_tracker_linux();

            // In tracking mode, it starts watching the system configuration for
            // changes. The current thread must have a MessageLoopForIO. In
            // non-tracking mode, once Init() returns, a snapshot of the system
            // configuration is available through GetOnlineLinks() and
            // GetAddressMap().
            void init(std::function<void(network_change_notifier::connection_type)> _init_callback = [](network_change_notifier::connection_type){});

            address_map get_address_map() const;

            // Returns set of interface indicies for online interfaces.
            std::unordered_set<int> get_online_links() const;

            // Implementation of NetworkChangeNotifierLinux::GetCurrentConnectionType().
            // Safe to call from any thread, but will block until Init() has completed.
            network_change_notifier::connection_type get_current_connection_type() const;

            // Returns the name for the interface with interface index |interface_index|.
            // |buf| should be a pointer to an array of size IFNAMSIZ. The returned
            // pointer will point to |buf|. This function acts like if_indextoname which
            // cannot be used as net/if.h cannot be mixed with linux/if.h. We'll stick
            // with exclusively talking to the kernel and not the C library.
            static char* get_interface_name(int _interface_index, char* _buf);

            // Does |name| refer to a tunnel interface?
            static bool is_tunnel_interface_name(const char* _name);

        private:

            // A function that returns the name of an interface given the interface index
            // in |interface_index|. |ifname| should be a buffer of size IFNAMSIZ. The
            // function should return a pointer to |ifname|.
            //typedef char* (*GetInterfaceNameFunction)(int interface_index, char* ifname);

            // Sets |*address_changed| to indicate whether |address_map_| changed and
            // sets |*link_changed| to indicate if |online_links_| changed and sets
            // |*tunnel_changed| to indicate if |online_links_| changed with regards to a
            // tunnel interface while reading messages from |netlink_fd_|.
            void read_messages(bool* _address_changed, bool* _link_changed, bool* _tunnel_changed);

            // Sets |*address_changed| to true if |address_map_| changed, sets
            // |*link_changed| to true if |online_links_| changed, sets |*tunnel_changed|
            // to true if |online_links_| changed with regards to a tunnel interface while
            // reading the message from |buffer|.
            void handle_message(const char* _buffer, int _length, bool* _address_changed, bool* _link_changed, bool* _tunnel_changed);

            // Call when some part of initialization failed; forces online and unblocks.
            void abort_and_force_online();

            // Called by when |netlink_fd_| can be read without blocking on watch thrad.
            void on_file_can_read_without_blocking();

            // Does |interface_index| refer to a tunnel interface?
            bool is_tunnel_interface(int _interface_index) const;

            // Is interface with index |interface_index| in list of ignored interfaces?
            bool is_interface_ignored(int _interface_index) const;

            // Updates current_connection_type_ based on the network list.
            void update_current_connection_type();

            std::function<void()> address_callback_;
            std::function<void()> link_callback_;
            std::function<void()> tunnel_callback_;


            int netlink_fd_;

            mutable std::mutex address_map_mutex_;
            address_map address_map_;

            // Set of interface indices for links that are currently online.
            mutable std::mutex online_links_mutex_;
            std::unordered_set<int> online_links_;

            // Set of interface names that should be ignored.
            const std::unordered_set<std::string> ignored_interfaces_;

            bool connection_type_initialized_;
            mutable std::mutex connection_type_mutex_;
            mutable std::condition_variable connection_type_initialized_cv_;
            network_change_notifier::connection_type current_connection_type_;

            std::thread watcher_thread_;
            std::atomic_bool abort_watch_;

        };

    }  // namespace internal
}  // namespace core

