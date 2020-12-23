// Copyright (c) 2012 The Chromium Authors. All rights reserved.

#include "address_tracker_linux.h"

#include <utility>
#include <errno.h>
#include <net/if.h>
#include <stdint.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <sys/epoll.h>

#include "network_interfaces_linux.h"
#include "../../core.h"
#include "../../utils.h"
#include "../../../common.shared/string_utils.h"

// For some reason, glibc doesn't include newer flags from linux/if.h
// However, we cannot include linux/if.h directly as it conflicts
// with the glibc version
#ifndef IFF_LOWER_UP
#define IFF_LOWER_UP	0x10000		// driver signals L1 up
#endif

namespace core
{
    namespace internal
    {
        namespace
        {

            // Some kernel functions such as wireless_send_event and rtnetlink_ifinfo_prep
            // may send spurious messages over rtnetlink. RTM_NEWLINK messages where
            // ifi_change == 0 and rta_type == IFLA_WIRELESS should be ignored.
            bool ignore_wireless_change(const struct ifinfomsg* _msg, int _length)
            {
                for (const struct rtattr* attr = IFLA_RTA(_msg); RTA_OK(attr, _length); attr = RTA_NEXT(attr, _length))
                {
                    if (attr->rta_type == IFLA_WIRELESS && _msg->ifi_change == 0)
                        return true;
                }
                return false;
            }

            // Retrieves address from NETLINK address message.
            // Sets |really_deprecated| for IPv6 addresses with preferred lifetimes of 0.
            // Precondition: |header| must already be validated with NLMSG_OK.
            bool get_address(const struct nlmsghdr* _header, int _header_length, ip_address* _out, bool* _really_deprecated)
            {
                if (_really_deprecated)
                    *_really_deprecated = false;

                // Extract the message and update |header_length| to be the number of
                // remaining bytes.
                const struct ifaddrmsg* msg = reinterpret_cast<const struct ifaddrmsg*>(NLMSG_DATA(_header));
                _header_length -= NLMSG_HDRLEN;

                size_t address_length = 0;
                switch (msg->ifa_family)
                {
                case AF_INET:
                    address_length = ip_address::ip_v4_address_size;
                    break;
                case AF_INET6:
                    address_length = ip_address::ip_v6_address_size;
                    break;
                default:
                    // Unknown family.
                    return false;
                }
                // Use IFA_ADDRESS unless IFA_LOCAL is present. This behavior here is based on
                // getaddrinfo in glibc (check_pf.c). Judging from kernel implementation of
                // NETLINK, IPv4 addresses have only the IFA_ADDRESS attribute, while IPv6
                // have the IFA_LOCAL attribute.
                uint8_t* address = NULL;
                uint8_t* local = NULL;
                int length = IFA_PAYLOAD(_header);
                if (length > _header_length)
                {
                    network_change_notifier::log_text("ifaddrmsg length exceeds bounds");
                    return false;
                }
                for (const struct rtattr* attr = reinterpret_cast<const struct rtattr*>(IFA_RTA(msg)); RTA_OK(attr, length); attr = RTA_NEXT(attr, length))
                {
                    switch (attr->rta_type)
                    {
                    case IFA_ADDRESS:
                        if (RTA_PAYLOAD(attr) < address_length)
                        {
                            network_change_notifier::log_text("attr does not have enough bytes to read an address");
                            return false;
                        }
                        address = reinterpret_cast<uint8_t*>(RTA_DATA(attr));
                        break;
                    case IFA_LOCAL:
                        if (RTA_PAYLOAD(attr) < address_length)
                        {
                            network_change_notifier::log_text("attr does not have enough bytes to read an address");
                            return false;
                        }
                        local = reinterpret_cast<uint8_t*>(RTA_DATA(attr));
                        break;
                    case IFA_CACHEINFO:
                    {
                        if (RTA_PAYLOAD(attr) < sizeof(struct ifa_cacheinfo))
                        {
                            network_change_notifier::log_text("attr does not have enough bytes to read an ifa_cacheinfo");
                            return false;
                        }
                        const struct ifa_cacheinfo* cache_info = reinterpret_cast<const struct ifa_cacheinfo*>(RTA_DATA(attr));
                        if (_really_deprecated)
                            *_really_deprecated = (cache_info->ifa_prefered == 0);
                    }
                    break;
                    default:
                        break;
                    }
                }
                if (local)
                    address = local;
                if (!address)
                    return false;
                *_out = ip_address(address, address_length);
                return true;
            }

