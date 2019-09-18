#include "stdafx.h"
#include "Win7Features.h"
#include "shell_link.h"

namespace installer
{
	namespace links
	{
		#define PIN_TO_TASKBAR 5386
		#define PIN_TO_STARTMENU 5381
		#define UNPIN_FROM_STARTMENU 5382

		CWin7Features::CWin7Features(void)
		: m_lpChangeWindowMessageFilter_Func(NULL),
			m_lpSetCurrentProcessExplicitAppUserModelID_Func(NULL),

			m_lpShell_NotifyIconGetRect_Func(NULL),
			m_lpSHGetKnownFolderPath(NULL),
			m_lpSHCreateItemFromParsingName_Func(NULL),
			m_lpDwmSetIconicThumbnail_Func(NULL),
			m_lpDwmSetIconicLivePreviewBitmap_Func(NULL),
			m_lpDwmInvalidateIconicBitmaps_Func(NULL),
			m_lpDwmSetWindowAttribute_Func(NULL),
			m_lpDwmGetWindowAttribute_Func(NULL),
			m_lpDwmIsCompositionEnabled_Func(NULL),
			m_lpDwmExtendFrameIntoClientArea_Func(NULL),
			m_lpCalculatePopupWindowPosition_Func(0),

			osVer()
		{

		}

		CWin7Features::~CWin7Features(void)
		{
		}

		void CWin7Features::CreateJumpList(JumpList &items)
		{
			if(!win7())
			{
				return;
			}
			ICustomDestinationList *pcdl;
			HRESULT hr = CoCreateInstance(CLSID_DestinationList, NULL, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&pcdl));
			if (SUCCEEDED(hr))
			{
				TCHAR szAppPath[1024];
				::GetModuleFileName(NULL, szAppPath, 1024);

				UINT cMinSlots;
				IObjectArray *poaRemoved;
				hr = pcdl->BeginList(&cMinSlots, IID_PPV_ARGS(&poaRemoved));
				if (SUCCEEDED(hr))
				{
					IObjectCollection *poc;
					HRESULT hr = CoCreateInstance(CLSID_EnumerableObjectCollection, NULL, CLSCTX_INPROC, IID_PPV_ARGS(&poc));
					if (SUCCEEDED(hr))
					{
						InitJumpList(poc, items);
						IObjectArray * poa;
						hr = poc->QueryInterface(IID_PPV_ARGS(&poa));
						if (SUCCEEDED(hr))
						{
							hr = pcdl->AddUserTasks(poa);
							poa->Release();
						}
						if (SUCCEEDED(hr))
						{
							hr = pcdl->CommitList();
						}
					}
					poaRemoved->Release();
				}
				pcdl->Release();
			}
		}

