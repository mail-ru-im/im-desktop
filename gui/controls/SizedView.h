#pragma once

#include <QTreeView>

namespace Ui
{

class SizedView : public QTreeView
{
    Q_OBJECT

public:
    SizedView(QWidget* _parent);

protected:
    virtual void paintEvent(QPaintEvent* _event) override;
    virtual void mouseReleaseEvent(QMouseEvent* _event) override;
};


}
