#pragma once

#include "Status.h"

namespace Statuses
{
    struct LocalStatus
    {
        QString status_;
        QString description_ = Statuses::Status::emptyDescription();
        std::chrono::seconds duration_ = std::chrono::seconds::zero();
    };

    class LocalStatuses_p;

    class LocalStatuses : public QObject
    {
        Q_OBJECT
    public:
        ~LocalStatuses();

        static LocalStatuses* instance();

        const LocalStatus& getStatus(const QString& _statusCode) const;

        void setDuration(const QString& _statusCode, std::chrono::seconds _duration);
        std::chrono::seconds statusDuration(const QString& _statusCode) const;

        const QString& statusDescription(const QString& _statusCode) const;

        const std::vector<LocalStatus>& statuses() const;

        void resetToDefaults();

    private Q_SLOTS:
        void onOmicronUpdated();

    private:
        LocalStatuses();

        std::unique_ptr<LocalStatuses_p> d;
    };

}
