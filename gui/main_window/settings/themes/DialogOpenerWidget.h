#pragma once

#include "controls/ClickWidget.h"
#include "controls/TextUnit.h"

namespace Ui
{
    class DialogOpenerWidget : public ClickableWidget
    {
        Q_OBJECT

    public:
        DialogOpenerWidget(QWidget* _parent, const QString& _caption);

    protected:
        void paintEvent(QPaintEvent* _e) override;

    private:
        TextRendering::TextUnitPtr caption_;
    };
}