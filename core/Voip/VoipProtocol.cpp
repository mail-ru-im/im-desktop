#include "stdafx.h"
#include "VoipProtocol.h"

#define RESEND_TIMES 5

voip_manager::VoipProtocol::VoipProtocol()
 : core::async_executer("VoipProtocol")
{

}

voip_manager::VoipProtocol::~VoipProtocol() {

}

std::shared_ptr<core::async_task_handlers> voip_manager::VoipProtocol::post_packet(std::shared_ptr<core::wim::wim_packet> packet, std::function<void(int32_t)> error_handler, std::shared_ptr<core::async_task_handlers> handlers) {
    auto __this = weak_from_this();
    std::shared_ptr<core::async_task_handlers> callback_handlers = handlers ? handlers : std::make_shared<core::async_task_handlers>();

    auto internal_handlers = run_async_function([packet] ()->int32_t {
        return packet->execute();
    });

    internal_handlers->on_result_ = [__this, packet, error_handler, callback_handlers] (int32_t error) {
        auto ptr_this = __this.lock();
        if (!ptr_this) { return; }

        if (core::wim::wim_protocol_internal_error::wpie_network_error == error  && packet->get_repeat_count() < RESEND_TIMES) {
            ptr_this->post_packet(packet, error_handler, callback_handlers);
            return;
        }

        if (callback_handlers->on_result_) {
            callback_handlers->on_result_(error);
        }

        if (0 != error && !!error_handler) {
            error_handler(error);
        }
    };

    return callback_handlers;
}


std::shared_ptr<core::async_task_handlers> voip_manager::VoipProtocol::post_packet(std::shared_ptr<core::wim::wim_packet> packet, const std::function<void(int32_t)> error_handler) {
    return post_packet(packet, error_handler, nullptr);
}
