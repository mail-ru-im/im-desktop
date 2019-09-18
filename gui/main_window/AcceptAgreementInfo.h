#pragma once

namespace Ui {

struct AcceptAgreementInfo
{
    enum class AgreementAction
    {
        Accept = 0,
        Ignore = 1
    };

    AcceptAgreementInfo(AgreementAction _action)
        : action_(_action)
    { }

    AgreementAction action_ = AgreementAction::Accept;
    bool reset_ = false;
};

}
