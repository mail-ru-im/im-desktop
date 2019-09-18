#include "stdafx.h"
#include "ShowQualityReasonsPopupConfig.h"

namespace Ui
{

ShowQualityReasonsPopupConfig::ShowQualityReasonsPopupConfig(const core::coll_helper& _coll)
{
    unserialize(_coll);
}

bool ShowQualityReasonsPopupConfig::unserialize(const core::coll_helper &_coll)
{
    clear();

    core::iarray* categories = _coll.get_value_as_array("categories");
    for (decltype(categories->size()) i = 0; i < categories->size(); ++i)
    {
        gui_coll_helper category_coll(categories->get_at(i)->get_as_collection(), false);

        ReasonCategory category;
        category.unserialize(category_coll);

        categories_.push_back(category);
    }

    surveyId_ = QString::fromUtf8(_coll.get_value_as_string("survey_id"));
    surveyTitle_ = QString::fromUtf8(_coll.get_value_as_string("survey_title"));

    setValid(true);
    return true;
}

ShowQualityReasonsPopupConfig ShowQualityReasonsPopupConfig::defaultConfig()
{
    return ShowQualityReasonsPopupConfig();
}

const std::vector<ShowQualityReasonsPopupConfig::ReasonCategory> &ShowQualityReasonsPopupConfig::categories() const
{
    return categories_;
}

const QString& ShowQualityReasonsPopupConfig::surveyId() const
{
    return surveyId_;
}

const QString& ShowQualityReasonsPopupConfig::surveyTitle() const
{
    return surveyTitle_;
}

ShowQualityReasonsPopupConfig::ShowQualityReasonsPopupConfig()
{
    clear();
}

void ShowQualityReasonsPopupConfig::clear()
{
    surveyId_.clear();
    categories_.clear();
    setValid(false);
}

void ShowQualityReasonsPopupConfig::setValid(bool _valid)
{
    isValid_  = _valid;
}


// Reason
void ShowQualityReasonsPopupConfig::Reason::unserialize(const Ui::gui_coll_helper &_coll)
{
    display_text_ = QString::fromUtf8(_coll.get_value_as_string("display_string"));
    reason_id_ = QString::fromUtf8(_coll.get_value_as_string("reason_id"));
}

// ReasonCategory
void ShowQualityReasonsPopupConfig::ReasonCategory::unserialize(const Ui::gui_coll_helper &_coll)
{
    category_id_.clear();
    reasons_.clear();

    if (!_coll.is_value_exist("category_id") || !_coll.is_value_exist("reasons"))
    {
        assert(!"required fields missing in ReasonCategory::unserialize");
        return;
    }

    category_id_ = QString::fromUtf8(_coll.get_value_as_string("category_id"));

    auto reasons_arr = _coll->get_value("reasons")->get_as_array();
    reasons_.reserve(reasons_arr->size());

    for (decltype(reasons_arr->size()) i = 0; i < reasons_arr->size(); ++i)
    {
        Reason reason;
        Ui::gui_coll_helper coll(reasons_arr->get_at(i)->get_as_collection(), false);
        reason.unserialize(coll);

        reasons_.push_back(reason);
    }
}

}
