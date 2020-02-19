#include "stdafx.h"

#include "utils/gui_coll_helper.h"
#include "poll.h"

namespace
{
    Data::PollData::Type pollTypeFromStr(const QString& _str)
    {
        if (_str == ql1s("public"))
            return Data::PollData::PublicPoll;
        else if (_str == ql1s("anon"))
            return Data::PollData::AnonymousPoll;
        else
            return Data::PollData::None;
    }
}


namespace Data
{
    void PollData::serialize(core::icollection* _collection) const
    {
        core::coll_helper coll(_collection, false);
        Ui::gui_coll_helper pollCollection(_collection->create_collection(), true);

        pollCollection.set_value_as_qstring("id", id_);

        core::ifptr<core::iarray> answersArray(_collection->create_array());
        answersArray->reserve(answers_.size());

        for (const auto& answer : answers_)
        {
            Ui::gui_coll_helper answerCollection(_collection->create_collection(), true);
            answerCollection.set_value_as_qstring("answer", answer->text_);

            core::ifptr<core::ivalue> answersValue(_collection->create_value());
            answersValue->set_as_collection(answerCollection.get());
            answersArray->push_back(answersValue.get());
        }

        pollCollection.set_value_as_array("answers", answersArray.get());
        coll.set_value_as_collection("poll", pollCollection.get());
    }

    void PollData::unserialize(core::icollection* _collection)
    {
        Ui::gui_coll_helper helper(_collection, false);

        if (helper.is_value_exist("my_answer_id"))
            myAnswerId_ = helper.get<QString>("my_answer_id");

        if (helper.is_value_exist("id"))
            id_ = helper.get<QString>("id");

        if (helper.is_value_exist("can_stop"))
            canStop_ = helper.get<bool>("can_stop");

        if (helper.is_value_exist("closed"))
            closed_ = helper.get<bool>("closed");

        if (helper.is_value_exist("type"))
            type_ = pollTypeFromStr(helper.get<QString>("type"));

        if (helper.is_value_exist("answers"))
        {
            const auto answers = helper.get_value_as_array("answers");
            const auto answers_size = answers->size();
            answers_.reserve(answers_size);

            int64_t votes = 0;

            for (auto i = 0; i < answers_size; ++i)
            {
                core::coll_helper coll_answer(answers->get_at(i)->get_as_collection(), false);
                AnswerPtr answer = std::make_shared<PollData::Answer>();

                if (coll_answer.is_value_exist("answer"))
                    answer->text_ = coll_answer.get<QString>("answer");

                if (coll_answer.is_value_exist("answer_id"))
                    answer->answerId_ = coll_answer.get<QString>("answer_id");

                if (coll_answer.is_value_exist("votes"))
                    answer->votes_ = coll_answer.get_value_as_int64("votes");

                votes += answer->votes_;
                if (!answer->answerId_.isEmpty())
                    answersIndex_[answer->answerId_] = answer;

                answers_.push_back(std::move(answer));

            }

            votes_ = votes;
        }
    }

}
