#ifndef __GET_STICKERS_PACK_INFO_H_
#define __GET_STICKERS_PACK_INFO_H_

#pragma once

#include "../wim_packet.h"
#include "../../../tools/file_sharing.h"

namespace core
{
    namespace tools
    {
        class http_request_simple;
    }
}

namespace core
{
    namespace tools
    {
        class binary_stream;
    }

    namespace wim
    {
        class get_stickers_pack_info_packet : public wim_packet
        {
            const int32_t pack_id_;

            const std::string store_id_;

            const core::tools::filesharing_id filesharing_id_;

            std::shared_ptr<core::tools::binary_stream> response_;

            int32_t init_request(const std::shared_ptr<core::http_request_simple>& _request) override;

            int32_t parse_response(const std::shared_ptr<core::tools::binary_stream>& _response) override;

        public:

            get_stickers_pack_info_packet(wim_packet_params _params, const int32_t _pack_id, const std::string& _store_id, const core::tools::filesharing_id& _filesharing_id);
            virtual ~get_stickers_pack_info_packet();

            const std::shared_ptr<core::tools::binary_stream>& get_response() const;

            priority_t get_priority() const override;
            std::string_view get_method() const override;
            int minimal_supported_api_version() const override;
        };
    }
}

#endif //__GET_STICKERS_PACK_INFO_H_
