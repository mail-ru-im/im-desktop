#include "stdafx.h"

#include "LoaderOverlay.h"
#include "LoaderSpinner.h"

#include "utils/utils.h"

namespace
{
    QSize getSpinnerSize()
    {
        return Utils::scale_value(QSize(64, 64));
    }
}

namespace Ui
{
    LoaderOverlay::LoaderOverlay(QWidget* _parent)
        : ClickableWidget(_parent)
        , spinner_(new LoaderSpinner(this, getSpinnerSize()))
    {
        connect(spinner_, &LoaderSpinner::clicked, this, &LoaderOverlay::onClicked);
        connect(this, &ClickableWidget::clicked, this, &LoaderOverlay::onClicked);
    }

    void LoaderOverlay::setCenteringRect(const QRect& _rect)
    {
        centerRect_ = _rect;
        onResize();
    }

    QRect LoaderOverlay::getCenteringRect() const
    {
        return centerRect_.isValid() ? centerRect_ : rect();
    }

    void LoaderOverlay::resizeEvent(QResizeEvent* _event)
    {
        onResize();
    }

    void LoaderOverlay::showEvent(QShowEvent* _event)
    {
        onResize();
        spinner_->startAnimation();
    }

    void LoaderOverlay::hideEvent(QHideEvent* _event)
    {
        spinner_->stopAnimation();
    }

    void LoaderOverlay::onClicked()
    {
        emit cancelled(QPrivateSignal());
    }

    void LoaderOverlay::onResize()
    {
        auto spinnerRect = spinner_->rect();
        spinnerRect.moveCenter(getCenteringRect().center());
        spinner_->setGeometry(spinnerRect);
    }

}