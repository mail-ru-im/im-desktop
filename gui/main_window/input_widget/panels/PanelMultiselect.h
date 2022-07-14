#pragma once

#include "../InputWidgetUtils.h"

namespace Ui
{
    class RoundButton;

    class InputPanelMultiselect : public QWidget, public StyledInputElement
    {
        Q_OBJECT
    public:
        InputPanelMultiselect(QWidget* _parent, const QString& _aimId);
        void updateElements();

    protected:
        void updateStyleImpl(const InputStyleMode _mode) override;
        void resizeEvent(QResizeEvent* _event) override;

    private Q_SLOTS:
        void multiSelectCurrentElementChanged();
        void selectedCount(const QString& _aimid, int _totalCount, int _unsupported, int _plainFiles, bool _deleteEnabled = true);
        bool isDisabled() const;

    private:
        RoundButton* delete_;
        RoundButton* favorites_;
        RoundButton* copy_;
        RoundButton* reply_;
        RoundButton* forward_;

        QString aimid_;
    };
}
