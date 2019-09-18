#ifndef EMOJI_CODE
#define EMOJI_CODE

#include <functional>

namespace Emoji
{
    namespace details {
        template<class> struct is_ref_wrapper : std::false_type {};
        template<class T> struct is_ref_wrapper<std::reference_wrapper<T>> : std::true_type {};

        template<class T>
        using not_ref_wrapper = std::negation<is_ref_wrapper<std::decay_t<T>>>;

        template <class D, class...> struct return_type_helper { using type = D; };
        template <class... Types>
        struct return_type_helper<void, Types...> : std::common_type<Types...> {
            static_assert(std::conjunction_v<not_ref_wrapper<Types>...>,
                "Types cannot contain reference_wrappers when D is void");
        };

        template <class D, class... Types>
        using return_type = std::array<typename return_type_helper<D, Types...>::type,
            sizeof...(Types)>;
    }

    template < class D = void, class... Types>
    constexpr details::return_type<D, Types...> make_array(Types&&... t) {
        return { std::forward<Types>(t)... };
    }

    QString migration(const std::string&);


    class EmojiCode
    {
    public:
        using codePointType = uint32_t;
    private:
        static constexpr size_t maxCodepoints = 8;
        using baseType = std::array<codePointType, maxCodepoints>;

    public:
        template<typename... Args>
        explicit constexpr EmojiCode(Args&&... args) noexcept : code{ makeComplexCodepoint(std::forward<Args>(args)...) } {}

        explicit constexpr EmojiCode() noexcept : code{ 0 } {}

        EmojiCode(EmojiCode&&) = default;
        EmojiCode& operator=(EmojiCode&&) = default;

        EmojiCode(const EmojiCode&) = default;
        EmojiCode& operator=(const EmojiCode&) = default;

        constexpr bool isNull() const noexcept
        {
            for (auto x : code)
            {
                if (x != 0)
                    return false;
            }
            return true;
            //return std::all_of(code.begin(), code.end(), [](auto x) { return x == 0; }); c++20
        }

        constexpr codePointType codepointAt(size_t idx) const noexcept { assert(idx >= 0 && idx < size()); return code[idx]; }

        std::size_t hash() const noexcept
        {
            size_t res = 0;
            for (auto n : code)
                res += n;
            return std::hash<size_t>()(res);
            // return std::accumulate(code.begin(), code.end(), res); c++23
        }

        constexpr bool contains(codePointType codePoint) const noexcept
        {
            for (auto x : code)
            {
                if (x == codePoint)
                    return true;
            }
            return false;
            //return std::any_of(code.begin(), code.end(), [codePoint](auto x) { return x == codePoint; }); c++20
        }

        constexpr size_t size() const noexcept
        {
            size_t s = 0;
            for (auto x : code)
            {
                if (x)
                    ++s;
                else
                    break;
            }
            return s;
        }

        static constexpr size_t maxSize() noexcept
        {
            return maxCodepoints;
        }

        QString serialize2() const
        {
            return toQString(code);
        }

        constexpr EmojiCode chopped(size_t count) const noexcept
        {
            assert(count < maxCodepoints);
            if (count == 0 || isNull())
                return *this;

            if (count < maxCodepoints)
            {
                auto currentSize = size();
                if (currentSize <= count)
                    return EmojiCode(0);

                auto newCode = code;
                while (count--)
                    newCode[--currentSize] = 0;
                auto res = EmojiCode(std::move(newCode));
                assert(res.size() == currentSize);
                return res;
            }
            return EmojiCode(0);
        }

        static constexpr EmojiCode addCodePoint(const EmojiCode& a, codePointType codePoint) noexcept
        {
            if (a.isNull())
                return EmojiCode(codePoint);
            const auto size = a.size();
            assert(size < maxCodepoints);
            auto code = a.code;
            code[size] = codePoint;
            return EmojiCode(std::move(code));
        }

        static constexpr EmojiCode removeAllEqual(const EmojiCode& a, codePointType codePoint) noexcept
        {
            EmojiCode result;
            for (size_t i = 0; i < a.size(); ++i)
            {
                auto current = a.codepointAt(i);
                if (current != codePoint)
                    result = EmojiCode::addCodePoint(result, current);
            }
            return result;
        }

        static QString toQString(const EmojiCode& code)
        {
            if (code.isNull())
                return QString();
            return QString::fromUcs4(code.code.data(), code.size());
        }

        QString toHexString(QChar _delimiter = ql1c('-')) const
        {
            QString result;
            const auto s = size();
            for (size_t i = 0; i < s; ++i)
            {
                auto codePointStr = QString::number(codepointAt(i), 16);
                while (codePointStr.size() < 4)
                    codePointStr.prepend(ql1c('0'));

                result += codePointStr;
                if (i < s - 1)
                    result += _delimiter;
            }
            return result;
        }

        static EmojiCode unserialize(const std::string& str) noexcept
        {
            return fromQString(migration(str));
        }

        static EmojiCode unserialize2(const QStringRef& str) noexcept
        {
            return fromQString(str);
        }

        static EmojiCode fromQString(const QStringRef& str)
        {
            EmojiCode code;
            const auto ucs = str.toUcs4();
            if (ucs.size() > maxCodepoints)
            {
                assert(!"EmojiCode: wrong str length");
                return code;
            }
            for (auto character : ucs)
                code = addCodePoint(code, character);

            return code;
        }

        static EmojiCode fromQString(const QString& str)
        {
            return fromQString(QStringRef(&str));
        }

        friend constexpr inline bool operator==(const EmojiCode& lhs, const EmojiCode& rhs) noexcept;

    private:
        constexpr EmojiCode(baseType a) noexcept : code{ std::move(a) } {}

        static constexpr baseType makeComplexCodepoint(codePointType a) noexcept
        {
            baseType res{0};
            res.front() = a;
            return res;
        }

        template<typename... Args>
        static constexpr baseType makeComplexCodepoint(codePointType a, Args&&... args) noexcept
        {
            static_assert((sizeof...(args)) < maxCodepoints, "if you need more than 8 arg, extend maxCodepoints to, e.g. 10");
            static_assert(std::conjunction_v<std::is_integral<std::decay_t<Args>>...>);
            auto argsArray = make_array(std::forward<Args>(args)...);
            baseType res{0};
            res.front() = a;
            for (size_t i = 0; i < argsArray.size(); ++i)
                res[i + 1] = argsArray[i];
            return res;
        }

    private:
        baseType code;
    };

    constexpr inline bool operator==(const EmojiCode& lhs, const EmojiCode& rhs) noexcept
    {
        for (size_t i = 0; i < EmojiCode::maxSize(); ++i)
        {
            if (lhs.code[i] != rhs.code[i])
                return false;
        }
        return true;

        //return lhs.code == rhs.code; c++20
    }

    constexpr inline bool operator!=(const EmojiCode& lhs, const EmojiCode& rhs) noexcept
    {
        return !(lhs == rhs);
    }

    static_assert(sizeof (uint32_t) == 4, "sizeof (uint32_t) is not 4 bytes");
}

Q_DECLARE_METATYPE(Emoji::EmojiCode)

#endif // EMOJI_CODE
