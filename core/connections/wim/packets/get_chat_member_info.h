#pragma once

#include "../robusto_packet.h"
#include "../persons_packet.h"
#include "../chat_info.h"

namespace core
{
    namespace tools
    {
        class http_request_simple;
    }

    namespace wim
    {
        class get_chat_member_info: public robusto_packet, public persons_packet
        {
            virtual int32_t init_request(std::shared_ptr<core::http_request_simple> _request) override;
            virtual int32_t parse_results(const rapidjson::Value& _node_results) override;
            virtual int32_t on_response_error_code() override;

            std::string aimid_;
            std::vector<std::string> members_;

            chat_members result_;

        public:
            get_chat_member_info(wim_packet_params _params, std::string_view _aimid, const std::vector<std::string>& _members);
            virtual ~get_chat_member_info();

            const std::shared_ptr<core::archive::persons_map>& get_persons() const override { return result_.get_persons(); }

            const auto& get_result() const { return result_; }
        };
    }
}
