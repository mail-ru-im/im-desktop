#include "stdafx.h"
#include "MainStackedWidget.h"
#include "main_window/LocalPIN.h"
#include "../utils/utils.h"

using namespace Ui;

MainStackedWidget::MainStackedWidget(QWidget* _parent)
    : QStackedWidget(_parent)
{
    Utils::setDefaultBackground(this);
}

void MainStackedWidget::setCurrentIndex(int _index, ForceLocked _forceLocked)
{
    if (LocalPIN::instance()->locked() && _forceLocked != ForceLocked::Yes)
    {
        assert(!"setCurrentIndex while locked");
        return;
    }

    QStackedWidget::setCurrentIndex(_index);
}

void MainStackedWidget::setCurrentWidget(QWidget* _widget, ForceLocked _forceLocked)
{
    if (LocalPIN::instance()->locked() && _forceLocked != ForceLocked::Yes)
    {
        assert(!"setCurrentWidget while locked");
        return;
    }

    QStackedWidget::setCurrentWidget(_widget);
}
