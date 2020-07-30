#pragma once

#include "TopPanel.h"

namespace Ui
{
    class ContactListTopPanel : public TopPanelHeader
    {
        Q_OBJECT
    public:
        explicit ContactListTopPanel(QWidget* _parent = nullptr);
        ~ContactListTopPanel();

        void setState(const LeftPanelState _state) override;

        SearchWidget* getSearchWidget() const noexcept;

        void addButtonToLeft(HeaderTitleBarButton* _button);
        void addButtonToRight(HeaderTitleBarButton* _button);

        void setTitle(const QString& _title);

    private:
        HeaderTitleBar * titleBar_;
        SearchWidget* searchWidget_;

        LeftPanelState state_;
    };
}
