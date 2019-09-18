#ifndef __REMOVE_STICKERS_PACK_H_
#define __REMOVE_STICKERS_PACK_H_

#pragma once

#include "../wim_packet.h"

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
        class remove_stickers_pack_packet : public wim_packet
        {
            const int32_t pack_id_;

            const std::string store_id_;

            std::shared_ptr<core::tools::binary_stream> response_;

            virtual int32_t init_request(std::shared_ptr<core::http_request_simple> _request) override;
            virtual int32_t parse_response(std::shared_ptr<core::tools::binary_stream> _response) override;
            virtual int32_t execute_request(std::shared_ptr<core::http_request_simple> _request) override;

        public:

            remove_stickers_pack_packet(wim_packet_params _params, const int32_t _pack_id, std::string _store_id);
            virtual ~remove_stickers_pack_packet();
        };
    }
}

#endif //__REMOVE_STICKERS_PACK_H_