#include "RecentsView.h"

using namespace Ui;

RecentsView::RecentsView(QWidget* _parent)
    : ListViewWithTrScrollBar(_parent)
{
}


RecentsView::~RecentsView()
{
}

void RecentsView::init()
{
    auto scrollBar = new TransparentScrollBarV();

    setScrollBarV(scrollBar);

    setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

    setFocusPolicy(Qt::NoFocus);

    scrollBar->setGetContentSizeFunc([this]() { return contentSize(); });

    setFrameShape(QFrame::NoFrame);
    setLineWidth(0);
    setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    setSizeAdjustPolicy(QAbstractScrollArea::AdjustToContents);
    setAutoScroll(false);
    setEditTriggers(QAbstractItemView::NoEditTriggers);
    setUniformItemSizes(false);
    setBatchSize(50);

    setMouseTracking(true);
    setAcceptDrops(true);
    setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);
    setResizeMode(QListView::Adjust);
    setSelectByMouseHover(true);
}

RecentsView* RecentsView::create(QWidget* _parent)
{
    auto result = new RecentsView(_parent);

    result->init();

    return result;
}
