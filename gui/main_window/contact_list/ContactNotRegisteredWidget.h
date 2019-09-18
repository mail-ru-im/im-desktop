#pragma once

#include <QWidget>
#include "controls/TextUnit.h"

class QVBoxLayout;
class QSpacerItem;

namespace Ui
{

class ContactNotRegisteredWidget: public QWidget
{
    Q_OBJECT

Q_SIGNALS:
    void enterPressed();

public:
    explicit ContactNotRegisteredWidget(QWidget* _parent = nullptr);
    void setInfo(const QString& _phoneNumber, const QString& _firstName);

protected:
    void paintEvent(QPaintEvent* _event);
    void keyPressEvent(QKeyEvent* _event);

private:
    TextRendering::TextUnitPtr textUnit_;
    QPixmap catPixmap_;
};

}
