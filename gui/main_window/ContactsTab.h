#pragma once

namespace Ui
{
    class ContactListWidget;
    class ContactListTopPanel;
    class HeaderTitleBarButton;
    class SearchWidget;
    enum class LeftPanelState;

    class ContactsTab : public QWidget
    {
        Q_OBJECT

    public:
        explicit ContactsTab(QWidget* _parent = nullptr);

        void setState(const LeftPanelState _state);
        void setClWidth(int _width);

        ContactListWidget* getContactListWidget() const;
        SearchWidget* getSearchWidget() const;

    private:
        void dragPositionUpdate(QPoint _pos, bool _fromScroll);

    private Q_SLOTS:
        void onDragPositionUpdate(QPoint _pos);
        void dropMimeData(QPoint _pos, const QMimeData* _mimeData);
        void autoScroll();

    private:
        LeftPanelState state_;
        ContactListTopPanel* header_;
        ContactListWidget* contactListWidget_;
        HeaderTitleBarButton* addButton_;
        QWidget* spacer_;

        QTimer* scrollTimer_;
        QPoint lastDragPos_;
        int scrollMultipler_;
    };
}