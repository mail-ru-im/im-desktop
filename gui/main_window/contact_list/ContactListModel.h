#pragma once

#include "CustomAbstractListModel.h"

#include "../../types/contact.h"
#include "../../types/message.h"
#include "../../types/chat.h"

#include "ContactItem.h"

Q_DECLARE_LOGGING_CATEGORY(clModel)

namespace core
{
    struct icollection;
}

namespace Ui
{
    class HistoryControlPage;

    struct Input
    {
        Input() = default;

        Input(const QString& _text, const int _pos)
            : text_(_text)
            , pos_(_pos)
        {}

        QString text_;
        int pos_ = 0;
    };
}

namespace Logic
{
    namespace ContactListSorting
    {
        const int maxTopContactsByOutgoing = 5;
        typedef std::function<bool (const Logic::ContactItem&, const Logic::ContactItem&)> contact_sort_pred;

        struct ItemLessThanDisplayName
        {
            ItemLessThanDisplayName(bool _force_cyrillic = false)
                : force_cyrillic_(_force_cyrillic)
                , collator_(Utils::GetTranslator()->getLocale())
            {
                collator_.setCaseSensitivity(Qt::CaseInsensitive);
            }

            inline bool operator() (const Logic::ContactItem& _first, const Logic::ContactItem& _second)
            {
                const auto& firstName = _first.Get()->GetDisplayName();
                const auto& secondName = _second.Get()->GetDisplayName();

                const auto firstNotLetter = Utils::startsNotLetter(firstName);
                const auto secondNotLetter = Utils::startsNotLetter(secondName);
                if (firstNotLetter != secondNotLetter)
                    return secondNotLetter;

                if (force_cyrillic_)
                {
                    const auto firstCyrillic = Utils::startsCyrillic(firstName);
                    const auto secondCyrillic = Utils::startsCyrillic(secondName);

                    if (firstCyrillic != secondCyrillic)
                        return firstCyrillic;
                }

                return collator_(firstName, secondName);
            }

            bool force_cyrillic_;
            QCollator collator_;
        };

        struct ItemLessThan
        {
            inline bool operator() (const Logic::ContactItem& _first, const Logic::ContactItem& _second)
            {
                if (_first.Get()->GroupId_ == _second.Get()->GroupId_)
                {
                    if (_first.is_group() && _second.is_group())
                        return false;

                    if (_first.is_group())
                        return true;

                    if (_second.is_group())
                        return false;

                    return ItemLessThanDisplayName()(_first, _second);
                }

                return _first.Get()->GroupId_ < _second.Get()->GroupId_;
            }
        };

        struct ItemLessThanNoGroups
        {
            ItemLessThanNoGroups(const QDateTime& _current)
                : current_(_current)
            {
            }

            inline bool operator() (const Logic::ContactItem& _first, const Logic::ContactItem& _second)
            {
                if (_first.Get()->IsChecked_ != _second.Get()->IsChecked_)
                    return _first.Get()->IsChecked_;

                const auto firstIsActive = _first.is_active(current_);
                const auto secondIsActive = _second.is_active(current_);
                if (firstIsActive != secondIsActive)
                    return firstIsActive;

                return ItemLessThanDisplayName()(_first, _second);
            }

            QDateTime current_;
        };
    }

    class contact_profile;
    using profile_ptr = std::shared_ptr<contact_profile>;

    enum class UpdateChatSelection
    {
        No,
        Yes
    };

    class ContactListModel : public CustomAbstractListModel
    {
        Q_OBJECT

    Q_SIGNALS:
        void currentDlgStateChanged() const;
        void selectedContactChanged(const QString& _new, const QString& _prev);
        void contactChanged(const QString&) const;
        void select(const QString&, qint64, qint64 quote_id = -1, Logic::UpdateChatSelection mode = Logic::UpdateChatSelection::No) const;
        void profile_loaded(profile_ptr _profile) const;
        void contact_added(const QString& _contact, bool _result);
        void contact_removed(const QString& _contact);
        void leave_dialog(const QString& _contact);
        void needSwitchToRecents();
        void liveChatJoined(const QString&);
        void liveChatRemoved(const QString&);
        void switchTab(const QString&);
        void ignore_contact(const QString&);
        void youRoleChanged(const QString&);
        void contactAdd(const QString&);
        void contactRename(const QString&);

