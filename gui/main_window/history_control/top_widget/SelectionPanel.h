#pragma once

namespace Ui
{
    class ClickableTextWidget;
    class CustomButton;

    class SelectionPanel : public QFrame
    {
        Q_OBJECT

    public:
        SelectionPanel(const QString& _aimId, QWidget* _parent);

    private Q_SLOTS:
        void setSelectedCount(const QString& _aimId, int _count, int, bool);
        void multiSelectCurrentElementChanged();

    private:
        ClickableTextWidget* label_;
        CustomButton* cancel_;
        QString aimId_;
    };
}
