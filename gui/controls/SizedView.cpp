#include "SizedView.h"

namespace Ui
{

SizedView::SizedView(QWidget* _parent)
    : QTreeView(_parent)
{
}

void SizedView::paintEvent(QPaintEvent* _event)
{
    const auto expectedWidth = width() * 0.65;
    if (header()->sectionSize(0) != expectedWidth)
    {
        header()->resizeSection(0, expectedWidth);
        header()->resizeSection(1, width() * 0.3);
    }

    QTreeView::paintEvent(_event);
}

void SizedView::mouseReleaseEvent(QMouseEvent* _event)
{
    QTreeView::mouseReleaseEvent(_event);
    if (_event->button() == Qt::RightButton)
    {
        auto selected = selectionModel()->selectedIndexes();
        if (!selected.isEmpty())
            emit clicked(selected.constFirst());
    }
}

}
