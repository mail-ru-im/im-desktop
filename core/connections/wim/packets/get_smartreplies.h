#pragma once

#include "../robusto_packet.h"

namespace core
{
    namespace tools
    {
        class http_request_simple;
    }

    namespace smartreply
    {
        class suggest;
        enum class type;
    }

    namespace wim
    {
        class get_smartreplies : public robusto_packet
        {
            std::string aimid_;
            int64_t msgid_;
            std::vector<smartreply::type> types_;
            std::vector<smartreply::suggest> suggests_;

            int32_t init_request(const std::shared_ptr<core::http_request_simple>& _request) override;
            int32_t parse_results(const rapidjson::Value& _data) override;

        public:
            get_smartreplies(wim_packet_params _params, std::string _aimid, int64_t _msgid, std::vector<smartreply::type> _types);
            virtual ~get_smartreplies() = default;

            const std::vector<smartreply::suggest>& get_suggests() const noexcept { return suggests_; }
            const std::string& get_contact() const noexcept { return aimid_; }
            int64_t get_msgid() const noexcept { return msgid_; }
            std::string_view get_method() const override;
            int minimal_supported_api_version() const override;
        };
    }
}
