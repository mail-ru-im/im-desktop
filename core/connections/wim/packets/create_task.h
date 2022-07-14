#pragma once

#include "../robusto_packet.h"

#include "../../../tasks/task.h"

namespace core::wim
{

	class create_task : public robusto_packet
	{
	public:
		create_task(wim_packet_params _params, const core::tasks::task_data& _task);
		std::string_view get_method() const override;
		int minimal_supported_api_version() const override;
		bool auto_resend_on_fail() const override { return true; }

	private:
		int32_t init_request(const std::shared_ptr<core::http_request_simple>& _request) override;

		core::tasks::task_data task_;
	};

}
