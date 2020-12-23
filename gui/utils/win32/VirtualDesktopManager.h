// Copyright(c) 2015, 2016 Jari Pennanen

#include "VirtualDesktopAPI.h"

namespace VirtualDesktops
{
	// Used for compability of COM with QObject without QAxAggregated, wndProc, etc.
	class VirtualDesktopNotificationHelper : public QObject
	{
		Q_OBJECT

	Q_SIGNALS:
		void desktopChanged(GUID _new);
	};

    class VirtualDesktopNotification : public API::IVirtualDesktopNotification, virtual public VirtualDesktopNotificationHelper
    {
	private:
		ULONG _referenceCount;
	public:
		// Inherited via IVirtualDesktopNotification
		virtual HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, void** ppvObject) override
		{
			// Always set out parameter to NULL, validating it first.
			if (!ppvObject)
				return E_INVALIDARG;
			*ppvObject = NULL;

			if (riid == IID_IUnknown || riid == API::IID_IVirtualDesktopNotification)
			{
				// Increment the reference count and return the pointer.
				*ppvObject = (LPVOID)this;
				AddRef();
				return S_OK;
			}
			return E_NOINTERFACE;
		}

		virtual ULONG STDMETHODCALLTYPE AddRef() override
		{
			return InterlockedIncrement(&_referenceCount);
		}

		virtual ULONG STDMETHODCALLTYPE Release() override
		{
			ULONG result = InterlockedDecrement(&_referenceCount);
			if (result == 0)
				delete this;
			return 0;
		}

		virtual HRESULT STDMETHODCALLTYPE CurrentVirtualDesktopChanged(
			API::IVirtualDesktop* pDesktopOld,
			API::IVirtualDesktop* pDesktopNew) override
		{
			Q_UNUSED(pDesktopOld);

			GUID newId;
			if (pDesktopNew != nullptr)
				pDesktopNew->GetID(&newId);

			Q_EMIT desktopChanged(newId);
			return S_OK;
		}

		// unused (to avoid abstract)
		virtual HRESULT STDMETHODCALLTYPE VirtualDesktopCreated(API::IVirtualDesktop* pDesktop) override
		{
			return S_OK;
		}
		virtual HRESULT STDMETHODCALLTYPE VirtualDesktopDestroyBegin(API::IVirtualDesktop* pDesktopDestroyed, API::IVirtualDesktop* pDesktopFallback) override
		{
			return S_OK;
		}
		virtual HRESULT STDMETHODCALLTYPE VirtualDesktopDestroyFailed(API::IVirtualDesktop* pDesktopDestroyed, API::IVirtualDesktop* pDesktopFallback) override
		{
			return S_OK;
		}
		virtual HRESULT STDMETHODCALLTYPE VirtualDesktopDestroyed(API::IVirtualDesktop* pDesktopDestroyed, API::IVirtualDesktop* pDesktopFallback) override
		{
			return S_OK;
		}
		virtual HRESULT STDMETHODCALLTYPE ViewVirtualDesktopChanged(API::IApplicationView* pView) override
		{
			return S_OK;
		}

    };

    class VirtualDesktopManager : public QObject
    {
		Q_OBJECT
	public:
		VirtualDesktopManager();
        ~VirtualDesktopManager();

		void registerService();
		void moveWidgetToCurrentDesktop(QWidget* _w);

	private:
		void registerNotifications();
		void unregisterNotifications();
		void setCurrentDesktop(GUID _guid);

	private:
		IServiceProvider* serviceProvider_;
		API::IVirtualDesktopManager* desktopManager_;
		API::IVirtualDesktopNotificationService* notificationService_;
		VirtualDesktopNotification* notification_;
		GUID currentDesktop_;
		DWORD notificationServiceId_ = 0;
    };

	VirtualDesktopManager* getVirtualDesktopManager();
	void initVirtualDesktopManager();
	void resetVirtualDesktopManager();
}