		void CWin7Features::RemoveJumpList()
		{
			if(!win7())
			{
				return;
			}
			ICustomDestinationList *pcdl;
			HRESULT hr = CoCreateInstance(CLSID_DestinationList, NULL, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&pcdl));
			if (SUCCEEDED(hr))
			{
				hr = pcdl->DeleteList(NULL);
				pcdl->Release();
			}

		}

		void CWin7Features::RegisterTab( HWND hTab, HWND hMain )
		{
			ITaskbarList3 *pTaskbarList;
			HRESULT hr = CoCreateInstance(CLSID_TaskbarList, NULL, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&pTaskbarList));
			if (SUCCEEDED(hr))
			{
				hr = pTaskbarList->HrInit();
				if (SUCCEEDED(hr))
				{
					pTaskbarList->RegisterTab(hTab, hMain);
					pTaskbarList->SetTabOrder(hTab, NULL);
					pTaskbarList->SetTabActive(hTab, hMain, 0);
				}

				pTaskbarList->Release();
			}
		}

		DWORD CWin7Features::GetTaskbarButtonMessage()
		{
			static DWORD nMsg = 0;
			if(!nMsg)
			{
				nMsg = ::RegisterWindowMessage(L"TaskbarButtonCreated");
				ChangeWindowMessageFilter(nMsg, MSGFLT_ADD);
			}
			return nMsg;
		}

		void CWin7Features::UnregisterTab( HWND hTab )
		{
			ITaskbarList3 *pTaskbarList;
			HRESULT hr = CoCreateInstance(CLSID_TaskbarList, NULL, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&pTaskbarList));
			if (SUCCEEDED(hr))
			{
				hr = pTaskbarList->HrInit();
				if (SUCCEEDED(hr))
				{
					pTaskbarList->UnregisterTab(hTab);
				}

				pTaskbarList->Release();
			}
		}

		void CWin7Features::ActivateTab( HWND hTab )
		{
			ITaskbarList3 *pTaskbarList;
			HRESULT hr = CoCreateInstance(CLSID_TaskbarList, NULL, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&pTaskbarList));
			if (SUCCEEDED(hr))
			{
				hr = pTaskbarList->HrInit();
				if (SUCCEEDED(hr))
				{
					pTaskbarList->ActivateTab(hTab);
				}

				pTaskbarList->Release();
			}
		}

		void CWin7Features::SetIconThumbnail( HWND hWnd, HBITMAP hBmp )
		{
			if(m_lpDwmSetIconicThumbnail_Func)
			{
				m_lpDwmSetIconicThumbnail_Func(hWnd, hBmp, 0);
			}
		}

		void CWin7Features::SetAppUserModelID( LPWSTR szID )
		{
			if(m_lpSetCurrentProcessExplicitAppUserModelID_Func)
			{
				m_lpSetCurrentProcessExplicitAppUserModelID_Func(szID);
			}
		}

		void CWin7Features::EnableCustomPreviews(HWND, BOOL)
		{
			return;
		/*	BOOL bForceIconic = bEnable;
			BOOL bHasIconicBitmap = bEnable;

			if(m_lpDwmSetWindowAttribute_Func)
			{
				m_lpDwmSetWindowAttribute_Func(
					hWnd,
					DWMWA_FORCE_ICONIC_REPRESENTATION,
					&bForceIconic,
					sizeof(bForceIconic)
				);

				m_lpDwmSetWindowAttribute_Func(
					hWnd,
					DWMWA_HAS_ICONIC_BITMAP,
					&bHasIconicBitmap,
					sizeof(bHasIconicBitmap)
				);
			}*/
		}

		void CWin7Features::SetPreview(HWND, HBITMAP, POINT)
		{
			if(m_lpDwmSetIconicLivePreviewBitmap_Func)
			{
		//		m_lpDwmSetIconicLivePreviewBitmap_Func( hWnd, hBmp, &pnt, 0);
			}
		}

		void CWin7Features::InvalidateThumbnails( HWND hWnd )
		{
			if(m_lpDwmInvalidateIconicBitmaps_Func)
			{
				m_lpDwmInvalidateIconicBitmaps_Func(hWnd);
			}
		}

		void CWin7Features::InitJumpList( IObjectCollection * poc, JumpList &items )
		{
			TCHAR szAppPath[1024];
			::GetModuleFileName(NULL, szAppPath, 1024);
			for (JumpList::iterator it = items.begin(); it != items.end(); it++)
			{
				CComPtr<IShellLink> psl;
				if(it->IsSeparator())
				{
					psl.Attach(CreateSeparator());
				}
				else
				{
					psl.Attach(CreateShellLink(szAppPath,it->m_url, _T(""), it->m_id));
					SetShellLinkTitle(psl,it->m_title);
				}
				if (psl != NULL)
				{
					poc->AddObject(psl);
				}
			}
			return;
		}

		IShellLink * CWin7Features::CreateSeparator()
		{
			IShellLink *psl = NULL;
			HRESULT hres = S_FALSE;
			hres = CoCreateInstance(CLSID_ShellLink, NULL, CLSCTX_INPROC_SERVER, IID_IShellLink, (void**)&psl);
			if( !SUCCEEDED(hres))
			{
				return NULL;
			}

			// Get an IPropertyStore interface.
			IPropertyStore *lpPropStore = NULL;
			psl->QueryInterface(IID_IPropertyStore,(void**)&lpPropStore);
			PROPVARIANT pv;
			if ( lpPropStore )
			{
				hres = InitPropVariantFromBoolean ( TRUE, &pv );
				hres = lpPropStore->SetValue ( PKEY_AppUserModel_IsDestListSeparator, pv );
				PropVariantClear ( &pv );
				hres = lpPropStore->Commit();
				lpPropStore->Release();
			}

			return psl;

		}

		void CWin7Features::Init(bool)
		{
			osVer.dwOSVersionInfoSize = sizeof(osVer);
			if(!GetVersionEx(&osVer))
			{
				osVer = OSVERSIONINFO();
			}


			HMODULE hLib = ::GetModuleHandle(_T("user32.dll"));

			if(hLib)
			{
				m_lpChangeWindowMessageFilter_Func = (ChangeWindowMessageFilter_Type)::GetProcAddress(hLib,"ChangeWindowMessageFilter");
				m_lpCalculatePopupWindowPosition_Func = (CalculatePopupWindowPosition_Type)::GetProcAddress(hLib,"CalculatePopupWindowPosition");
			}
			hLib=NULL;


			hLib = ::GetModuleHandle(_T("shell32.dll"));
			if(hLib)
			{
				m_lpSetCurrentProcessExplicitAppUserModelID_Func = (SetCurrentProcessExplicitAppUserModelID_Type)::GetProcAddress(hLib,"SetCurrentProcessExplicitAppUserModelID");
				m_lpShell_NotifyIconGetRect_Func = (Shell_NotifyIconGetRect_Type)::GetProcAddress(hLib,"Shell_NotifyIconGetRect");
				m_lpSHCreateItemFromParsingName_Func = (SHCreateItemFromParsingName_Type)::GetProcAddress(hLib,"SHCreateItemFromParsingName");
				m_lpSHGetKnownFolderPath = (SHGetKnownFolderPath_Type)::GetProcAddress(hLib,"SHGetKnownFolderPath");

				LoadStringW(hLib, PIN_TO_TASKBAR, m_TaskBarPinCommand.GetBuffer(1023), 1024);
				m_TaskBarPinCommand.ReleaseBuffer();
				LoadStringW(hLib, PIN_TO_STARTMENU, m_StartMenuPinCommand.GetBuffer(1023), 1024);
				m_StartMenuPinCommand.ReleaseBuffer();
				LoadStringW(hLib, UNPIN_FROM_STARTMENU, m_StartMenuUnpinCommand.GetBuffer(1023), 1024);
				m_StartMenuUnpinCommand.ReleaseBuffer();

			}
			hLib=NULL;

			hLib = ::GetModuleHandle(_T("dwmapi.dll"));
			if(hLib)
			{
				m_lpDwmSetIconicThumbnail_Func = (DwmSetIconicThumbnail_Type)::GetProcAddress(hLib,"DwmSetIconicThumbnail");
				m_lpDwmSetIconicLivePreviewBitmap_Func = (DwmSetIconicLivePreviewBitmap_Type)::GetProcAddress(hLib,"DwmSetIconicLivePreviewBitmap");
				m_lpDwmInvalidateIconicBitmaps_Func = (DwmInvalidateIconicBitmaps_Type)::GetProcAddress(hLib,"DwmInvalidateIconicBitmaps");
				m_lpDwmSetWindowAttribute_Func = (DwmSetWindowAttribute_Type)::GetProcAddress(hLib,"DwmSetWindowAttribute");
				m_lpDwmGetWindowAttribute_Func = (DwmGetWindowAttribute_Type)::GetProcAddress(hLib,"DwmGetWindowAttribute");
				m_lpDwmIsCompositionEnabled_Func = (DwmIsCompositionEnabled_Type)::GetProcAddress(hLib,"DwmIsCompositionEnabled");
				m_lpDwmExtendFrameIntoClientArea_Func = (DwmExtendFrameIntoClientArea_Type)::GetProcAddress(hLib,"DwmExtendFrameIntoClientArea");
			}
			hLib=NULL;

		}

		void CWin7Features::SetOverlayIcon( HWND hWnd, HICON hIcon, LPCWSTR szDiscription )
		{
			ITaskbarList3 *pTaskbarList;
			HRESULT hr = CoCreateInstance(CLSID_TaskbarList, NULL, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&pTaskbarList));
			if (SUCCEEDED(hr))
			{
				hr = pTaskbarList->HrInit();
				if (SUCCEEDED(hr))
				{
					pTaskbarList->SetOverlayIcon(hWnd, hIcon, szDiscription);
				}

				pTaskbarList->Release();
			}

		}

		RECT CWin7Features::GetNotificationIconRect( HWND hWnd, DWORD nID )
		{
			RECT rc = {0};
			NOTIFYICONIDENTIFIER nii = {0};
			nii.cbSize = sizeof(NOTIFYICONIDENTIFIER);
			nii.hWnd = hWnd;
			nii.uID = nID;
			if(m_lpShell_NotifyIconGetRect_Func)
			{
				m_lpShell_NotifyIconGetRect_Func(&nii, &rc);
			}
			return rc;
		}

		void CWin7Features::ChangeWindowMessageFilter( DWORD nMsg, DWORD dwFlag )
		{
			if(m_lpChangeWindowMessageFilter_Func)
			{
				m_lpChangeWindowMessageFilter_Func(nMsg, dwFlag);
			}
		}

		void CWin7Features::AddThumbBarButtons( HWND hWnd, int nCount, THUMBBUTTON *lpButtons )
		{
			ITaskbarList3 *pTaskbarList = NULL;
			HRESULT hr = CoCreateInstance(CLSID_TaskbarList, NULL, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&pTaskbarList));
			if (SUCCEEDED(hr))
			{
				hr=pTaskbarList->ThumbBarAddButtons(hWnd, nCount, (LPTHUMBBUTTON)lpButtons);
				pTaskbarList->Release();
			}
		}

		void CWin7Features::UpdateThumbBarButtons( HWND hWnd, int nCount, THUMBBUTTON *lpButtons )
		{
			ITaskbarList3 *pTaskbarList = NULL;
			HRESULT hr = CoCreateInstance(CLSID_TaskbarList, NULL, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&pTaskbarList));
			if (SUCCEEDED(hr))
			{
				hr=pTaskbarList->ThumbBarUpdateButtons(hWnd, nCount, (LPTHUMBBUTTON)lpButtons);
				pTaskbarList->Release();
			}

		}

		THUMBBUTTON CWin7Features::InitThumbButton( LPCTSTR szTip, UINT nID, HICON icon, THUMBBUTTONFLAGS flags, THUMBBUTTONMASK mask )
		{
			THUMBBUTTON button;
			_tcscpy_s(button.szTip, sizeof(button.szTip)/sizeof(TCHAR), szTip);
			button.iId = nID;
			button.hIcon = icon;
			button.dwMask = mask;
			button.dwFlags = flags;
			return button;
		}

		bool CWin7Features::IsAero()
		{
			return false;
		}

		void CWin7Features::SetMargins( HWND hwnd, MARGINS margins )
		{
			if(!m_lpDwmExtendFrameIntoClientArea_Func)
			{
				return;
			}
			m_lpDwmExtendFrameIntoClientArea_Func(hwnd, &margins);
		}

		CWin7Features* CWin7Features::instance()
		{
			static std::unique_ptr<CWin7Features> inst;
			if(!inst)
			{
				inst = std::make_unique<CWin7Features>();
				inst->Init(true);
			}
			return inst.get();
		}

		bool CWin7Features::win7()
		{
			return osVer.dwMajorVersion >= 6 && osVer.dwMinorVersion >= 1;
		}

		bool CWin7Features::winVista()
		{
			return osVer.dwMajorVersion >= 6;
		}

		void CWin7Features::EnableNCRender(HWND hWnd)
		{
			if(!m_lpDwmSetWindowAttribute_Func)
			{
				return;
			}
			DWMNCRENDERINGPOLICY ncrp = DWMNCRP_ENABLED;
			m_lpDwmSetWindowAttribute_Func(hWnd, DWMWA_NCRENDERING_POLICY, &ncrp, sizeof(ncrp));

		}

		bool CWin7Features::IsNCRender(HWND hWnd)
		{
			if(!m_lpDwmGetWindowAttribute_Func)
			{
				return false;
			}
			if(IsMaximized(hWnd))
			{
				return false;
			}
			BOOL enabled = FALSE;
			m_lpDwmGetWindowAttribute_Func(hWnd, DWMWA_NCRENDERING_ENABLED, &enabled, sizeof(enabled));
			return (IsAero() && enabled);

		}

		void CWin7Features::DisableNCRender( HWND hWnd )
		{
			if(!m_lpDwmSetWindowAttribute_Func)
			{
				return;
			}
			DWMNCRENDERINGPOLICY ncrp = DWMNCRP_DISABLED;
			m_lpDwmSetWindowAttribute_Func(hWnd, DWMWA_NCRENDERING_POLICY, &ncrp, sizeof(ncrp));

		}

		HRESULT CWin7Features::SHCreateItemFromParsingName( PCWSTR name, IBindCtx* context, REFIID id, void** item )
		{
			//return S_FALSE;
			if(!m_lpSHCreateItemFromParsingName_Func)
			{
				return S_FALSE;
			}
			return m_lpSHCreateItemFromParsingName_Func(name, context, id, item);
		}

		CAtlString CWin7Features::GetTaskBarPinCommand()
		{
			return m_TaskBarPinCommand;
		}

		CAtlString CWin7Features::GetStartMenuPinCommand()
		{
			return m_StartMenuPinCommand;
		}

		CAtlString CWin7Features::GetStartMenuUnpinCommand()
		{
			return m_StartMenuUnpinCommand;
		}

		BOOL CWin7Features::CalculatePopupWindowPosition(const POINT *anchorPoint,
											const SIZE *windowSize,
											UINT flags,
											RECT *excludeRect,
											RECT *popupWindowPosition)
		{
			if (!m_lpCalculatePopupWindowPosition_Func)
				return FALSE;

			return m_lpCalculatePopupWindowPosition_Func(anchorPoint, windowSize, flags, excludeRect, popupWindowPosition);
		}

		CAtlString CWin7Features::SHGetKnownFolderPath(REFKNOWNFOLDERID rfid)
		{
			CAtlString sResult;

			if (m_lpSHGetKnownFolderPath)
			{
				LPWSTR lpszPath = nullptr;

				HRESULT hr = m_lpSHGetKnownFolderPath(rfid, 0, 0, &lpszPath);

				if (S_OK == hr)
				{
					sResult = lpszPath;
					CoTaskMemFree(lpszPath);					// we are responsible to free that memory
				}
			}

			return sResult;
		}

	}
}

