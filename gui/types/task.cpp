#include "stdafx.h"

#include "utils/gui_coll_helper.h"
#include "../my_info.h"
#include "task.h"

namespace
{
    namespace detail
    {
        template <typename T>
        void unserializeDateTimeIfExists(const Ui::gui_coll_helper& _helper, std::string_view _name, T& _dateTime)
        {
            if (_helper.is_value_exist(_name))
            {
                const auto dateTime = _helper.get_value_as_int64(_name);
                _dateTime = dateTime == 0 ? QDateTime() : QDateTime::fromSecsSinceEpoch(dateTime);
            }
        }
    }

    void unserializeDateTimeIfExists(const Ui::gui_coll_helper& _helper, std::string_view _name, QDateTime& _dateTime)
    {
        detail::unserializeDateTimeIfExists(_helper, _name, _dateTime);
    }

    void unserializeDateTimeIfExists(const Ui::gui_coll_helper& _helper, std::string_view _name, std::optional<QDateTime>& _dateTime)
    {
        detail::unserializeDateTimeIfExists(_helper, _name, _dateTime);
    }

}

namespace Data
{
    void TaskParams::serialize(core::icollection* _collection) const
    {
        core::coll_helper coll(_collection, false);

        Ui::gui_coll_helper paramsCollection(_collection->create_collection(), true);

        paramsCollection.set_value_as_qstring("title", title_);
        if (!assignee_.isEmpty())
            paramsCollection.set_value_as_qstring("assignee", assignee_);
        if (date_.isValid())
            paramsCollection.set_value_as_int64("end_time", date_.toSecsSinceEpoch());
        paramsCollection.set_value_as_enum("status", status_);
        if (!creator_.isEmpty())
            paramsCollection.set_value_as_qstring("creator", creator_);
        if (!threadId_.isEmpty())
            paramsCollection.set_value_as_qstring("thread_id", threadId_);
        if (createTime_.isValid())
            paramsCollection.set_value_as_int64("create_time", createTime_.toSecsSinceEpoch());
        if (lastChange_.isValid())
            paramsCollection.set_value_as_int64("last_change", lastChange_.toSecsSinceEpoch());

        core::ifptr<core::iarray> array(paramsCollection->create_array());
        array->reserve(allowedStatuses_.size());
        for (const auto status : allowedStatuses_)
        {
            core::ifptr<core::ivalue> ival(paramsCollection->create_value(), true);
            ival->set_as_int(int(status));
            array->push_back(ival.get());
        }
        paramsCollection.set_value_as_array("allowed_statuses", array.get());
        coll.set_value_as_collection("params", paramsCollection.get());
    }

    void TaskParams::unserialize(core::icollection* _collection)
    {
        Ui::gui_coll_helper helper(_collection, false);

        if (helper.is_value_exist("title"))
            title_ = helper.get<QString>("title");
        if (helper.is_value_exist("assignee"))
            assignee_ = helper.get<QString>("assignee");
        unserializeDateTimeIfExists(helper, "end_time", date_);
        if (helper.is_value_exist("status"))
            status_ = helper.get_value_as_enum<decltype(status_)>("status");
        if (helper.is_value_exist("creator"))
            creator_ = helper.get<QString>("creator");
        if (helper.is_value_exist("thread_id"))
            threadId_ = helper.get<QString>("thread_id");
        unserializeDateTimeIfExists(helper, "create_time", createTime_);
        unserializeDateTimeIfExists(helper, "last_change", lastChange_);

        if (helper.is_value_exist("allowed_statuses"))
        {
            core::iarray* statuses = helper.get_value_as_array("allowed_statuses");
            const auto size = statuses->size();
            allowedStatuses_.reserve(size);

            for (core::iarray::size_type i = 0; i < size; ++i)
                allowedStatuses_.push_back(core::tasks::status(statuses->get_at(i)->get_as_int()));
        }
    }

    TaskData::TaskData()
        : threadUpdate_{ std::make_shared<Data::ThreadUpdate>() }
    {
    }

    void TaskData::serialize(core::icollection* _collection) const
    {
        core::coll_helper coll(_collection, false);
        Ui::gui_coll_helper taskCollection(_collection->create_collection(), true);

        if (!id_.isEmpty())
            taskCollection.set_value_as_qstring("id", id_);
        params_.serialize(taskCollection.get());

        coll.set_value_as_collection("task", taskCollection.get());
    }

