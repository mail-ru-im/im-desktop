#pragma once

namespace installer
{
    namespace logic
    {
        installer::error write_registry();
        installer::error write_registry_autorun();
        installer::error clear_registry();
        installer::error write_update_version();
        installer::error write_to_uninstall_key();

        void register_url_scheme(const std::wstring& _scheme, const std::wstring& _name, const QString& _exe_path);
    }
}
