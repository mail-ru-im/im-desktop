#pragma once

#include "CustomAbstractListModel.h"

#include "../../types/contact.h"
#include "../../types/message.h"
#include "../../types/typing.h"

namespace Ui
{
    class MainWindow;
    enum class MessagesBuddiesOpt;
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
        void dlgStateChanged(const Data::DlgState&);
        void refreshAll();

    public Q_SLOTS:
        void refresh();
        void refreshUnknownMessages();

    private Q_SLOTS:

        void activeDialogHide(const QString&);
        void contactChanged(const QString&);
        void selectedContactChanged(const QString& _new, const QString& _prev);
        void contactAvatarChanged(const QString&);
        void dlgStates(const QVector<Data::DlgState>&);
        void messageBuddies(const Data::MessageBuddies&, const QString&, Ui::MessagesBuddiesOpt, bool, qint64, int64_t);
        void sortDialogs();
        void contactRemoved(const QString&);
        void typingStatus(const Logic::TypingFires& _typing, bool _isTyping);

    public:
        explicit RecentsModel(QObject *parent);

        int rowCount(const QModelIndex &parent = QModelIndex()) const override;
        QVariant data(const QModelIndex &index, int role) const override;

        Data::DlgState getDlgState(const QString& aimId = QString(), bool fromDialog = false);
        void unknownToRecents(const Data::DlgState&);

        void toggleFavoritesVisible();

        void unknownAppearance();

        enum class ReadMode
        {
            ReadAll,
            ReadText
        };

        void sendLastRead(const QString& aimId = QString(), bool force = false, ReadMode _mode = ReadMode::ReadText);
        int markAllRead();
        void setAttention(const QString& _aimId, const bool _value);
        bool getAttention(const QString& _aimId) const;
        void hideChat(const QString& aimId);
        void muteChat(const QString& aimId, bool mute);

        bool isFavorite(const QString& aimid) const;
        bool isSuspicious(const QString& _aimid) const;
        bool isStranger(const QString& _aimid);
        bool isServiceItem(const QModelIndex& _index) const override;
        bool isClickableItem(const QModelIndex& _index) const override;
        bool isFavoritesGroupButton(const QModelIndex& i) const;
        bool isFavoritesVisible() const;
        quint16 getFavoritesCount() const;
        bool isRecentsHeader(const QModelIndex& i) const;

        bool isServiceAimId(const QString& _aimId) const;

        bool contains(const QString& _aimId) const override;
        QModelIndex contactIndex(const QString& _aimId) const;

        int64_t getLastMsgId(const QString& _aimId) const;

        void setItemFriendlyText(const QString& _aimId, const Recents::FriendlyItemText& _frText);

        int totalUnreads() const;
        int recentsUnreads() const;
        int favoritesUnreads() const;

        bool hasAttentionDialogs() const;

        QString firstContact() const;
        QString nextUnreadAimId(const QString& _current) const;
        QString nextAimId(const QString& aimId) const;
        QString prevAimId(const QString& aimId) const;
        int getUnreadCount(const QString& _aimId) const;

        int32_t getTime(const QString& _aimId) const;

        std::vector<QString> getSortedRecentsContacts() const;

        void moveToTop(const QString& _aimId);

        int dialogsCount() const;

    private:
        int correctIndex(int i) const;
        int getVisibleIndex(int i) const;
        int visibleContactsInFavorites() const;

        int getFavoritesHeaderIndex() const;
        int getRecentsHeaderIndex() const;
        int getVisibleServiceItemInFavorites() const;

        void scheduleRefreshTimer();

        void makeIndexes();

        std::vector<Data::DlgState> Dialogs_;
        QHash<QString, int> Indexes_;
        QTimer* Timer_;
        quint16 FavoritesCount_;
        bool FavoritesVisible_;

        std::map<QString, Recents::FriendlyItemText> friendlyTexts_;
        QTimer* refreshTimer_;
    };

    RecentsModel* getRecentsModel();
    void ResetRecentsModel();
}
