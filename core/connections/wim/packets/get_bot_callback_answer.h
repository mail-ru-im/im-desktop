#pragma once

#include "../robusto_packet.h"
#include "../chat_info.h"
#include "../bot_payload.h"

namespace core
{
    namespace tools
    {
        class http_request_simple;
    }
}


namespace core
{
    namespace wim
    {
        class get_bot_callback_answer : public robusto_packet
        {
            int32_t init_request(const std::shared_ptr<core::http_request_simple>& _request) override;
            int32_t parse_results(const rapidjson::Value& _node_results) override;
            bool is_status_code_ok() const override;

            std::string chat_id_;
            std::string callback_data_;
            int64_t msg_id_;

            std::string req_id_;
            bot_payload payload_;

        public:

            get_bot_callback_answer(wim_packet_params _params, const std::string_view _chat_id, const std::string_view _callback_data, int64_t _msg_id);

            virtual ~get_bot_callback_answer() = default;

            const std::string& get_bot_req_id() const noexcept;
            const bot_payload& get_payload() const noexcept;
            std::string_view get_method() const override;
            int minimal_supported_api_version() const override;
        };

    }

}