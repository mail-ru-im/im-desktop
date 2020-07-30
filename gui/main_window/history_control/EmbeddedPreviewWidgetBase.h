#pragma once

#include "HistoryControlPageItem.h"

namespace Ui
{

    class ActionButtonWidget;

    class EmbeddedPreviewWidgetBase : public QWidget
    {
    public:
        static EmbeddedPreviewWidgetBase* makePlainPreview(QPixmap &pixmap);

        EmbeddedPreviewWidgetBase(QWidget *parent);

    private:
        ActionButtonWidget *ActionButton_;

    };

}