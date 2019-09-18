#pragma once

#include "controls/TextUnit.h"
#include "controls/ClickWidget.h"
#include "types/message.h"
#include "../InputWidgetUtils.h"

namespace Ui
{
    class CustomButton;

    class EditBlockText : public ClickableWidget
    {
        Q_OBJECT

    public:
        EditBlockText(QWidget* _parent);

        void setMessage(Data::MessageBuddySptr _message);

    protected:
        void paintEvent(QPaintEvent*) override;
        void resizeEvent(QResizeEvent*) override;

    private:
        void elide();

    private:
        TextRendering::TextUnitPtr caption_;
        TextRendering::TextUnitPtr text_;
    };

    class EditMessageWidget : public QWidget, public StyledInputElement
    {
        Q_OBJECT

    Q_SIGNALS:
        void messageClicked(QPrivateSignal);
        void cancelClicked(QPrivateSignal);

    public:
        EditMessageWidget(QWidget* _parent);

        void setMessage(Data::MessageBuddySptr _message);

    protected:
        void updateStyleImpl(const InputStyleMode _mode) override;

    private:
        EditBlockText* editBlockText_;
        CustomButton* buttonCancel_;
    };
}