#pragma once

namespace Utils
{
    template <Qt::CaseSensitivity cs>
    struct QStringComparator
    {
        using is_transparent = std::true_type;

        bool operator()(QStringView lhs, QStringView rhs) const { return lhs.compare(rhs, cs) < 0; }

        bool operator()(const QString& lhs, QLatin1String rhs) const { return lhs.compare(rhs, cs) < 0; }
        bool operator()(const QStringRef& lhs, QLatin1String rhs) const { return lhs.compare(rhs, cs) < 0; }

        bool operator()(QLatin1String lhs, const QString& rhs) const { return rhs.compare(lhs, cs) > 0; }
        bool operator()(QLatin1String lhs, const QStringRef& rhs) const { return rhs.compare(lhs, cs) > 0; }
        bool operator()(QLatin1String lhs, QLatin1String rhs) const
        {
            if constexpr (cs == Qt::CaseSensitive)
                return lhs < rhs;
            else
                return qstrnicmp(lhs.data(), lhs.size(), rhs.data(), rhs.size()) < 0;
        }

        template<typename T>
        bool operator()(const char* lhs, const T& rhs) const { return operator()(ql1s(lhs), rhs); }

        template<typename T>
        bool operator()(const T& lhs, const char* rhs) const { return operator()(lhs, ql1s(rhs)); }

        bool operator()(const char* lhs, const char* rhs) const { return operator()(ql1s(lhs), ql1s(rhs)); }
    };

    using StringComparator = QStringComparator<Qt::CaseSensitive>;
    using StringComparatorInsensitive = QStringComparator<Qt::CaseInsensitive>;
}
