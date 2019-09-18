#include "stdafx.h"
#include "voip_call_quality_popup_conf.h"

#include "../../../corelib/collection_helper.h"

using namespace core;
using namespace wim;



voip_call_quality_popup_conf::voip_call_quality_popup_conf()
{
    clear();
}

void voip_call_quality_popup_conf::clear()
{
    show_rate_window_ = false;
    show_on_duration_min_secs_ = std::numeric_limits<int32_t>::max();
    show_on_call_number_.clear();
    second_popup_stars_.clear();
    reason_categories_.clear();
    survey_id_.clear();
}

bool voip_call_quality_popup_conf::unserialize(const rapidjson::Document &_doc)
{
    auto iter_show_rate_window = _doc.FindMember("show_voip_rating");
    if (iter_show_rate_window != _doc.MemberEnd())
        show_rate_window_ = iter_show_rate_window->value.GetBool();
    else
        show_rate_window_ = false;

    if (!show_rate_window_)
        // NOTE: do not parse the rest of the JSON, it's currently invalid
        return true;

    auto iter_voip_rating = _doc.FindMember("voip_rating");
    if (iter_voip_rating != _doc.MemberEnd())
    {
        auto& voip_rating = iter_voip_rating->value;
        (void) voip_rating;

        auto iter_sec_pop_stars = voip_rating.FindMember("second_popup_stars");
        if (iter_sec_pop_stars != voip_rating.MemberEnd())
        {
             auto& sec_pop_stars = iter_sec_pop_stars->value;
             second_popup_stars_.reserve(sec_pop_stars.Size());
             for (rapidjson::SizeType i = 0; i < sec_pop_stars.Size(); ++i)
                second_popup_stars_.push_back(sec_pop_stars[i].GetInt());
        }

        auto iter_show_on_call_num = voip_rating.FindMember("show_on_call");
        if (iter_show_on_call_num != voip_rating.MemberEnd())
        {
            auto& show_on_call_num = iter_show_on_call_num->value;
            show_on_call_number_.reserve(show_on_call_num.Size());
            for (rapidjson::SizeType i = 0; i < show_on_call_num.Size(); ++i)
                show_on_call_number_.push_back(show_on_call_num[i].GetInt());
        }

        auto iter_show_on_duration = voip_rating.FindMember("show_on_duration");
        if (iter_show_on_duration != voip_rating.MemberEnd())
            show_on_duration_min_secs_ = iter_show_on_duration->value.GetInt();

        auto iter_survey_id = voip_rating.FindMember("survey_id");
        if (iter_survey_id != voip_rating.MemberEnd())
            survey_id_ = rapidjson_get_string(iter_survey_id->value);
    }

    auto iter_problem_groups = _doc.FindMember("problemGroups");
    auto iter_translates = _doc.FindMember("translates");
    if (iter_problem_groups != _doc.MemberEnd() && iter_translates != _doc.MemberEnd())
    {
        auto& problem_groups = iter_problem_groups->value; // array
        auto& translates = iter_translates->value;

        auto iter_reason_translates = translates.FindMember("reason");
        assert(iter_reason_translates != translates.MemberEnd());

        auto& reason_tr = iter_reason_translates->value;

        for (rapidjson::SizeType i = 0; i < problem_groups.Size(); ++i)
        {
            auto& problem_group = problem_groups[i];

            assert(problem_group.MemberCount() == 1);
            for (auto it = problem_group.MemberBegin(); it != problem_group.MemberEnd(); ++it)
            {
                reason_category category;
                category.category_id_ = rapidjson_get_string(it->name);

                auto& reason_ids = it->value;
                reason_categories_.reserve(reason_ids.Size());
                for (rapidjson::SizeType j = 0; j < reason_ids.Size(); ++j)
                {
                    reason cur_reason;
                    cur_reason.id_ = rapidjson_get_string(reason_ids[j]);

                    auto tr_reason = reason_tr.FindMember(cur_reason.id_);
                    if (tr_reason == reason_tr.MemberEnd())
                        continue;
                    cur_reason.display_string_ = rapidjson_get_string(tr_reason->value);

                    category.reasons_.push_back(std::move(cur_reason));
                }

                reason_categories_.push_back(std::move(category));
            }
        }

        auto survey_title_iter = translates.FindMember("title");
        if (survey_title_iter != translates.MemberEnd())
            survey_title_ = rapidjson_get_string(survey_title_iter->value);

        auto rate_title_iter = translates.FindMember("rate_title");
        if (rate_title_iter != translates.MemberEnd())
            rate_title_ = rapidjson_get_string(rate_title_iter->value);
    }

    return true;
}

void voip_call_quality_popup_conf::serialize(icollection *_coll) const
{
    coll_helper cl(_coll, false);

    cl.set_value_as_bool("show_rate_window", show_rate_window_);
    cl.set_value_as_string("rate_title", rate_title_);
    cl.set_value_as_int("show_on_call_dur_min_secs", show_on_duration_min_secs_);

    ifptr<iarray> show_on_call_num(_coll->create_array());
    show_on_call_num->reserve(static_cast<int32_t>(show_on_call_number_.size()));
    for (const auto& number: show_on_call_number_)
    {
        ifptr<ivalue> val(_coll->create_value());
        val->set_as_int(number);

        show_on_call_num->push_back(val.get());
    }
    cl.set_value_as_array("show_on_call_number", show_on_call_num.get());


    ifptr<iarray> second_popup_stars(_coll->create_array());
    second_popup_stars->reserve(static_cast<int32_t>(second_popup_stars_.size()));
    for (const auto& star: second_popup_stars_)
    {
        ifptr<ivalue> val(_coll->create_value());
        val->set_as_int(star);

        second_popup_stars->push_back(val.get());
    }
    cl.set_value_as_array("second_popup_stars", second_popup_stars.get());


    ifptr<iarray> categories(_coll->create_array());
    categories->reserve(static_cast<int32_t>(reason_categories_.size()));

    for (auto& category: reason_categories_)
    {
        coll_helper cat_coll(_coll->create_collection(), true);
        category.serialize(cat_coll.get());

        ifptr<ivalue> val_category(_coll->create_value());
        val_category->set_as_collection(cat_coll.get());
        categories->push_back(val_category.get());
    }

    cl.set_value_as_array("categories", categories.get());
    cl.set_value_as_string("survey_id", survey_id_);
    cl.set_value_as_string("survey_title", survey_title_);
}

// Reason
void voip_call_quality_popup_conf::reason::serialize(icollection *_coll) const
{
    coll_helper cl(_coll, false);
    cl.set_value_as_string("display_string", display_string_);
    cl.set_value_as_string("reason_id", id_);
}

// Reason Category
void voip_call_quality_popup_conf::reason_category::serialize(icollection *_coll) const
{
    coll_helper cat_coll(_coll, false);

    cat_coll.set_value_as_string("category_id", category_id_);

    ifptr<iarray> reasons(cat_coll->create_array());
    reasons->reserve(reasons_.size());

    for (auto& reason: reasons_)
    {
        coll_helper reason_coll(cat_coll->create_collection(), false);
        reason.serialize(reason_coll.get());

        ifptr<ivalue> val_reason(cat_coll->create_value());
        val_reason->set_as_collection(reason_coll.get());

        reasons->push_back(val_reason.get());
    }

    cat_coll.set_value_as_array("reasons", reasons.get());
}
