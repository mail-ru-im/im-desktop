#pragma once

namespace Ui
{
    class DialogButton;
    enum class DialogButtonRole;
}

namespace Statuses
{
    using namespace Ui;

    void drawEditIcon(QPainter& _p, const QPoint& _topLeft);

    DialogButton* createButton(QWidget* _parent, const QString& _text, const DialogButtonRole _role, bool _enabled = true);
}
