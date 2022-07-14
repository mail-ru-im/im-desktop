#include "stdafx.h"
#include "fstring.h"
#include "../utils/utils.h"

namespace Data
{
    FString Data::FString::Builder::finalize(bool _mergeMentions)
    {
        using ftype = core::data::format_type;
        auto prevUrl = std::string();

        auto isMention = [](const FString& _s) -> std::string_view
        {
            for (const auto& [type, range, data] : _s.formatting().formats())
            {
                if (type == ftype::mention && range.size_ == _s.size() && data)
                    return *data;
            }
            return {};
        };

        auto result = FString();
        for (const auto& part : parts_)
        {
            // These cumbersome ifs are due to std::visit and std::get are not available in XCode 12.2, see https://stackoverflow.com/q/52310835
            if (std::holds_alternative<QStringView>(part))
            {
                result += *std::get_if<QStringView>(&part);
            }
            else if (std::holds_alternative<QString>(part))
            {
                result += *std::get_if<QString>(&part);
            }
            else if (std::holds_alternative<FString>(part))
            {
                auto fs = *std::get_if<FString>(&part);
                if (_mergeMentions)
                {
                    const auto url = isMention(fs);

                    if (url.empty() || prevUrl != url)
                        result += std::move(fs);
                    prevUrl = url;
                }
                else
                {
                    result += std::move(fs);
                }
            }
            else if (std::holds_alternative<FStringView>(part))
            {
                im_assert(!_mergeMentions);
                result += *std::get_if<FStringView>(&part);
            }
        }
        return result;
    }

    struct FString::SharedData
    {
        QString string_;
        core::data::format format_;

        SharedData() = default;

        SharedData(int _n, QChar _c)
            : string_(_n, _c)
        {}

        SharedData(const QString& _string, const core::data::format& _format, bool _doDebugValidityCheck)
            : string_(_string)
            , format_(_format)
        {
            im_assert(!_doDebugValidityCheck || isFormatValid());
        }

        SharedData(const QString& _string, core::data::range_format&& _format)
            : string_(_string)
            , format_(std::move(_format))
        {}

        SharedData(QString&& _string, core::data::range_format&& _format)
            : string_(std::move(_string))
            , format_(std::move(_format))
        {}

        SharedData(QString&& _string, core::data::format_type _type, core::data::format_data _data = {})
            : string_(std::move(_string))
            , format_({ _type, core::data::range{0, static_cast<int>(string_.size())}, _data })
        {}

        bool isFormatValid() const
        {
            using ftype = core::data::format_type;
            for (const auto& [type, range, data] : format_.formats())
            {
                const auto [offset, size, startIndex] = range;
                if (type == ftype::link && (!data || Utils::isMentionLink(QString::fromStdString(*data))))
                {
                    im_assert(false);
                    return false;
                }
                if (type == ftype::mention && !Utils::isMentionLink(QStringView(string_).mid(offset, size)))
                {
                    im_assert(false);
                    return false;
                }
            }
            return true;
        }

        static const FString::SharedDataPtr& sharedEmpty()
        {
            static const SharedDataPtr instance = std::make_shared<SharedData>();
            return instance;
        }
    };

    FString::FString()
        : sharedData_(std::make_shared<SharedData>())
    {}

    FString::FString(int _n, QChar _c)
        : sharedData_(std::make_shared<SharedData>(_n, _c))
    {}

    FString::FString(const QString& _string, const core::data::format& _format, bool _doDebugValidityCheck)
        : sharedData_(std::make_shared<SharedData>(_string, _format, _doDebugValidityCheck))
    {}

    FString::FString(const QString& _string, core::data::range_format&& _format)
        : sharedData_(std::make_shared<SharedData>(_string, std::move(_format)))
    {}

    FString::FString(QString&& _string, core::data::range_format&& _format)
        : sharedData_(std::make_shared<SharedData>(std::move(_string), std::move(_format)))
    {}

    FString::FString(QString&& _string, core::data::format_type _type, core::data::format_data _data)
        : sharedData_(std::make_shared<SharedData>(std::move(_string), _type, _data))
    {}

    FString::FString(const FString& _other)
        : sharedData_(std::make_shared<SharedData>(*_other.sharedData_))
    {
    }

    FString& FString::operator=(const FString& _other)
    {
        if (std::addressof(_other) == this)
            return (*this); // defending from self-assignment

        sharedData_ = std::make_shared<SharedData>(*_other.sharedData_);
        return (*this);
    }

    bool FString::hasFormatting() const { return !sharedData_->format_.empty(); }

