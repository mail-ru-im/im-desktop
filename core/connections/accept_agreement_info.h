#pragma once

namespace core {

struct accept_agreement_info
{
    enum class agreement_action
    {
        accept = 0,
        ignore = 1
    };

    accept_agreement_info(agreement_action _action,
                          bool _reset = false)
        : action_(_action),
          reset_(_reset)
    { }

    accept_agreement_info(const accept_agreement_info& _info) = default;

    agreement_action action_ = agreement_action::accept;
    bool reset_ = false;
};

}
