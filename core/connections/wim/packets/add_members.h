#pragma once

#include "../robusto_packet.h"

namespace core
{
    namespace tools
    {
        class http_request_simple;
    }

    enum class add_member_failure;
}

namespace core
{
    namespace wim
    {
        class add_members : public robusto_packet
        {
            int32_t init_request(const std::shared_ptr<core::http_request_simple>& _request) override;
            int32_t parse_results(const rapidjson::Value& _node_results) override;
            int32_t on_response_error_code() override;
            bool is_status_code_ok() const override;

            std::string	aimid_;
            std::vector<std::string> members_to_add_;
            bool unblock_ = false;

            std::map<chat_member_failure, std::vector<std::string>> failures_;

        public:
            add_members(wim_packet_params _params, std::string _aimid, std::vector<std::string> _members_to_add, bool _unblock);

            const std::map<chat_member_failure, std::vector<std::string>>& get_failures() const noexcept { return failures_; }
            const std::string& get_aimid() const noexcept { return aimid_; }

            virtual std::string_view get_method() const override;
        };
    }
}