    bool FString::containsFormat(core::data::format_type _type) const
    {
        return std::any_of(formatting().formats().cbegin(), formatting().formats().cend(),
            [_type](auto _ft) { return _ft.type_ == _type; });
    }

    bool FString::containsAnyFormatExceptFor(FormatTypes _types) const
    {
        return std::any_of(formatting().formats().cbegin(), formatting().formats().cend(),
            [_types](auto _ft) { return !_types.testFlag(_ft.type_); });
    }

    void FString::replace(QChar _old, QChar _new) { sharedData_->string_.replace(_old, _new); }
    bool FString::isEmpty() const { return sharedData_->string_.isEmpty(); }
    bool FString::isTrimmedEmpty() const { return QStringView(sharedData_->string_).trimmed().isEmpty(); }
    const QString& FString::string() const { return sharedData_->string_; }
    const core::data::format& FString::formatting() const { return sharedData_->format_; }
    core::data::format& FString::formatting() { return sharedData_->format_; }
    int FString::size() const { return sharedData_->string_.size(); }
    QChar FString::at(int _pos) const { return sharedData_->string_.at(_pos); }
    QChar FString::operator[](uint _pos) const { return sharedData_->string_[_pos]; }

    bool FString::startsWith(const QString& _prefix) const { return sharedData_->string_.startsWith(_prefix); }
    bool FString::endsWith(const QString& _suffix) const { return sharedData_->string_.endsWith(_suffix); }
    bool FString::startsWith(QChar _ch) const { return sharedData_->string_.startsWith(_ch); }
    bool FString::endsWith(QChar _ch) const { return sharedData_->string_.endsWith(_ch); }

    int FString::indexOf(QChar _ch, int _from) const { return sharedData_->string_.indexOf(_ch, _from); }
    int FString::lastIndexOf(QChar _char) const { return sharedData_->string_.lastIndexOf(_char); }

    QChar FString::front() const { return sharedData_->string_.front(); }
    QChar FString::back() const { return sharedData_->string_.back(); }

    void FString::reserve(int _size) { sharedData_->string_.reserve(_size); }

    void FString::chop(int _n)
    {
        _n = qBound(0, _n, sharedData_->string_.size());
        sharedData_->format_.cut_at(sharedData_->string_.size() - _n);
        sharedData_->string_.chop(_n);
        im_assert(isFormatValid());
    }

    bool FString::contains(const QString& _str) const { return string().contains(_str); }
    bool FString::contains(QChar _ch) const { return string().contains(_ch); }

    FString& FString::operator+=(QChar _ch) { sharedData_->string_ += _ch; return *this; }
    FString& FString::operator+=(const QString& _other) { sharedData_->string_ += _other; return *this; }

    FString& FString::operator+=(QStringView _other)
    {
#if defined(__linux__)
        sharedData_->string_ += _other.toString();  // Our Qt version on linux doesn't support it yet
#else
        sharedData_->string_ += _other;
#endif
        return *this;
    }

    FString& FString::operator+=(const FString& _other)
    {
        auto newFormats = sharedData_->format_.formats();
        const auto& fsOther = _other.formatting().formats();

        for (auto newOne : fsOther)
        {
            newOne.range_.offset_ += sharedData_->string_.size();
            auto didExtendExistingRange = false;
            for (auto& oldOne : newFormats)
            {
                if (oldOne.type_ == newOne.type_)
                {
                    auto& r = oldOne.range_;
                    if (r.offset_ + r.size_ == newOne.range_.offset_)
                    {
                        r.size_ += newOne.range_.size_;
                        didExtendExistingRange = true;
                        break;
                    }
                }
            }
            if (!didExtendExistingRange)
                newFormats.emplace_back(std::move(newOne));
        }

        sharedData_->format_ = core::data::format(std::move(newFormats));
        sharedData_->string_.append(_other.string());

        return *this;
    }

    FString& FString::operator+=(FStringView _other)
    {
        *this += _other.toFString();
        return *this;
    }

    void FString::clear()
    {
        sharedData_->string_.clear();
        sharedData_->format_.clear();
    }

    void FString::clearFormat()
    {
        sharedData_->format_.clear();
    }

    void FString::setFormatting(const core::data::format& _formatting)
    {
        sharedData_->format_ = _formatting;
        sharedData_->format_.cut_at(sharedData_->string_.size());
    }

    void FString::addFormat(core::data::format_type _type, core::data::format_data _data)
    {
        auto builder = formatting().get_builder();
        builder %= {_type, core::data::range{ 0, size() }, _data};
        setFormatting(builder.finalize());
    }

    bool FString::operator==(const FString& _other) const
    {
        return (string() == _other.string()) && (formatting() == _other.formatting());
    }

