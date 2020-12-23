// Copyright (c) 2012 The Chromium Authors. All rights reserved.

#pragma once

#include <memory>
#include <unordered_set>


#include "../network_change_notifier.h"
#include "address_tracker_linux.h"

namespace core {

    class network_change_notifier_linux : public network_change_notifier
    {
    public:
        // Creates network_change_notifier_linux with a list of ignored interfaces.
        explicit network_change_notifier_linux(std::unique_ptr<network_change_observer> _observer);

        ~network_change_notifier_linux() override;
    protected:
        virtual connection_type get_current_connection_type() const override;
        virtual void init() override;
    private:
        static network_change_calculator_params network_change_calculator_params_linux();

        std::unique_ptr<internal::address_tracker_linux> address_tracker_;



    };

}  // namespace core

