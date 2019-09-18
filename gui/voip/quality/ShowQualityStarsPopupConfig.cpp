#include "ShowQualityStarsPopupConfig.h"


namespace Ui
{

ShowQualityStarsPopupConfig::ShowQualityStarsPopupConfig(const core::coll_helper &_coll)
{
    unserialize(_coll);
}

ShowQualityStarsPopupConfig::ShowQualityStarsPopupConfig()
{
    clear();
}

const std::vector<int> &ShowQualityStarsPopupConfig::secondPopupStars() const
{
    return secondPopupStars_;
}

const std::vector<int> &ShowQualityStarsPopupConfig::showOnCallNumber() const
{
    return showOnCallNumber_;
}

unsigned int ShowQualityStarsPopupConfig::showOnDurationMin() const
{
    return showOnDurationMin_;
}

bool ShowQualityStarsPopupConfig::showPopup() const
{
    return showPopup_;
}

const QString &ShowQualityStarsPopupConfig::dialogTitle() const
{
    return title_;
}

bool ShowQualityStarsPopupConfig::unserialize(const core::coll_helper &_coll)
{
    clear();

    // "voip_rating": { "second_popup_stars":[1,2,3,4], "show_on_call":[1,3,7], "show_on_duration":[20] }
    core::icollection* voip_rating_coll = _coll.get();
    if (!voip_rating_coll)
        return false;

    if (voip_rating_coll->is_value_exist("show_rate_window"))
    {
       auto showPopupVal = voip_rating_coll->get_value("show_rate_window");
       showPopup_ = showPopupVal->get_as_bool();
    }

    if (voip_rating_coll->is_value_exist("show_on_call_number"))
    {
        core::ivalue* show_on_call = voip_rating_coll->get_value("show_on_call_number");
        core::iarray* show_on_call_array = show_on_call->get_as_array();

        for (decltype(show_on_call_array->size()) i = 0; i < show_on_call_array->size(); ++i)
        {
            const auto* call_number = show_on_call_array->get_at(i);
            showOnCallNumber_.push_back(call_number->get_as_int());
        }
    }

    if (voip_rating_coll->is_value_exist("second_popup_stars"))
    {
        core::ivalue* second_popup_stars = voip_rating_coll->get_value("second_popup_stars");
        core::iarray* stars_array = second_popup_stars->get_as_array();

        for (decltype(stars_array->size()) i = 0; i < stars_array->size(); ++i)
        {
            const auto* star = stars_array->get_at(i);
            secondPopupStars_.push_back(star->get_as_int());
        }
    }

    if (voip_rating_coll->is_value_exist("show_on_call_dur_min_secs"))
    {
        core::ivalue* show_on_duration = voip_rating_coll->get_value("show_on_call_dur_min_secs");
        showOnDurationMin_ = static_cast<decltype(showOnDurationMin_)>(show_on_duration->get_as_int());
    }

    if (voip_rating_coll->is_value_exist("rate_title"))
    {
        core::ivalue* rate_title_val = voip_rating_coll->get_value("rate_title");
        title_ = QString::fromUtf8(rate_title_val->get_as_string());
    }

    setValid(true);
    return true;
}

bool ShowQualityStarsPopupConfig::isValid() const
{
    return isValid_;
}

ShowQualityStarsPopupConfig ShowQualityStarsPopupConfig::defaultConfig()
{
    return ShowQualityStarsPopupConfig();
}

void ShowQualityStarsPopupConfig::clear()
{
    showOnDurationMin_ = std::numeric_limits<int32_t>::max();
    showOnCallNumber_.clear();
    secondPopupStars_.clear();
    showPopup_ = false;

    setValid(false);
}

void ShowQualityStarsPopupConfig::setValid(bool _valid)
{
    isValid_ = _valid;
}

}
