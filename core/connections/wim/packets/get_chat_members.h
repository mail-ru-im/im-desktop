#pragma once

#include "../robusto_packet.h"
#include "../persons_packet.h"
#include "../chat_info.h"

namespace core
{
    namespace tools
    {
        class http_request_simple;
        class binary_stream;
    }

    namespace wim
    {
        class get_chat_members: public robusto_packet, public persons_packet
        {
            virtual int32_t init_request(std::shared_ptr<core::http_request_simple> _request) override;
            virtual int32_t parse_response(std::shared_ptr<core::tools::binary_stream> _response) override;
            virtual int32_t parse_results(const rapidjson::Value& _node_results) override;
            virtual int32_t on_response_error_code() override;

            std::string aimid_;
            std::string role_;
            std::string cursor_;
            uint32_t page_size_;

            bool reset_pages_;
            chat_members result_;

        public:
            get_chat_members(wim_packet_params _params, std::string_view _aimid, std::string_view _role, uint32_t _page_size, std::string_view _cursor = std::string_view());
            virtual ~get_chat_members();

            const std::shared_ptr<core::archive::persons_map>& get_persons() const override { return result_.get_persons(); }

            const auto& get_result() const { return result_; }

            bool is_reset_pages() const { return reset_pages_; }
        };
    }
}
