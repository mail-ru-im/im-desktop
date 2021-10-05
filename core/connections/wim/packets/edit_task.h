#pragma once

#include "../robusto_packet.h"
#include "../../../tasks/task.h"

namespace core::wim
{
    class edit_task : public robusto_packet
    {
    public:
        edit_task(wim_packet_params _params, const core::tasks::task_change& _task);
        std::string_view get_method() const override;
        bool auto_resend_on_fail() const override { return true; }

    private:
        int32_t init_request(const std::shared_ptr<core::http_request_simple>& _request) override;

    private:
        core::tasks::task_change task_;
    };
}

