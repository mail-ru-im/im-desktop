#pragma once

namespace Ui
{
    class ContactListWidget;
    class ContactListTopPanel;
    class HeaderTitleBarButton;
    class SearchWidget;
    enum class LeftPanelState;

    class CallsTab : public QWidget
    {
        Q_OBJECT

    public:
        explicit CallsTab(QWidget* _parent = nullptr);
        ~CallsTab();

        void setState(const LeftPanelState _state);
        void setClWidth(int _width);

        ContactListWidget* getContactListWidget() const;
        SearchWidget* getSearchWidget() const;

        void setPictureOnlyView(bool _isPictureOnly);

    private:
        LeftPanelState state_;
        ContactListTopPanel* header_;
        ContactListWidget* contactListWidget_;
        HeaderTitleBarButton* callButton_;
        QWidget* spacer_;
    };
}