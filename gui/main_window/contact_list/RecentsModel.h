#pragma once

#include "CustomAbstractListModel.h"

#include "../../types/contact.h"
#include "../../types/message.h"
#include "../../types/typing.h"
#include "../../utils/utils.h"

namespace Ui
{
    class MainWindow;
    enum class MessagesBuddiesOpt;
    enum class ClosePage;
}

namespace Logic
{
    namespace Recents
    {
        struct FriendlyItemText
        {
            int64_t msgId_;
            QString friendlyText_;
        };
    }

    class RecentsModel : public CustomAbstractListModel
    {
        Q_OBJECT

    Q_SIGNALS:
        void orderChanged();
        void updated();
        void readStateChanged(const QString&);
        void selectContact(const QString&);
        void dlgStatesHandled(const QVector<Data::DlgState>&);
        void favoriteChanged(const QString&);
        void unimportantChanged(const QString&);
        void dlgStateChanged(const Data::DlgState&);
        void refreshAll();

    public Q_SLOTS:
        void refresh();
        void refreshUnknownMessages();

    private Q_SLOTS:

        void activeDialogHide(const QString&, Ui::ClosePage _closePage);
        void contactChanged(const QString&);
        void selectedContactChanged(const QString& _new, const QString& _prev);
        void contactAvatarChanged(const QString&);
        void dlgStates(const QVector<Data::DlgState>&);
        void messageBuddies(const Data::MessageBuddies&, const QString&, Ui::MessagesBuddiesOpt, bool _havePending, qint64 _seq, int64_t _lastMsgId);
        void messagesEmpty(const QString& _aimId, qint64 _seq);
        void sortDialogs();
        void contactRemoved(const QString&);
        void typingStatus(const Logic::TypingFires& _typing, bool _isTyping);
        void onUnreadThreadsCount(int _unreadCount, int _unreadMentionsCount);
        void onThreadUpdates(const Data::ThreadUpdates& _updates);

    public:
        explicit RecentsModel(QObject* _parent = nullptr);

        int rowCount(const QModelIndex& _index = QModelIndex()) const override;
        QVariant data(const QModelIndex& _index, int _role) const override;

        Data::DlgState getDlgState(const QString& _aimId = {}, bool _fromDialog = false);
        void unknownToRecents(const Data::DlgState& _dlgState);

        void togglePinnedVisible();
        void toggleUnimportantVisible();

        void unknownAppearance();

        enum class ReadMode
        {
            ReadAll,
            ReadText
        };

        void sendLastRead(const QString& aimId = QString(), bool force = false, ReadMode _mode = ReadMode::ReadText);
        std::pair<int, int> markAllRead();
        void setAttention(const QString& _aimId, const bool _value);
        bool getAttention(const QString& _aimId) const;
        void hideChat(const QString& aimId);
        void muteChat(const QString& aimId, bool mute);

        bool isFavorite(const QString& aimid) const;
        bool isUnimportant(const QString& _aimid) const;
        bool isSuspicious(const QString& _aimid) const;
        bool isStranger(const QString& _aimid);
        bool isServiceItem(const QModelIndex& _index) const override;
        bool isClickableItem(const QModelIndex& _index) const override;
        bool isDropableItem(const QModelIndex& _index) const override;
        bool isPinnedServiceItem(const QModelIndex& _index) const;
        bool isPinnedGroupButton(const QModelIndex& i) const;
        bool isPinnedVisible() const;
        quint16 getPinnedCount() const;
        bool isUnimportantGroupButton(const QModelIndex& i) const;
        bool isUnimportantVisible() const;
        quint16 getUnimportantCount() const;
        bool isRecentsHeader(const QModelIndex& i) const;

        bool contains(const QString& _aimId) const override;
        QModelIndex contactIndex(const QString& _aimId) const;

        int64_t getLastMsgId(const QString& _aimId) const;

        void setItemFriendlyText(const QString& _aimId, const Recents::FriendlyItemText& _frText);

        int totalUnreads() const;
        int unimportantUnreads() const;
        int pinnedUnreads() const;

        bool hasAttentionDialogs() const;
        bool hasAttentionPinned() const;
        bool hasAttentionUnimportant() const;

        bool hasMentionsInPinned() const;
        bool hasMentionsInUnimportant() const;

        int getMutedPinnedWithMentions() const;
        int getMutedUnimportantWithMentions() const;

        QString firstContact() const;
        QString nextUnreadAimId(const QString& _current) const;
        QString nextAimId(const QString& aimId) const;
        QString prevAimId(const QString& aimId) const;
        int getUnreadCount(const QString& _aimId) const;
        int getUnreadMentionsCount(const QString& _aimId) const;

        int getUnreadThreadsCount() const;
        int getUnreadThreadsMentionsCount() const;

        int32_t getTime(const QString& _aimId) const;

        std::vector<QString> getSortedRecentsContacts() const;

        void moveToTop(const QString& _aimId);

        int dialogsCount() const;

    private:
        int correctIndex(int i) const;
        int getVisibleIndex(int i) const;
        int visiblePinnedContacts() const;
        int visibleContactsInUnimportant() const;

        int getPinnedHeaderIndex() const;
        int getUnimportantHeaderIndex() const;
        int getRecentsHeaderIndex() const;
        int getVisibleServiceItems() const;
        std::optional<Data::DlgState> serviceItemByIndex(int _index) const;
        void initPinnedItemsIndexes();
        int pinnedServiceItemsCount() const;
        int pinnedServiceItemIndex(Data::DlgState::PinnedServiceItemType _type) const;
        int pinnedFavoritesIndex() const;
        int scheduledMessagesItemIndex() const;
        int threadsItemIndex() const;
        int remindersItemIndex() const;

        void scheduleRefreshTimer();
        void makeIndexes();
        void requestFavoritesLastMessage();

        bool isSpecialAndHidden(const Data::DlgState& _s) const;

        Data::DlgState pinnedServiceFavoriteState_;
        std::vector<Data::DlgState> Dialogs_;
        std::vector<Data::DlgState> HiddenDialogs_;
        std::unordered_map<QString, size_t, Utils::QStringHasher> Indexes_;
        QTimer* Timer_;
        quint16 PinnedCount_;
        bool PinnedVisible_;
        quint16 UnimportantCount_;
        bool UnimportantVisible_;
        bool needToAddFavorites_;
        int64_t favoritesLastMessageSeq_;

        std::map<QString, Recents::FriendlyItemText> friendlyTexts_;
        QTimer* refreshTimer_;

        std::unordered_map<Data::DlgState::PinnedServiceItemType, size_t> pinnedItemsIndexes_;

        struct
        {
            int unreadCount_ = 0;
            int unreadMentionsCount_ = 0;
        } threadsFeed_;
    };

    RecentsModel* getRecentsModel();
    void ResetRecentsModel();
}
