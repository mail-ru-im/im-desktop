#pragma once

#include "../../controls/TransparentScrollBar.h"

namespace Ui
{
    class RecentsView : public ListViewWithTrScrollBar
    {
        Q_OBJECT

        void init();

    public:
        RecentsView(QWidget* _parent = nullptr);
        ~RecentsView();

    };
}
