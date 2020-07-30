#ifndef __GET_STICKERS_INDEX_H_
#define __GET_STICKERS_INDEX_H_

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
        class get_stickers_index : public wim_packet
        {
            const std::string md5_;
            std::shared_ptr<core::tools::binary_stream> response_;

            virtual int32_t init_request(const std::shared_ptr<core::http_request_simple>& _request) override;
            virtual int32_t parse_response(const std::shared_ptr<core::tools::binary_stream>& _response) override;

        public:

            get_stickers_index(wim_packet_params _params, const std::string& _md5);
            virtual ~get_stickers_index();

            const std::shared_ptr<core::tools::binary_stream>& get_response() const noexcept;

            virtual priority_t get_priority() const override;
        };
    }
}

#endif //__GET_STICKERS_INDEX_H_
