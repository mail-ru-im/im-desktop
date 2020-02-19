#include "stdafx.h"
#include "shell_link.h"
#include "Win7Features.h"

#include <filesystem>

namespace installer
{
	namespace links
	{
		void RemoveLink(LPCTSTR lpszPathLink)
		{
			assert(lpszPathLink);

			UnPin(lpszPathLink);
			::DeleteFile(lpszPathLink);
		}

		HRESULT CreateLink(LPCTSTR lpszPathObj, LPCTSTR arg, LPCTSTR lpszPathLink, LPCTSTR lpszPathDesc, int icon)
		{
			CComPtr<IShellLink> link;
			link.Attach(CreateShellLink(lpszPathObj, arg, lpszPathDesc, icon));
			if(link == NULL)
			{
				return S_FALSE;
			}
			SaveShellLink(link, lpszPathLink);
			return S_OK;
		}

		IShellLink* CreateShellLink( LPCTSTR lpszPathObj, LPCTSTR arg, LPCTSTR lpszPathDesc, int icon )
		{
			CComPtr<IShellLink> link;
			HRESULT hres = link.CoCreateInstance(CLSID_ShellLink, NULL, CLSCTX_INPROC_SERVER);
			if( !SUCCEEDED(hres))
			{
				return NULL;
			}
			link->SetPath(lpszPathObj);
			if (arg)
			{
				link->SetArguments(arg);
			}
			if (icon != -1)
			{
				link->SetIconLocation(lpszPathObj, icon);
			}
			link->SetDescription(lpszPathDesc);
			return link.Detach();
		}

		void SetShellLinkTitle( IShellLink* psl, LPCTSTR szTitle )
		{
			SetShellLinkProperty(psl, PKEY_Title, szTitle);
		}

		void SetShellLinkAppID( IShellLink* psl, LPCTSTR szID )
		{
			SetShellLinkProperty(psl, PKEY_AppUserModel_ID, szID);
		}

		void SaveShellLink( IShellLink* psl, LPCTSTR path )
		{
			CComQIPtr<IPersistFile, &IID_IPersistFile> ppf = psl;
			if(ppf != NULL)
			{
				ppf->Save(CAtlString(path), TRUE);
			}

		}

		void PinToTaskbar(const CAtlString& _link)
		{
			if (!CWin7Features::instance()->win7())
				return;

			ExecuteVerb(_link, CWin7Features::instance()->GetTaskBarPinCommand());
		}

		void PinToStartmenu(const CAtlString& _link)
		{
			if (!CWin7Features::instance()->win7())
				return;

			ExecuteVerb(_link, CWin7Features::instance()->GetStartMenuPinCommand());
		}

		void UnPinXP(const CAtlString& _link)
		{
			ExecuteVerb(_link, CWin7Features::instance()->GetStartMenuUnpinCommand());
		}

		void ExecuteVerb(const CAtlString& _filePath, const CAtlString& _command )
		{
			if(_command.IsEmpty())
			{
				return;
			}
			CComPtr<FolderItemVerbs> verbs;
			verbs.Attach(GetVerbs(_filePath));
			if(verbs == NULL)
			{
				return;
			}
			long count = 0;
			HRESULT hr = verbs->get_Count(&count);
			if (FAILED(hr))
			{
				return;
			}

			CComVariant index;
			for (int i = 0; i < count; ++i)
			{
				index = i;
				CComPtr<FolderItemVerb> verb;
				hr = verbs->Item(index, &verb);
				if (FAILED(hr) || verb == NULL)
				{
					continue;
				}

				BSTR name = NULL;
				hr = verb->get_Name(&name);
				if (FAILED(hr))
				{
					continue;
				}

				if ( _command == name )
				{
					hr = verb->DoIt();
					return;
				}
			}
		}

