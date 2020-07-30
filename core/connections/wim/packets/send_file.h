#ifndef __SEND_FILE_H_
#define __SEND_FILE_H_

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
    namespace wim
    {
        struct send_file_params
        {
            int64_t	size_already_sent_ = 0;
            int64_t	current_chunk_size_ = 0;
            int64_t	full_data_size_ = 0;
            int64_t	session_id_ = 0;

            std::string file_name_;
            char* data_ = nullptr;
        };

        class send_file : public wim_packet
        {
            std::string					host_;
            std::string					url_;

            const send_file_params&		chunk_;

            std::string					file_url_;


            int32_t init_request(const std::shared_ptr<core::http_request_simple>& _request) override;
            int32_t parse_response(const std::shared_ptr<core::tools::binary_stream>& _response) override;
            int32_t execute_request(const std::shared_ptr<core::http_request_simple>& _request) override;

        public:

            send_file(
                wim_packet_params _params,
                const send_file_params& _chunk,
                const std::string& _host,
                const std::string& _url);

            virtual ~send_file();

            const std::string& get_file_url() const;
            virtual bool is_post() const override { return true; }
        };
    }
}

#endif //__SEND_FILE_H_