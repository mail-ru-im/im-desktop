#include "stdafx.h"

#include "FriendlyContainer.h"

#include "../../core_dispatcher.h"
#include "../../utils/utils.h"
#include "../../utils/InterConnector.h"

Q_LOGGING_CATEGORY(friendlyContainer, "friendlyContainer")

namespace Logic
{
    FriendlyContainer::FriendlyContainer(QObject* _parent)
        : QObject(_parent)
    {
        Utils::ensureMainThread();
        setObjectName(qsl("FriendlyContainer"));
        connect(Ui::GetDispatcher(), &Ui::core_dispatcher::friendlyUpdate, this, &FriendlyContainer::onFriendlyChangedFromCore);

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
        if (_aimid.contains(u'@') && !Utils::isChat(_aimid))
            return _aimid;

        return getFriendly2(_aimid).nick_;
    }

    bool FriendlyContainer::getOfficial(const QString& _aimid) const
    {
        return getFriendly2(_aimid).official_;
    }

    Data::Friendly FriendlyContainer::getFriendly2(const QString& _aimid) const
    {
        im_assert(!_aimid.isEmpty());
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

    void FriendlyContainer::onFriendlyChangedFromCore(const QString& _aimid, const Data::Friendly& _friendly)
    {
        Utils::ensureMainThread();
        auto& friendly = friendlyMap_[_aimid];
        qCDebug(friendlyContainer) << "onFriendlyChangedFromCore update friendly: aimid" << _aimid
            << "old friendly" << friendly.name_ << Utils::makeNick(friendly.nick_) << friendly.official_
            << "new friendly" << _friendly.name_ << Utils::makeNick(_friendly.nick_) << _friendly.official_;

        friendly = _friendly;
        Q_EMIT friendlyChanged(_aimid, _friendly.name_, QPrivateSignal());
        Q_EMIT friendlyChanged2(_aimid, _friendly, QPrivateSignal());
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