#include "stdafx.h"

#include "../../corelib/collection_helper.h"
#include "../../common.shared/json_helper.h"
#include "history_message.h"
#include "poll.h"


namespace core::archive
{

void poll_data::serialize(icollection* _collection) const
{
    coll_helper coll(_collection, false);
    coll_helper poll_collection(_collection->create_collection(), true);

    core::ifptr<core::iarray> answers_array(_collection->create_array());
    answers_array->reserve(answers_.size());

    for (const auto& answer : answers_)
    {
        coll_helper answer_collection(_collection->create_collection(), true);
        answer_collection.set_value_as_int64("votes", answer.votes_);
        answer_collection.set_value_as_string("answer", answer.text_);
        answer_collection.set_value_as_string("answer_id", answer.answer_id_);

        core::ifptr<core::ivalue> answers_value(_collection->create_value());
        answers_value->set_as_collection(answer_collection.get());
        answers_array->push_back(answers_value.get());
    }
    poll_collection.set_value_as_array("answers", answers_array.get());
    poll_collection.set_value_as_string("my_answer_id", my_answer_id_);
    poll_collection.set_value_as_string("id", id_);
    poll_collection.set_value_as_string("type", type_);
    poll_collection.set_value_as_bool("can_stop", can_stop_);
    poll_collection.set_value_as_bool("closed", closed_);

    coll.set_value_as_collection("poll", poll_collection.get());
}

bool poll_data::unserialize(icollection* _coll)
{
    coll_helper helper(_coll, false);

    if (helper.is_value_exist("id"))
        id_ = helper.get_value_as_string("id");

    if (helper.is_value_exist("type"))
        type_ = helper.get_value_as_string("type");

    if (helper.is_value_exist("answers"))
    {
        const auto answers = helper.get_value_as_array("answers");
        const auto answers_size = answers->size();
        answers_.reserve(answers_size);

        for (auto i = 0; i < answers_size; ++i)
        {
            core::coll_helper coll_answer(answers->get_at(i)->get_as_collection(), false);
            poll_data::answer answer;

            if (coll_answer.is_value_exist("answer"))
                answer.text_ = coll_answer.get_value_as_string("answer");

            if (coll_answer.is_value_exist("votes"))
                answer.votes_ = coll_answer.get_value_as_int64("votes");

            answers_.push_back(std::move(answer));
        }
    }

    return true;
}

bool poll_data::unserialize(const rapidjson::Value& _node)
{
    tools::unserialize_value(_node, "id", id_);
    tools::unserialize_value(_node, "myAnswerId", my_answer_id_);
    tools::unserialize_value(_node, "closed", closed_);
    tools::unserialize_value(_node, "canStop", can_stop_);
    tools::unserialize_value(_node, "type", type_);

    auto answers = _node.FindMember("answers");
    if (answers != _node.MemberEnd() && answers->value.IsArray())
    {
        answers_.reserve(answers->value.Size());
        for (const auto& answer_node : answers->value.GetArray())
        {
            poll_data::answer answer;
            tools::unserialize_value(answer_node, "answerId", answer.answer_id_);
            tools::unserialize_value(answer_node, "text", answer.text_);
            tools::unserialize_value(answer_node, "votes", answer.votes_);
            answers_.push_back(std::move(answer));
        }
    }

    return true;
}

}