            // SafelyCastNetlinkMsgData<T> performs a bounds check before casting |header|'s
            // data to a |T*|. When the bounds check fails, returns nullptr.
            template <typename T>
            T* safely_cast_netlink_msg_data(const struct nlmsghdr* _header, int _length)
            {
                assert(NLMSG_OK(_header, static_cast<__u32>(_length)));
                if (_length <= 0 || static_cast<size_t>(_length) < NLMSG_HDRLEN + sizeof(T))
                    return nullptr;
                return reinterpret_cast<const T*>(NLMSG_DATA(_header));
            }

        }  // namespace

        // static
        char* address_tracker_linux::get_interface_name(int _interface_index, char* _buf)
        {
            memset(_buf, 0, IFNAMSIZ);
            int ioctl_socket = socket_for_ioctl();
            if (ioctl_socket < 0)
                return _buf;

            struct ifreq ifr = {};
            ifr.ifr_ifindex = _interface_index;

            if (ioctl(ioctl_socket, SIOCGIFNAME, &ifr) == 0)
                strncpy(_buf, ifr.ifr_name, IFNAMSIZ - 1);
            close(ioctl_socket);
            return _buf;
        }

        address_tracker_linux::address_tracker_linux(std::function<void()> _address_callback, std::function<void()> _link_callback, std::function<void()> _tunnel_callback, std::unordered_set<std::string> _ignored_interfaces)
            : address_callback_(_address_callback),
              link_callback_(_link_callback),
              tunnel_callback_(_tunnel_callback),
              ignored_interfaces_(_ignored_interfaces),
              connection_type_initialized_(false),
              current_connection_type_(network_change_notifier::CONNECTION_NONE),
              abort_watch_(false)
        {

        }

        address_tracker_linux::~address_tracker_linux()
        {
            abort_watch_ = true;
            watcher_thread_.join();

            if (netlink_fd_ != -1)
                close(netlink_fd_);
        }

