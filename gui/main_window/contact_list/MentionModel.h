#pragma once

#include "AbstractSearchModel.h"
#include "../history_control/history/History.h"
#include "../history_control/history/Message.h"
#include "../../types/mention_suggest.h"

Q_DECLARE_LOGGING_CATEGORY(mentionsModel)

namespace Logic
{
    class ChatMembersModel;
    class ContactSearcher;

    class MentionModel : public AbstractSearchModel
    {
        Q_OBJECT

    Q_SIGNALS:
        void mentionSelected(const QString& _aimid, const QString& _friendly);

    private Q_SLOTS:
        void onContactListResults();
        void messageBuddies(const Data::MessageBuddies& _buddies, const QString& _aimId, Ui::MessagesBuddiesOpt _option, bool _havePending, qint64 _seq, int64_t _last_msgid);
        void onContactSelected(const QString&);
        void avatarLoaded(const QString& _aimid);
        void friendlyChanged(const QString& _aimId, const QString& _friendlyName);

        void onServerTimeout();
        void onServerResults(const int64_t _seq, const MentionsSuggests& _suggests);

        void search();

    public:
        MentionModel(QObject* _parent);

        int rowCount(const QModelIndex& _parent = QModelIndex()) const override;
        QVariant data(const QModelIndex& _index, int _role) const override;

        enum class SearchCondition
        {
            IfPatternChanged,
            Force
        };

        void setSearchPattern(const QString& _pattern, const SearchCondition _condition = SearchCondition::IfPatternChanged);
        inline void setSearchPattern(const QString& _pattern) override
        {
            setSearchPattern(_pattern, SearchCondition::IfPatternChanged);
        }

        void setDialogAimId(const QString& _aimid);
        QString getDialogAimId() const;

        bool isServiceItem(const QModelIndex& _index) const override;
        bool isClickableItem(const QModelIndex& _index) const override;
        bool isFromChatSenders(const QModelIndex& _index) const;

    private:
        void requestServer();

        void onRequestReturned();
        void composeResults();

        void filterLocal();

    private:
        ContactSearcher* localSearcher_;

        QString dialogAimId_;
        bool sendServerRequests_;

        MentionsSuggests localResults_;
        MentionsSuggests serverResults_;
        MentionsSuggests clResults_;
        MentionsSuggests match_;

        std::map<QString, MentionsSuggests> chatSenders_;
        int curChatSendersCount_;

        QTimer* typingTimer_;
        QTimer* responseTimer_;
        int64_t requestId_;

        bool serverFailed_;
        int responseTime_;

        int requestsReturnedCount_;
        int requestsReturnedNeeded_;
    };

    MentionModel* GetMentionModel();
    void ResetMentionModel();
}
