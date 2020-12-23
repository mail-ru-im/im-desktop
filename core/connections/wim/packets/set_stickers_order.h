#ifndef __SET_STICKERS_ORDER_H_
#define __SET_STICKERS_ORDER_H_

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
        class set_stickers_order_packet : public wim_packet
        {
            std::vector<int32_t> values_;

            std::shared_ptr<core::tools::binary_stream> response_;

            virtual int32_t init_request(const std::shared_ptr<core::http_request_simple>& _request) override;
            virtual int32_t parse_response(const std::shared_ptr<core::tools::binary_stream>& _response) override;
            virtual int32_t execute_request(const std::shared_ptr<core::http_request_simple>& _request) override;

        public:

            set_stickers_order_packet(wim_packet_params _params, std::vector<int32_t>&& _values);
            virtual ~set_stickers_order_packet();

            virtual priority_t get_priority() const override;
            virtual bool is_post() const override { return true; }
            virtual std::string_view get_method() const override;
        };
    }
}

#endif //__SET_STICKERS_ORDER_H_