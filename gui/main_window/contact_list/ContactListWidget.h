#pragma once
#include "RecentsTab.h"
#include "../../utils/InterConnector.h"
#include "../gui/utils/PersonTooltip.h"

namespace Logic
{
    class CustomAbstractListModel;
    class SearchModel;
    class SearchItemDelegate;
    enum class UpdateChatSelection;
    class CommonChatsModel;
    class CallsModel;
    class StatusProxyModel;
}

namespace Ui
{
    using highlightsV = std::vector<QString>;

    class FocusableListView;
    class SearchWidget;
    class ContextMenu;
    class Placeholder;

    class ContactListWidget : public QWidget
    {
        Q_OBJECT

    Q_SIGNALS:
        void searchEnd();
        void itemSelected(const QString& _aimid, qint64 _msgid, qint64 _threadMsgid, const highlightsV& _highlights, bool _ignoreScroll = false);
        void itemClicked(const QString& _aimid);
        void groupClicked(int);
        void changeSelected(const QString&);
        void searchSuggestSelected(const QString& _pattern);
        void searchSuggestRemoved(const QString& _contact, const QString& _pattern);
        void clearSearchSelection();
        void forceSearchClear(const bool _force);

        //sidebar
        void selected(const QString&);
        void removeClicked(const QString&);
        void moreClicked(const QString&);
        void approve(const QString, bool);

    public:
        ContactListWidget(QWidget* _parent,
            const Logic::MembersWidgetRegim& _regim,
            Logic::CustomAbstractListModel* _chatMembersModel,
            Logic::AbstractSearchModel* _searchModel   = nullptr,
            Logic::CommonChatsModel* _commonChatsModel = nullptr,
            Logic::DrawIcons needDrawIcons_            = Logic::NoNeedDrawIcons);
        ~ContactListWidget();

        void connectSearchWidget(SearchWidget* _widget);

        void installEventFilterToView(QObject* _filter);

        QWidget* indexWidget(int _index);
        void setIndexWidget(int index, QWidget* widget);

        void setClDelegate(Logic::AbstractItemDelegateWithRegim* _delegate);
        void setWidthForDelegate(int _width);
        void setDragIndexForDelegate(const QModelIndex& _index);
        void setPictureOnlyForDelegate(bool _value);

        void setEmptyLabelVisible(bool _isVisible);
        void setSearchInDialog(const QString& _contact, bool _switchModel = true);
        bool getSearchInDialog() const;
        const QString& getSearchInDialogContact() const;

        bool isSearchMode() const;
        QString getSelectedAimid() const;

        Logic::MembersWidgetRegim getRegim() const;
        void setRegim(const Logic::MembersWidgetRegim _regim);

        void enableCLDelegateOpacity(bool _enable);
        bool isCLDelegateOpacityEnabled() const;

        Logic::AbstractSearchModel* getSearchModel() const;
        FocusableListView* getView() const;

        void showSearch();

        void rewindToTop();

        QRect getAvatarRect(const QModelIndex& _index) const;

        void setPageOpenedAs(PageOpenedAs _openedAs);

        void setCloseOnThreadResult(bool _val);

    public Q_SLOTS:
        void searchResult();
        void searchUpPressed();
        void searchDownPressed();
        void selectionChanged(const QModelIndex &);
        void select(const QString& _aimId);
        void select(const QString& _aimId, const qint64 _messageId, const highlightsV& _highlights, Logic::UpdateChatSelection _mode, bool _ignoreScroll = false, PageOpenedAs _openedAs = PageOpenedAs::MainPage);
        void showContactsPopupMenu(const QString& aimId, bool _is_chat);
        void showCallsPopupMenu(Data::CallInfoPtr _call);
        void showContactsNotFromCLPopupMenu(const QString& _aimId);

    private Q_SLOTS:
        void onItemClicked(const QModelIndex&);
        void onItemPressed(const QModelIndex&);
        void onItemPressedImpl(const QModelIndex&, Qt::MouseButtons);
        void onMouseMoved(const QPoint& _pos, const QModelIndex& _index);
        void onMouseWheeled();
        void onMouseWheeledStats();
        void onSearchResults();
        void onSearchSuggests();
        void searchResults(const QModelIndex &, const QModelIndex &);
        void searchClicked(const QModelIndex& _current);
        void showPopupMenu(QAction* _action);

        void onSearchInputCleared();
    public Q_SLOTS:
        void searchPatternChanged(const QString&);
    private Q_SLOTS:
        void onDisableSearchInDialog();
        void touchScrollStateChanged(QScroller::State _state);

        void scrollToCategory(const SearchCategory _category, bool _animated = true);
        void scrollToItem(const QString& _aimId, bool _animated = true);

        void removeSuggestPattern(const QString& _contact, const QString& _pattern);

        void showPlaceholder();
        void hidePlaceholder();
        void showNoSearchResults();
        void hideNoSearchResults();
        void showSearchSpinner();
        void hideSearchSpinner();

        void scrolled(const int _value);

        void onThemeChanged();

    protected:
        void leaveEvent(QEvent* _event) override;

    private:
        void switchToInitial(bool _initial);
        void searchUpOrDownPressed(bool _isUp);
        void setKeyboardFocused(bool _isFocused);
        void initSearchModel(Logic::AbstractSearchModel* _searchModel);
        bool isSelectMembersRegim() const;
        void ensureScrollToItemAnimationInitialized();

        void selectCurrentSearchCategory();

        void sendGlobalSearchResultStats();

        using SearchHeaders = std::vector<std::pair<SearchCategory, QModelIndex>>;
        SearchHeaders getCurrentCategories() const;

        void hideTooltip();

    private:
        QVBoxLayout* layout_;
        QVBoxLayout* viewLayout_;
        FocusableListView* view_;

        EmptyListLabel* EmptyListLabel_ = nullptr;
        DialogSearchViewHeader* dialogSearchViewHeader_ = nullptr;
        GlobalSearchViewHeader* globalSearchViewHeader_ = nullptr;
        QVariantAnimation* scrollToItemAnim_ = nullptr;

        std::string scrollStatWhere_;
        QTimer* scrollStatsTimer_;

        Logic::AbstractItemDelegateWithRegim* clDelegate_ = nullptr;
        Logic::AbstractItemDelegateWithRegim* searchDelegate_ = nullptr;

        Placeholder* contactsPlaceholder_ = nullptr;
        QWidget* noSearchResults_ = nullptr;
        QWidget* searchSpinner_ = nullptr;
        QWidget* viewContainer_;
        QLabel* noResultsPlaceholder_ = nullptr;

        Logic::MembersWidgetRegim regim_;
        Logic::CustomAbstractListModel* chatMembersModel_;
        Logic::AbstractSearchModel* searchModel_;
        Logic::ContactListWithHeaders* clModel_;
        Logic::CommonChatsModel* commonChatsModel_;
        Logic::CallsModel* callsModel_;

        ContextMenu* popupMenu_ = nullptr;

        Utils::PersonTooltip* personTooltip_ = nullptr;
        QModelIndex tooltipIndex_;

        PageOpenedAs openedAs_ = PageOpenedAs::MainPage;

        QString searchDialogContact_;
        Logic::DrawIcons needDrawIcons_;
        bool noSearchResultsShown_ = false;
        bool searchResultsRcvdFirst_ = false;
        bool searchResultsStatsSent_ = false;
        bool initial_ = false;
        bool closeOnThreadResult_ = false;
    };
}
