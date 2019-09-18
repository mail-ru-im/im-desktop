#pragma once

#include "ClickWidget.h"

namespace Ui
{
    class LoaderSpinner;

    class LoaderOverlay : public ClickableWidget
    {
        Q_OBJECT

    Q_SIGNALS:
        void cancelled(QPrivateSignal);

    public:
        LoaderOverlay(QWidget* _parent);

        void setCenteringRect(const QRect& _rect);
        QRect getCenteringRect() const;

    protected:
        void resizeEvent(QResizeEvent* _event) override;
        void showEvent(QShowEvent* _event) override;
        void hideEvent(QHideEvent* _event) override;

    private:
        void onClicked();
        void onResize();

        QRect centerRect_;
        LoaderSpinner* spinner_;
    };
}