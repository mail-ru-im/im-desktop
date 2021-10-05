#pragma once

#include "../../common.shared/tasks/task_types.h"
#include "../archive/thread_update.h"

using rapidjson_allocator = rapidjson::MemoryPoolAllocator<rapidjson::CrtAllocator>;

namespace core
{
    struct icollection;

    namespace tasks
    {
        struct task_params
        {
            std::string title_;
            std::string assignee_;
            std::chrono::system_clock::time_point end_time_;
            tasks::status status_ = tasks::status::unknown;
            std::string creator_;
            std::string thread_id_;
            std::chrono::system_clock::time_point create_time_;
            std::chrono::system_clock::time_point last_change_;
            std::vector<tasks::status> allowed_statuses_;

            void serialize(icollection* _collection) const;
            bool unserialize(icollection* _collection);
            void serialize(rapidjson::Value& _node, rapidjson_allocator& _a) const;
            bool unserialize(const rapidjson::Value& _node);
        };

        struct task_change;

        struct task_data
        {
            void serialize(icollection* _collection) const;
            bool unserialize(icollection* _collection);
            void serialize(rapidjson::Value& _node, rapidjson_allocator& _a) const;
            bool unserialize(const rapidjson::Value& _node);

            int64_t end_time_to_seconds() const;
            void set_end_time_from_seconds(int64_t _secs);

            void merge(const task_change& _change, std::string_view _for_user);
            task_user task_user_for_aim_id(std::string_view _aim_id);

            bool is_empty() const;

            std::string id_;
            task_params params_;
            std::chrono::system_clock::time_point last_used_;

            core::archive::thread_update thread_update_;
        };
        using task = std::optional<task_data>;

        struct task_change
        {
            void serialize(icollection* _collection) const;
            bool unserialize(icollection* _coll);
            rapidjson::Value serialize(rapidjson_allocator& _a) const;

            int64_t end_time_to_seconds() const;
            void set_end_time_from_seconds(int64_t _secs);

            std::string id_;
            std::optional<std::string> title_;
            std::optional<std::string> assignee_;
            std::optional<std::chrono::system_clock::time_point> end_time_;
            std::optional<tasks::status> status_;
        };

    }

}
