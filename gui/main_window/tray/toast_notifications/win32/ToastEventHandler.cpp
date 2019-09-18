/*#include "stdafx.h"
#include "ToastEventHandler.h"
#include "ToastManager.h"

using namespace ABI::Windows::UI::Notifications;

namespace Ui
{

    ToastEventHandler::ToastEventHandler(ToastManager* manager, Microsoft::WRL::ComPtr<ABI::Windows::UI::Notifications::IToastNotification> notification,
    const QString& aimId)
        : _Ref(1)
        , Manager_(manager)
        , Notification_(notification)
        , AimId_(aimId)
    {
    }

    ToastEventHandler::~ToastEventHandler()
    {
    }

    IFACEMETHODIMP ToastEventHandler::Invoke(_In_ IToastNotification*, _In_ IInspectable*)
    {
        Manager_->Activated(AimId_);
        return S_OK;
    }

    IFACEMETHODIMP ToastEventHandler::Invoke(_In_ IToastNotification*, _In_ IToastDismissedEventArgs*)
    {
        Manager_->Remove(Notification_, AimId_);
        return S_OK;
    }

    IFACEMETHODIMP ToastEventHandler::Invoke(_In_ IToastNotification*, _In_ IToastFailedEventArgs*)
    {
        Manager_->Remove(Notification_, AimId_);
        return S_OK;
    }
}*/