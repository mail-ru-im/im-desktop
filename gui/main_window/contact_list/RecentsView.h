#pragma once

#include "../../controls/TransparentScrollBar.h"

namespace Ui
{


    class RecentsView : public ListViewWithTrScrollBar
    {
        Q_OBJECT

        void init();

    public:
        RecentsView(QWidget* _parent);
        ~RecentsView();

        static RecentsView* create(QWidget* _parent);
    };
}
