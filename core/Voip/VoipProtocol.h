#ifndef __VOIP_PROTOCOL_H__
#define __VOIP_PROTOCOL_H__
#include "../async_task.h"
#include "../connections/wim/wim_packet.h"

namespace voip_manager {

    class VoipProtocol : public core::async_executer, public std::enable_shared_from_this<VoipProtocol> {
        std::shared_ptr<core::async_task_handlers> post_packet(
            std::shared_ptr<core::wim::wim_packet> _packet,
            std::function<void(int32_t)> _error_handler,
            std::shared_ptr<core::async_task_handlers> _handlers);

    public:
        std::shared_ptr<core::async_task_handlers> post_packet(std::shared_ptr<core::wim::wim_packet> _packet, std::function<void(int32_t)> _error_handler);

        VoipProtocol();
        virtual ~VoipProtocol();
    };

}

#endif//__VOIP_PROTOCOL_H__