		FolderItemVerbs* GetVerbs(const CAtlString& _filePath)
		{
            std::filesystem::path file_path = _filePath.GetString();

			CComPtr<IShellDispatch> shell;
			HRESULT hr = shell.CoCreateInstance(CLSID_Shell, NULL,CLSCTX_INPROC_SERVER);
			if(shell == NULL || FAILED(hr))
			{
				return NULL;
			}

			CAtlString path_string = file_path.parent_path().wstring().c_str();
			path_string.Replace('/', '\\');

			CComVariant path(CComBSTR((LPCWSTR) path_string));
			CComPtr<Folder> folder;
			hr = shell->NameSpace(path, &folder);
			if( folder == NULL || FAILED(hr))
			{
				return NULL;
			}
			CComPtr<FolderItem> file;
			CComBSTR full_filename((LPCWSTR)_filePath);
			hr = folder->ParseName(full_filename, &file);
			if( file == NULL || FAILED(hr) )
			{
				return NULL;
			}

			CComPtr<FolderItemVerbs> verbs;
			hr = file->Verbs(&verbs);
			if(verbs == NULL || FAILED(hr))
			{
				return NULL;
			}

			return verbs.Detach();
		}

		void UnPin(const CAtlString& _link)
		{
			if (CWin7Features::instance()->winVista())
			{
				CComPtr<IShellItem> item;
				HRESULT hr = CWin7Features::instance()->SHCreateItemFromParsingName(_link, NULL, IID_PPV_ARGS(&item));
				//HRESULT hr = ::SHCreateItemFromParsingName(link, NULL, IID_PPV_ARGS(&item));
				if (SUCCEEDED(hr) && item!=NULL)
				{
					CComPtr<IStartMenuPinnedList> list;
					hr = list.CoCreateInstance(CLSID_StartMenuPin, NULL, CLSCTX_INPROC_SERVER);
					if (SUCCEEDED(hr))
					{
						list->RemoveFromList(item);
					}
				}
			}
			else //for XP.
			{
				UnPinXP(_link);
			}
		}

		void SetShellLinkProperty( IShellLink* psl, PROPERTYKEY key, LPCTSTR szProperty )
		{
			CComQIPtr<IPropertyStore, &IID_IPropertyStore> prop = psl;
			if( prop == NULL )
			{
				return;
			}
			PROPVARIANT propvar;
			propvar.vt = VT_LPWSTR;
			HRESULT hr = SHStrDupW(szProperty, &propvar.pwszVal);
			if (FAILED(hr))
			{
				memset ( &propvar, 0, sizeof(PROPVARIANT) );
			}

			if (SUCCEEDED(hr))
			{
				hr = prop->SetValue(key, propvar);
				if (SUCCEEDED(hr))
				{
					hr = prop->Commit();
				}
				PropVariantClear(&propvar);
			}

		}

		BOOL ResetAllUserLinks(CAtlString dir, CAtlString linkOldGoal, CAtlString linkNewGoal, bool bIsUninstall, bool bRecurse, bool bRemoveFromMFU/* = false*/, CAtlString sArgs/* = _T("")*/, TShortcutErrorList & ErrorList)
		{
			CComPtr<IShellLink> spLink;
			HRESULT hr = spLink.CoCreateInstance(CLSID_ShellLink, NULL, CLSCTX_INPROC_SERVER);
			if( !SUCCEEDED(hr))
			{
				return FALSE;
			}

			CComQIPtr<IPersistFile, &IID_IPersistFile> spLinkFile = spLink;
			if(!spLinkFile)
			{
				return FALSE;
			}

			int recurseCount = 0;

			return ResetAllUserLinksRecurse(dir, linkOldGoal, linkNewGoal, spLink, spLinkFile, bIsUninstall, &recurseCount, bRecurse, bRemoveFromMFU, sArgs, ErrorList);
		}

