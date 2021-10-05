#include "stdafx.h"

#include "../../corelib/collection_helper.h"
#include "../../common.shared/json_helper.h"
#include "task.h"

namespace
{
    int64_t to_seconds(std::chrono::system_clock::time_point _point)
    {
        const auto point_sec = std::chrono::time_point_cast<std::chrono::seconds>(_point);
        return point_sec.time_since_epoch().count();
    }

    std::chrono::system_clock::time_point from_seconds(int64_t _secs)
    {
        const std::chrono::seconds sec(_secs);
        return std::chrono::system_clock::time_point(sec);
    }
}

namespace core::tasks
{
    void task_params::serialize(icollection* _collection) const
    {
        coll_helper coll(_collection, false);
        coll_helper params_collection(_collection->create_collection(), true);
        params_collection.set_value_as_string("title", title_);
        params_collection.set_value_as_string("assignee", assignee_);
        params_collection.set_value_as_int64("end_time", to_seconds(end_time_));
        params_collection.set_value_as_enum("status", status_);
        params_collection.set_value_as_string("creator", creator_);
        params_collection.set_value_as_string("thread_id", thread_id_);
        params_collection.set_value_as_int64("create_time", to_seconds(create_time_));
        params_collection.set_value_as_int64("last_change", to_seconds(last_change_));

        core::ifptr<core::iarray> array(params_collection->create_array());
        array->reserve(allowed_statuses_.size());
        for (const auto status : allowed_statuses_)
        {
            core::ifptr<core::ivalue> ival(params_collection->create_value(), true);
            ival->set_as_int(int(status));
            array->push_back(ival.get());
        }
        params_collection.set_value_as_array("allowed_statuses", array.get());

        coll.set_value_as_collection("params", params_collection.get());
    }

    bool task_params::unserialize(icollection* _collection)
    {
        coll_helper helper(_collection, false);

        title_ = helper.get_value_as_string("title", "");
        assignee_ = helper.get_value_as_string("assignee", "");

        if (helper.is_value_exist("end_time"))
            end_time_ = from_seconds(helper.get_value_as_int64("end_time"));

        status_ = helper.get_value_as_enum("status", tasks::status::unknown);
        creator_ = helper.get_value_as_string("creator", "");
        thread_id_ = helper.get_value_as_string("thread_id", "");

        if (helper.is_value_exist("create_time"))
            create_time_ = from_seconds(helper.get_value_as_int64("create_time"));
        if (helper.is_value_exist("last_change"))
            last_change_ = from_seconds(helper.get_value_as_int64("last_change"));

        if (helper.is_value_exist("allowed_statuses"))
        {
            core::iarray* statuses = helper.get_value_as_array("allowed_statuses");
            const auto size = statuses->size();
            allowed_statuses_.reserve(size);

            for (core::iarray::size_type i = 0; i < size; ++i)
                allowed_statuses_.push_back(tasks::status(statuses->get_at(i)->get_as_int()));
        }

        return true;
    }

    void task_params::serialize(rapidjson::Value& _node, rapidjson_allocator& _a) const
    {
        _node.AddMember("title", title_, _a);
        _node.AddMember("assignee", assignee_, _a);
        _node.AddMember("endTime", to_seconds(end_time_), _a);
        if (status::unknown != status_)
            _node.AddMember("status", tools::make_string_ref(status_to_string(status_)), _a);
        _node.AddMember("creator", creator_, _a);
        _node.AddMember("threadId", thread_id_, _a);
        _node.AddMember("createTime", to_seconds(create_time_), _a);
        _node.AddMember("lastChange", to_seconds(last_change_), _a);

        rapidjson::Value allowed_statuses(rapidjson::Type::kArrayType);
        allowed_statuses.Reserve(allowed_statuses_.size(), _a);
        for (auto status : allowed_statuses_)
            allowed_statuses.PushBack(tools::make_string_ref(status_to_string(status)), _a);
        _node.AddMember("allowedStatuses", std::move(allowed_statuses), _a);
    }

    bool task_params::unserialize(const rapidjson::Value& _node)
    {
        tools::unserialize_value(_node, "title", title_);
        tools::unserialize_value(_node, "assignee", assignee_);
        if (int64_t secs = 0; tools::unserialize_value(_node, "endTime", secs))
            end_time_ = from_seconds(secs);

        std::string task_status;
        tools::unserialize_value(_node, "status", task_status);
        if (!task_status.empty())
            status_ = tasks::string_to_status(task_status);

        tools::unserialize_value(_node, "creator", creator_);
        tools::unserialize_value(_node, "threadId", thread_id_);
        if (int64_t secs = 0; tools::unserialize_value(_node, "createTime", secs))
            create_time_ = from_seconds(secs);
        if (int64_t secs = 0; tools::unserialize_value(_node, "lastChange", secs))
            last_change_ = from_seconds(secs);

        if (const auto iter_statuses = _node.FindMember("allowedStatuses"); iter_statuses != _node.MemberEnd() && iter_statuses->value.IsArray())
        {
            auto statuses_array = iter_statuses->value.GetArray();
            allowed_statuses_.reserve(statuses_array.Size());
            for (const auto& status : statuses_array)
            {
                const auto allowed_status = rapidjson_get_string(status);
                allowed_statuses_.push_back(tasks::string_to_status(allowed_status));
            }
        }
        return true;
    }

