#pragma once

#include "controls/ClickWidget.h"
#include "controls/TextUnit.h"

namespace Ui
{
    class PageOpenerWidget : public ClickableWidget
    {
        Q_OBJECT

    public:
        PageOpenerWidget(QWidget* _parent, const QString& _caption);
        void setText(const QString& _text);

    protected:
        void paintEvent(QPaintEvent* _e) override;

    private:
        TextRendering::TextUnitPtr caption_;
    };
}