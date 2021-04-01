#pragma once

#include "../robusto_packet.h"

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
        class block_chat_member : public robusto_packet
        {
            int32_t init_request(const std::shared_ptr<core::http_request_simple>& _request) override;
            int32_t parse_results(const rapidjson::Value& _node_results) override;
            int32_t on_response_error_code() override;
            bool is_status_code_ok() const override;

            std::string aimid_;
            std::string contact_;
            bool block_;
            bool remove_messages_;

            std::map<chat_member_failure, std::vector<std::string>> failures_;

        public:
            block_chat_member(wim_packet_params _params, std::string_view _aimId, std::string_view _contact, bool _block, bool _remove_messages);
            virtual ~block_chat_member();

            const std::string& get_contact() const noexcept { return contact_; }
            const std::string& get_chat() const noexcept { return aimid_; }
            const std::map<chat_member_failure, std::vector<std::string>>& get_failures() const noexcept { return failures_; }

            std::string_view get_method() const override;
        };

    }

}
