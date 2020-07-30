#include "stdafx.h"

#include "utils/utils.h"

#include "MessageReactions.h"
#include "ReactionsLoader.h"
#include "core_dispatcher.h"

namespace
{
    constexpr std::chrono::milliseconds subscriptionInterval = std::chrono::seconds(60);
}

namespace Ui
{

//////////////////////////////////////////////////////////////////////////
// ReactionsLoader_p
//////////////////////////////////////////////////////////////////////////

class ReactionsLoader_p
{
public:
    void onReactionDestroyed(const QString& _contact, int64_t _msgId)
    {
        auto& loader = loaders_[_contact];
        loader.unsubscribe(_msgId);

        if (loader.empty())
            loaders_.erase(_contact);
    }

    std::unordered_map<QString, ChatReactionsLoader, Utils::QStringHasher> loaders_;
};

//////////////////////////////////////////////////////////////////////////
// ReactionsLoader
//////////////////////////////////////////////////////////////////////////

ReactionsLoader::ReactionsLoader()
    : d(std::make_unique<ReactionsLoader_p>())
{
    connect(GetDispatcher(), &core_dispatcher::reactions, this, &ReactionsLoader::onReactions);
}

ReactionsLoader::~ReactionsLoader()
{

}

ReactionsLoader* ReactionsLoader::instance()
{
    static ReactionsLoader instance;
    return &instance;
}

void ReactionsLoader::subscribe(MessageReactions* _reactions)
{
    const auto contact = _reactions->contact();
    const auto msgId = _reactions->msgId();

    auto& loader = d->loaders_[contact];
    loader.subscribe(_reactions);

    connect(_reactions, &MessageReactions::destroyed, this, [this, contact, msgId]() { d->onReactionDestroyed(contact, msgId); });

    GetDispatcher()->getReactions(contact, { msgId }, true);
}

void ReactionsLoader::onReactions(const QString& _contact, const std::vector<Data::Reactions>& _reactions)
{
    auto it = d->loaders_.find(_contact);

    if (it != d->loaders_.end())
        it->second.setReactions(_reactions);
}

//////////////////////////////////////////////////////////////////////////
// ChatReactionsLoader_p
//////////////////////////////////////////////////////////////////////////

class ChatReactionsLoader_p
{
public:
    QTimer timer_;
    QString chatId_;
    std::unordered_map<int64_t, QPointer<MessageReactions>> reactions_;
};

//////////////////////////////////////////////////////////////////////////
// ChatReactionsLoader
//////////////////////////////////////////////////////////////////////////

ChatReactionsLoader::ChatReactionsLoader()
    : d(std::make_unique<ChatReactionsLoader_p>())
{
    connect(&d->timer_, &QTimer::timeout, this, &ChatReactionsLoader::loadReactions);
    d->timer_.setInterval(subscriptionInterval.count());
}

ChatReactionsLoader::~ChatReactionsLoader()
{

}

void ChatReactionsLoader::subscribe(MessageReactions* _reactions)
{
    d->reactions_[_reactions->msgId()] = _reactions;

    if (d->chatId_.isEmpty())
        d->chatId_ = _reactions->contact();

    if (!d->timer_.isActive())
        d->timer_.start();
}

void ChatReactionsLoader::unsubscribe(int64_t _msgId)
{
    d->reactions_.erase(_msgId);
}

bool ChatReactionsLoader::empty() const
{
    return d->reactions_.empty();
}

void ChatReactionsLoader::setReactions(const std::vector<Data::Reactions>& _reactions)
{
    for (auto& reactions : _reactions)
    {
        auto it = d->reactions_.find(reactions.msgId_);
        if (it != d->reactions_.end())
        {
            if (auto messageReactions = it->second)
                messageReactions->setReactions(reactions);
        }
    }
}

void ChatReactionsLoader::loadReactions()
{
    std::vector<int64_t> ids;

    for (auto& [id, _] : d->reactions_)
        ids.push_back(id);

    GetDispatcher()->getReactions(d->chatId_, ids);
}

}