    private Q_SLOTS:
        void contactList(std::shared_ptr<Data::ContactList>, const QString&);
        void avatarLoaded(const QString&);
        void presence(std::shared_ptr<Data::Buddy>);
        void contactRemoved(const QString&);
        void outgoingMsgCount(const QString& _aimid, const int _count);
        void dialogGalleryState(const QString& _aimId, const Data::DialogGalleryState& _state);

    public Q_SLOTS:
        void chatInfo(qint64, const std::shared_ptr<Data::ChatInfo>&, const int _requestMembersLimit);

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
        void setCurrentCallbackHappened(Ui::HistoryControlPage* _page);

        void setCurrent(const QString& _aimId, qint64 id, bool _select = false, std::function<void(Ui::HistoryControlPage*)> _getPageCallback = nullptr, qint64 quote_id = -1);

        const ContactItem* getContactItem(const QString& _aimId) const;

        const QString& selectedContact() const;
        QString selectedContactName() const;

        void add(const QString& _aimId, const QString& _friendly);

        void setContactVisible(const QString& _aimId, bool _visible);

        Ui::Input getInputText(const QString& _aimId) const;
        void setInputText(const QString& _aimId, const Ui::Input& _input);
        QString getState(const QString& _aimId) const;
        int getOutgoingCount(const QString& _aimId) const;
        bool isChat(const QString& _aimId) const;
        bool isMuted(const QString& _aimId) const;
        bool isLiveChat(const QString& _aimId) const;
        bool isOfficial(const QString& _aimId) const;
        QModelIndex contactIndex(const QString& _aimId) const;

        void addContactToCL(const QString& _aimId, std::function<void(bool)> _callBack = [](bool) {});
        void getContactProfile(const QString& _aimId, std::function<void(profile_ptr, int32_t)> _callBack = [](profile_ptr, int32_t) {});
        void ignoreContact(const QString& _aimId, bool _ignore);
        bool ignoreContactWithConfirm(const QString& _aimId);
        bool isYouAdmin(const QString& _aimId) const;
        QString getYourRole(const QString& _aimId) const;
        void setYourRole(const QString& _aimId, const QString& _role);
        void removeContactFromCL(const QString& _aimId);
        void renameChat(const QString& _aimId, const QString& _friendly);
        void renameContact(const QString& _aimId, const QString& _friendly, std::function<void(bool)> _callBack = nullptr);
        void static getIgnoreList();

        void removeContactsFromModel(const QVector<QString>& _vcontacts);

        std::vector<QString> GetCheckedContacts() const;
        void clearChecked();
        void setChecked(const QString& _aimId, bool _isChecked);
        void setIsWithCheckedBox(bool);
        bool getIsChecked(const QString& _aimId) const;

        bool isWithCheckedBox() const;
        QString contactToTryOnTheme() const;
        void joinLiveChat(const QString& _stamp, bool _silent);

        void next();
        void prev();

        bool contains(const QString& _aimId) const override;

        int getTopSortedCount() const;

        int contactsCount() const;

        bool isChannel(const QString& _aimId) const;
        bool isReadonly(const QString& _aimId) const; // no write access to this chat, for channel check use isChannel

        Data::DialogGalleryState getGalleryState(const QString& _aimid) const;

        QString getChatStamp(const QString& _aimid) const;

    private:
        std::function<void(Ui::HistoryControlPage*)> gotPageCallback_;
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

        QMap<QString, Data::DialogGalleryState> galleryStates_;

        QSet<QString> deletedContacts_;

        QMap<QString, QString> chatStamps_;
    };

    ContactListModel* getContactListModel();
    void ResetContactListModel();
}

Q_DECLARE_METATYPE(Logic::UpdateChatSelection);
