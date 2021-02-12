#pragma once

#include "controls/FlatMenu.h"

namespace Statuses
{
    class StatusDurationMenu_p;

    //////////////////////////////////////////////////////////////////////////
    // StatusDurationMenu
    //////////////////////////////////////////////////////////////////////////

    class StatusDurationMenu : public Ui::FlatMenu
    {
        Q_OBJECT
    public:
        StatusDurationMenu(QWidget* _parent = nullptr);
        ~StatusDurationMenu();

    Q_SIGNALS:
        void durationClicked(std::chrono::seconds _duration, QPrivateSignal);
        void customTimeClicked(QPrivateSignal);

    private Q_SLOTS:
        void onActionTriggered(QAction* _action);

    private:
        std::unique_ptr<StatusDurationMenu_p> d;
    };

}
