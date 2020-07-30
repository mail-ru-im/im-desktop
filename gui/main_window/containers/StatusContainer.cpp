#include "stdafx.h"

#include "StatusContainer.h"
#include "core_dispatcher.h"
#include "utils/InterConnector.h"

namespace
{
    constexpr std::chrono::milliseconds updateInterval = std::chrono::seconds(5);
}

namespace Logic
{
    StatusContainer::StatusContainer(QObject* _parent)
        : QObject(_parent)
        , updateTimer_(new QTimer(this))
    {
        setObjectName(qsl("StatusContainer"));

        connect(Ui::GetDispatcher(), &Ui::core_dispatcher::updateStatus, this, &StatusContainer::setStatus);
        connect(&Utils::InterConnector::instance(), &Utils::InterConnector::changeMyStatus, this, [this](const Statuses::Status& _status)
        {
            setStatus(Ui::MyInfo()->aimId(), _status);
        });

        updateTimer_->setInterval(updateInterval);
        connect(updateTimer_, &QTimer::timeout, this, &StatusContainer::onUpdateTimer);
    }

    const Statuses::Status& StatusContainer::getStatus(const QString& _aimid) const
    {
        if (!_aimid.isEmpty())
        {
            const auto it = statuses_.find(_aimid);
            if (_aimid != Ui::MyInfo()->aimId())
                updateSubscription(_aimid);

            if (it != statuses_.end())
                return it->second;
        }

        static const Statuses::Status empty;
        return empty;
    }

    void StatusContainer::setStatus(const QString& _aimid, const Statuses::Status& _status)
    {
        if (_aimid.isEmpty())
            return;

        statuses_[_aimid] = _status;
        Q_EMIT statusChanged(_aimid, QPrivateSignal());
    }

    void StatusContainer::onUpdateTimer()
    {
        const auto now = QDateTime::currentDateTime();
        std::vector<QString> toUnsub;
        toUnsub.reserve(subscriptions_.size());

        auto iter = subscriptions_.begin();
        while (iter != subscriptions_.end())
        {
            const auto subscriptionTime = iter->second;
            const auto unsub =
                visibleAvatars_.find(iter->first) == visibleAvatars_.end() &&
                subscriptionTime.isValid() &&
                subscriptionTime.msecsTo(now) > updateInterval.count() * 2;

            if (unsub)
            {
                toUnsub.push_back(iter->first);
                iter = subscriptions_.erase(iter);
                continue;
            }
            ++iter;
        }

        if (!toUnsub.empty())
            Ui::GetDispatcher()->unsubscribeStatus(toUnsub);

        if (subscriptions_.empty() && visibleAvatars_.empty())
            updateTimer_->stop();
    }

    void StatusContainer::updateSubscription(const QString& _aimid) const
    {
        auto& sub = subscriptions_[_aimid];
        const auto firstTime = !sub.isValid();
        sub = QDateTime::currentDateTime();

        if (firstTime)
            Ui::GetDispatcher()->subscribeStatus({ _aimid });

        if (!updateTimer_->isActive())
            updateTimer_->start();
    }

    void StatusContainer::setAvatarVisible(const QString& _aimid, bool _visible)
    {
        if (_aimid.isEmpty() || _aimid == Ui::MyInfo()->aimId())
            return;

        auto node = visibleAvatars_.extract(_aimid);
        if (node.empty())
        {
            if (_visible)
            {
                visibleAvatars_.insert({ _aimid, 1 });
                updateSubscription(_aimid);
            }
            return;
        }

        node.mapped() += _visible ? 1 : -1;
        if (node.mapped() <= 0)
            return;

        visibleAvatars_.insert(std::move(node));
        updateSubscription(_aimid);
    }

    static std::unique_ptr<StatusContainer> g_StatusContainer;

    StatusContainer* GetStatusContainer()
    {
        Utils::ensureMainThread();
        if (!g_StatusContainer)
            g_StatusContainer = std::make_unique<StatusContainer>(nullptr);

        return g_StatusContainer.get();
    }

    void ResetStatusContainer()
    {
        Utils::ensureMainThread();
        g_StatusContainer.reset();
    }
}
