#pragma once

namespace Ui
{
    class HeaderTitleBarButton;
    enum class FrameCountMode;

    class ThreadHeaderPanel : public QWidget
    {
        Q_OBJECT

    public:
        ThreadHeaderPanel(const QString& _aimId, QWidget* _parent);
        void updateCloseIcon(FrameCountMode _mode);

    private:
        void onInfoClicked();
        void onCloseClicked();

    private:
        QString aimId_;
        HeaderTitleBarButton* closeButton_;
    };
}