#pragma once

#include "fs_loader_task.h"

enum class loader_errors;

namespace core
{
    namespace wim
    {
        struct upload_progress_handler;
        class web_file_info;
        struct upload_file_params;

        class upload_task : public fs_loader_task, public std::enable_shared_from_this<upload_task>
        {
            std::unique_ptr<upload_file_params> file_params_;
            std::wstring file_name_short_;
            std::ifstream file_stream_;
            int64_t file_size_;
            int64_t bytes_sent_;

            std::string upload_host_;
            std::string upload_url_;

            core::tools::binary_stream out_buffer_;

            std::string file_url_;

            std::shared_ptr<upload_progress_handler> handler_;

            const std::chrono::time_point<std::chrono::system_clock> start_time_;

            int64_t session_id_;

            loader_errors read_data_from_file();
            loader_errors send_data_to_server();

            virtual void resume(loader& _loader) override;

        public:

            upload_task(const std::string &_id, const wim_packet_params& _params, upload_file_params&& _file_params);
            virtual ~upload_task();

            bool is_end() const;

            loader_errors get_gate();
            loader_errors open_file();
            loader_errors send_next_range();

            const std::string& get_file_url() const;

            void set_handler(std::shared_ptr<upload_progress_handler> _handler);
            std::shared_ptr<upload_progress_handler> get_handler() const;

            virtual void on_result(int32_t _error) override;
            virtual void on_progress() override;

            std::chrono::time_point<std::chrono::system_clock> get_start_time() const;
            int64_t get_file_size() const;

            std::shared_ptr<web_file_info> make_info() const;
        };

    }
}