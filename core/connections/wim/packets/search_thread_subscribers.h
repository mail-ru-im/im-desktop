#pragma once

#include "../robusto_packet.h"
#include "../persons_packet.h"
#include "../thread_info.h"

namespace core
{
    namespace tools
    {
        class http_request_simple;
        class binary_stream;
    }

    namespace wim
    {
        class search_thread_subscribers : public robusto_packet, public persons_packet
        {
            int32_t init_request(const std::shared_ptr<core::http_request_simple>& _request) override;
            int32_t parse_results(const rapidjson::Value& _node_results) override;
            int32_t on_response_error_code() override;
            bool is_status_code_ok() const override;

            std::string thread_id_;
            std::string role_;
            std::string keyword_;
            uint32_t page_size_;
            std::string cursor_;

            bool reset_pages_;
            thread_subscribers result_;

        public:
            search_thread_subscribers(wim_packet_params _params, std::string_view _aimid, std::string_view _role, std::string_view _keyword, uint32_t _page_size, std::string_view _cursor = std::string_view());
            virtual ~search_thread_subscribers();

            const std::shared_ptr<core::archive::persons_map>& get_persons() const override { return result_.get_persons(); }
            const auto& get_result() const { return result_; }
            const std::string& get_role() const noexcept { return role_; }

            bool is_reset_pages() const { return reset_pages_; }

            std::string_view get_method() const override;
            int minimal_supported_api_version() const override;
        };
    }
}
