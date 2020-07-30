#include "stdafx.h"
#include "gui_coll_helper.h"

namespace Ui
{
    gui_coll_helper::gui_coll_helper(core::icollection* _collection, bool _autoRelease)
        : core::coll_helper(_collection, _autoRelease)
    {

    }

    void gui_coll_helper::set_value_as_qstring(std::string_view _name, QStringView _value)
    {
        const auto value = _value.toUtf8();
        return core::coll_helper::set_value_as_string(_name, value.data(), value.size());
    }

    void gui_coll_helper::set_value_as_qstring(std::string_view _name, QString&& _value)
    {
        const auto value = std::move(_value).toUtf8();
        return core::coll_helper::set_value_as_string(_name, value.data(), value.size());
    }

    core::ifptr<core::ivalue> gui_coll_helper::create_qstring_value(QStringView _value)
    {
        const auto valueText = _value.toUtf8();
        core::ifptr<core::ivalue> val(get()->create_value());
        val->set_as_string(valueText.data(), valueText.size());
        return val;
    }

    core::ifptr<core::ivalue> gui_coll_helper::create_qstring_value(QString&& _value)
    {
        const auto valueText = std::move(_value).toUtf8();
        core::ifptr<core::ivalue> val(get()->create_value());
        val->set_as_string(valueText.data(), valueText.size());
        return val;
    }

}
