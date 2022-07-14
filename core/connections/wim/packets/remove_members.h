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
        class remove_members : public robusto_packet
        {
            int32_t init_request(const std::shared_ptr<core::http_request_simple>& _request) override;
            int32_t parse_results(const rapidjson::Value& _node_results) override;
            int32_t on_response_error_code() override;
            bool is_status_code_ok() const override;

            std::string	aimid_;
            std::vector<std::string> members_to_delete_;

            std::map<chat_member_failure, std::vector<std::string>> failures_;

        public:
            remove_members(wim_packet_params _params, std::string _aimid, std::vector<std::string> _members_to_delete);

            const std::map<chat_member_failure, std::vector<std::string>>& get_failures() const noexcept { return failures_; }
            const std::string& get_aimid() const noexcept { return aimid_; }

            std::string_view get_method() const override;
            int minimal_supported_api_version() const override;
        };
    }
}