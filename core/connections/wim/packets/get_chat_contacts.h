#pragma once

#include "../robusto_packet.h"
#include "../chat_info.h"
#include "../persons_packet.h"

namespace core
{
    namespace tools
    {
        class http_request_simple;
    }

    namespace wim
    {
        class get_chat_contacts : public robusto_packet, public persons_packet
        {
            virtual int32_t init_request(std::shared_ptr<core::http_request_simple> _request) override;
            virtual int32_t parse_results(const rapidjson::Value& _node_results) override;
            virtual int32_t on_response_error_code() override;

            std::string aimid_;
            std::string cursor_;
            uint32_t page_size_;

            chat_contacts result_;

        public:
            get_chat_contacts(wim_packet_params _params, std::string_view _aimid, std::string_view _cursor, uint32_t _page_size);
            virtual ~get_chat_contacts();

            const std::shared_ptr<core::archive::persons_map>& get_persons() const override { return result_.get_persons(); }

            const auto& get_result() const { return result_; }
        };
    }
}
