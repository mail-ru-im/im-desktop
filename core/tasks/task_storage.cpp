#include "stdafx.h"

#include "../../corelib/collection_helper.h"
#include "../../common.shared/json_helper.h"
#include "task_storage.h"
#include "../core.h"
#include "../tools/features.h"

namespace
{
    core::archive::thread_update create_empty_task_thread_update(std::string_view _task_id, std::string_view _thread_id)
    {
        core::archive::thread_update update;

        update.id_ = _thread_id;
        update.parent_topic_.type_ = core::threads::parent_topic::type::task;
        update.parent_topic_.task_id_ = _task_id;
        return update;
    };
}

namespace core::tasks
{
    bool task_storage::contains(const std::string& _task_id) const
    {
        const auto task_iter = tasks_.find(_task_id);
        return task_iter == tasks_.end();
    }

    const task_data& task_storage::get_task(const std::string& _task_id) const
    {
        const auto task_iter = tasks_.find(_task_id);
        if (task_iter == tasks_.end())
        {
            im_assert(!"access not existed task");
            static task_data empty;
            empty.id_ = _task_id;
            return empty;
        }
        return task_iter->second;
    }

    bool task_storage::update_task(const task_data& _task)
    {
        const auto& id = _task.id_;
        im_assert(!id.empty());
        if (id.empty())
            return false;

        auto& task = tasks_[id];
        if (!task.is_empty() && task.params_.last_change_ >= _task.params_.last_change_)
            return false;

        const auto& new_thread_id = _task.params_.thread_id_;
        const bool ignore_thread_update = task.params_.thread_id_.empty() && new_thread_id.empty();
        if (ignore_thread_update)
        {
            task = _task;
            task.last_used_ = std::chrono::system_clock::now();
        }
        else
        {
            const bool create_thread_update = task.params_.thread_id_.empty() && !new_thread_id.empty();
            auto update = create_thread_update ? create_empty_task_thread_update(_task.id_, new_thread_id) : task.thread_update_;
            const auto last_used = task.last_used_;
            task = _task;
            task.thread_update_ = std::move(update);
            task.last_used_ = last_used;
        }
        set_changed(true);
        return true;
    }

    void task_storage::edit_task(const core::tasks::task_change& _change, std::string_view _for_user)
    {
        const auto task_iter = tasks_.find(_change.id_);
        if (task_iter == tasks_.end())
            return;
        auto& task = task_iter->second;
        task.merge(_change, _for_user);
        set_changed(true);
    }

    void task_storage::update_last_used(const std::string& _task_id)
    {
        im_assert(!_task_id.empty());
        if (_task_id.empty())
            return;

        const auto task_iter = tasks_.find(_task_id);
        if (task_iter == tasks_.end())
            return;

        auto& task = task_iter->second;
        task.last_used_ = std::chrono::system_clock::now();
        set_changed(true);
    }

    int32_t task_storage::load(std::wstring_view _filename)
    {
        core::tools::binary_stream bstream;
        if (!bstream.load_from_file(_filename))
            return -1;

        bstream.write<char>('\0');

        rapidjson::Document doc;
        if (doc.ParseInsitu((char*)bstream.read(bstream.available())).HasParseError())
            return -1;

        const auto iter_tasks = doc.FindMember("tasks");
        if (iter_tasks == doc.MemberEnd() || !iter_tasks->value.IsArray())
            return -1;

        for (const auto& task_json : iter_tasks->value.GetArray())
        {
            task_data parsed_task;
            parsed_task.unserialize(task_json);

            const auto& id = parsed_task.id_;
            im_assert(!id.empty());
            if (id.empty())
                continue;

            auto& task = tasks_[id];
            if (task.is_empty() || task.params_.last_change_ < parsed_task.params_.last_change_)
                task = std::move(parsed_task);
        }

        return 0;
    }

    void task_storage::erase_old_tasks()
    {
        const auto threshold_time = std::chrono::system_clock::now() - features::task_cache_lifetime();
        for (auto it = tasks_.begin(); it != tasks_.end();)
        {
            if (it->second.last_used_ < threshold_time)
                it = tasks_.erase(it);
            else
                ++it;
        }
    }

    bool task_storage::apply_thread_update(const core::archive::thread_update& _thread_update)
    {
        const auto& parent_topic = _thread_update.parent_topic_;
        const auto type = parent_topic.type_;
        const auto& task_id = _thread_update.parent_topic_.task_id_;

        using core::archive::thread_parent_topic;
        im_assert(thread_parent_topic::type::task == type && !task_id.empty());
        if (thread_parent_topic::type::task != type || task_id.empty())
            return false;
        const auto task_iter = tasks_.find(task_id);
        if (task_iter == tasks_.end())
            return false;

        auto& task = task_iter->second;
        task.thread_update_ = _thread_update;
        set_changed(true);
        return true;
    }

    void task_storage::serialize(rapidjson::Value& _node, rapidjson_allocator& _a) const
    {
        rapidjson::Value node_tasks(rapidjson::Type::kArrayType);
        node_tasks.Reserve(tasks_.size(), _a);

        for (const auto& [_, task] : tasks_)
        {
            rapidjson::Value node_task(rapidjson::Type::kObjectType);
            task.serialize(node_task, _a);
            node_tasks.PushBack(std::move(node_task), _a);
        }

        _node.AddMember("tasks", std::move(node_tasks), _a);
    }

    void task_storage::serialize(icollection* _collection) const
    {
        coll_helper coll(_collection, false);

        core::ifptr<core::iarray> array(coll->create_array());
        array->reserve(tasks_.size());

        for (const auto& [_, task] : tasks_)
        {
            coll_helper coll_task(coll->create_collection(), true);
            task.serialize(coll_task.get());
            ifptr<ivalue> task_value(coll->create_value());
            task_value->set_as_collection(coll_task.get());
            array->push_back(task_value.get());
        }

        coll.set_value_as_array("tasks", array.get());
    }

    bool task_storage::is_changed() const
    {
        return changed_;
    }

    void task_storage::set_changed(bool _changed)
    {
        changed_ = _changed;
    }
}