        void address_tracker_linux::init(std::function<void(network_change_notifier::connection_type)> _init_callback)
        {
            assert(g_core->is_core_thread());
            netlink_fd_ = socket(AF_NETLINK, SOCK_RAW, NETLINK_ROUTE);
            if (netlink_fd_ == -1)
            {
                network_change_notifier::log_text("Could not create NETLINK socket");
                abort_and_force_online();
                return;
            }
            int rv;

            rv = fcntl(netlink_fd_, F_SETFL, O_NONBLOCK);
            if (rv < 0)
            {
                network_change_notifier::log_text("Could not fcntl NETLINK socket");
                abort_and_force_online();
                return;
            }

            // Request notifications.
            struct sockaddr_nl addr = {};
            addr.nl_family = AF_NETLINK;
            addr.nl_pid = getpid();
            addr.nl_groups = RTMGRP_IPV4_IFADDR | RTMGRP_IPV6_IFADDR | RTMGRP_NOTIFY | RTMGRP_LINK;
            rv = bind(netlink_fd_, reinterpret_cast<struct sockaddr*>(&addr), sizeof(addr));
            if (rv < 0)
            {
                network_change_notifier::log_text("Could not bind NETLINK socket");
                abort_and_force_online();
                return;
            }


            // Request dump of addresses.
            struct sockaddr_nl peer = {};
            peer.nl_family = AF_NETLINK;

            struct
            {
                struct nlmsghdr header;
                struct rtgenmsg msg;
            } request = {};

            request.header.nlmsg_len = NLMSG_LENGTH(sizeof(request.msg));
            request.header.nlmsg_type = RTM_GETADDR;
            request.header.nlmsg_flags = NLM_F_REQUEST | NLM_F_DUMP;
            request.header.nlmsg_pid = getpid();
            request.msg.rtgen_family = AF_UNSPEC;

            rv = sendto(netlink_fd_, &request, request.header.nlmsg_len, 0, reinterpret_cast<struct sockaddr*>(&peer), sizeof(peer));
            if (rv < 0)
            {
                network_change_notifier::log_text("Could not send to NETLINK socket");
                abort_and_force_online();
                return;
            }

            // Consume pending message to populate the AddressMap, but don't notify.
            // Sending another request without first reading responses results in EBUSY.
            bool address_changed;
            bool link_changed;
            bool tunnel_changed;
            read_messages(&address_changed, &link_changed, &tunnel_changed);

            // Request dump of link state
            request.header.nlmsg_type = RTM_GETLINK;

            rv = sendto(netlink_fd_, &request, request.header.nlmsg_len, 0, reinterpret_cast<struct sockaddr*>(&peer), sizeof(peer));
            if (rv < 0)
            {
                network_change_notifier::log_text("Could not send NETLINK request");
                abort_and_force_online();
                return;
            }

            // Consume pending message to populate links_online_, but don't notify.
            read_messages(&address_changed, &link_changed, &tunnel_changed);
            {
                std::scoped_lock<std::mutex> lock(connection_type_mutex_);
                connection_type_initialized_ = true;
            }
            connection_type_initialized_cv_.notify_all();

            _init_callback(current_connection_type_);

            auto wacher_task = [this]
            {
                utils::set_this_thread_name("Address tracker linux");
                int epollfd = epoll_create1(0);
                if (epollfd == -1)
                {
                    network_change_notifier::log_text("epoll_create fail");
                    return;
                }
                int nfds;
                const int event_count = 1;
                struct epoll_event event, events[event_count];
                event.events = EPOLLIN | EPOLLET | EPOLLHUP | EPOLLERR;
                event.data.fd = netlink_fd_;

                if (epoll_ctl(epollfd, EPOLL_CTL_ADD, netlink_fd_, &event) == -1)
                {
                    network_change_notifier::log_text("epoll_ctl fail");
                    return;
                }

                for (;;)
                {
                    nfds = epoll_wait(epollfd, events, event_count, 1000);
                    if (abort_watch_.load())
                        return;
                    if (nfds < 0 && errno != EINTR)
                        network_change_notifier::log_text(su::concat("epoll_wait error", std::to_string(errno)));
                    else if (nfds > 0)
                        on_file_can_read_without_blocking();

                    // watch again
                    if (abort_watch_.load())
                        return;
                }
            };

            watcher_thread_ = std::thread(wacher_task);

        }

        void address_tracker_linux::abort_and_force_online()
        {
            assert(g_core->is_core_thread());
            abort_watch_ = true;
            netlink_fd_ = -1;
            current_connection_type_ = network_change_notifier::CONNECTION_UNKNOWN;
        }

        address_tracker_linux::address_map address_tracker_linux::get_address_map() const
        {
            std::scoped_lock<std::mutex> lock(address_map_mutex_);
            return address_map_;
        }

        network_change_notifier::connection_type address_tracker_linux::get_current_connection_type() const
        {
            assert(g_core->is_core_thread());
            std::unique_lock<std::mutex> lock(connection_type_mutex_);

            if (!connection_type_initialized_)
                connection_type_initialized_cv_.wait(lock, [this]{ return connection_type_initialized_; });

            return current_connection_type_;
        }

        std::unordered_set<int> address_tracker_linux::get_online_links() const
        {
            // call from core thread once in init
            std::scoped_lock<std::mutex> locker(online_links_mutex_);
            return online_links_;
        }

        bool address_tracker_linux::is_interface_ignored(int _interface_index) const
        {
            if (ignored_interfaces_.empty())
                return false;

            char buf[IFNAMSIZ] = { 0 };
            const char* interface_name = get_interface_name(_interface_index, buf);
            return ignored_interfaces_.find(interface_name) != ignored_interfaces_.end();
        }


