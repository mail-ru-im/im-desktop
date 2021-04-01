// Copyright(c) 2015, 2016 Jari Pennanen

#include "stdafx.h"
#include "VirtualDesktopManager.h"

namespace VirtualDesktops
{
    VirtualDesktopManager::VirtualDesktopManager()
        : serviceProvider_(nullptr)
        , desktopManager_(nullptr)
        , notificationService_(nullptr)
        , notification_(nullptr)
    {
    }

    VirtualDesktopManager::~VirtualDesktopManager()
    {
        unregisterNotifications();

        if (notificationService_)
            notificationService_->Release();

        if (desktopManager_)
            desktopManager_->Release();

        if (serviceProvider_)
            serviceProvider_->Release();
    }

    void VirtualDesktopManager::registerService()
    {
        ::CoCreateInstance(API::CLSID_ImmersiveShell, NULL, CLSCTX_LOCAL_SERVER, __uuidof(IServiceProvider), (PVOID*)&serviceProvider_);
        im_assert(serviceProvider_);

        if (serviceProvider_)
        {
            serviceProvider_->QueryService(__uuidof(API::IVirtualDesktopManager), &desktopManager_);
            im_assert(desktopManager_);

            serviceProvider_->QueryService(API::CLSID_IVirtualNotificationService, __uuidof(API::IVirtualDesktopNotificationService), (PVOID*)&notificationService_);
            im_assert(notificationService_);
        }

        registerNotifications();
    }

    void VirtualDesktopManager::moveWidgetToCurrentDesktop(QWidget* _w)
    {
        // if currentDesktop_ is invalid, moves to the first desktop
        if (desktopManager_)
            desktopManager_->MoveWindowToDesktop((HWND)_w->winId(), currentDesktop_);
    }

    void VirtualDesktopManager::registerNotifications()
    {
        if (notificationService_)
        {
            if (!notification_)
            {
                notification_ = new VirtualDesktopNotification();
                connect(notification_, &VirtualDesktopNotification::desktopChanged, this, &VirtualDesktopManager::setCurrentDesktop);
            }

            notificationService_->Register(static_cast<API::IVirtualDesktopNotification*>(notification_), &notificationServiceId_);
        }
    }

    void VirtualDesktopManager::unregisterNotifications()
    {
        if (notificationService_ && notificationServiceId_ > 0)
            notificationService_->Unregister(notificationServiceId_);
    }

    void VirtualDesktopManager::setCurrentDesktop(GUID _guid)
    {
        currentDesktop_ = _guid;
    }

    std::unique_ptr<VirtualDesktopManager> g_desktopManager;

    VirtualDesktopManager* getVirtualDesktopManager()
    {
        if (!g_desktopManager)
            g_desktopManager = std::make_unique<VirtualDesktopManager>();
        return g_desktopManager.get();
    }

    void initVirtualDesktopManager()
    {
        getVirtualDesktopManager()->registerService();
    }

    void resetVirtualDesktopManager()
    {
        g_desktopManager.reset();
    }
}