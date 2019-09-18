#pragma once

#include "AbstractSearchModel.h"

#include "../../types/contact.h"
#include "../../types/message.h"
#include "../../types/chat.h"
#include "../../types/search_result.h"
#include "../../core_dispatcher.h"
#include "ContactItem.h"

namespace Logic
{
    const unsigned int InitMembersLimit = 20;
    const unsigned int MembersPageSize = 40;

    class ChatMembersModel : public AbstractSearchModel
    {
        Q_OBJECT

    private Q_SLOTS:
        void onAvatarLoaded(const QString& _aimId);
        void onChatMembersPage(const qint64 _seq, const std::shared_ptr<Data::ChatMembersPage>& _page, const int _pageSize, const bool _resetPages);
        void onSearchChatMembersPage(const qint64 _seq, const std::shared_ptr<Data::ChatMembersPage>& _page, const int _pageSize, const bool _resetPages);

        void doSearch();

    public:
        ChatMembersModel(QObject* _parent);
        ChatMembersModel(const std::shared_ptr<Data::ChatInfo>& _info, QObject* _parent);

        ~ChatMembersModel();

        int rowCount(const QModelIndex& _parent = QModelIndex()) const override;
        QVariant data(const QModelIndex& _index, int _role) const override;

        void setFocus() override;
        void setSearchPattern(const QString& _pattern) override;

        Data::ChatMemberSearchResultSptr getMemberItem(const QString& _aimId) const;
        int getMembersCount() const;

        void loadAllMembers();
        void loadBlocked();
        void loadPending();
        void loadAdmins();
        void requestNextPage();

        bool isFullMembersListLoaded() const;

        bool contains(const QString& _aimId) const override;
        QString getChatAimId() const;
        void updateInfo(const std::shared_ptr<Data::ChatInfo> &_info);

        unsigned getVisibleRowsCount() const;

        static int get_limit(int _limit);

        bool isAdmin() const;
        bool isModer() const;
        void setAimId(const QString& _aimId);

        //!! todo: temporarily for voip
        const std::vector<Data::ChatMemberInfo>& getMembers() const;

    private:
        enum class MembersMode
        {
            All,
            Admins,
            Pending,
            Blocked
        };

        QString getRoleName(MembersMode _mode) const;

        void addResults(const std::shared_ptr<Data::ChatMembersPage>& _page);
        void clearResults();

        void loadChatMembersPage();
        void searchChatMembersPage();

        bool isSeqRelevant(qint64 _answerSeq) const;

        mutable Data::SearchResultsV results_;
        //!! todo: temporarily for voip
        std::vector<Data::ChatMemberInfo> members_;

        int membersCount_;
        QString AimId_;
        qint64 adminInfoSequence_;
        QString YourRole_;
        QString cursorNextPage_;
        bool isDoingRequest_;

        MembersMode mode_;

        QTimer* timerSearch_;

        /* sequences_[mode_] == last req sequence */
        std::map<MembersMode, qint64> sequences_;
    };
}
