#pragma once

#include "../robusto_packet.h"
#include "../../../stickers/suggests.h"

namespace core
{
    namespace tools
    {
        class binary_stream;
        class http_request_simple;
    }

    namespace smartreply
    {
        class suggest;
        enum class type;
    }

    namespace wim
    {
        class get_smartreply_suggests : public robusto_packet
        {
            std::string aimid_;
            std::string text_;
            std::vector<smartreply::type> types_;
            std::vector<smartreply::suggest> suggests_;

            virtual int32_t init_request(std::shared_ptr<core::http_request_simple> _request) override;
            virtual int32_t parse_results(const rapidjson::Value& _data) override;

        public:
            get_smartreply_suggests(wim_packet_params _params, std::string _aimid, std::vector<smartreply::type> _types, std::string _text);
            virtual ~get_smartreply_suggests() = default;

            const std::vector<smartreply::suggest>& get_suggests() const { return suggests_; }
            const std::string& get_contact() const { return aimid_; }
        };
    }
}
