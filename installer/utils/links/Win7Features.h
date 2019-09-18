#pragma once

#include "JumpListItem.h"

namespace installer
{
	namespace links
	{
		class CWin7Features
		{
		public:

			struct NOTIFYICONIDENTIFIER {
				DWORD cbSize;
				HWND hWnd;
				UINT uID;
				GUID guidItem;
			};

			typedef BOOL (__stdcall* ChangeWindowMessageFilter_Type)(UINT,DWORD);
			typedef HRESULT  (__stdcall * SetCurrentProcessExplicitAppUserModelID_Type)(PCWSTR);
			typedef HRESULT  (__stdcall * Shell_NotifyIconGetRect_Type)(const NOTIFYICONIDENTIFIER*,RECT*);
			typedef HRESULT (__stdcall *SHCreateItemFromParsingName_Type)(PCWSTR, IBindCtx*, REFIID, void** );
			typedef HRESULT (__stdcall *CalculatePopupWindowPosition_Type)(const POINT*, const SIZE*, UINT, RECT*, RECT*);


			typedef HRESULT (__stdcall* DwmSetIconicThumbnail_Type)( HWND, HBITMAP, DWORD);
			typedef HRESULT (__stdcall* DwmSetIconicLivePreviewBitmap_Type)(HWND,HBITMAP,POINT*,DWORD);
			typedef HRESULT (__stdcall* DwmInvalidateIconicBitmaps_Type)(HWND);
			typedef HRESULT (__stdcall* DwmSetWindowAttribute_Type)(HWND, DWORD, LPCVOID, DWORD );
			typedef HRESULT (__stdcall* DwmGetWindowAttribute_Type)(HWND, DWORD, LPCVOID, DWORD );
			typedef HRESULT (__stdcall* DwmIsCompositionEnabled_Type)( BOOL* );
			typedef HRESULT (__stdcall* DwmExtendFrameIntoClientArea_Type)(HWND hWnd, const MARGINS *pMarInset );

			// REFKNOWNFOLDERID type is &GUID
			typedef HRESULT (__stdcall* SHGetKnownFolderPath_Type)(_In_ REFKNOWNFOLDERID rfid, _In_  DWORD dwFlags, _In_opt_  HANDLE hToken, _Out_  PWSTR *ppszPath);

			typedef std::list<JumpListItem> JumpList;

		public:
			CWin7Features(void);
			virtual ~CWin7Features(void);

			static CWin7Features* instance();
			void Init(bool bWin7);
			void SetAppUserModelID(LPWSTR szID);
			DWORD GetTaskbarButtonMessage();
			void ChangeWindowMessageFilter( DWORD nMsg, DWORD dwFlag );

			void CreateJumpList(JumpList &items);
			void InitJumpList( IObjectCollection * poc, JumpList &items );
			void RemoveJumpList();
			IShellLink *CreateSeparator();

			void AddThumbBarButtons(HWND hWnd, int nCount, THUMBBUTTON *lpButtons);
			void UpdateThumbBarButtons(HWND hWnd, int nCount, THUMBBUTTON *lpButtons);
			THUMBBUTTON InitThumbButton(
				LPCTSTR szTip,
				UINT nID,
				HICON icon,
				THUMBBUTTONFLAGS flags = THBF_DISMISSONCLICK,
				THUMBBUTTONMASK mask = THB_ICON | THB_TOOLTIP | THB_FLAGS
				);

			void RegisterTab(HWND hTab, HWND hMain);
			void UnregisterTab(HWND hTab);
			void ActivateTab(HWND hTab);
			void SetIconThumbnail(HWND hWnd, HBITMAP hBmp);
			void SetPreview(HWND hWnd, HBITMAP hBmp, POINT pnt);
			void SetOverlayIcon(HWND hWnd, HICON hIcon, LPCWSTR szDiscription);
			void InvalidateThumbnails(HWND hWnd);
			void EnableCustomPreviews(HWND hWnd, BOOL bEnable);
			void EnableNCRender(HWND hWnd);
			void DisableNCRender(HWND hWnd);
			bool IsNCRender(HWND hWnd);
			bool IsAero();
			bool winVista();
			bool win7();
			void SetMargins(HWND hwnd, MARGINS margins);
			CAtlString SHGetKnownFolderPath(REFKNOWNFOLDERID rfid);

			RECT GetNotificationIconRect(HWND hWnd, DWORD nID);
			HRESULT SHCreateItemFromParsingName(PCWSTR name, IBindCtx* context, REFIID id, void** item);
			BOOL CalculatePopupWindowPosition(const POINT *anchorPoint, const SIZE *windowSize, UINT flags,
				RECT *excludeRect, RECT *popupWindowPosition);


			CAtlString GetTaskBarPinCommand();
			CAtlString GetStartMenuPinCommand();
			CAtlString GetStartMenuUnpinCommand();

