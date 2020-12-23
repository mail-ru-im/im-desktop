// Copyright(c) 2015, 2016 Jari Pennanen

#pragma once

#include "stdafx.h"
#include <objbase.h>
#include <ObjectArray.h>

namespace VirtualDesktops
{
    namespace API
    {
        const CLSID CLSID_ImmersiveShell = {
            0xC2F03A33, 0x21F5, 0x47FA, 0xB4, 0xBB, 0x15, 0x63, 0x62, 0xA2, 0xF2, 0x39 };

        const CLSID CLSID_IVirtualNotificationService = {
            0xA501FDEC, 0x4A09, 0x464C, 0xAE, 0x4E, 0x1B, 0x9C, 0x21, 0xB8, 0x49, 0x18 };

        const IID IID_IVirtualDesktopNotification = {
            0xC179334C, 0x4295, 0x40D3, 0xBE, 0xA1, 0xC6, 0x54, 0xD9, 0x65, 0x60, 0x5A };


        // see IApplicationView in Windows Runtime
        struct IApplicationView : public IUnknown
        {
        public:

        };

        // Virtual Desktop
        EXTERN_C const IID IID_IVirtualDesktop;

        MIDL_INTERFACE("FF72FFDD-BE7E-43FC-9C03-AD81681E88E4")
            IVirtualDesktop : public IUnknown
        {
        public:
            virtual HRESULT STDMETHODCALLTYPE IsViewVisible(
                IApplicationView * pView,
                int* pfVisible) = 0;

            virtual HRESULT STDMETHODCALLTYPE GetID(
                GUID* pGuid) = 0;
        };

        // Public virtual desktop manager [https://msdn.microsoft.com/en-us/library/windows/desktop/mt186440(v=vs.85).aspx]

        EXTERN_C const IID IID_IVirtualDesktopManager;

        MIDL_INTERFACE("a5cd92ff-29be-454c-8d04-d82879fb3f1b")
            IVirtualDesktopManager : public IUnknown
        {
        public:
            virtual HRESULT STDMETHODCALLTYPE IsWindowOnCurrentVirtualDesktop(
                /* [in] */ __RPC__in HWND topLevelWindow,
                /* [out] */ __RPC__out BOOL * onCurrentDesktop) = 0;

            virtual HRESULT STDMETHODCALLTYPE GetWindowDesktopId(
                /* [in] */ __RPC__in HWND topLevelWindow,
                /* [out] */ __RPC__out GUID* desktopId) = 0;

            virtual HRESULT STDMETHODCALLTYPE MoveWindowToDesktop(
                /* [in] */ __RPC__in HWND topLevelWindow,
                /* [in] */ __RPC__in REFGUID desktopId) = 0;
        };

        // Undocumented interface for receiving notifications about virtual desktops changes
        EXTERN_C const IID IID_IVirtualDesktopNotification;

        MIDL_INTERFACE("C179334C-4295-40D3-BEA1-C654D965605A")
            IVirtualDesktopNotification : public IUnknown
        {
        public:
            virtual HRESULT STDMETHODCALLTYPE VirtualDesktopCreated(
                IVirtualDesktop * pDesktop) = 0;

            virtual HRESULT STDMETHODCALLTYPE VirtualDesktopDestroyBegin(
                IVirtualDesktop* pDesktopDestroyed,
                IVirtualDesktop* pDesktopFallback) = 0;

            virtual HRESULT STDMETHODCALLTYPE VirtualDesktopDestroyFailed(
                IVirtualDesktop* pDesktopDestroyed,
                IVirtualDesktop* pDesktopFallback) = 0;

            virtual HRESULT STDMETHODCALLTYPE VirtualDesktopDestroyed(
                IVirtualDesktop* pDesktopDestroyed,
                IVirtualDesktop* pDesktopFallback) = 0;

            virtual HRESULT STDMETHODCALLTYPE ViewVirtualDesktopChanged(
                IApplicationView* pView) = 0;

            virtual HRESULT STDMETHODCALLTYPE CurrentVirtualDesktopChanged(
                IVirtualDesktop* pDesktopOld,
                IVirtualDesktop* pDesktopNew) = 0;

        };

        EXTERN_C const IID IID_IVirtualDesktopNotificationService;

        MIDL_INTERFACE("0CD45E71-D927-4F15-8B0A-8FEF525337BF")
            IVirtualDesktopNotificationService : public IUnknown
        {
        public:
            // Register implementation of IVirtualDesktopNotification interface
            virtual HRESULT STDMETHODCALLTYPE Register(
                IVirtualDesktopNotification * pNotification,
                DWORD * pdwCookie) = 0;

            virtual HRESULT STDMETHODCALLTYPE Unregister(
                DWORD dwCookie) = 0;
        };
    }
}
