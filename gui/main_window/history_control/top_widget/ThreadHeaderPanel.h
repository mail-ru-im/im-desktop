#pragma once

namespace Ui
{
    class HeaderTitleBar;
    class HeaderTitleBarButton;
    enum class FrameCountMode;

    class ThreadHeaderPanel : public QWidget
    {
        Q_OBJECT

    public:
        ThreadHeaderPanel(const QString& _aimId, QWidget* _parent);
        void updateCloseIcon(FrameCountMode _mode);
        bool isSearchIsActive() const;

    private:
        void onInfoClicked();
        void onCloseClicked();
        void onSearchClicked();

        void activateSearchButton(const QString& _searchedDialog);
        void deactivateSearchButton();
        void updateParentChatName(const QString& _aimId, const QString& _friendly);

    private:
        QString aimId_;
        FrameCountMode mode_;
        HeaderTitleBar* titleBar_;
        HeaderTitleBarButton* closeButton_;
        HeaderTitleBarButton* searchButton_;
    };
}