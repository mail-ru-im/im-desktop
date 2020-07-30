#pragma once

namespace core
{
    class coll_helper;
}

namespace Data
{
    class SessionInfo
    {
    public:
        void unserialize(const core::coll_helper& _coll);

        const QString& getOS() const noexcept { return os_; }
        const QString& getClientName() const noexcept { return clientName_; }
        const QString& getLocation() const noexcept { return location_; }
        const QString& getIp() const noexcept { return ip_; }
        const QString& getHash() const noexcept { return hash_; }
        const QDateTime& getStartedTime() const noexcept { return startedTime_; }
        bool isCurrent() const noexcept { return isCurrent_; }

        static const SessionInfo& emptyCurrent();

    private:
        QString os_;
        QString clientName_;
        QString location_;
        QString ip_;
        QString hash_;
        QDateTime startedTime_;
        bool isCurrent_ = false;
    };
}