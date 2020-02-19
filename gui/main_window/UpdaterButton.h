#pragma once

#include "controls/ClickWidget.h"

namespace Ui
{
    namespace TextRendering
    {
        class TextUnit;
    }

    class UpdaterButton : public ClickableWidget
    {
        Q_OBJECT

    public:
        explicit UpdaterButton(QWidget* _parent);
        ~UpdaterButton();

    protected:
        void paintEvent(QPaintEvent* _event) override;

    private:
        std::unique_ptr<TextRendering::TextUnit> textUnit_;
    };
}