		protected:
			CAtlString m_TaskBarPinCommand;
			CAtlString m_StartMenuPinCommand;
			CAtlString m_StartMenuUnpinCommand;
			OSVERSIONINFO osVer;
			ChangeWindowMessageFilter_Type m_lpChangeWindowMessageFilter_Func;
			SetCurrentProcessExplicitAppUserModelID_Type m_lpSetCurrentProcessExplicitAppUserModelID_Func;
			Shell_NotifyIconGetRect_Type m_lpShell_NotifyIconGetRect_Func;
			SHGetKnownFolderPath_Type m_lpSHGetKnownFolderPath;
			SHCreateItemFromParsingName_Type  m_lpSHCreateItemFromParsingName_Func;
			DwmSetIconicThumbnail_Type m_lpDwmSetIconicThumbnail_Func;
			DwmSetIconicLivePreviewBitmap_Type m_lpDwmSetIconicLivePreviewBitmap_Func;
			DwmInvalidateIconicBitmaps_Type m_lpDwmInvalidateIconicBitmaps_Func;
			DwmSetWindowAttribute_Type m_lpDwmSetWindowAttribute_Func;
			DwmGetWindowAttribute_Type m_lpDwmGetWindowAttribute_Func;
			DwmIsCompositionEnabled_Type m_lpDwmIsCompositionEnabled_Func;
			DwmExtendFrameIntoClientArea_Type m_lpDwmExtendFrameIntoClientArea_Func;
			CalculatePopupWindowPosition_Type m_lpCalculatePopupWindowPosition_Func;

			//	_Type m_lp_Func;

		};

#define WM_DWMSENDICONICTHUMBNAIL           0x0323
#define WM_DWMSENDICONICLIVEPREVIEWBITMAP   0x0326

#define MSGFLT_ADD 1
#define MSGFLT_REMOVE 2

		DEFINE_PROPERTYKEY(PKEY_Title, 0xF29F85E0, 0x4FF9, 0x1068, 0xAB, 0x91, 0x08, 0x00, 0x2B, 0x27, 0xB3, 0xD9, 2);

		//////////////////////////////////////////////////////////////////////////////////
		// #define MSGFLT_ADD 1
		// #define MSGFLT_REMOVE 2
		// WINUSERAPI BOOL WINAPI ChangeWindowMessageFilter(__in UINT message, __in DWORD dwFlag);


		extern RPC_IF_HANDLE __MIDL_itf_shobjidl_0000_0188_v0_0_c_ifspec;
		extern RPC_IF_HANDLE __MIDL_itf_shobjidl_0000_0188_v0_0_s_ifspec;

#ifndef __ICustomDestinationList_INTERFACE_DEFINED__
#define __ICustomDestinationList_INTERFACE_DEFINED__

		/* interface ICustomDestinationList */
		/* [unique][object][uuid] */ 

		typedef /* [v1_enum] */ 
			enum KNOWNDESTCATEGORY
		{	KDC_FREQUENT	= 1,
		KDC_RECENT	= ( KDC_FREQUENT + 1 ) 
		} 	KNOWNDESTCATEGORY;


		EXTERN_C const IID IID_ICustomDestinationList;

#if defined(__cplusplus) && !defined(CINTERFACE)

		MIDL_INTERFACE("6332debf-87b5-4670-90c0-5e57b408a49e")
ICustomDestinationList : public IUnknown
		{
		public:
			virtual HRESULT STDMETHODCALLTYPE SetAppID( 
				/* [string][in] */ __RPC__in_string LPCWSTR pszAppID) = 0;

			virtual HRESULT STDMETHODCALLTYPE BeginList( 
				/* [out] */ __RPC__out UINT *pcMinSlots,
				/* [in] */ __RPC__in REFIID riid,
				/* [iid_is][out] */ __RPC__deref_out_opt void **ppv) = 0;

			virtual HRESULT STDMETHODCALLTYPE AppendCategory( 
				/* [string][in] */ __RPC__in_string LPCWSTR pszCategory,
				/* [in] */ __RPC__in_opt IObjectArray *poa) = 0;

			virtual HRESULT STDMETHODCALLTYPE AppendKnownCategory( 
				/* [in] */ KNOWNDESTCATEGORY category) = 0;

			virtual HRESULT STDMETHODCALLTYPE AddUserTasks( 
				/* [in] */ __RPC__in_opt IObjectArray *poa) = 0;

			virtual HRESULT STDMETHODCALLTYPE CommitList( void) = 0;

			virtual HRESULT STDMETHODCALLTYPE GetRemovedDestinations( 
				/* [in] */ __RPC__in REFIID riid,
				/* [iid_is][out] */ __RPC__deref_out_opt void **ppv) = 0;

			virtual HRESULT STDMETHODCALLTYPE DeleteList( 
				/* [string][unique][in] */ __RPC__in_opt_string LPCWSTR pszAppID) = 0;

			virtual HRESULT STDMETHODCALLTYPE AbortList( void) = 0;

		};

#else 	/* C style interface */