    void task_data::serialize(icollection* _collection) const
    {
        coll_helper coll(_collection, false);
        coll_helper task_collection(_collection->create_collection(), true);

        task_collection.set_value_as_string("id", id_);
        params_.serialize(task_collection.get());

        coll_helper thread_update_collection(task_collection->create_collection(), true);
        thread_update_.serialize(thread_update_collection);
        task_collection.set_value_as_collection("thread_update", thread_update_collection.get());

        coll.set_value_as_collection("task", task_collection.get());
    }

    bool task_data::unserialize(icollection* _collection)
    {
        coll_helper helper(_collection, false);

        if (helper.is_value_exist("id"))
            id_ = helper.get_value_as_string("id");

        params_.unserialize(helper.get_value_as_collection("params"));

        return true;
    }

    void task_data::serialize(rapidjson::Value& _node, rapidjson_allocator& _a) const
    {
        _node.AddMember("taskId", id_, _a);
        params_.serialize(_node, _a);
        if (core::threads::parent_topic::type::invalid != thread_update_.parent_topic_.type_)
        {
            rapidjson::Value thread_update_node(rapidjson::Type::kObjectType);
            thread_update_.serialize(thread_update_node, _a);
            _node.AddMember("threadUpdate", thread_update_node, _a);
        }
        _node.AddMember("lastUsed", to_seconds(last_used_), _a);
    }

    bool task_data::unserialize(const rapidjson::Value& _node)
    {
        tools::unserialize_value(_node, "taskId", id_);

        params_.unserialize(_node);
        if (int64_t secs = 0; tools::unserialize_value(_node, "lastUsed", secs))
            last_used_ = from_seconds(secs);
        if (const auto thread_update_it = _node.FindMember("threadUpdate"); thread_update_it != _node.MemberEnd() && thread_update_it->value.IsObject())
        {
            const auto& value = thread_update_it->value;
            thread_update_.unserialize(value);
        }

        return true;
    }

    int64_t task_data::end_time_to_seconds() const
    {
        return to_seconds(params_.end_time_);
    }

    void task_data::set_end_time_from_seconds(int64_t _secs)
    {
        params_.end_time_ = from_seconds(_secs);
    }

    void task_data::merge(const task_change& _change, std::string_view _for_user)
    {
        im_assert(id_ == _change.id_);
        if (id_ != _change.id_)
            return;
        if (const auto& title = _change.title_)
            params_.title_ = *title;
        if (const auto& assignee = _change.assignee_)
            params_.assignee_ = *assignee;
        if (const auto& end_time = _change.end_time_)
            params_.end_time_ = *end_time;
        if (const auto& status = _change.status_; status && *status != params_.status_)
        {
            params_.status_ = *status;
            params_.allowed_statuses_ = core::tasks::offline_available_statuses(params_.status_, task_user_for_aim_id(_for_user));
        }
    }

    task_user task_data::task_user_for_aim_id(std::string_view _aim_id)
    {
        if (_aim_id == params_.creator_)
            return core::tasks::task_user::reporter;
        if (_aim_id == params_.assignee_)
            return core::tasks::task_user::assignee;
        return core::tasks::task_user::unknown;
    }

    bool task_data::is_empty() const
    {
        return id_.empty() && params_.title_.empty();
    }

    void task_change::serialize(icollection* _collection) const
    {
        coll_helper coll(_collection, false);
        coll_helper task_collection(_collection->create_collection(), true);

        task_collection.set_value_as_string("id", id_);
        if (title_)
            task_collection.set_value_as_string("title", *title_);
        if (assignee_)
            task_collection.set_value_as_string("assignee", *assignee_);
        if (end_time_)
            task_collection.set_value_as_int64("end_time", to_seconds(*end_time_));
        if (status_)
            task_collection.set_value_as_enum("status", *status_);

        coll.set_value_as_collection("task", task_collection.get());
    }

    bool task_change::unserialize(icollection* _coll)
    {
        coll_helper helper(_coll, false);
        if (helper.is_value_exist("id"))
            id_ = helper.get_value_as_string("id");
        if (helper.is_value_exist("title"))
            title_ = helper.get_value_as_string("title");
        if (helper.is_value_exist("assignee"))
            assignee_ = helper.get_value_as_string("assignee");
        if (helper.is_value_exist("end_time"))
            end_time_ = from_seconds(helper.get_value_as_int64("end_time"));
        if (helper.is_value_exist("status"))
            status_ = helper.get_value_as_enum("status", tasks::status::unknown);
        return true;
    }

    rapidjson::Value task_change::serialize(rapidjson_allocator& _a) const
    {
        rapidjson::Value node(rapidjson::Type::kObjectType);
        node.AddMember("taskId", id_, _a);
        if (title_)
            node.AddMember("title", *title_, _a);
        if (end_time_)
            node.AddMember("endTime", to_seconds(*end_time_), _a);
        if (assignee_)
            node.AddMember("assignee", *assignee_, _a);
        if (status_)
            node.AddMember("status", tools::make_string_ref(tasks::status_to_string(*status_)), _a);
        return node;
    }

    int64_t task_change::end_time_to_seconds() const
    {
        im_assert(end_time_);
        if (!end_time_)
            return -1;
        return to_seconds(*end_time_);
    }

    void task_change::set_end_time_from_seconds(int64_t _secs)
    {
        end_time_ = from_seconds(_secs);
    }
}
