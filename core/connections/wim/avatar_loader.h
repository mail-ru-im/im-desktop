#ifndef __AVATAR_LOADER_H_
#define __AVATAR_LOADER_H_

#pragma once


#include "wim_packet.h"

namespace coretools
{
    class binary_stream;
}

namespace core
{
    class async_executer;

    namespace wim
    {
        struct avatar_context
        {
            int32_t avatar_size_;
            std::string_view avatar_type_;
            std::string contact_;
            time_t write_time_;
            std::wstring avatars_data_path_;
            std::wstring avatar_file_path_;
            core::tools::binary_stream avatar_data_;
            bool avatar_exist_;
            bool force_;

            avatar_context(int32_t _avatar_size, std::string _contact, std::wstring _avatars_data_path)
                :
                avatar_size_(_avatar_size),
                contact_(std::move(_contact)),
                write_time_(0),
                avatars_data_path_(std::move(_avatars_data_path)),
                avatar_exist_(false),
                force_(false){}
        };

        struct avatar_load_handlers
        {
            std::function<void(std::shared_ptr<avatar_context>)> completed_;
            std::function<void(std::shared_ptr<avatar_context>)> updated_;
            std::function<void(std::shared_ptr<avatar_context>, int32_t)> failed_;
        };

        class avatar_task
        {
            std::shared_ptr<avatar_context> context_;
            std::shared_ptr<avatar_load_handlers> handlers_;

            int64_t task_id_;

        public:

            std::shared_ptr<avatar_context> get_context() const;
            std::shared_ptr<avatar_load_handlers> get_handlers() const;

            int64_t get_id() const;

            avatar_task(
                int64_t task_id_,
                const std::shared_ptr<avatar_context>& _context,
                const std::shared_ptr<avatar_load_handlers>& _handlers);
        };

        class avatar_loader : public std::enable_shared_from_this<avatar_loader>
        {
            int64_t task_id_;

            bool working_;
            bool network_error_;

            std::shared_ptr<wim_packet_params> wim_params_;

            std::shared_ptr<async_executer> local_thread_;
            std::shared_ptr<async_executer> server_thread_;

            std::list<std::shared_ptr<avatar_task>> failed_tasks_;

            std::list<std::shared_ptr<avatar_task>> requests_queue_;

            void remove_task(std::shared_ptr<avatar_task> _task);
            void add_task(std::shared_ptr<avatar_task> _task);
            std::shared_ptr<avatar_task> get_next_task();
            void execute_task(std::shared_ptr<avatar_task> _task, std::function<void(int32_t)> _on_complete);
            void run_tasks_loop();

            std::wstring get_avatar_path(const std::wstring& _avatars_data_path, std::string_view _contact, std::string_view _avatar_type = {}) const;
            std::string_view get_avatar_type_by_size(int32_t _size) const;


            void load_avatar_from_server(const std::shared_ptr<avatar_context>& _context, const std::shared_ptr<avatar_load_handlers>& _handlers);

        public:

            avatar_loader();
            virtual ~avatar_loader();

            void resume(const wim_packet_params& _params);

            std::shared_ptr<avatar_load_handlers> get_contact_avatar_async(const wim_packet_params& _params, std::shared_ptr<avatar_context> _context);

            std::shared_ptr<core::async_task_handlers> remove_contact_avatars(
                const std::string& _contact,
                const std::wstring& _avatars_data_path);

            void show_contact_avatar(const std::string& _contact, const int32_t _avatar_size);
        };

    }
}


#endif //__AVATAR_LOADER_H_
