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
        class group_inviteblacklist_search : public robusto_packet, public persons_packet
        {
            int32_t init_request(const std::shared_ptr<core::http_request_simple>& _request) override;
            int32_t parse_results(const rapidjson::Value& _node_results) override;
            bool is_status_code_ok() const override;
            bool is_search_request() const;

            std::string keyword_;
            std::string cursor_;
            uint32_t page_size_ = 0;
            bool reset_pages_ = false;

            std::vector<std::string> contacts_;
            std::shared_ptr<core::archive::persons_map> persons_;

        public:
            group_inviteblacklist_search(wim_packet_params _params, std::string_view _keyword, std::string_view _cursor, uint32_t _page_size);
            virtual ~group_inviteblacklist_search();

            const std::shared_ptr<core::archive::persons_map>& get_persons() const override { return persons_; }
            const std::vector<std::string>& get_contacts() const noexcept { return contacts_; }
            bool is_reset_pages() const noexcept { return reset_pages_; }
            const std::string& get_cursor() const noexcept { return cursor_; }
            const std::string& get_keyword() const noexcept { return keyword_; }
            bool auto_resend_on_fail() const override { return true; }
            virtual std::string_view get_method() const override;
        };
    }
}