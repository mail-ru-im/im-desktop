#pragma once
#include "../main_window/contact_list/SelectionContactsForGroupChat.h"
#include "../main_window/contact_list/ChatMembersModel.h"
#include "../../core/Voip/VoipManagerDefines.h"
#include "../main_window/contact_list/Common.h"
#include "../main_window/contact_list/SearchModel.h"

namespace Ui
{
    class ListViewWithTrScrollBar;
    class ConferenceSearchModel;
    class CustomButton;
    class ClickableWidget;


    class CurrentConferenceMembers : public QWidget
    {
        Q_OBJECT

    Q_SIGNALS:
        void memberClicked(const QString& _aimid, QPrivateSignal);
        void sizeChanged(QPrivateSignal);

    public:
        CurrentConferenceMembers(QWidget* _parent, Logic::ChatMembersModel* _membersModel, bool _chatRoomCall);
        void updateConferenceListSize();

    protected:
        void resizeEvent(QResizeEvent*) override;

    private Q_SLOTS:
        void onMouseMoved(const QPoint& _pos, const QModelIndex& _index);

    private:
        void onMembersLabelClicked();
        void itemClicked(const QModelIndex& _current);
        QString getLabelCaption() const;
        QRect getAvatarRect(const QModelIndex& _index);

    private:
        QWidget* membersLabelHost_;
        QLabel* memberLabel_;
        CustomButton* memberShowAll_;
        CustomButton* memberHide_;
        ListViewWithTrScrollBar* conferenceContacts_;
        Logic::ChatMembersModel* conferenceMembersModel_;
    };


    class SelectionContactsForConference : public SelectContactsWidget
    {
        Q_OBJECT

    public:
        SelectionContactsForConference(
            Logic::ChatMembersModel* _conferenceMembersModel,
            const QString& _labelText,
            QWidget* _parent,
            bool _chatRoomCall,
            SelectContactsWidgetOptions options);

        void onVoipCallDestroyed(const voip_manager::ContactEx& _contactEx);

        // Set maximum restriction for selected item count. Used for video conference.
        void setMaximumSelectedCount(int _number) override;

        bool show() override;

        QPointer<CurrentConferenceMembers> createCurMembersPtr();
        void clearCurMembersPtr();

    public Q_SLOTS:
        void updateSize();

    private Q_SLOTS:
        void updateMemberList();
        void onCurrentMemberClicked(const QString& _aimid);
        void onVoipCallNameChanged(const voip_manager::ContactsList& _contacts);
        void updateMaxSelectedCount();

    private:
        QPointer<CurrentConferenceMembers> curMembers_;
        Logic::ChatMembersModel* conferenceMembersModel_;
        int videoConferenceMaxUsers_;
        bool chatRoomCall_ = false;

        // Make this dialog singleton visible. If second instance is show, first will be hide.
        static SelectionContactsForConference* currentVisibleInstance_;
    };

    // Draw only avatars of users for contacts list.
    // Used in horizontal mode in add to video conference dialog.
    class ContactListItemHorizontalDelegate : public Logic::AbstractItemDelegateWithRegim
    {
    public:
        explicit ContactListItemHorizontalDelegate(QObject* _parent);

        void paint(QPainter* _painter, const QStyleOptionViewItem& _option, const QModelIndex& _index) const override;

        QSize sizeHint(const QStyleOptionViewItem& _option, const QModelIndex& _index) const override;

        void setFixedWidth(int _width) override;

        void setLeftMargin(int _margin);

        void setRightMargin(int _margin);

        void blockState(bool _value) override {}

        void setDragIndex(const QModelIndex& _index) override {}

        void setRegim(int _regim) override;

    private:
        ViewParams viewParams_;
    };

    class ConferenceSearchModel : public Logic::AbstractSearchModel
    {
        Q_OBJECT

    public:
        ConferenceSearchModel(Logic::AbstractSearchModel* _sourceModel);

        int rowCount(const QModelIndex& _index = QModelIndex()) const override;
        QVariant data(const QModelIndex& _index, int _role) const override;
        void setSearchPattern(const QString& _pattern) override;

        void setView(ContactListWidget* _view) { view_ = _view; }
        void setDialog(SelectionContactsForConference* _dialog) { dialog_ = _dialog; }

        bool isCheckedItem(const QString &_name) const override;
        void setCheckedItem(const QString& _name, bool _checked) override;
        int getCheckedItemsCount() const override;

        void addTemporarySearchData(const QString& _aimid) override;

    private Q_SLOTS:
        void onSourceDataChanged(const QModelIndex& _topLeft, const QModelIndex& _bottomRight);

    private:
        int indexShift() const noexcept { return !inSearchMode_ ? 1 : 0; }
        QModelIndex mapFromSource(const QModelIndex &_sourceIndex) const;

        QPointer<CurrentConferenceMembers> membersWidget_;
        bool inSearchMode_ = false;
        ContactListWidget* view_ = nullptr;
        SelectionContactsForConference* dialog_ = nullptr;
        Logic::AbstractSearchModel* sourceModel_ = nullptr;
    };
}
