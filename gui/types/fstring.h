#pragma once

#include "../../common.shared/message_processing/text_formatting.h"

namespace Data
{
    Q_DECLARE_FLAGS(FormatTypes, core::data::format_type)
    Q_DECLARE_OPERATORS_FOR_FLAGS(FormatTypes)

    class FStringView;
    class FString
    {
        friend class FStringView;
    public:
        class Builder
        {
        public:
            template <typename T>
            Builder& operator%=(const T& _part)
            {
                if (_part.size() == 0)
                    return *this;

                size_ += _part.size();
                parts_.emplace_back(_part);
                return *this;
            }

            template <typename T>
            Builder& operator%=(T&& _part)
            {
                if (_part.size() == 0)
                    return *this;

                size_ += _part.size();
                parts_.emplace_back(std::move(_part));
                return *this;
            }

            FString finalize(bool _mergeMentions = false);
            qsizetype size() const { return size_; }
            bool isEmpty() const { return parts_.empty(); }
        protected:
            std::vector<std::variant<FString, FStringView, QString, QStringView>> parts_;
            qsizetype size_ = 0;
        };

    public:
        struct SharedData;
        using SharedDataPtr = std::shared_ptr<SharedData>;

        FString();
        FString(int _n, QChar _c);
        FString(const QString& _string, const core::data::format& _format = {}, bool _doDebugValidityCheck = true);
        FString(const QString& _string, core::data::range_format&& _format);
        FString(QString&& _string, core::data::range_format&& _format);
        FString(QString&& _string, core::data::format_type _type, core::data::format_data _data = {});
        FString(const FString& _other);
        FString(FString&& _other) noexcept = default;
        ~FString() = default;

        FString& operator=(const FString& _other);
        FString& operator=(FString&& _other) noexcept = default;

        bool hasFormatting() const;
        bool containsFormat(core::data::format_type _type) const;
        bool containsAnyFormatExceptFor(FormatTypes _types) const;
        void replace(QChar _old, QChar _new);
        bool isEmpty() const;
        bool isTrimmedEmpty() const;
        const QString& string() const;
        const core::data::format& formatting() const;
        core::data::format& formatting();
        int size() const;
        QChar at(int _pos) const;
        QChar operator[](uint _pos) const;

        template <typename T>
        Builder operator%(const T& _part) const { return (Builder() %= *this) %= _part; }

        bool startsWith(const QString& _prefix) const;
        bool endsWith(const QString& _suffix) const;

        bool startsWith(QChar _ch) const;
        bool endsWith(QChar _ch) const;

        int indexOf(QChar _ch, int _from = 0) const;
        int lastIndexOf(QChar _char) const;

        QChar front() const;
        QChar back() const;

        void reserve(int _size);

        void chop(int _n);

        bool contains(const QString& _str) const;
        bool contains(QChar _ch) const;

        FString& operator+=(QChar _ch);
        FString& operator+=(const QString& _other);
        FString& operator+=(QStringView _other);
        FString& operator+=(const FString& _other);
        FString& operator+=(FStringView _other);

        void clear();
        void clearFormat();
        void setFormatting(const core::data::format& _formatting);
        void addFormat(core::data::format_type _type, core::data::format_data _data = {});
        bool operator==(const FString& _other) const;
        bool operator!=(const FString& _other) const;

        bool isFormatValid() const;

        void removeAllBlockFormats();
        void removeMentionFormats();

    private:
        std::shared_ptr<SharedData> sharedData_;
    };

    //! NB: FormattedStringView doesn't take care of keeping any ownership
    class FStringView
    {
        FStringView(FString::SharedDataPtr _shared, int _offset, int _size);
    public:
        using const_iterator = QString::const_iterator;
        using const_reverse_iterator = QString::const_reverse_iterator;

        FStringView() noexcept;
        FStringView(const FStringView&) = default;
        FStringView(FStringView&&) noexcept = default;
        FStringView(const FString& _string, int _offset = 0, int _size = -1);
        ~FStringView() = default;

        FStringView& operator=(const FStringView& _other) = default;
        FStringView& operator=(FStringView&& _other) noexcept = default;

        bool hasFormatting() const noexcept;
        bool containsFormat(core::data::format_type _type) const;
        bool isEmpty() const noexcept;
        int size() const noexcept;

        QChar front() const;
        QChar back()  const;

        std::vector<core::data::range_format> getStyles() const;

        [[nodiscard]] FStringView trimmed() const;

        [[nodiscard]] FStringView mid(qsizetype _offset) const;
        [[nodiscard]] FStringView mid(qsizetype _offset, qsizetype _size) const;
        [[nodiscard]] FStringView left(qsizetype _size) const;
        [[nodiscard]] FStringView right(qsizetype _size) const;

        void chop(int _n);

        QChar at(int _pos) const;
        QChar lastChar() const;
        int indexOf(QChar _char, qsizetype _from = 0) const;
        int indexOf(QStringView _string, qsizetype _from = 0) const;
        int lastIndexOf(QChar _char) const;
        bool startsWith(QStringView _prefix) const;
        bool endsWith(QStringView _prefix) const;

        FString toFString() const;

        //! Check if entire view has these format types
        bool isAnyOf(FormatTypes _types) const noexcept;

        bool operator==(QStringView _other) const { return string() == _other; }
        bool operator!=(QStringView _other) const { return !operator==(_other); }

        QStringView string() const;
        QString toString() const;
        FStringView sourceView() const noexcept;
        core::data::range sourceRange() const noexcept;
        core::data::range getRangeOf(const FString& _word) const;

        //! Replace plain string and extend range if it occupied entire string
        FString replaceByString(QStringView _newString, bool _keepMentionFormat = true) const;

        bool tryToAppend(FStringView _other);
        bool tryToAppend(QChar _ch);
        bool tryToAppend(QStringView _text);

    private:
        [[nodiscard]] core::data::range cutRangeToFitView(core::data::range _range) const;
        [[nodiscard]] core::data::format getFormat() const;

    private:
        FString::SharedDataPtr sharedData() const;

    private:
        mutable std::weak_ptr<FString::SharedData> data_;
        mutable int offset_;
        mutable int size_;
    };
}