        void address_tracker_linux::read_messages(bool* _address_changed, bool* _link_changed, bool* _tunnel_changed)
        {
            *_address_changed = false;
            *_link_changed = false;
            *_tunnel_changed = false;
            char buffer[4096];
            bool first_loop = true;
            {

                for (;;)
                {
                    // Block the first time through loop.
                    int rv = recv(netlink_fd_, buffer, sizeof(buffer), first_loop ? 0 : MSG_DONTWAIT);
                    first_loop = false;
                    if (rv == 0)
                    {
                        network_change_notifier::log_text("Unexpected shutdown of NETLINK socket.");
                        return;
                    }
                    if (rv < 0)
                    {
                        if ((errno == EAGAIN) || (errno == EWOULDBLOCK))
                            break;
                        network_change_notifier::log_text("Failed to recv from netlink socket");
                        return;
                    }
                    handle_message(buffer, rv, _address_changed, _link_changed, _tunnel_changed);
                }
            }
            if (*_link_changed || *_address_changed)
                update_current_connection_type();
        }

        void address_tracker_linux::handle_message(const char* _buffer, int _length, bool* _address_changed, bool* _link_changed, bool* _tunnel_changed)
        {
            assert(_buffer);
            // Note that NLMSG_NEXT decrements |length| to reflect the number of bytes
            // remaining in |buffer|.
            for (const struct nlmsghdr* header = reinterpret_cast<const struct nlmsghdr*>(_buffer); _length >= 0 && NLMSG_OK(header, static_cast<__u32>(_length)); header = NLMSG_NEXT(header, _length))
            {
                // The |header| pointer should never precede |buffer|.
                assert(_buffer <= reinterpret_cast<const char*>(header));
                switch (header->nlmsg_type)
                {
                case NLMSG_DONE:
                    return;
                case NLMSG_ERROR:
                {
                    const struct nlmsgerr* msg = safely_cast_netlink_msg_data<const struct nlmsgerr>(header, _length);
                    if (msg == nullptr)
                        return;
                    network_change_notifier::log_text(su::concat("Unexpected netlink error ", std::to_string(msg->error), "."));
                } return;
                case RTM_NEWADDR:
                {
                    ip_address address;
                    bool really_deprecated;
                    const struct ifaddrmsg* msg = safely_cast_netlink_msg_data<const struct ifaddrmsg>(header, _length);
                    if (msg == nullptr)
                        return;
                    if (is_interface_ignored(msg->ifa_index))
                        break;
                    if (get_address(header, _length, &address, &really_deprecated))
                    {
                        struct ifaddrmsg msg_copy = *msg;
                        // Routers may frequently (every few seconds) output the IPv6 ULA
                        // prefix which can cause the linux kernel to frequently output two
                        // back-to-back messages, one without the deprecated flag and one with
                        // the deprecated flag but both with preferred lifetimes of 0. Avoid
                        // interpretting this as an actual change by canonicalizing the two
                        // messages by setting the deprecated flag based on the preferred
                        // lifetime also.  http://crbug.com/268042
                        if (really_deprecated)
                            msg_copy.ifa_flags |= IFA_F_DEPRECATED;
                        // Only indicate change if the address is new or ifaddrmsg info has
                        // changed.
                        std::scoped_lock<std::mutex> lock(address_map_mutex_);
                        auto it = address_map_.find(address);
                        if (it == address_map_.end())
                        {
                            address_map_.insert(it, std::make_pair(address, msg_copy));
                            *_address_changed = true;
                        }
                        else if (memcmp(&it->second, &msg_copy, sizeof(msg_copy)))
                        {
                            it->second = msg_copy;
                            *_address_changed = true;
                        }
                    }
                } break;
                case RTM_DELADDR:
                {
                    ip_address address;
                    const struct ifaddrmsg* msg = safely_cast_netlink_msg_data<const struct ifaddrmsg>(header, _length);
                    if (msg == nullptr)
                        return;
                    if (is_interface_ignored(msg->ifa_index))
                        break;
                    if (get_address(header, _length, &address, nullptr))
                    {
                        std::scoped_lock<std::mutex> lock(address_map_mutex_);
                        if (address_map_.erase(address))
                            *_address_changed = true;
                    }
                } break;
                case RTM_NEWLINK:
                {
                    const struct ifinfomsg* msg = safely_cast_netlink_msg_data<const struct ifinfomsg>(header, _length);
                    if (msg == nullptr)
                        return;
                    if (is_interface_ignored(msg->ifi_index))
                        break;
                    if (ignore_wireless_change(msg, IFLA_PAYLOAD(header)))
                        break;                    
                    if (!(msg->ifi_flags & IFF_LOOPBACK) && (msg->ifi_flags & IFF_UP) && (msg->ifi_flags & IFF_LOWER_UP) && (msg->ifi_flags & IFF_RUNNING))
                    {
                        std::scoped_lock<std::mutex> locker(online_links_mutex_);
                        if (online_links_.insert(msg->ifi_index).second)
                        {
                            *_link_changed = true;
                            if (is_tunnel_interface(msg->ifi_index))
                                *_tunnel_changed = true;
                        }
                    }
                    else
                    {
                        std::scoped_lock<std::mutex> locker(online_links_mutex_);
                        if (online_links_.erase(msg->ifi_index))
                        {
                            *_link_changed = true;
                            if (is_tunnel_interface(msg->ifi_index))
                                *_tunnel_changed = true;
                        }
                    }
                } break;
                case RTM_DELLINK:
                {
                    const struct ifinfomsg* msg = safely_cast_netlink_msg_data<const struct ifinfomsg>(header, _length);
                    if (msg == nullptr)
                        return;
                    if (is_interface_ignored(msg->ifi_index))
                        break;
                    std::scoped_lock<std::mutex> locker(online_links_mutex_);
                    if (online_links_.erase(msg->ifi_index))
                    {
                        *_link_changed = true;
                        if (is_tunnel_interface(msg->ifi_index))
                            *_tunnel_changed = true;
                    }
                } break;
                default:
                    break;
                }
            }
        }

