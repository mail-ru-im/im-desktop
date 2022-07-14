#include "stdafx.h"

#include "StatusContainer.h"
#include "core_dispatcher.h"
#include "utils/InterConnector.h"
#include "utils/features.h"
#include "main_window/contact_list/ServiceContacts.h"
#include "main_window/contact_list/Common.h"
#include "my_info.h"

namespace
{
    constexpr std::chrono::milliseconds updateInterval = std::chrono::seconds(5);
    bool isValidContact(const QString& _contact)
    {
        return !_contact.isEmpty() && !ServiceContacts::isServiceContact(_contact);
    }
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

        connect(Ui::GetDispatcher(), &Ui::core_dispatcher::externalUrlConfigUpdated, this, &StatusContainer::updateStatusBannerEmoji);
        connect(&Utils::InterConnector::instance(), &Utils::InterConnector::omicronUpdated, this, &StatusContainer::updateStatusBannerEmoji);
        updateStatusBannerEmoji();
    }

    const Statuses::Status& StatusContainer::getStatus(const QString& _aimid) const
    {
        if (isValidContact(_aimid))
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
        if (!isValidContact(_aimid))
            return;

        const auto endTime = _status.getEndTime();
        if (endTime.isValid() && endTime < QDateTime::currentDateTime())
            statuses_[_aimid] = Statuses::Status();
        else
            statuses_[_aimid] = _status;

        Q_EMIT statusChanged(_aimid, QPrivateSignal());
    }

    bool StatusContainer::isStatusBannerNeeded(const QString& _aimid) const
    {
        const auto& status = getStatus(_aimid);
        return !status.isEmpty() && getStatusBannerEmoji().contains(status.toString());
    }

    void StatusContainer::onUpdateTimer()
    {
        discardMyStatusIfTimeIsOver();

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

    void StatusContainer::discardMyStatusIfTimeIsOver()
    {
        const auto myAimId = Ui::MyInfo()->aimId();

        const auto discardMyStatus = [this, myAimId]()
        {
            if (const auto myStatusIt = statuses_.find(myAimId); myStatusIt != std::end(statuses_))
            {
                const auto& status = myStatusIt->second;
                if (!status.isEmpty() && QDateTime::currentDateTime() >= status.getEndTime())
                    setStatus(myAimId, {});
            }
        };

        if (const auto myStatusIt = statuses_.find(myAimId); myStatusIt != std::end(statuses_))
        {
            if (myStatusIt->second.getEndTime().isValid())
            {
                const auto msecsLeft = std::chrono::milliseconds{ QDateTime::currentDateTime().msecsTo(myStatusIt->second.getEndTime()) };
                QTimer::singleShot(std::max(std::chrono::milliseconds(0), msecsLeft), this, discardMyStatus);
            }
        }
    }

    void StatusContainer::updateStatusBannerEmoji()
    {
        statusBannerEmoji_ = Features::statusBannerEmojis().split(ql1c(','), Qt::SkipEmptyParts);
        for (auto& e : statusBannerEmoji_)
            e = std::move(e).trimmed();
    }

    void StatusContainer::setAvatarVisible(const QString& _aimid, bool _visible)
    {
        if (!isValidContact(_aimid) || _aimid == Ui::MyInfo()->aimId())
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

    static QObjectUniquePtr<StatusContainer> g_StatusContainer;

    StatusContainer* GetStatusContainer()
    {
        Utils::ensureMainThread();
        if (!g_StatusContainer)
            g_StatusContainer = makeUniqueQObjectPtr<StatusContainer>();

        return g_StatusContainer.get();
    }

    void ResetStatusContainer()
    {
        Utils::ensureMainThread();
        g_StatusContainer.reset();
    }
}
