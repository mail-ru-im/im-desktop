#include "stdafx.h"
#include "write_registry.h"
#include "../tools.h"
#include "../../../common.shared/version_info_constants.h"
#include "../../../common.shared/config/config.h"
#include <regex>

namespace installer
{
    namespace logic
    {
        installer::error write_to_uninstall_key()
        {
            CAtlString uninstall_command = (LPCWSTR)(QString(ql1c('"') + get_installer_exe().replace(ql1c('/'), ql1c('\\')) % ql1s("\" -uninstall")).utf16());
            CAtlString display_name = (LPCWSTR)(QString(get_product_display_name() % ql1s(" (") % QT_TRANSLATE_NOOP("write_to_uninstall", "version") % ql1c(' ') % QString::fromUtf8(VERSION_INFO_STR) % ql1c(')')).utf16());
            CAtlString exe = (LPCWSTR)get_icq_exe().replace(ql1c('/'), ql1c('\\')).utf16();
            CAtlString install_path = (LPCWSTR)get_install_folder().replace(ql1c('/'), ql1c('\\')).utf16();

            CRegKey key_current_version;
            if (ERROR_SUCCESS != key_current_version.Open(HKEY_CURRENT_USER, L"Software\\Microsoft\\Windows\\CurrentVersion"))
                return installer::error(errorcode::open_registry_key, ql1s("open registry key: Software\\Microsoft\\Windows\\CurrentVersion"));

            CRegKey key_uninstall;
            if (ERROR_SUCCESS != key_uninstall.Create(key_current_version, L"Uninstall"))
                return installer::error(errorcode::create_registry_key, ql1s("create registry key: Software\\Microsoft\\Windows\\CurrentVersion\\Uninstall"));

            CRegKey key_icq;
            if (ERROR_SUCCESS != key_icq.Create(key_uninstall, (LPCWSTR)get_product_name().utf16()))
                return installer::error(errorcode::create_registry_key, ql1s("create registry key: Software\\Microsoft\\Windows\\CurrentVersion\\Uninstall\\") % get_product_name());

            QString version = QString::fromUtf8(VERSION_INFO_STR);
            QString major_version, minor_version;
            const QStringList version_list = version.split(ql1c('.'));
            if (version_list.size() > 2)
            {
                major_version = version_list[0];
                minor_version = version_list[1];
            }

            const auto help_link = config::get().url(config::urls::installer_help);

            const auto link = help_link.empty() ? std::wstring() : L"https://" + QString::fromUtf8(help_link.data(), help_link.size()).toStdWString();

            const auto setValueResults =
            {
                key_icq.SetStringValue(L"DisplayName", display_name),
                key_icq.SetStringValue(L"DisplayIcon", CAtlString(L"\"") + exe + L"\""),
                key_icq.SetStringValue(L"DisplayVersion", (LPCWSTR)version.utf16()),
                key_icq.SetStringValue(L"VersionMajor", (LPCWSTR)major_version.utf16()),
                key_icq.SetStringValue(L"VersionMinor", (LPCWSTR)minor_version.utf16()),
                key_icq.SetStringValue(L"Publisher", (LPCWSTR)get_company_name().utf16()),
                key_icq.SetStringValue(L"InstallLocation", install_path),
                key_icq.SetStringValue(L"UninstallString", uninstall_command),
                key_icq.SetDWORDValue(L"NoModify", (DWORD)1),
                key_icq.SetDWORDValue(L"NoRepair", (DWORD)1),
                (link.empty() ? ERROR_SUCCESS : key_icq.SetStringValue(L"HelpLink", link.c_str())),
                (link.empty() ? ERROR_SUCCESS : key_icq.SetStringValue(L"URLInfoAbout", link.c_str())),
                (link.empty() ? ERROR_SUCCESS : key_icq.SetStringValue(L"URLUpdateInfo", link.c_str())),
            };

            if (std::any_of(setValueResults.begin(), setValueResults.end(), [](const auto _result) { return _result != ERROR_SUCCESS; }))
                return installer::error(errorcode::set_registry_value, ql1s("set registry value: Software\\Microsoft\\Windows\\CurrentVersion\\Uninstall\\") % get_product_name() % ql1s("\\..."));

            return installer::error();
        }

