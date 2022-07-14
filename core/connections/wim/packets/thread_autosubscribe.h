#pragma once

#include "../robusto_packet.h"

namespace core::wim
{
	struct thread_autosubscribe_params
	{
		std::string chat_id_;
		bool enable_ = false;
		bool with_existing_ = false;
	};

	class thread_autosubscribe : public robusto_packet
	{
	public:
		thread_autosubscribe(wim_packet_params _params, thread_autosubscribe_params _thread_params);

		virtual ~thread_autosubscribe() = default;
		virtual int minimal_supported_api_version() const override;
	
	private:
		virtual int32_t init_request(const std::shared_ptr<core::http_request_simple>& _request) override;

		virtual priority_t get_priority() const override { return top_priority(); }

		virtual std::string_view get_method() const override;

	private:
		thread_autosubscribe_params params_;
	};
}