    void TaskData::unserialize(core::icollection* _collection)
    {
        Ui::gui_coll_helper helper(_collection, false);

        if (helper.is_value_exist("params"))
            params_.unserialize(helper.get_value_as_collection("params"));

        if (helper.is_value_exist("id"))
            id_ = helper.get<QString>("id");

        if (helper.is_value_exist("thread_update"))
        {
            Ui::gui_coll_helper threadUpdateHelper(helper.get_value_as_collection("thread_update"), false);
            threadUpdate_->unserialize(threadUpdateHelper);
        }

    }

    bool TaskData::needToLoadData() const
    {
        return hasId() && params_.title_.isEmpty();
    }

    bool TaskData::hasId() const
    {
        return !id_.isEmpty();
    }

    bool TaskData::hasIdAndData() const
    {
        return hasId() && !params_.title_.isEmpty();
    }

    bool TaskData::hasAllowedStatuses() const
    {
        return !params_.allowedStatuses_.empty();
    }

    bool TaskData::isEmpty() const
    {
        return !hasId() && params_.title_.isEmpty();
    }

    bool TaskData::needToSend() const
    {
        return !hasId() && !params_.title_.isEmpty();
    }

    bool TaskData::didReceiveFetch() const
    {
        return !params_.threadId_.isEmpty();
    }

    QString TaskData::statusDescription(core::tasks::status _status)
    {
        using core::tasks::status;
        switch (_status)
        {
        case status::new_task:
            return QT_TRANSLATE_NOOP("task", "New");
        case status::in_progress:
            return QT_TRANSLATE_NOOP("task", "In progress");
        case status::ready:
            return QT_TRANSLATE_NOOP("task", "Ready");
        case status::rejected:
            return QT_TRANSLATE_NOOP("task", "Rejected");
        case status::closed:
            return QT_TRANSLATE_NOOP("task", "Closed");
        default:
            break;
        }
        im_assert(!"Unknown task status");
        return QT_TRANSLATE_NOOP("task", "New");
    }

    TaskChange::TaskChange(const TaskData& _initialTask, const TaskData& _changedTask)
        : id_{ _initialTask.id_ }
        , title_{ _initialTask.params_.title_ != _changedTask.params_.title_ ? std::make_optional(_changedTask.params_.title_) : std::nullopt }
        , assignee_{ _initialTask.params_.assignee_ != _changedTask.params_.assignee_ ? std::make_optional(_changedTask.params_.assignee_) : std::nullopt }
        , date_{ _initialTask.params_.date_ != _changedTask.params_.date_ ? std::make_optional(_changedTask.params_.date_) : std::nullopt }
        , status_{ _initialTask.params_.status_ != _changedTask.params_.status_ ? std::make_optional(_changedTask.params_.status_) : std::nullopt }
    {
        im_assert(_initialTask.id_ == _changedTask.id_);
        using core::tasks::status;
        if (assignee_ && !status_)
            status_ = status::new_task;
    }

    TaskChange::TaskChange(const QString& _id, core::tasks::status _newStatus)
        : id_{ _id }
        , status_{ _newStatus }
    {
    }

    void TaskChange::serialize(core::icollection* _collection) const
    {
        core::coll_helper coll(_collection, false);
        Ui::gui_coll_helper taskCollection(_collection->create_collection(), true);

        taskCollection.set_value_as_qstring("id", id_);
        if (title_)
            taskCollection.set_value_as_qstring("title", *title_);
        if (assignee_)
            taskCollection.set_value_as_qstring("assignee", *assignee_);
        if (date_)
        {
            const auto count = date_->isValid() ? date_->toSecsSinceEpoch() : 0;
            taskCollection.set_value_as_int64("end_time", count);
        }
        if (status_)
            taskCollection.set_value_as_enum("status", *status_);

        coll.set_value_as_collection("task", taskCollection.get());
    }

    void TaskChange::unserialize(core::icollection* _collection)
    {
        Ui::gui_coll_helper helper(_collection, false);

        if (helper.is_value_exist("title"))
            title_ = helper.get<QString>("title");
        if (helper.is_value_exist("assignee"))
            assignee_ = helper.get<QString>("assignee");
        unserializeDateTimeIfExists(helper, "end_time", date_);
        if (helper.is_value_exist("status"))
            status_ = helper.get_value_as_enum<decltype(status_)::value_type>("status");
    }

}
