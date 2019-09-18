#include "stdafx.h"

#include "FriendlyContainer.h"

#include "../contact_list/ContactListModel.h"
#include "../../core_dispatcher.h"
#include "../../utils/gui_coll_helper.h"
#include "../../utils/utils.h"

Q_LOGGING_CATEGORY(friendlyContainer, "friendlyContainer")

namespace
{
    constexpr std::chrono::milliseconds LAST_SEEN_UPDATE_TIMEOUT = std::chrono::minutes(1);
    constexpr std::chrono::milliseconds TEMP_SUBSCRIPTION_TIMEOUT = std::chrono::minutes(2);
}

namespace Logic
{
    FriendlyContainer::FriendlyContainer(QObject* _parent)
        : QObject(_parent)
        , lastseenUpdateTimer_(new QTimer(this))
    {
        Utils::ensureMainThread();
        setObjectName(qsl("FriendlyContainer"));
        connect(Ui::GetDispatcher(), &Ui::core_dispatcher::friendlyUpdate, this, &FriendlyContainer::onFriendlyChangedFromCore);
        connect(Ui::GetDispatcher(), &Ui::core_dispatcher::userLastSeen, this, &FriendlyContainer::userLastSeen);

        lastseenUpdateTimer_->setInterval(LAST_SEEN_UPDATE_TIMEOUT.count());
        connect(lastseenUpdateTimer_, &QTimer::timeout, this, &FriendlyContainer::updateLastSeens);
        qCDebug(friendlyContainer) << "friendlyContainer ctor";
    }

    FriendlyContainer::~FriendlyContainer()
    {
        Utils::ensureMainThread();
        qCDebug(friendlyContainer) << "friendlyContainer dtor";
    }

    QString FriendlyContainer::getFriendly(const QString& _aimid) const
    {
        return getFriendly2(_aimid).name_;
    }

    QString FriendlyContainer::getNick(const QString& _aimid) const
    {
        if (_aimid.contains(ql1c('@')) && !Utils::isChat(_aimid))
            return _aimid;

        return getFriendly2(_aimid).nick_;
    }

    bool FriendlyContainer::getOfficial(const QString& _aimid) const
    {
        return getFriendly2(_aimid).official_;
    }

    Data::Friendly FriendlyContainer::getFriendly2(const QString& _aimid) const
    {
        assert(!_aimid.isEmpty());
        Utils::ensureMainThread();
        if (const auto it = friendlyMap_.find(_aimid); it != friendlyMap_.end() && !it.value().name_.isEmpty())
            return it.value();
        return { _aimid, _aimid };
    }

    void FriendlyContainer::updateFriendly(const QString& _aimid)
    {
        if (getFriendly(_aimid) == _aimid)
            Ui::GetDispatcher()->getIdInfo(_aimid);
    }

    void FriendlyContainer::setContactLastSeen(const QString& _aimid, int32_t _lastSeen)
    {
        auto changed = true;
        if (const auto& seen = contactsSeens_.find(_aimid); seen != contactsSeens_.end())
            changed = _lastSeen != seen->second;

        if (changed)
        {
            auto subscription = subscriptions_.find(_aimid);
            if (subscription != subscriptions_.end())
            {
                subscription->second.lastseen_ = _lastSeen;
                emit lastseenChanged(_aimid, QPrivateSignal());
            }
        }

        contactsSeens_[_aimid] = _lastSeen;
    }

    int32_t FriendlyContainer::getLastSeen(const QString& _aimid, bool _subscribe)
    {
        if (!lastseenUpdateTimer_->isActive())
            lastseenUpdateTimer_->start();

        auto s = subscriptions_.find(_aimid);
        if (s != subscriptions_.end())
            return s->second.lastseen_;

        int32_t result = -1;
        if (const auto& seen = contactsSeens_.find(_aimid); seen != contactsSeens_.end())
            result = seen->second;
        else
            Ui::GetDispatcher()->getUserLastSeen({ _aimid });

        LastSeenSubscription subscription;
        subscription.lastseen_ = result;
        if (!_subscribe)
            subscription.subscriptionTime_ = QDateTime::currentDateTime();
        subscriptions_[_aimid] = std::move(subscription);

        return result;
    }

