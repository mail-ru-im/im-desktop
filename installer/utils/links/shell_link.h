#pragma once

namespace installer
{
	namespace links
	{
		struct PairKeyValue
		{
			CAtlString strKey;
			CAtlString strValue;
		};


		void RemoveLink(LPCTSTR lpszPathLink);
		HRESULT CreateLink(LPCTSTR lpszPathObj, LPCTSTR arg, LPCTSTR lpszPathLink, LPCTSTR lpszPathDesc, int icon);
		IShellLink* CreateShellLink(LPCTSTR lpszPathObj, LPCTSTR arg, LPCTSTR lpszPathDesc, int icon);
		void SetShellLinkProperty(IShellLink* psl, PROPERTYKEY key, LPCTSTR szProperty);
		void SetShellLinkTitle(IShellLink* psl, LPCTSTR szTitle);
		void SetShellLinkAppID(IShellLink* psl, LPCTSTR szID);
		void SaveShellLink(IShellLink* psl, LPCTSTR path);

		void PinToTaskbar(const CAtlString& _link);
		void PinToStartmenu(const CAtlString& _link);
		void UnPinXP(const CAtlString& _link);
		void UnPin(const CAtlString& _link);
		void ExecuteVerb(const CAtlString& _filePath, const CAtlString& _command);
		FolderItemVerbs* GetVerbs(const CAtlString& _filePath);

		typedef std::list<std::pair<int, CAtlString>> TShortcutErrorList;
		BOOL ResetAllUserLinks(
			CAtlString dir, 
			CAtlString linkOldGoal, 
			CAtlString linkNewGoal, 
			bool bIsUninstall, 
			bool bRecurse, 
			bool bRemoveFromMFU = false, 
			CAtlString sArgs = _T(""), 
			TShortcutErrorList & ErrorList = TShortcutErrorList());

		BOOL ResetAllUserLinksRecurse(
			CAtlString dir, 
			CAtlString linkOldGoal, 
			CAtlString linkNewGoal, 
			IShellLink *spLink, 
			IPersistFile *spLinkFile, 
			bool bIsUninstall, 
			int *recurseCount, 
			bool bRecurse, 
			bool bRemoveFromMFU, 
			CAtlString sArgs = _T(""), 
			TShortcutErrorList & ErrorList = TShortcutErrorList());

		BOOL RemoveFromMFUList(CAtlString exeName);
		CAtlString GetQuickLaunchDir();
	}
}
