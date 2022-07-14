#pragma once

#include "../robusto_packet.h"
#include "../persons_packet.h"

namespace core
{
    namespace tools
    {
        class http_request_simple;
    }

    namespace wim
    {
        class group_inviteblacklist_in_cl : public robusto_packet, public persons_packet
        {
            int32_t init_request(const std::shared_ptr<core::http_request_simple>& _request) override;
            int32_t parse_results(const rapidjson::Value& _node_results) override;

            std::vector<std::string> contacts_;
            std::shared_ptr<core::archive::persons_map> persons_;
            std::string cursor_;
            uint32_t page_size_ = 0;

        public:
            group_inviteblacklist_in_cl(wim_packet_params _params, std::string_view _cursor, uint32_t _page_size);
            virtual ~group_inviteblacklist_in_cl();

            const std::shared_ptr<core::archive::persons_map>& get_persons() const override { return persons_; }
            const std::vector<std::string>& get_contacts() const noexcept { return contacts_; }
            const std::string& get_cursor() const noexcept { return cursor_; }
            bool auto_resend_on_fail() const override { return true; }
            std::string_view get_method() const override;
            int minimal_supported_api_version() const override;
        };
    }
}