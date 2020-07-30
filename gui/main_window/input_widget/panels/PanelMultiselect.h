#pragma once

#include "../InputWidgetUtils.h"

namespace Ui
{
    class RoundButton;

    class InputPanelMultiselect : public QWidget, public StyledInputElement
    {
        Q_OBJECT
    public:
        InputPanelMultiselect(QWidget* _parent);
        void updateElements();
        void setContact(const QString& _aimid);

    protected:
        void updateStyleImpl(const InputStyleMode _mode) override;
        void resizeEvent(QResizeEvent* _event) override;

    private Q_SLOTS:
        void multiSelectCurrentElementChanged();
        void selectedCount(int, int);

    private:
        RoundButton* delete_;
        RoundButton* favorites_;
        RoundButton* copy_;
        RoundButton* reply_;
        RoundButton* forward_;

        QString aimid_;
    };
}
