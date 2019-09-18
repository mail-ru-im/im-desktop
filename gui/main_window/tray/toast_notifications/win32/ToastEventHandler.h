/*#pragma once

#include "ToastManager.h"

namespace Ui
{
    class ToastManager;

    typedef ABI::Windows::Foundation::ITypedEventHandler<ABI::Windows::UI::Notifications::ToastNotification *, ::IInspectable *> DesktopToastActivatedEventHandler;
    typedef ABI::Windows::Foundation::ITypedEventHandler<ABI::Windows::UI::Notifications::ToastNotification *, ABI::Windows::UI::Notifications::ToastDismissedEventArgs *> DesktopToastDismissedEventHandler;
    typedef ABI::Windows::Foundation::ITypedEventHandler<ABI::Windows::UI::Notifications::ToastNotification *, ABI::Windows::UI::Notifications::ToastFailedEventArgs *> DesktopToastFailedEventHandler;

    class ToastEventHandler : public Microsoft::WRL::Implements<DesktopToastActivatedEventHandler,
        DesktopToastDismissedEventHandler, DesktopToastFailedEventHandler>
    {
    public:
        ToastEventHandler::ToastEventHandler(ToastManager* manager, Microsoft::WRL::ComPtr<ABI::Windows::UI::Notifications::IToastNotification> notification, const QString& aimId);
        ~ToastEventHandler();

        IFACEMETHODIMP Invoke(_In_ ABI::Windows::UI::Notifications::IToastNotification*, _In_ IInspectable*);
        IFACEMETHODIMP Invoke(_In_ ABI::Windows::UI::Notifications::IToastNotification*, _In_ ABI::Windows::UI::Notifications::IToastDismissedEventArgs*);
        IFACEMETHODIMP Invoke(_In_ ABI::Windows::UI::Notifications::IToastNotification*, _In_ ABI::Windows::UI::Notifications::IToastFailedEventArgs*);
        IFACEMETHODIMP_(ULONG) AddRef()
        {
            return InterlockedIncrement(&_Ref);
        }

        IFACEMETHODIMP_(ULONG) Release()
        {
            ULONG l = InterlockedDecrement(&_Ref);
            if (l == 0) delete this;
            return l;
        }

        IFACEMETHODIMP QueryInterface(_In_ REFIID riid, _COM_Outptr_ void **ppv)
        {
            if (IsEqualIID(riid, IID_IUnknown))
                *ppv = static_cast<IUnknown*>(static_cast<DesktopToastActivatedEventHandler*>(this));
            else if (IsEqualIID(riid, __uuidof(DesktopToastActivatedEventHandler)))
                *ppv = static_cast<DesktopToastActivatedEventHandler*>(this);
            else if (IsEqualIID(riid, __uuidof(DesktopToastDismissedEventHandler)))
                *ppv = static_cast<DesktopToastDismissedEventHandler*>(this);
            else if (IsEqualIID(riid, __uuidof(DesktopToastFailedEventHandler)))
                *ppv = static_cast<DesktopToastFailedEventHandler*>(this);
            else *ppv = nullptr;

            if (*ppv)
            {
                reinterpret_cast<IUnknown*>(*ppv)->AddRef();
                return S_OK;
            }

            return E_NOINTERFACE;
        }

    private:
        ULONG _Ref;
        Microsoft::WRL::ComPtr<ABI::Windows::UI::Notifications::IToastNotification> Notification_;
        QString AimId_;
        ToastManager* Manager_;
    };
}*/