#pragma once

namespace Ui
{
    class ContactListWidget;
    class ContactListTopPanel;
    class HeaderTitleBarButton;
    enum class LeftPanelState;

    class ContactsTab : public QWidget
    {
        Q_OBJECT

    public:
        explicit ContactsTab(QWidget* _parent = nullptr);
        ~ContactsTab();

        void setState(const LeftPanelState _state);
        void setClWidth(int _width);

        ContactListWidget* getContactListWidget() const;

    private:
        LeftPanelState state_;
        ContactListTopPanel* header_;
        ContactListWidget* contactListWidget_;
        HeaderTitleBarButton* addButton_;
        QWidget* spacer_;
    };
}