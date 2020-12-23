#pragma once

#include "network_change_notifier.h"

namespace core 
{
    class network_change_observer : public network_change_notifier::network_change_observer 
    {
    public:
        network_change_observer();
        ~network_change_observer();
        virtual void on_network_down() override;
        virtual void on_network_up() override;
        virtual void on_network_change() override;
    };
}; // namespace core