    bool FString::operator!=(const FString& _other) const
    {
        return !operator==(_other);
    }

    bool FString::isFormatValid() const
    {
        return sharedData_->isFormatValid();
    }

    void FString::removeAllBlockFormats()
    {
        formatting().remove_formats(core::data::is_block_format);
    }

    void FString::removeMentionFormats()
    {
        formatting().remove_formats([](core::data::format_type _type) { return _type == core::data::format_type::mention; });
    }


    FStringView::FStringView(FString::SharedDataPtr _shared, int _offset, int _size)
        : data_(_shared)
        , offset_(_offset)
        , size_(_size)
    {
        im_assert(offset_ >= 0);
        im_assert(size_ >= 0);
        im_assert(offset_ + size_ <= _shared->string_.size());
        if (offset_ < 0 || size_ < 0 || offset_ + size_ > _shared->string_.size())
        {
            offset_ = 0;
            size_ = 0;
        }
    }

    FStringView::FStringView() noexcept
        : FStringView(FString::SharedData::sharedEmpty(), 0, 0)
    {}

    FStringView::FStringView(const FString& _string, int _offset, int _size)
        : FStringView(_string.sharedData_, _offset, _size == -1 ? _string.size() - _offset : _size)
    {}

    FString::SharedDataPtr FStringView::sharedData() const
    {
        if (auto p = data_.lock())
            return p;

        const auto& empty = FString::SharedData::sharedEmpty();
        data_ = empty;
        offset_ = 0;
        size_ = 0;
        return empty;
    }

    QStringView FStringView::string() const
    {
        const auto fullView = QStringView(sharedData()->string_);
        if (offset_ + size_ <= fullView.size())
            return fullView.mid(offset_, size_);
        return {};
    }

    QString FStringView::toString() const { return string().toString(); }
    FStringView FStringView::sourceView() const noexcept { return { sharedData(), 0, sharedData()->string_.size() }; }
    core::data::range FStringView::sourceRange() const noexcept { return { offset_, size_ }; }

    bool FStringView::hasFormatting() const noexcept
    {
        if (sharedData()->format_.empty())
            return false;

        for (const auto& ft : sharedData()->format_.formats())
        {
            if (cutRangeToFitView(ft.range_).size_ > 0)
                return true;
        }
        return false;
    }

    bool FStringView::containsFormat(core::data::format_type _type) const
    {
        const auto formats = getStyles();
        return std::any_of(formats.cbegin(), formats.cend(),
            [_type](auto _ft) { return _ft.type_ == _type; });
    }

    bool FStringView::isEmpty() const noexcept { return size_ == 0 || sharedData()->string_.isEmpty(); }
    int FStringView::size() const noexcept { return size_; }

    QChar FStringView::front() const
    {
        const QString& s = sharedData()->string_;
        im_assert(!s.isEmpty() && offset_ < s.size());
        return s[offset_];
    }

    QChar FStringView::back()  const
    {
        const QString& s = sharedData()->string_;
        const int i = offset_ + size_ - 1;
        im_assert(!s.isEmpty() && i < s.size());
        return s[i];
    }

    FStringView FStringView::trimmed() const
    {
        const auto str = string();

        auto start = 0;
        for (; start < str.size(); ++start)
        {
            if (!str.at(start).isSpace())
                break;
        }

        auto last = size() - 1;
        for (; last > start; --last)
        {
            if (!str.at(last).isSpace())
                break;
        }

        const auto result = mid(start, last - start + 1);
        im_assert(!result.string().startsWith(u' '));
        im_assert(!result.string().endsWith(u' '));
        return result;
    }

    FStringView FStringView::mid(qsizetype _offset) const
    {
        im_assert(_offset <= size_);
        return { sharedData(), static_cast<int>(offset_ + qBound(qsizetype(0), _offset, qsizetype(size_))), static_cast<int>(size_ - qBound(qsizetype(0), _offset, qsizetype(size_))) };
    }

    FStringView FStringView::mid(qsizetype _offset, qsizetype _size) const
    {
        im_assert((_offset + _size) <= size_);
        return { sharedData(), static_cast<int>(offset_ + qBound(qsizetype(0), _offset, qsizetype(size_))), static_cast<int>(qBound(qsizetype(0), _offset + _size, qsizetype(size_)) - qBound(qsizetype(0), _offset, qsizetype(size_))) };
    }
    FStringView FStringView::left(qsizetype _size) const { return mid(0, _size); }
    FStringView FStringView::right(qsizetype _size) const { return mid(size() - _size, _size); }

