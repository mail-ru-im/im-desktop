#pragma once

#include "RecentItemDelegate.h"
#include "../../controls/ClickWidget.h"

namespace Logic
{
    class SearchModel;

    enum MembersWidgetRegim
    {
        CONTACT_LIST = 0,
        SELECT_MEMBERS = 1,
        MEMBERS_LIST = 2,
        IGNORE_LIST = 3,
        ADMIN_MEMBERS = 4,
        SHARE = 5,
        PENDING_MEMBERS = 6,
        UNKNOWN = 7,
        FROM_ALERT = 8,
        HISTORY_SEARCH = 9,
        VIDEO_CONFERENCE = 10,
        CONTACT_LIST_POPUP = 11,
        COMMON_CHATS = 12,
        SHARE_CONTACT = 13,
        COUNTRY_LIST = 14,
        SELECT_CHAT_MEMBERS = 15,
        CALLS_LIST = 16,
        SHARE_VIDEO_CONFERENCE = 17,
        STATUS_LIST = 18,
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

    QMap<QString, QVariant> makeData(const QString& _command, const QString& _aimid = QString());
    QMap<QString, QVariant> makeData(const QString& _command, Data::CallInfoPtr _call);
    void showContactListPopup(QAction* _action);
}

namespace Ui
{
    class ServiceContact;

    class EmptyIgnoreListLabel : public QWidget
    {
        Q_OBJECT

    public:
        explicit EmptyIgnoreListLabel(QWidget* _parent);
        ~EmptyIgnoreListLabel();

    protected:
        void paintEvent(QPaintEvent*) override;

    private:
        std::unique_ptr<Ui::ServiceContact> serviceContact_;
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
        QColor getTextColor() const;

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
        Messages
    };

    class GlobalSearchViewHeader : public QWidget
    {
        Q_OBJECT

    Q_SIGNALS:
        void categorySelected(SearchCategory _category);

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

        TextRendering::TextUnitPtr label_;
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
