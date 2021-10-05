#pragma once

#include "types/reactions.h"

namespace Ui
{

class MessageReactions;

class ReactionsLoader_p;

//////////////////////////////////////////////////////////////////////////
// ReactionsLoader
//////////////////////////////////////////////////////////////////////////

class ReactionsLoader : public QObject
{
    Q_OBJECT

public:
    static ReactionsLoader* instance();
    void subscribe(MessageReactions* _reactions);

private Q_SLOTS:
    void onReactions(const QString& _contact, const std::vector<Data::Reactions>& _reactions);

private:
    ReactionsLoader();
    ~ReactionsLoader();

    ReactionsLoader(ReactionsLoader const&) = delete;
    void operator=(ReactionsLoader const&)  = delete;

    std::unique_ptr<ReactionsLoader_p> d;
};

class ChatReactionsLoader_p;

//////////////////////////////////////////////////////////////////////////
// ChatReactionsLoader
//////////////////////////////////////////////////////////////////////////

class ChatReactionsLoader : public QObject
{
    Q_OBJECT

Q_SIGNALS:
    void unsubscribed(int64_t _msgId, QPrivateSignal);

public:
    ChatReactionsLoader();
    ~ChatReactionsLoader();

    void subscribe(MessageReactions* _reactions);
    void unsubscribe(int64_t _msgId);

    bool empty() const;
    void setReactions(const std::vector<Data::Reactions>& _reactions);

private Q_SLOTS:
    void loadReactions();

private:
    std::unique_ptr<ChatReactionsLoader_p> d;
};


}