    void FStringView::chop(int _n)
    {
        im_assert(_n >= 0 && _n <= size_);
        size_ -= qBound(0, _n, size_);
    }

    QChar FStringView::at(int _pos) const { im_assert(_pos < size_);  return sharedData()->string_.at(offset_ + _pos); }
    QChar FStringView::lastChar() const { return string().back(); }
    int FStringView::indexOf(QChar _char, qsizetype _from) const { return string().indexOf(_char, _from); }
    int FStringView::indexOf(QStringView _string, qsizetype _from) const { return string().indexOf(_string, _from); }
    int FStringView::lastIndexOf(QChar _char) const { return string().lastIndexOf(_char); }
    bool FStringView::startsWith(QStringView _prefix) const { return string().startsWith(_prefix); }
    bool FStringView::endsWith(QStringView _prefix) const { return string().endsWith(_prefix); }

    FString FStringView::toFString() const { return { string().toString(), getFormat() }; }

    bool FStringView::isAnyOf(FormatTypes _types) const noexcept
    {
        for (const auto& [type, range, _] : sharedData()->format_.formats())
        {
            if (_types.testFlag(type) && cutRangeToFitView(range).size_ == size())
                return true;
        }
        return false;
    }

    std::vector<core::data::range_format> Data::FStringView::getStyles() const
    {
        auto result = std::vector<core::data::range_format>();
        for (const auto& [type, range, data] : sharedData()->format_.formats())
        {
            if (const auto r = cutRangeToFitView(range); r.size_ > 0)
                result.emplace_back(type, r, data);
        }
        return result;
    }

    bool FStringView::tryToAppend(FStringView _other)
    {
        if (isEmpty())
        {
            *this = _other;
            return true;
        }

        if (_other.isEmpty())
            return true;

        const QString& otherString = _other.sharedData()->string_;
        const QString& thatString = sharedData()->string_;
        im_assert(otherString == thatString);
        if (otherString != thatString)
            return false;

        static QString toSkip = QChar::Space % QChar::LineFeed % QChar::CarriageReturn;
        auto thisEnd = offset_ + size_;
        while (thisEnd < _other.offset_ && toSkip.contains(thatString.at(thisEnd)))
            ++thisEnd;

        if (thisEnd == _other.offset_)
        {
            size_ = _other.size_ + _other.offset_ - offset_;
            return true;
        }

        return false;
    }

    bool FStringView::tryToAppend(QChar _ch)
    {
        const QString& str = sharedData()->string_;
        if ((str.size() < offset_ + size_ + 1) || (str.at(offset_ + size_) != _ch))
            return false;

        size_ += 1;
        return true;
    }

    bool FStringView::tryToAppend(QStringView _text)
    {
        const QString& str = sharedData()->string_;
        if ((str.size() < offset_ + size_ + _text.size()) || (str.midRef(offset_ + size_, _text.size()) != _text))
            return false;

        size_ += _text.size();
        return true;
    }

    core::data::range FStringView::getRangeOf(const FString& _str) const
    {
        const auto offset = string().indexOf(_str.string());
        if (offset == -1)
            return {};

        return { offset_ + static_cast<int>(offset), _str.size() };
    }

    FString FStringView::replaceByString(QStringView _newString, bool _keepMentionFormat) const
    {
        const auto oldSize = string().size();
        auto newFormats = std::vector<core::data::range_format>();
        const auto format = getFormat();
        for (auto [type, range, data] : format.formats())
        {
            if ((!_keepMentionFormat && type == core::data::format_type::mention))
                continue;
            if (range.offset_ != 0 || range.size_ != oldSize)
                continue;

            range.size_ = _newString.size();
            if (type == core::data::format_type::mention)
                data = string().toString().toStdString();
            newFormats.emplace_back(type, range, std::move(data));
        }
        return { _newString.toString(), { std::move(newFormats) } , false };
    }

    core::data::range FStringView::cutRangeToFitView(core::data::range _range) const
    {
        im_assert(_range.offset_ >= 0);
        im_assert(_range.size_ >= 0);
        const auto left = std::max(offset_, _range.offset_);
        const auto right = std::min(offset_ + size_, _range.offset_ + _range.size_);
        _range.offset_ = left - offset_;
        _range.size_ = std::max(0, right - left);
        return _range;
    }

    core::data::format FStringView::getFormat() const
    {
        auto builder = core::data::format::builder();
        for (const auto& [type, range, data] : sharedData()->format_.formats())
        {
            if (const auto r = cutRangeToFitView(range); r.size_ > 0)
                builder %= { type, r, data };
        }
        return builder.finalize();
    }
}