        CAtlString get_default_referer()
        {
            return (const wchar_t*) get_product_name().utf16();
        }

        CAtlString get_referer()
        {
            CAtlString referer = get_default_referer();

            wchar_t buffer[1025] = {0};

            if (::GetModuleBaseName(::GetCurrentProcess(), 0, buffer, 1024))
            {
                CAtlString module_name = buffer;

                std::wcmatch mr;
                std::wregex rx(L"[^a-zA-Z0-9_-]");
                if (std::regex_search((const wchar_t*) module_name, mr, rx))
                {
                    CAtlString omit = mr[0].first;
                    module_name = module_name.Mid(0, module_name.Find(omit));
                }

                CAtlString referer_prefix = L"_rfr";
                int rfr_pos = module_name.Find(referer_prefix);
                if (rfr_pos != -1)
                {
                    referer = module_name.Mid(rfr_pos + referer_prefix.GetLength());
                }
            }

            return referer;
        }

        void write_referer(CRegKey& _key_product)
        {
            CAtlString referer = get_referer();

            _key_product.SetStringValue(L"referer", referer);

            ULONG len = 1024;
            wchar_t buffer[1025];
            if (_key_product.QueryStringValue(L"referer_first", buffer, &len) != ERROR_SUCCESS)
                _key_product.SetStringValue(L"referer_first", referer);
        }

        installer::error write_registry()
        {
            installer::error err;

            const QString exe_path = get_icq_exe().replace(ql1c('/'), ql1c('\\'));
            CRegKey key_software;

            auto error = write_registry_autorun();
            if (!error.is_ok())
            {
                assert(!"failed to write registry autorun");
            }

            if (ERROR_SUCCESS != key_software.Open(HKEY_CURRENT_USER, L"Software"))
                return installer::error(errorcode::open_registry_key, ql1s("open registry key: Software"));

            CRegKey key_product;
            if (ERROR_SUCCESS != key_product.Create(key_software, (const wchar_t*) get_product_name().utf16()))
                return installer::error(errorcode::create_registry_key, ql1s("create registry key: ") % get_product_name());

            if (ERROR_SUCCESS != key_product.SetStringValue(L"path", (const wchar_t*) exe_path.utf16()))
                return installer::error(errorcode::set_registry_value, ql1s("set registry value: Software\\") % get_product_name() % ql1s("path"));

            write_referer(key_product);

            const auto scheme_csv = config::get().string(config::values::register_url_scheme_csv);
            const auto splitted = QString::fromUtf8(scheme_csv.data(), scheme_csv.size()).split(QLatin1Char(','));

            for (int i = 0; i < splitted.size(); i += 2)
                register_url_scheme(splitted.at(i).toStdWString(), splitted.at(i + 1).toStdWString(), exe_path);

            err = write_to_uninstall_key();
            if (!err.is_ok())
                return err;

            return err;
        }

        installer::error write_registry_autorun()
        {
            const QString exe_path = get_icq_exe().replace(ql1c('/'), ql1c('\\'));

            CRegKey key_software_run;
            if (ERROR_SUCCESS != key_software_run.Open(HKEY_CURRENT_USER, L"Software\\Microsoft\\Windows\\CurrentVersion\\Run", KEY_SET_VALUE | KEY_QUERY_VALUE))
                return installer::error(errorcode::open_registry_key, ql1s("open registry key: Software\\Microsoft\\Windows\\CurrentVersion\\Run"));

            bool haveAutorun = false;
            {
                ULONG len = 1024;
                wchar_t buffer[1025];
                if (key_software_run.QueryStringValue((const wchar_t*) get_product_name().utf16(), buffer, &len) == ERROR_SUCCESS)
                    haveAutorun = true;
            }

            if (ERROR_SUCCESS != key_software_run.SetStringValue((const wchar_t*) get_product_name().utf16(), (const wchar_t*)(QString(ql1c('"') % exe_path % ql1s("\" /startup")).utf16())))
                return installer::error(errorcode::set_registry_value, ql1s("set registry value: Software\\Microsoft\\Windows\\CurrentVersion\\Run ") % get_product_name());

            return installer::error();
        }