		typedef struct ICustomDestinationListVtbl
		{
			BEGIN_INTERFACE

				HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
				__RPC__in ICustomDestinationList * This,
				/* [in] */ __RPC__in REFIID riid,
				/* [annotation][iid_is][out] */ 
				__RPC__deref_out  void **ppvObject);

				ULONG ( STDMETHODCALLTYPE *AddRef )( 
					__RPC__in ICustomDestinationList * This);

				ULONG ( STDMETHODCALLTYPE *Release )( 
					__RPC__in ICustomDestinationList * This);

				HRESULT ( STDMETHODCALLTYPE *SetAppID )( 
					__RPC__in ICustomDestinationList * This,
					/* [string][in] */ __RPC__in_string LPCWSTR pszAppID);

				HRESULT ( STDMETHODCALLTYPE *BeginList )( 
					__RPC__in ICustomDestinationList * This,
					/* [out] */ __RPC__out UINT *pcMinSlots,
					/* [in] */ __RPC__in REFIID riid,
					/* [iid_is][out] */ __RPC__deref_out_opt void **ppv);

				HRESULT ( STDMETHODCALLTYPE *AppendCategory )( 
					__RPC__in ICustomDestinationList * This,
					/* [string][in] */ __RPC__in_string LPCWSTR pszCategory,
					/* [in] */ __RPC__in_opt IObjectArray *poa);

				HRESULT ( STDMETHODCALLTYPE *AppendKnownCategory )( 
					__RPC__in ICustomDestinationList * This,
					/* [in] */ KNOWNDESTCATEGORY category);

				HRESULT ( STDMETHODCALLTYPE *AddUserTasks )( 
					__RPC__in ICustomDestinationList * This,
					/* [in] */ __RPC__in_opt IObjectArray *poa);

				HRESULT ( STDMETHODCALLTYPE *CommitList )( 
					__RPC__in ICustomDestinationList * This);

				HRESULT ( STDMETHODCALLTYPE *GetRemovedDestinations )( 
					__RPC__in ICustomDestinationList * This,
					/* [in] */ __RPC__in REFIID riid,
					/* [iid_is][out] */ __RPC__deref_out_opt void **ppv);

				HRESULT ( STDMETHODCALLTYPE *DeleteList )( 
					__RPC__in ICustomDestinationList * This,
					/* [string][unique][in] */ __RPC__in_opt_string LPCWSTR pszAppID);

				HRESULT ( STDMETHODCALLTYPE *AbortList )( 
					__RPC__in ICustomDestinationList * This);

			END_INTERFACE
		} ICustomDestinationListVtbl;

		interface ICustomDestinationList
		{
			CONST_VTBL struct ICustomDestinationListVtbl *lpVtbl;
		};



#ifdef COBJMACROS


#define ICustomDestinationList_QueryInterface(This,riid,ppvObject)	\
	( (This)->lpVtbl -> QueryInterface(This,riid,ppvObject) ) 

#define ICustomDestinationList_AddRef(This)	\
	( (This)->lpVtbl -> AddRef(This) ) 

#define ICustomDestinationList_Release(This)	\
	( (This)->lpVtbl -> Release(This) ) 


#define ICustomDestinationList_SetAppID(This,pszAppID)	\
	( (This)->lpVtbl -> SetAppID(This,pszAppID) ) 

#define ICustomDestinationList_BeginList(This,pcMinSlots,riid,ppv)	\
	( (This)->lpVtbl -> BeginList(This,pcMinSlots,riid,ppv) ) 

#define ICustomDestinationList_AppendCategory(This,pszCategory,poa)	\
	( (This)->lpVtbl -> AppendCategory(This,pszCategory,poa) ) 

#define ICustomDestinationList_AppendKnownCategory(This,category)	\
	( (This)->lpVtbl -> AppendKnownCategory(This,category) ) 

#define ICustomDestinationList_AddUserTasks(This,poa)	\
	( (This)->lpVtbl -> AddUserTasks(This,poa) ) 

#define ICustomDestinationList_CommitList(This)	\
	( (This)->lpVtbl -> CommitList(This) ) 

#define ICustomDestinationList_GetRemovedDestinations(This,riid,ppv)	\
	( (This)->lpVtbl -> GetRemovedDestinations(This,riid,ppv) ) 

#define ICustomDestinationList_DeleteList(This,pszAppID)	\
	( (This)->lpVtbl -> DeleteList(This,pszAppID) ) 

#define ICustomDestinationList_AbortList(This)	\
	( (This)->lpVtbl -> AbortList(This) ) 

#endif /* COBJMACROS */


#endif 	/* C style interface */




#endif 	/* __ICustomDestinationList_INTERFACE_DEFINED__ */

	}
}

