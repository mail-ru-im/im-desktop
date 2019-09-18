#pragma once

#include "../utils/utils.h"
#include "../styles/ThemeParameters.h"

namespace Utils
{
    // cached type must have method
    // void fill(const QColor& _color)
    // that generates insides with provided color

    template <class T>
    class ColoredCache
    {
    public:
        ColoredCache() : var_(Styling::StyleVariable::BASE) {} //TODO change var
        ColoredCache(const Styling::StyleVariable _var, const QString& _aimId = QString()) : var_(_var), aimId_(_aimId) {}

        // direct by color
        inline const T& get(const QColor& _color) const
        {
            auto it = cache_.find(_color);
            if (it == cache_.end())
            {
                T entry;
                entry.fill(_color);

                it = cache_.emplace(_color, std::move(entry)).first;
            }

            return it->second;
        }

        inline const T& operator[](const QColor& _color) const { return get(_color); }

        // without aimId (from global theme)
        inline const T& get(const Styling::StyleVariable _var) const { return get(_var, QString()); }
        inline const T& operator[](const Styling::StyleVariable _var) const { return get(_var); }

        // with aimId and var from ctor
        inline const T& get() const { return get(var_, aimId_); }

        // with provided aimId and/or var
        const T& get(const Styling::StyleVariable _var, const QString& _aimId) const
        {
            return get(Styling::getParameters(_aimId).getColor(_var));
        }

        inline const T& get(const QString& _aimId) const { return get(var_, _aimId); }

    private:
        Styling::StyleVariable var_;
        QString aimId_;

        mutable std::unordered_map<QColor, T, Utils::QColorHasher> cache_;
    };
}