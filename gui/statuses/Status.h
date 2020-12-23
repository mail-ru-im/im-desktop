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
        Status(const QString& _code = QString(), const QString& _description = QString());
        ~Status() = default;

        bool operator== (const Status& _other) const;

        QImage getImage(int _size = -1) const;

        QString toString() const;
        const QString& getDescription() const noexcept;

        QString getTimeString() const;
        const QDateTime& getEndTime() const { return endTime_; }

        bool isEmpty() const;

        void setStartTime(const QDateTime& _time);
        void setEndTime(const QDateTime& _time);

        void unserialize(core::coll_helper& _coll);
        void unserialize(const rapidjson::Value& _node);

        void update(const Status& _other);

        static QString emptyDescription();

    private:
        Emoji::EmojiCode code_;
        mutable QString description_;
        QDateTime startTime_;
        QDateTime endTime_;
    };
}

Q_DECLARE_METATYPE(Statuses::Status);
