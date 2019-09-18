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
        class mod_chat_alpha : public robusto_packet
        {
        private:
            virtual int32_t init_request(std::shared_ptr<core::http_request_simple> _request) override;
            virtual int32_t parse_response_data(const rapidjson::Value& _data) override;

            std::string	aimid_;
            chat_params chat_params_;

        public:
            mod_chat_alpha(wim_packet_params _params, const std::string& _aimId);
            virtual ~mod_chat_alpha();

            chat_params& get_chat_params();
            void set_chat_params(chat_params _chat_params);
        };

    }

}
