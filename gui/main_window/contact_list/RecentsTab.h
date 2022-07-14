#pragma once

#include "../../types/typing.h"
#include "../../controls/TransparentScrollBar.h"
#include "Common.h"
#include "ContactListUtils.h"

namespace Logic
{
    class UnknownItemDelegate;
    class ContactListItemDelegate;
    class ChatMembersModel;
    class AbstractSearchModel;
    class ContactListWithHeaders;
    enum class UpdateChatSelection;
}

namespace Data
{
    class Contact;
}

namespace Tooltip
{
    enum class ArrowDirection;
    enum class ArrowPointPos;
}

namespace Utils
{
    class PersonTooltip;
}

namespace Ui
{
    class CustomButton;
    class HorScrollableView;
    class ContactListWidget;
    class SettingsTab;
    class ContextMenu;
    class RecentsTab;
    class SearchWidget;
    class RecentsView;
    class RecentItemDelegate;
    class RecentsPlaceholder;
    enum class PageOpenedAs;

    using highlightsV = std::vector<QString>;

    enum CurrentTab
    {
        RECENTS = 0,
        SEARCH,
    };

    class RecentsTab : public QWidget
    {
        Q_OBJECT

    Q_SIGNALS :
        void itemSelected(const QString& /*_aimId*/, qint64 _messageId, qint64 /*_threadId*/, const highlightsV& _highlights, bool _ignoreScroll = false);
        void groupClicked(int);
        void tabChanged(int);
        void createGroupChatClicked();
        void createChannelClicked();
        void addContactClicked(QPrivateSignal);
        void topItemUpPressed(QPrivateSignal);
        void needClearSearch();

    public Q_SLOTS:
        void select(const QString& _aimId);
        void scrollToItem(const QString& _aimId, QAbstractItemView::ScrollHint _hint);
        void scrollToTop();

        void changeSelected(const QString& _aimId);
        void recentsClicked();

        void setSearchInAllDialogs();
        void setSearchInDialog(const QString& _contact);
        bool getSearchInDialog() const;
        const QString& getSearchInDialogContact() const;

        void showSearch();

    private Q_SLOTS:
        void itemClicked(const QModelIndex&);
        void itemPressed(const QModelIndex&);
        void itemPressedImpl(const QModelIndex&, Qt::MouseButtons);
        void statsRecentItemPressed(const QModelIndex&);
        void statsCLItemPressed(const QModelIndex&);

        void onDragPositionUpdate(QPoint _pos);
        void dropMimeData(QPoint _pos, const QMimeData* _mimeData);

        void guiSettingsChanged();
        void touchScrollStateChangedRecents(QScroller::State);

        void showNoRecentsYet();
        void hideNoRecentsYet();

        void messagesReceived(const QString&, const QVector<QString>&);
        void showPopupMenu(QAction* _action);
        void autoScroll();

        void dialogClosed(const QString& _aimid);
        void myProfileBack();

        void recentsScrollActionTriggered(int value);

        void searchEnterPressed();
        void searchUpPressed();
        void searchDownPressed();

        void highlightFirstItem();

    public:

        RecentsTab(QWidget* _parent);
        ~RecentsTab();

        void setSearchMode(bool);
        bool isSearchMode() const;
        void changeTab(CurrentTab _currTab, bool silent = false);
        CurrentTab currentTab() const;

        void setPictureOnlyView(bool _isPictureOnly);
        bool getPictureOnlyView() const;
        void setItemWidth(int _newWidth);

        QString getSelectedAimid() const;

        bool isRecentsOpen() const;
        bool isUnknownsOpen() const;
        bool isCLWithHeadersOpen() const;

        bool isKeyboardFocused() const;
        void dropKeyboardFocus();

        void switchToRecents(const QString& _contactToSelect = {});
        void switchToUnknowns();

        enum class SwichType
        {
            Simple,
            Animated
        };
        void switchToContactListWithHeaders(const SwichType _switchType);

        ContactListWidget* getContactListWidget() const;

        void connectSearchWidget(SearchWidget* _searchWidget);

        void highlightContact(const QString& _aimId);
        void setNextSelectWithOffset();
        QRect getAvatarRect(const QModelIndex& _index) const;
        QRect getContactFriendlyRect(const QModelIndex& _index) const;

        void setPageOpenedAs(PageOpenedAs _openedAs);

        void setCloseOnThreadResult(bool _val);

    protected:
        void resizeEvent(QResizeEvent* _event) override;
        void leaveEvent(QEvent* _event) override;

    private:
        void updateTabState();
        void showRecentsPopupMenu(const QModelIndex& _current);

        void searchUpOrDownPressed(const bool _isUpPressed);
        void setKeyboardFocused(const bool _isFocused);
        void onMouseMoved(const QPoint& _pos, const QModelIndex& _index);

        void showTooltip(const Data::FString& _text, const QRect& _rect, Tooltip::ArrowDirection _arrowDir, Tooltip::ArrowPointPos _arrowPos);
        void hideTooltip();
        void dragPositionUpdate(QPoint _pos, bool _fromScroll);

        bool isOverUnknownsCloseButton(const QModelIndex& _current) const;

    private:
        QPointer<RecentsPlaceholder> recentsPlaceholder_;
        QPointer<Ui::ContextMenu> popupMenu_;

        Ui::RecentItemDelegate* recentsDelegate_;
        Logic::UnknownItemDelegate* unknownsDelegate_;
        Logic::ContactListItemDelegate* clDelegate_;

        QVBoxLayout* recentsLayout_;
        QWidget* recentsPage_;

        RecentsView* recentsView_;
        QWidget* recentsContainer_;
        QVBoxLayout* recentsContainerLayout_;

        ContactListWidget* contactListWidget_;
        Logic::ContactListWithHeaders* clWithHeaders_;

        QStackedWidget* stackedWidget_;
        QTimer* scrollTimer_;
        FocusableListView* scrolledView_;
        int scrollMultipler_;
        QPoint lastDragPos_;

        QLabel* transitionLabel_;
        QPropertyAnimation* transitionAnim_;

        QTimer* tooltipTimer_;
        QModelIndex tooltipIndex_;

        int currentTab_;
        int prevTab_;
        PageOpenedAs openedAs_;
        bool pictureOnlyView_;
        bool nextSelectWithOffset_;

        Utils::PersonTooltip* personTooltip_ = nullptr;
    };
}
