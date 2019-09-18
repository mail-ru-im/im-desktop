#pragma once

#include "../robusto_packet.h"
#include "../id_info.h"
#include "../persons_packet.h"

namespace core::tools
{
    class http_request_simple;
}

namespace core::wim
{
    class get_id_info : public robusto_packet, public persons_packet
    {
    public:
        get_id_info(wim_packet_params _params, const std::string_view _id);

        const id_info_response& get_response() const;
        const std::shared_ptr<core::archive::persons_map>& get_persons() const override;

    private:
        int32_t init_request(std::shared_ptr<core::http_request_simple> _request) override;
        int32_t parse_results(const rapidjson::Value& _node_results) override;

        std::string id_;

        id_info_response response_;
        std::shared_ptr<core::archive::persons_map> persons_;
    };
}
