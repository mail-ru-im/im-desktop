#include "stdafx.h"

#include "Status.h"
#include "StatusUtils.h"
#include "LocalStatuses.h"
#include "utils/JsonUtils.h"
#include "utils/gui_coll_helper.h"
#include "../styles/ThemeParameters.h"
#include "../styles/ThemesContainer.h"

using namespace Statuses;

Status::Status(const QString& _code, const QString& _description)
    : code_(Emoji::EmojiCode::fromQString(_code)),
      description_(_description)
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
    if (description_.isEmpty())
    {
        static const auto emptyDescription = Statuses::Status::emptyDescription();
        const auto& localDescription = Statuses::LocalStatuses::instance()->statusDescription(toString());
        return localDescription.isEmpty() ? emptyDescription : localDescription;
    }

    return description_;
}

QImage Status::getImage(int _size) const
{
    if (code_.isNull())
    {
        static const QImage empty;
        return empty;
    }

    auto image = Emoji::GetEmoji(code_, _size == -1 ? Emoji::getEmojiSize() : Emoji::EmojiSizePx(_size));
    Utils::check_pixel_ratio(image);
    return image;
}

QString Status::getTimeString() const
{
    if (endTime_.isNull())
    {
        const auto secondsPassed = startTime_.secsTo(QDateTime::currentDateTime());
        return Statuses::getTimeString(std::chrono::seconds(secondsPassed), TimeMode::Passed);
    }
    else
    {
        const auto secondsLeft = QDateTime::currentDateTime().secsTo(endTime_);
        return Statuses::getTimeString(std::chrono::seconds(secondsLeft), TimeMode::Left);
    }
}

void Status::setStartTime(const QDateTime& _time)
{
    startTime_ = _time;
}

void Status::setEndTime(const QDateTime& _time)
{
    endTime_ = _time;
}

void Status::unserialize(core::coll_helper &_coll)
{
    const auto status = QString::fromUtf8(_coll.get_value_as_string("status"));
    code_ = Emoji::EmojiCode::fromQString(status);
    description_ = QString::fromUtf8(_coll.get_value_as_string("description", ""));
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
}

void Status::update(const Status& _other)
{
    if (_other.startTime_.isValid())
        startTime_ = _other.startTime_;

    endTime_ = _other.endTime_;

    if (!_other.description_.isEmpty())
        description_ = _other.description_;
}

QString Status::emptyDescription()
{
    return QT_TRANSLATE_NOOP("status", "Custom status");
}
