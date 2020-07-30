#include "stdafx.h"

#include "Status.h"
#include "StatusUtils.h"
#include "utils/JsonUtils.h"
#include "utils/gui_coll_helper.h"
#include "../styles/ThemeParameters.h"
#include "../styles/ThemesContainer.h"

namespace
{
    inline QImage getEmptyStatusIcon()
    {
        return Utils::renderSvg(qsl(":/statuses/empty_status_icon"), Utils::scale_value(QSize(34, 34)), Styling::getParameters().getColor(Styling::StyleVariable::BASE_TERTIARY)).toImage();
    }
}

using namespace Statuses;

Status::Status(const QString& _code, const QString& _description, std::chrono::seconds _duration)
    : code_(Emoji::EmojiCode::fromQString(_code))
    , description_(_description)
    , defaultDuration_(_duration)
    , duration_(_duration)
    , startTime_(QDateTime())
    , endTime_(QDateTime())
    , selected_(false)
    , timeMode_(TimeMode::Common)
{
}

bool Status::operator==(const Status &_other) const
{
    return code_ == _other.code_;
}

bool Status::isEmpty() const
{
    return code_.isNull();
}

QString Status::toString() const
{
    return Emoji::EmojiCode::toQString(code_);
}

const QString& Status::getDescription() const noexcept
{
    return description_;
}

QImage Status::getImage(int _size) const
{
    if (code_.isNull())
    {
        if (!description_.isEmpty())
        {
            return getEmptyStatusIcon();
        }
        else
        {
            static const QImage empty;
            return empty;
        }
    }

    auto image = Emoji::GetEmoji(code_, _size == -1 ? Emoji::getEmojiSize() : Emoji::EmojiSizePx(_size));
    Utils::check_pixel_ratio(image);
    return image;
}

QString Status::getTimeString() const
{
    if (selected_ && !isEmpty() && !statusExpired())
    {
        const auto isFor = startTime_.secsTo(QDateTime::currentDateTime());
        if (duration_.count() == 0)
            return Statuses::getTimeString(std::chrono::seconds(isFor), TimeMode::Passed);
        else
            return Statuses::getTimeString(std::chrono::seconds(duration_.count() - isFor), TimeMode::Left);
    }

    return Statuses::getTimeString(duration_, timeMode_);
}

void Status::setDuration(const std::chrono::seconds _time)
{
    duration_ = _time;
    if (!selected_)
        defaultDuration_ = _time;
    setExpirationTime();
}

const std::chrono::seconds Status::getDefaultDuration() const noexcept
{
    return defaultDuration_;
}

void Status::setSelected(bool _selected)
{
    selected_ = _selected;
    if (selected_)
        setExpirationTime();
    else
        resetTime();
}

void Status::setExpirationTime()
{
    if (!startTime_.isValid() || startTime_.isNull())
        startTime_ = QDateTime::currentDateTime();
    if (!endTime_.isValid() || endTime_.isNull())
        endTime_ = startTime_.addSecs(duration_.count());
    timeMode_ = duration_.count() == 0 ? TimeMode::Common : TimeMode::AlwaysOn;
}

void Status::resetTime()
{
    duration_ = defaultDuration_;
    startTime_ = QDateTime();
    endTime_ = QDateTime();
    timeMode_ = TimeMode::Common;
}

bool Status::statusExpired() const
{
    return duration_.count() != 0 && QDateTime::currentDateTime() > endTime_;
}

void Status::unserialize(core::coll_helper &_coll)
{
    const auto status = QString::fromUtf8(_coll.get_value_as_string("status"));
    code_ = Emoji::EmojiCode::fromQString(status);
    startTime_ = QDateTime::fromMSecsSinceEpoch(_coll.get_value_as_int64("startTime") * 1000);
    if (_coll.is_value_exist("endTime"))
        endTime_ = QDateTime::fromMSecsSinceEpoch(_coll.get_value_as_int64("endTime") * 1000);
}

void Status::unserialize(const rapidjson::Value& _node)
{
    QString status;
    JsonUtils::unserialize_value(_node, "status", status);
    code_ = Emoji::EmojiCode::fromQString(status);
    JsonUtils::unserialize_value(_node, "text", description_);
    JsonUtils::unserialize_value(_node, "duration", defaultDuration_);
    duration_ = defaultDuration_;
}

void Status::serialize(Ui::gui_coll_helper &_coll) const
{
    _coll.set_value_as_qstring("status", isEmpty() ? QString() : toString());
    _coll.set_value_as_int64("duration", duration_.count());
}

void Status::update(const Status& _other)
{
    if (_other.startTime_.isValid())
        startTime_ = _other.startTime_;
    if (_other.endTime_.isValid() && _other.endTime_ >= _other.startTime_)
    {
        endTime_ = _other.endTime_;
        duration_ = std::chrono::seconds(startTime_.secsTo(endTime_));
    }
    if (!_other.description_.isEmpty())
    {
        description_ = _other.description_;
        defaultDuration_ = _other.defaultDuration_;
        duration_ = _other.duration_;
        selected_ = _other.selected_;
        timeMode_ = _other.timeMode_;
    }
}
