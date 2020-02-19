#pragma once

namespace Ui
{
    class ClickableTextWidget;
    class CustomButton;

    class SelectionPanel : public QFrame
    {
        Q_OBJECT

    public:
        SelectionPanel(QWidget* _parent);

    private Q_SLOTS:
        void setSelectedCount(int _count);
        void multiSelectCurrentElementChanged();

    private:
        ClickableTextWidget* label_;
        CustomButton* cancel_;
    };
}