    QString FriendlyContainer::getStatusString(int32_t _lastseen)
    {
        if (_lastseen == -1)
            return QString();

        if (_lastseen == 0)
            return QT_TRANSLATE_NOOP("state", "Online");

        QString res;

        const auto current = QDateTime::currentDateTime();
        auto lastSeen = QDateTime::fromTime_t(_lastseen);

        const std::chrono::seconds seconds(lastSeen.secsTo(current));

        QString suffix;
        if (seconds < std::chrono::hours(1))
        {
            const auto mins = seconds.count() / 60;

            if (mins <= 0)
            {
                suffix = QT_TRANSLATE_NOOP("date", "just now");
            }
            else
            {
                suffix = Utils::GetTranslator()->getNumberString(
                    mins,
                    QT_TRANSLATE_NOOP3("date", "%1 minute ago", "1"),
                    QT_TRANSLATE_NOOP3("date", "%1 minutes ago", "2"),
                    QT_TRANSLATE_NOOP3("date", "%1 minutes ago", "5"),
                    QT_TRANSLATE_NOOP3("date", "%1 minutes ago", "21")).arg(mins);
            }
        }
        else
        {
            const auto days = lastSeen.daysTo(current);
            const auto sameYear = lastSeen.date().year() == current.date().year();

            if (days == 0)
                suffix = QT_TRANSLATE_NOOP("date", "today");
            else if (days == 1)
                suffix = QT_TRANSLATE_NOOP("date", "yesterday");
            else if (days < 7)
                suffix = Utils::GetTranslator()->formatDayOfWeek(lastSeen.date());
            else if (days > 365)
                suffix = QT_TRANSLATE_NOOP("date", "a long time ago");
            else
                suffix = Utils::GetTranslator()->formatDate(lastSeen.date(), sameYear);

            if (sameYear || days < 7)
                suffix += QT_TRANSLATE_NOOP("date", " at ") % lastSeen.time().toString(Qt::SystemLocaleShortDate);
        }

        if (!suffix.isEmpty())
            res = QT_TRANSLATE_NOOP("date", "Seen ") % suffix;

        return res;
    }

    QString FriendlyContainer::getStatusString(const QString& _aimid)
    {
        return getStatusString(getLastSeen(_aimid));
    }

    void FriendlyContainer::unsubscribe(const QString& _aimid)
    {
        subscriptions_.erase(_aimid);
    }

    void FriendlyContainer::onFriendlyChangedFromCore(const QString& _aimid, const Data::Friendly& _friendly)
    {
        Utils::ensureMainThread();
        auto& friendly = friendlyMap_[_aimid];
        qCDebug(friendlyContainer) << "onFriendlyChangedFromCore update friendly: aimid" << _aimid
            << "old friendly" << friendly.name_ << Utils::makeNick(friendly.nick_) << friendly.official_
            << "new friendly" << _friendly.name_ << Utils::makeNick(_friendly.nick_) << _friendly.official_;

        friendly = _friendly;
        emit friendlyChanged(_aimid, _friendly.name_, QPrivateSignal());
        emit friendlyChanged2(_aimid, _friendly, QPrivateSignal());
    }

    void FriendlyContainer::userLastSeen(const int64_t _seq, const std::map<QString, int32_t>& _lastseens)
    {
        for (const auto& iter : _lastseens)
        {
            auto aimid = iter.first;
            auto subscription = subscriptions_.find(aimid);
            if (subscription != subscriptions_.end())
            {
                auto lastseen = iter.second;
                auto subscriptionTime = subscription->second.subscriptionTime_;
                auto changed = subscription->second.lastseen_ != lastseen;

                LastSeenSubscription s;
                s.lastseen_ = lastseen;
                s.subscriptionTime_ = subscriptionTime;
                subscriptions_[aimid] = std::move(s);

                if (!subscriptionTime.isValid() && changed)
                    emit lastseenChanged(aimid, QPrivateSignal());
            }
        }
    }

    void FriendlyContainer::updateLastSeens()
    {
        std::vector<QString> lastseens;
        auto iter = subscriptions_.begin();
        const auto now = QDateTime::currentDateTime();
        while (iter != subscriptions_.end())
        {
            auto subscriptionTime = iter->second.subscriptionTime_;
            if (subscriptionTime.isValid() && subscriptionTime.msecsTo(now) > TEMP_SUBSCRIPTION_TIMEOUT.count())
            {
                iter = subscriptions_.erase(iter);
                continue;
            }

            if (!Logic::getContactListModel()->getContactItem(iter->first))
                lastseens.push_back(iter->first);

            ++iter;
        }

        if (!lastseens.empty())
            Ui::GetDispatcher()->getUserLastSeen(lastseens);
    }

    static std::unique_ptr<FriendlyContainer> g_friendly_container; // global. TODO/FIXME encapsulate with other global objects like recentsModel, contactModel

    FriendlyContainer* GetFriendlyContainer()
    {
        Utils::ensureMainThread();
        if (!g_friendly_container)
            g_friendly_container = std::make_unique<FriendlyContainer>(nullptr);

        return g_friendly_container.get();
    }

    void ResetFriendlyContainer()
    {
        Utils::ensureMainThread();
        g_friendly_container.reset();
    }
}