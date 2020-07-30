#include "stdafx.h"

#include "LastseenContainer.h"
#include "../../core_dispatcher.h"
#include "main_window/contact_list/ContactListModel.h"

namespace
{
    constexpr std::chrono::milliseconds LAST_SEEN_REQUEST_TIMEOUT = std::chrono::seconds(1);
    constexpr std::chrono::milliseconds LAST_SEEN_UPDATE_TIMEOUT = std::chrono::minutes(1);
    constexpr std::chrono::milliseconds TEMP_SUBSCRIPTION_TIMEOUT = std::chrono::minutes(2);

    constexpr auto LASTSEEN_MAX_REQUEST_COUNT = 30;
}

namespace Logic
{
    LastseenContainer::LastseenContainer(QObject* _parent)
        : QObject(_parent)
        , updateTimer_(new QTimer(this))
        , requestTimer_(new QTimer(this))
    {
        setObjectName(qsl("LastseenContainer"));

        connect(Ui::GetDispatcher(), &Ui::core_dispatcher::userLastSeen, this, &LastseenContainer::userLastSeen);

        updateTimer_->setInterval(LAST_SEEN_UPDATE_TIMEOUT.count());
        connect(updateTimer_, &QTimer::timeout, this, &LastseenContainer::updateLastSeens);

        requestTimer_->setSingleShot(true);
        connect(requestTimer_, &QTimer::timeout, this, &LastseenContainer::requestLastSeens);
    }

    void LastseenContainer::setContactLastSeen(const QString& _aimid, const Data::LastSeen& _lastSeen)
    {
        if (_aimid.isEmpty() || Utils::isChat(_aimid))
            return;

        auto& seen = contactsSeens_[_aimid];
        if (seen.isBot())
            return;

        seen = _lastSeen;

        if (auto subscription = subscriptions_.find(_aimid); subscription != subscriptions_.end())
            emitChanged(_aimid);
    }

    void LastseenContainer::setContactBot(const QString& _aimid)
    {
        if (!_aimid.isEmpty())
            contactsSeens_[_aimid] = Data::LastSeen::bot();
    }

    bool LastseenContainer::isOnline(const QString& _aimId) const
    {
        return getLastSeen(_aimId).isOnline();
    }

    bool LastseenContainer::isBot(const QString& _aimId) const
    {
        return getLastSeen(_aimId, LastSeenRequestType::CacheOnly).isBot();
    }

    const Data::LastSeen& LastseenContainer::getLastSeen(const QString& _aimid, const LastSeenRequestType& _type) const
    {
        static const Data::LastSeen empty;

        if (_aimid.isEmpty() || Utils::isChat(_aimid) || Utils::isServiceAimId(_aimid))
            return empty;

        const auto& seen = contactsSeens_.find(_aimid);
        if (_type == CacheOnly)
            return seen == contactsSeens_.end() ? empty : seen->second;

        if (!updateTimer_->isActive())
            updateTimer_->start();

        auto [_, sendRequest] = subscriptions_.insert_or_assign(_aimid, _type == LastSeenRequestType::Subscribe ? QDateTime() : QDateTime::currentDateTime());

        if (seen != contactsSeens_.end())
            return seen->second;

        if (sendRequest)
        {
            toRequest_.push_back(_aimid);
            requestTimer_->start(toRequest_.size() < LASTSEEN_MAX_REQUEST_COUNT ? LAST_SEEN_REQUEST_TIMEOUT.count() : 0);
        }

        return empty;
    }

    void LastseenContainer::unsubscribe(const QString& _aimid)
    {
        if (_aimid.isEmpty() || Utils::isChat(_aimid))
            return;

        subscriptions_[_aimid] = QDateTime::currentDateTime();
    }

    void LastseenContainer::emitChanged(const QString& _aimid) const
    {
        Q_EMIT lastseenChanged(_aimid, QPrivateSignal());
        Logic::getContactListModel()->emitContactChanged(_aimid);
    }

    void LastseenContainer::userLastSeen(const int64_t, const std::map<QString, Data::LastSeen>& _lastseens)
    {
        for (const auto& [aimid, lastSeen] : _lastseens)
        {
            if (!aimid.isEmpty())
            {
                auto& seen = contactsSeens_[aimid];
                if (seen.isBot())
                    continue;

                seen = lastSeen;
                emitChanged(aimid);
            }
        }
    }

    void LastseenContainer::updateLastSeens()
    {
        auto iter = subscriptions_.begin();
        const auto now = QDateTime::currentDateTime();
        while (iter != subscriptions_.end())
        {
            const auto& subscrTime = iter->second;
            if (subscrTime.isValid() && std::chrono::milliseconds(subscrTime.msecsTo(now)) > TEMP_SUBSCRIPTION_TIMEOUT)
            {
                iter = subscriptions_.erase(iter);
                continue;
            }

            if (!Logic::getContactListModel()->contains(iter->first))
                toRequest_.push_back(iter->first);

            ++iter;
        }

        requestLastSeens();
    }

    void LastseenContainer::requestLastSeens()
    {
        if (toRequest_.empty())
            return;

        Ui::GetDispatcher()->getUserLastSeen(toRequest_);
        toRequest_.clear();
    }

    static std::unique_ptr<LastseenContainer> g_lastseen_container;

    LastseenContainer* GetLastseenContainer()
    {
        Utils::ensureMainThread();
        if (!g_lastseen_container)
            g_lastseen_container = std::make_unique<LastseenContainer>(nullptr);

        return g_lastseen_container.get();
    }

    void ResetLastseenContainer()
    {
        Utils::ensureMainThread();
        g_lastseen_container.reset();
    }

}