#pragma once

#include "CustomAbstractListModel.h"
#include "ContactItem.h"
#include "types/chat.h"
#include "types/thread.h"

Q_DECLARE_LOGGING_CATEGORY(clModel)

namespace Utils
{
    class InterConnector;
}

namespace core
{
    enum class group_chat_info_errors;
}

namespace Ui
{
    class PageBase;
}

namespace Data
{
    struct IdInfo;
}

namespace Logic
{
    namespace ContactListSorting
    {
        using contact_sort_pred = std::function<bool(const Logic::ContactItem&, const Logic::ContactItem&)>;
    }

    using profile_ptr = std::shared_ptr<class contact_profile>;

    enum class UpdateChatSelection
    {
        No,
        Yes
    };

    struct CachedChatData
    {
        QString stamp_;
        QString name_;
        QString description_;
        QString rules_;
        bool joinModeration_ = false;
        bool public_ = false;
        bool channel_ = false;
        bool trustRequired_ = false;
    };

    class ContactListModel : public CustomAbstractListModel
    {
        Q_OBJECT

    Q_SIGNALS:
        void currentDlgStateChanged() const;
        void selectedContactChanged(const QString& _new, const QString& _prev);
        void contactChanged(const QString&, QPrivateSignal) const;
        void select(const QString&, qint64, Logic::UpdateChatSelection mode = Logic::UpdateChatSelection::No, bool _ignoreScroll = false) const;
        void profile_loaded(profile_ptr _profile) const;
        void contact_added(const QString& _contact, bool _result);
        void contact_removed(const QString& _contact);
        void leave_dialog(const QString& _contact);
        void needSwitchToRecents();
        void liveChatJoined(const QString&);
        void liveChatRemoved(const QString&);
        void switchTab(const QString&);
        void ignore_contact(const QString&);
        void yourRoleChanged(const QString&);
        void contactAdd(const QString&);
        void contactRename(const QString&);

    private Q_SLOTS:
        void contactList(std::shared_ptr<Data::ContactList>, const QString&);
        void avatarLoaded(const QString&);
        void presence(std::shared_ptr<Data::Buddy>);
        void contactRemoved(const QString&);
        void outgoingMsgCount(const QString& _aimid, const int _count);
        void dialogGalleryState(const QString& _aimId, const Data::DialogGalleryState& _state);
        void onThreadUpdates(const Data::ThreadUpdates& _updates);
        void messageBuddies(const Data::MessageBuddies& _buddies);

    public Q_SLOTS:
        void chatInfo(qint64, const std::shared_ptr<Data::ChatInfo>&, const int _requestMembersLimit);
        void chatInfoFailed(qint64, core::group_chat_info_errors _errorCode, const QString& _aimId);
        void onIdInfo(qint64, const Data::IdInfo& _info);

        void sort();
        void forceSort();
        void scrolled(int);
        void groupClicked(int);

    public:
        explicit ContactListModel(QObject* _parent);

        using CustomAbstractListModel::sort;

        virtual QVariant data(const QModelIndex& _index, int _role) const override;
        virtual Qt::ItemFlags flags(const QModelIndex& _index) const override;
        virtual int rowCount(const QModelIndex& _parent = QModelIndex()) const override;

        ContactItem* getContactItem(const QString& _aimId);
        QString getAimidByABName(const QString& _name);
        void setCurrentCallbackHappened(Ui::PageBase* _page);

        const ContactItem* getContactItem(const QString& _aimId) const;

        const QString& selectedContact() const;

        void add(const QString& _aimId, const QString& _friendly, bool _isTemporary = false);

        void setContactVisible(const QString& _aimId, bool _visible);

        int getOutgoingCount(const QString& _aimId) const;
        bool isChat(const QString& _aimId) const;
        bool isMuted(const QString& _aimId) const;
        bool isLiveChat(const QString& _aimId) const;
        bool isOfficial(const QString& _aimId) const;
        bool isPublic(const QString& _aimId) const;
        QModelIndex contactIndex(const QString& _aimId) const;

        void addContactToCL(const QString& _aimId, std::function<void(bool)> _callBack = {});
        void ignoreContact(const QString& _aimId, bool _ignore);
        bool ignoreContactWithConfirm(const QString& _aimId);
        bool areYouAdmin(const QString& _aimId) const;
        QString getYourRole(const QString& _aimId) const;
        void setYourRole(const QString& _aimId, const QString& _role);
        void removeContactFromCL(const QString& _aimId);
        void renameContact(const QString& _aimId, const QString& _friendly, std::function<void(bool)> _callBack = {});
        void static getIgnoreList();