        installer::error clear_registry()
        {
            CAtlString key_product = CAtlString(L"Software\\") + (const wchar_t*) get_product_name().utf16();

            ::SHDeleteKey(HKEY_CURRENT_USER, key_product);
            ::SHDeleteKey(HKEY_CURRENT_USER, (LPCWSTR)(CAtlString(L"Software\\Microsoft\\Windows\\CurrentVersion\\Uninstall\\") + (const wchar_t*) get_product_name().utf16()));
            ::SHDeleteValue(HKEY_CURRENT_USER, L"Software\\Microsoft\\Windows\\CurrentVersion\\Run", (const wchar_t*) get_product_name().utf16());
            const auto hkey_class = config::get().string(config::values::installer_hkey_class_win);
            ::SHDeleteKey(HKEY_CURRENT_USER, CAtlString(L"Software\\Classes\\") + QString::fromUtf8(hkey_class.data(), hkey_class.size()).toStdWString().c_str());

            return installer::error();
        }

        installer::error write_update_version()
        {
            CRegKey key_software;

            if (ERROR_SUCCESS != key_software.Open(HKEY_CURRENT_USER, L"Software"))
                return installer::error(errorcode::open_registry_key, ql1s("open registry key: Software"));

            CRegKey key_product;
            if (ERROR_SUCCESS != key_product.Create(key_software, (const wchar_t*) get_product_name().utf16()))
                return installer::error(errorcode::create_registry_key, ql1s("create registry key: ") % get_product_name());

            if (ERROR_SUCCESS != key_product.SetStringValue(L"update_version", (const wchar_t*) QString::fromUtf8(VERSION_INFO_STR).utf16()))
                return installer::error(errorcode::set_registry_value, ql1s("set registry value: Software\\icq.desktop update_version"));

            return installer::error();
        }

        void register_url_scheme(const std::wstring& _scheme, const std::wstring& _name, const QString& _exe_path)
        {
            HKEY key_cmd, key_icq = 0, key_icon = 0;
            DWORD disp = 0;


            CAtlString sub_path = L"Software\\Classes\\";
            CAtlString url_link_string = L"URL:";

            sub_path += _scheme.c_str();
            url_link_string += _name.c_str();

            CAtlString command_string = CAtlString("\"") + (const wchar_t*) _exe_path.utf16() + L"\" -urlcommand \"%1\"";

            if (ERROR_SUCCESS == ::RegCreateKeyEx(HKEY_CURRENT_USER, (LPCWSTR) (sub_path + L"\\shell\\open\\command"), 0, L"", REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL, &key_cmd, &disp))
            {
                ::RegSetValueEx(key_cmd, NULL, 0, REG_SZ, (LPBYTE) command_string.GetBuffer(), sizeof(TCHAR)*(command_string.GetLength() + 1));

                if (ERROR_SUCCESS == ::RegOpenKeyEx(HKEY_CURRENT_USER, (LPCWSTR) sub_path, REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, &key_icq))
                {
                    ::RegSetValueEx(key_icq, NULL, 0, REG_SZ, (LPBYTE) url_link_string.GetBuffer(), sizeof(TCHAR)*(url_link_string.GetLength() + 1));
                    ::RegSetValueEx(key_icq, L"URL Protocol", 0, REG_SZ, (LPBYTE) L"", sizeof(TCHAR));


                    if (ERROR_SUCCESS == ::RegCreateKeyEx(key_icq, L"DefaultIcon", 0, L"", REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL, &key_icon, &disp))
                    {
                        CAtlString icon_path = (const wchar_t*) _exe_path.utf16();

                        ::RegSetValueEx(key_icon, NULL, 0, REG_SZ, (LPBYTE) icon_path.GetBuffer(), sizeof(TCHAR)*(icon_path.GetLength() + 1));

                        ::RegCloseKey(key_icon);
                    }

                    ::RegCloseKey(key_icq);
                }

                ::RegCloseKey(key_cmd);
            }
        }
    }
}