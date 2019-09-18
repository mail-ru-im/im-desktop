#pragma once

namespace Ui
{
    class SwitcherCheckbox : public QCheckBox
    {
        Q_OBJECT

    public:
        SwitcherCheckbox(QWidget* _parent);

    protected:
        void paintEvent(QPaintEvent* _e) override;
        bool hitButton(const QPoint &pos) const override;
    };
}