#pragma once

#include "../../corelib/collection_helper.h"

namespace Ui
{
    class gui_coll_helper : public core::coll_helper
    {
    public:
        gui_coll_helper(core::icollection* _collection, bool _autoRelease);

        void set_value_as_qstring(std::string_view _name, QStringView _value);
        void set_value_as_qstring(std::string_view _name, QString&& _value);

        core::ifptr<core::ivalue> create_qstring_value(QStringView _value);
        core::ifptr<core::ivalue> create_qstring_value(QString&& _value);
    };
}

namespace core
{

    template<>
    inline QString coll_helper::get<QString>(std::string_view _name) const
    {
        return QString::fromUtf8(get_value_as_string(_name));
    }

    template<>
    inline QString coll_helper::get<QString>(std::string_view _name, const char *_def) const
    {
        return QString::fromUtf8(get_value_as_string(_name, _def));
    }

    template<>
    inline void coll_helper::set<QStringView>(std::string_view _name, const QStringView& _value)
    {
        const auto asUtf8 = _value.toUtf8();
        set_value_as_string(_name, asUtf8.data(), asUtf8.size());
    }

    template<>
    inline void coll_helper::set<QString>(std::string_view _name, const QString &_value)
    {
        set<QStringView>(_name, _value);
    }
}

namespace Utils
{
    enum class TrimString
    {
        no,
        yes
    };
    template<typename T>
    T toContainerOfString(const core::iarray* array, TrimString _trim = TrimString::yes)
    {
        T container;
        if (array)
        {
            const auto size = array->size();
            container.reserve(size);
            for (std::remove_const_t<decltype(size)> i = 0; i < size; ++i)
            {
                auto str = QString::fromUtf8(array->get_at(i)->get_as_string());
                if (_trim == TrimString::yes)
                    str = std::move(str).trimmed();

                if (!str.isEmpty())
                    container.push_back(std::move(str));
            }
        }
        return container;
    }

    template<typename T>
    core::ifptr<core::iarray> toArrayOfStrings(const T& _cont, Ui::gui_coll_helper& _coll)
    {
        core::ifptr<core::iarray> arr(_coll->create_array());
        arr->reserve(_cont.size());
        for (const auto& item : _cont)
            arr->push_back(_coll.create_qstring_value(item).get());

        return arr;
    }
}