        void address_tracker_linux::on_file_can_read_without_blocking()
        {
            assert(!g_core->is_core_thread());
            bool address_changed;
            bool link_changed;
            bool tunnel_changed;
            network_change_notifier::log_text("interface changed");
            read_messages(&address_changed, &link_changed, &tunnel_changed);
            if (address_changed)
            {
                network_change_notifier::log_text("address changed");
                address_callback_();
            }
            if (link_changed)
            {
                network_change_notifier::log_text("link changed");
                link_callback_();
            }
            if (tunnel_changed)
            {
                network_change_notifier::log_text("tunnel changed");
                // tunnel change notification not used
            }
        }

        bool address_tracker_linux::is_tunnel_interface(int _interface_index) const
        {
            char buf[IFNAMSIZ] = { 0 };
            return is_tunnel_interface_name(get_interface_name(_interface_index, buf));
        }
        
        //// static
        bool address_tracker_linux::is_tunnel_interface_name(const char* _name)
        {
            // Linux kernel drivers/net/tun.c uses "tun" name prefix.
            return strncmp(_name, "tun", 3) == 0;
        }


        void address_tracker_linux::update_current_connection_type()
        {
            address_tracker_linux::address_map addr_map = get_address_map();
            std::unordered_set<int> online_links = get_online_links();

            // Strip out tunnel interfaces from online_links
            for (auto it = online_links.cbegin(); it != online_links.cend();) 
            {
                if (is_tunnel_interface(*it)) 
                    it = online_links.erase(it);                
                else 
                    ++it;                
            }

            network_interface_list networks;
            network_change_notifier::connection_type type = network_change_notifier::CONNECTION_NONE;

            if (get_network_list_impl(&networks, 0, online_links, addr_map))
                type = network_change_notifier::connection_type_from_interface_list(networks);
            else
                type = online_links.empty() ? network_change_notifier::CONNECTION_NONE : network_change_notifier::CONNECTION_UNKNOWN;

            std::scoped_lock<std::mutex> lock(connection_type_mutex_);
            current_connection_type_ = type;
        }

    }  // namespace internal
}  // namespace core