		BOOL ResetAllUserLinksRecurse(CAtlString dir, CAtlString linkOldGoal, CAtlString linkNewGoal, IShellLink *spLink, IPersistFile *spLinkFile, bool bIsUninstall, int *recurseCount, bool bRecurse, bool bRemoveFromMFU, CAtlString sArgs/* = _T("")*/, TShortcutErrorList & ErrorList)
		{
			/// body of function ----------------------------------------------

			if ( (*recurseCount) > 10) // max find recursion deep is 10
			{
				return TRUE;
			}

			HRESULT hr;
			CAtlString currentFile;
			CAtlString searchPath = dir + _T("\\*.*");
			WIN32_FIND_DATA findFileData;
			WIN32_FIND_DATA findFileDataLink;

			const int bufSize = 4096;
			TCHAR *pBuf = new(std::nothrow) TCHAR[bufSize];
			if (!pBuf)
				return FALSE;

			HANDLE hFind = FindFirstFile(searchPath, &findFileData);
			if (hFind == INVALID_HANDLE_VALUE)
			{
				delete[] pBuf;
				return FALSE;
			}

			do
			{
				if (findFileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
				{
					if (bRecurse)
					{
						// recursive find
						if ((_tcsncmp(findFileData.cFileName, _T("."), 1) == 0) || (_tcsncmp(findFileData.cFileName, _T(".."), 2) == 0))
							continue;

						dir += _T("\\");
						dir += findFileData.cFileName;
						(*recurseCount) += 1;
						ResetAllUserLinksRecurse(dir, linkOldGoal, linkNewGoal, spLink, spLinkFile, bIsUninstall, recurseCount, bRecurse, bRemoveFromMFU, sArgs, ErrorList);
						(*recurseCount) -= 1;
						dir = dir.Left( dir.ReverseFind(_T('\\')));
					}
				}
				else
				{
					TCHAR *pTemp = _tcsrchr(findFileData.cFileName, _T('.'));
					if (!pTemp)
						continue;

					if (_tcscmp(_T(".lnk"), pTemp) != 0)
						continue;

					currentFile = dir + _T("\\") + findFileData.cFileName;
					hr = spLinkFile->Load(currentFile, 0);
					if(hr == S_OK)
					{
						hr = spLink->GetPath(pBuf, bufSize-1, &findFileDataLink, SLGP_UNCPRIORITY);
						if (hr == S_OK)
						{
							pTemp = _tcsrchr(pBuf, _T('\\')) + 1;
							if (pTemp)
							{
								if (_tcscmp(linkOldGoal, pTemp) == 0)
								{
									if (bRemoveFromMFU)
									{
										if (!RemoveFromMFUList(findFileData.cFileName))
										{

										}
									}
									else
									{
										if (bIsUninstall)
										{	// unpin + delete
											UnPin(currentFile);
											if (! ::DeleteFile(currentFile))
											{

											}
										}
										else
										{
											hr = spLink->SetPath(linkNewGoal);
											if (FAILED(hr))
											{
											}
											hr = spLink->SetIconLocation(linkNewGoal, 0);//TODO ? arg, description
											if (FAILED(hr))
											{
											}
											if (!sArgs.IsEmpty())
											{
												hr = spLink->SetArguments((LPCTSTR)sArgs);
												if (FAILED(hr))
												{
													assert(FALSE);
												}
											}
											hr = spLinkFile->Save(0, TRUE);
											if (FAILED(hr))
											{
											}

										}
									}
								}
							}
						}
					}
				}
			}
			while (FindNextFile(hFind, &findFileData) != 0);

			delete[] pBuf;

			DWORD dwError = GetLastError();
			FindClose(hFind);
			if (dwError != ERROR_NO_MORE_FILES)
			{
				return FALSE;
			}

			return TRUE;
		}

		void CodeOn13(std::wstring &s)//ROT13
		{
			TCHAR c = 0;

			for (unsigned int i = 0; i < s.size(); i++)
			{
				c = s[i];
				if (c >= _T('a') && c <= _T('z') )
				{
					c += 13;
					if (c>_T('z'))
						c-= 26;

					s[i] = c;
				}

				if (c >= _T('A') && c <= _T('Z') )
				{
					c += 13;
					if (c>_T('Z'))
						c-= 26;

					s[i] = c;
				}
			}
		}


		BOOL RemoveFromMFUList(CAtlString exeName)
		{
			std::wstring strDelName = (LPCWSTR) exeName;

			CodeOn13(strDelName);

			//find key_values where is strDelName. And save they in vecForDel
			std::vector<std::shared_ptr<PairKeyValue>> vecForDel;

			HKEY hkeyUserAssist = NULL;
			CAtlString strSubKey = _T("Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\UserAssist\\");
			HRESULT hRes = RegOpenKeyEx(HKEY_CURRENT_USER, strSubKey, 0, KEY_ALL_ACCESS, &hkeyUserAssist);
			if (hRes==ERROR_SUCCESS && hkeyUserAssist)
			{
				DWORD sizeUserAssist = 0;
				hRes = RegQueryInfoKey(hkeyUserAssist,NULL,NULL,NULL,&sizeUserAssist,NULL,NULL,NULL,NULL,NULL,NULL,NULL);
				if (ERROR_SUCCESS != hRes)
					return FALSE;

				//for all subKeys in UserAssist_key
				for(DWORD i=0; i<sizeUserAssist; ++i)
				{
					TCHAR keyName[MAX_PATH*2];
					keyName[0] = _T('\0');
					DWORD dwNameSize = MAX_PATH*2;
					hRes = RegEnumKeyEx(hkeyUserAssist, i, keyName, &dwNameSize, NULL, NULL, NULL, NULL);
					if (hRes != ERROR_SUCCESS)
						return FALSE;

					CAtlString keyForEnum = strSubKey + keyName;

					HKEY hkeyBase = NULL;
					hRes = RegOpenKeyEx(HKEY_CURRENT_USER, keyForEnum, 0, KEY_ALL_ACCESS, &hkeyBase);
					if (ERROR_SUCCESS==hRes && hkeyBase)
					{
						HKEY hkeyCount = NULL;
						hRes = RegOpenKeyEx(hkeyBase, _T("count"), 0, KEY_ALL_ACCESS, &hkeyCount);
						if(ERROR_SUCCESS==hRes)
						{
							DWORD    valueCount = 0;

							//
							hRes = RegQueryInfoKey(hkeyCount,NULL,NULL,NULL,NULL,NULL,NULL,&valueCount,NULL,NULL,NULL,NULL);
							if(hRes == ERROR_SUCCESS)
							{
								if(valueCount)
								{
									TCHAR strValue[MAX_PATH*2] = {0};
									DWORD sizeValue = MAX_PATH*2;
									// For each value in key
									for(DWORD j = 0; j < valueCount; ++j)
									{
										sizeValue = MAX_PATH*2;
										strValue[0] = _T('\0');
										hRes = RegEnumValue(hkeyCount, j, strValue, &sizeValue, NULL, NULL, NULL, NULL);
										if (hRes == ERROR_SUCCESS )
										{
											if (lstrlen(strValue))
											{
												TCHAR *p = _tcsrchr(strValue, _T('\\'));
												if (p)
												{
													++p;// set after '\\'
													if (strDelName == p)
													{
														PairKeyValue *lpPairKeyValue = new(std::nothrow) PairKeyValue;
														if (!lpPairKeyValue)
															continue;

														lpPairKeyValue->strKey = keyForEnum + _T("\\count");
														lpPairKeyValue->strValue = strValue;
														vecForDel.push_back(std::shared_ptr<PairKeyValue>(lpPairKeyValue));
													}
												}
											}
										}
									}
								}
							}
							RegCloseKey(hkeyCount);
						}
						RegCloseKey(hkeyBase);
					}
				}
				RegCloseKey(hkeyUserAssist);
			}


			HKEY hKeyForDel;
			for (const auto& x : vecForDel)
			{
				hRes = RegOpenKeyEx(HKEY_CURRENT_USER, x->strKey, 0, KEY_ALL_ACCESS, &hKeyForDel);
				if (hRes != ERROR_SUCCESS)
					continue;

				RegDeleteValue(hKeyForDel, x->strValue);

				RegCloseKey(hKeyForDel);
			}
			return TRUE;
		}

		CAtlString GetQuickLaunchDir()
		{
			CAtlString sDir;
			if (::SHGetSpecialFolderPath(NULL, sDir.GetBuffer(4096), CSIDL_APPDATA, 1))
			{
				sDir.ReleaseBuffer();
				sDir = sDir.Trim(_T("\\"));
				sDir += _T("\\Microsoft\\Internet Explorer\\Quick Launch");

                std::filesystem::path path = ((const wchar_t*)sDir);
                std::error_code ec;
                if (!std::filesystem::exists(path, ec))
				{
                    if (!std::filesystem::create_directories(path, ec))
					{

					}
				}

				return sDir;
			}

			return _T("");
		}

	}
}

