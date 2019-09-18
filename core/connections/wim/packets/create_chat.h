#pragma once

#include "../robusto_packet.h"
#include "../chat_params.h"

namespace core
{
    namespace tools
    {
        class http_request_simple;
    }

    namespace wim
    {
        class create_chat : public robusto_packet
        {
        private:
            virtual int32_t init_request(std::shared_ptr<core::http_request_simple> _request) override;
            virtual int32_t parse_response_data(const rapidjson::Value& _data) override;

            std::string aimid_;
            std::vector<std::string> chat_members_;
            chat_params chat_params_;

        public:
            create_chat(wim_packet_params _params, const std::string& _aimId, const std::string& _chatName, std::vector<std::string> _chatMembers);
            virtual ~create_chat();

            const chat_params& get_chat_params() const;
            void set_chat_params(chat_params _chat_params);

            size_t members_count() const { return chat_members_.size(); }
        };

    }

}
