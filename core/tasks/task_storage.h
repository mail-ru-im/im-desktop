#pragma once

#include "../../common.shared/tasks/task_types.h"
#include "task.h"

namespace core
{
    struct icollection;

    namespace archive
    {
        struct thread_update;
    }

    namespace tasks
    {
        class task_storage
        {
        public:
            bool contains(const std::string& _task_id) const;
            const task_data& get_task(const std::string& _task_id) const;

            bool update_task(const task_data& _task);
            void edit_task(const core::tasks::task_change& _change, std::string_view _for_user);
            void update_last_used(const std::string& _task_id);
            void erase_old_tasks();

            bool apply_thread_update(const core::archive::thread_update& _thread_update);

            int32_t load(std::wstring_view _filename);
            void serialize(rapidjson::Value& _node, rapidjson_allocator& _a) const;

            void serialize(icollection* _collection) const;

            bool is_changed() const;
            void set_changed(bool _changed);

        private:
            std::unordered_map<std::string, task_data> tasks_;
            bool changed_ = false;
        };
    }
}
