#pragma once

namespace Ui
{

    enum class ForceLocked
    {
        No,
        Yes
    };

    class MainStackedWidget : public QStackedWidget
    {
        Q_OBJECT
    public:
        MainStackedWidget(QWidget* _parent);

    public Q_SLOTS:
        void setCurrentIndex(int _index, ForceLocked _forceLocked = ForceLocked::No);
        void setCurrentWidget(QWidget* _widget, ForceLocked _forceLocked = ForceLocked::No);
    };

}