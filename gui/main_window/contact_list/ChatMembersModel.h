#pragma once

#include "AbstractSearchModel.h"

#include "../../types/contact.h"
#include "../../types/message.h"
#include "../../types/chat.h"
#include "../../types/search_result.h"
#include "../../utils/utils.h"
#include "../../core_dispatcher.h"
#include "ContactItem.h"

namespace Logic
{
    class ContactSearcher;

    const unsigned int InitMembersLimit = 20;
    const unsigned int MembersPageSize = 40;

    class ChatMembersModel : public AbstractSearchModel
    {
        Q_OBJECT

    private Q_SLOTS:
        void onAvatarLoaded(const QString& _aimId);
        void onChatMembersPage(const qint64 _seq, const std::shared_ptr<Data::ChatMembersPage>& _page, const int _pageSize, const bool _resetPages);
        void onChatMembersPageCached(const std::shared_ptr<Data::ChatMembersPage>& _page, const QString& _role);
        void onSearchChatMembersPage(const qint64 _seq, const std::shared_ptr<Data::ChatMembersPage>& _page, const int _pageSize, const bool _resetPages);

        void doSearch();
        void onContactsServer();

    public:
        ChatMembersModel(QObject* _parent);
        ChatMembersModel(const std::shared_ptr<Data::ChatInfo>& _info, QObject* _parent);

        ~ChatMembersModel();

        int rowCount(const QModelIndex& _parent = QModelIndex()) const override;
        QVariant data(const QModelIndex& _index, int _role) const override;

        void clearCheckedItems() override;
        int getCheckedItemsCount() const override;
        void setCheckedItem(const QString& _name, bool _isChecked) override;
        bool isCheckedItem(const QString& _name) const override;

        std::vector<QString> getCheckedMembers() const;
        void setCheckedFirstMembers(int _count);

        void setFocus() override;
        void setSearchPattern(const QString& _pattern) override;

        Data::ChatMemberSearchResultSptr getMemberItem(const QString& _aimId) const;
        int getMembersCount() const;

        void loadAllMembers();
        void loadBlocked();
        void loadPending();
        void loadAdmins();
        void loadYourInvites();
        void requestNextPage();

        bool isFullMembersListLoaded() const;

        void addIgnoredMember(const QString& _aimId);
        bool isIgnoredMember(const QString& _aimId);
        void clearIgnoredMember();

        bool contains(const QString& _aimId) const override;
        QString getChatAimId() const;
        void updateInfo(const std::shared_ptr<Data::ChatInfo> &_info);

        unsigned getVisibleRowsCount() const;

        static int get_limit(int _limit);

        bool isAdmin() const;
        bool isModer() const;
        void setAimId(const QString& _aimId);

        //!! todo: temporarily for an old method "approve all" in GroupProfile::approveAllClicked()
        const std::vector<Data::ChatMemberInfo>& getMembers() const;

        bool isClickableItem(const QModelIndex& _index) const override;

    private:
        enum class MembersMode
        {
            All,
            Admins,
            Pending,
            Blocked,
            YourInvites,
        };

        enum class CheckMode
        {
            Normal,
            Force
        };

        void loadImpl(MembersMode _mode);

        QStringView getRoleName(MembersMode _mode) const;

        void addResults(const std::shared_ptr<Data::ChatMembersPage>& _page, bool _fromCache = false);
        void addSearcherResults();
        void add(const QString& _aimId);
        void clearResults();

        void loadChatMembersPage();
        void searchChatMembersPage();

        bool isSeqRelevant(qint64 _answerSeq) const;

        mutable Data::SearchResultsV results_;
        //!! todo: temporarily for an old method "approve all" in GroupProfile::approveAllClicked()
        std::vector<Data::ChatMemberInfo> members_;

        int membersCount_;
        int countFirstCheckedMembers_;
        QString AimId_;
        qint64 adminInfoSequence_;
        QString YourRole_;
        QString cursorNextPage_;
        bool isDoingRequest_;
        bool isInitialPage_;

        MembersMode mode_;

        QTimer* timerSearch_;

        /* sequences_[mode_] == last req sequence */
        std::map<MembersMode, qint64> sequences_;

        std::unordered_set<QString, Utils::QStringHasher> checkedMembers_;
        std::unordered_set<QString, Utils::QStringHasher> ignoredMembers_;

        ContactSearcher* contactSearcher_;
        CheckMode checkMode_;
    };
}