        bool areYouNotAMember(const QString& _aimid) const;
        bool areYouBlocked(const QString& _aimId) const;
        bool areYouAllowedToWriteInThreads(const QString& _aimId) const;

        void removeContactsFromModel(const QVector<QString>& _vcontacts, bool _emit = true);
        void removeTemporaryContactsFromModel();

        std::vector<QString> getCheckedContacts() const;
        void clearCheckedItems() override;
        int getCheckedItemsCount() const override;
        void setCheckedItem(const QString& _aimId, bool _isChecked) override;
        void setIsWithCheckedBox(bool);
        bool isCheckedItem(const QString& _aimId) const override;

        bool isWithCheckedBox() const;
        QString contactToTryOnTheme() const;
        void joinLiveChat(const QString& _stamp, bool _silent);

        void next();
        void prev();

        bool contains(const QString& _aimId) const override;
        bool hasContact(const QString& _aimId) const;

        int getTopSortedCount() const;

        int contactsCount() const;

        bool isChannel(const QString& _aimId) const;
        bool isReadonly(const QString& _aimId) const; // no write access to this chat, for channel check use isChannel
        bool isJoinModeration(const QString& _aimId) const;
        bool isTrustRequired(const QString& _aimId) const;

        enum class RequestIfEmpty
        {
            No,
            Yes,
        };
        const Data::DialogGalleryState& getGalleryState(const QString& _aimid, RequestIfEmpty _request = RequestIfEmpty::Yes) const;

        QString getChatStamp(const QString& _aimid) const;
        QString getChatName(const QString& _aimid) const;
        QString getChatDescription(const QString& _aimid) const;
        QString getChatRules(const QString& _aimId) const;

        const QString& getAimidByStamp(const QString& _aimId) const;

        void updateChatInfo(const Data::ChatInfo& _chat_info);

        void emitContactChanged(const QString& _aimId) const;

        bool isThread(const QString& _aimId) const;
        void markAsThread(const QString& _threadId, const QString& _parentChatId);
        std::optional<QString> getThreadParent(const QString& _threadId) const;

    private:
        void setCurrent(const QString& _aimId, qint64 _id = -1, bool _select = true, std::function<void(Ui::PageBase*)> _getPageCallback = {}, bool _ignoreScroll = false);
        void rebuildIndex();
        void rebuildVisibleIndex();
        int addItem(Data::ContactPtr _contact, QVector<QString>& _removed, const bool _updatePlaceholder = true);
        void pushChange(int i);
        void processChanges();
        int getIndexByOrderedIndex(int _index) const;
        int getOrderIndexByAimid(const QString& _aimId) const;
        void updateSortedIndexesList(std::vector<int>& _list, ContactListSorting::contact_sort_pred _less);
        ContactListSorting::contact_sort_pred getLessFuncCL(const QDateTime& current) const;
        void updateIndexesListAfterRemoveContact(std::vector<int>& _list, int _index);
        int innerRemoveContact(const QString& _aimId);

        int getAbsIndexByVisibleIndex(int _visibleIndex) const;

        bool isGroupsEnabled() const;

        std::function<void(Ui::PageBase*)> gotPageCallback_;
        std::vector<ContactItem> contacts_;
        std::vector<int> sorted_index_cl_;
        QHash<QString, int> indexes_;
        std::vector<int> visible_indexes_;
        bool sortNeeded_;
        QTimer* sortTimer_;

        int scrollPosition_;
        mutable int minVisibleIndex_;
        mutable int maxVisibleIndex_;
        std::vector<int> updatedItems_;

        QString currentAimId_;

        bool isWithCheckedBox_;
        int topSortedCount_;

        std::map<QString, Data::DialogGalleryState> galleryStates_;

        QSet<QString> deletedContacts_;

        QMap<QString, CachedChatData> chatsCache_;
        std::vector<QString> notAMemberChats_;
        std::vector<QString> youBlockedChats_;
        std::unordered_map<QString, QString> threads_; // threadId, parentChatId

        friend class Utils::InterConnector;
    };

    ContactListModel* getContactListModel();
    void ResetContactListModel();
}

Q_DECLARE_METATYPE(Logic::UpdateChatSelection);
