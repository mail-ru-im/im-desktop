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


        enum class scheme_app_type
        {
        	icq,
        	agent,
        	dit,
            biz
        };

        void register_url_scheme(scheme_app_type _app_type, const QString& _exe_path);
    }
}
