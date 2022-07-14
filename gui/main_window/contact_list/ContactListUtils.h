#pragma once

#include "RecentItemDelegate.h"
#include "../../controls/ClickWidget.h"
#include "accessibility/LinkAccessibleWidget.h"

namespace Logic
{
    class SearchModel;

    enum MembersWidgetRegim
    {
        CONTACT_LIST,
        SELECT_MEMBERS,
        MEMBERS_LIST,
        IGNORE_LIST,
        ADMIN_MEMBERS,
        SHARE,
        PENDING_MEMBERS,
        UNKNOWN,
        FROM_ALERT,
        HISTORY_SEARCH,
        VIDEO_CONFERENCE,
        CURRENT_VIDEO_CONFERENCE,
        CONTACT_LIST_POPUP,
        COMMON_CHATS,
        SHARE_CONTACT,
        COUNTRY_LIST,
        SELECT_CHAT_MEMBERS,
        CALLS_LIST,
        SHARE_VIDEO_CONFERENCE,
        YOUR_INVITES_LIST,
        DISALLOWED_INVITERS,
        DISALLOWED_INVITERS_ADD,
        TASK_ASSIGNEE,
        THREAD_SUBSCRIBERS
    };

    bool is_members_regim(int _regim);
    bool is_admin_members_regim(int _regim);
    bool is_select_members_regim(int _regim);
    bool is_select_chat_members_regim(int _regim);
    bool is_video_conference_regim(int _regim);
    bool is_share_regims(int _regim);
    bool isRegimWithGlobalContactSearch(int _regim);
    bool isAddMembersRegim(int _regim);

    QString aimIdFromIndex(const QModelIndex& _current);
    QString senderAimIdFromIndex(const QModelIndex& _index);

    QMap<QString, QVariant> makeData(const QString& _command, const QString& _aimid = QString());
    QMap<QString, QVariant> makeData(const QString& _command, Data::CallInfoPtr _call);
    void addItemToContextMenu(QMenu* _popupMenu, const QString& _aimId, const Data::DlgState& _dlg);
    void showContactListPopup(QAction* _action);
}

namespace Ui
{
    class ServiceContact;
    class TextWidget;

    class EmptyListLabel : public LinkAccessibleWidget
    {
        Q_OBJECT

    Q_SIGNALS:
        void linkClicked(QPrivateSignal) const;

    public:
        explicit EmptyListLabel(QWidget* _parent, Logic::MembersWidgetRegim _regim);
        ~EmptyListLabel();

        const TextRendering::TextUnitPtr& getTextUnit() const override { return label_; }

    protected:
        void paintEvent(QPaintEvent*) override;
        void resizeEvent(QResizeEvent*) override;
        void mouseMoveEvent(QMouseEvent*) override;
        void mouseReleaseEvent(QMouseEvent*) override;
        void leaveEvent(QEvent*) override;

    private:
        void setLinkHighlighted(bool _hl);

    private:
        TextRendering::TextUnitPtr label_;
    };


    class SearchCategoryButton : public ClickableWidget
    {
        Q_OBJECT

    public:
        SearchCategoryButton(QWidget* _parent, const QString& _text);
        ~SearchCategoryButton();

        bool isSelected() const;
        void setSelected(const bool _sel);

    protected:
        void paintEvent(QPaintEvent* _e) override;

    private:
        Styling::ThemeColorKey getTextColor() const;

    private:
        TextRendering::TextUnitPtr label_;
        bool selected_;
    };

    class HeaderScrollOverlay : public QWidget
    {
        Q_OBJECT

    public:
        HeaderScrollOverlay(QWidget* _parent, QScrollArea* _scrollArea);

    protected:
        void paintEvent(QPaintEvent* _event) override;
        bool eventFilter(QObject* _watched, QEvent* _event) override;

    private:
        QScrollArea* scrollArea_;
    };

    enum class SearchCategory
    {
        ContactsAndGroups,
        Messages,
        Threads,
        SingleThread
    };

    class GlobalSearchViewHeader : public QWidget
    {
        Q_OBJECT

    Q_SIGNALS:
        void categorySelected(SearchCategory _category, bool _animated = true);

    private Q_SLOTS:
        void onCategoryClicked();

    public:
        explicit GlobalSearchViewHeader(QWidget* _parent);

        enum class SelectType
        {
            Click,
            Quiet
        };
        void selectCategory(const SearchCategory _cat, const SelectType _selectType = SelectType::Quiet);
        void setCategoryVisible(const SearchCategory _cat, const bool _isVisible);
        void reset();

        void selectFirstVisible();

        bool hasVisibleCategories() const;

    protected:
        void wheelEvent(QWheelEvent* _event);

    private:
        enum class direction
        {
            left = 0,
            right = 1
        };
        void scrollStep(direction _direction);

        QScrollArea* viewArea_;
        QPropertyAnimation* animScroll_;

        std::vector<std::pair<SearchCategory, SearchCategoryButton*>> buttons_;
    };

    class SearchFilterButton : public QWidget
    {
        Q_OBJECT

    Q_SIGNALS:
        void removeClicked(QPrivateSignal);

    private Q_SLOTS:
        void onRemoveClicked();

    public:
        SearchFilterButton(QWidget* _parent);
        ~SearchFilterButton();

        void setLabel(const QString& _label);
        QSize sizeHint() const override;

    protected:
        void paintEvent(QPaintEvent* _event) override;
        void resizeEvent(QResizeEvent* _event) override;

    private:
        void elideLabel();

    private:
        TextWidget* labelWidget_ = nullptr;
    };

    class DialogSearchViewHeader : public QWidget
    {
        Q_OBJECT

    Q_SIGNALS:
        void contactFilterRemoved();

    public Q_SLOTS:
        void removeChatNameFilter();

    public:
        explicit DialogSearchViewHeader(QWidget* _parent);

        void setChatNameFilter(const QString& _name);

        void addFilter(const QString& _filterName, SearchFilterButton* _button);
        void removeFilter(const QString& _filterName);

    private:
        std::map<QString, SearchFilterButton*> filters_;
    };
}
