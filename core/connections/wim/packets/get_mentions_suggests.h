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
        struct mention_suggest
        {
            std::string aimid_;
            std::string friendly_;
            std::string nick_;
        };
        using mention_suggest_vec = std::vector<mention_suggest>;

        class get_mentions_suggests : public robusto_packet, public persons_packet
        {
            int32_t init_request(std::shared_ptr<core::http_request_simple> _request) override;
            int32_t parse_results(const rapidjson::Value& _data) override;

            std::string aimid_;
            std::string pattern_;

            mention_suggest_vec suggests_;
            std::shared_ptr<core::archive::persons_map> persons_;

        public:
            get_mentions_suggests(wim_packet_params _params, const std::string& _aimId, const std::string& _pattern);

            const mention_suggest_vec& get_suggests() const;

            const std::shared_ptr<core::archive::persons_map>& get_persons() const override { return persons_; }

            bool support_async_execution() const override { return true; }
        };
    }
}
