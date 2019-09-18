#pragma once

#include <string>

namespace core
{

struct icollection;

namespace wim
{

class voip_call_quality_popup_conf
{
public:
    struct reason
    {
        std::string id_; // ["interrupt_resume", "unexpected_interrupt", ...]
        std::string display_string_; // corresponding to id translated string

        void serialize(icollection* _coll) const;
    };

    struct reason_category
    {
        std::string category_id_; // "general"/"video"/"audio"
        std::vector<reason> reasons_;

        void serialize(icollection* _coll) const;
    };


public:
    voip_call_quality_popup_conf();

    void clear();

    bool unserialize(const rapidjson::Document &_doc);
    void serialize(icollection* _coll) const;

private:
    // 1st conf
    bool show_rate_window_;
    std::vector<int32_t> show_on_call_number_;
    int32_t show_on_duration_min_secs_;
    std::vector<int32_t> second_popup_stars_;
    std::string rate_title_;

    // 2nd conf
    std::vector<reason_category> reason_categories_;
    std::string survey_id_;
    std::string survey_title_;
};


}

}
