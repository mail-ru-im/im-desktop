#pragma once

#include "../utils/utils.h"
#include "../cache/emoji/EmojiCode.h"

namespace Ui
{
    namespace TextRendering
    {
        class TextUnit;
    }

    class gui_coll_helper;
}

namespace core
{
    class coll_helper;
}

namespace Statuses
{
    enum class TimeMode
    {
        Common = 0,
        Left,
        Passed,
        AlwaysOn
    };

    class Status
    {
    public:
        Status(const QString& _code = QString(), const QString& _description = QString(), std::chrono::seconds _duration = std::chrono::seconds::zero());
        Status(const Status& _other) = default;
        ~Status() = default;

        bool operator== (const Status& _other) const;

        QImage getImage(int _size = -1) const;

        QString toString() const;
        const QString& getDescription() const noexcept;

        void setDuration(const std::chrono::seconds _time);
        const std::chrono::seconds getDefaultDuration() const noexcept;
        QString getTimeString() const;
        QString getDurationString() const;

        bool isEmpty() const;

        void setSelected(bool _selected);
        bool isSelected() const { return selected_; }

        void setExpirationTime();
        void resetTime();
        bool statusExpired() const;

        void unserialize(core::coll_helper& _coll);
        void unserialize(const rapidjson::Value& _node);
        void serialize(Ui::gui_coll_helper& _coll) const;

        void update(const Status& _other);

    private:
        Emoji::EmojiCode code_;
        mutable QString description_;
        std::chrono::seconds defaultDuration_;
        std::chrono::seconds duration_;
        QDateTime startTime_;
        QDateTime endTime_;
        bool selected_;
        TimeMode timeMode_;
    };
}

Q_DECLARE_METATYPE(Statuses::Status);
