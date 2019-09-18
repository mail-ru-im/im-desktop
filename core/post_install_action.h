#pragma once

namespace core
{
    namespace installer_services
    {
        class post_install_action
        {
            std::wstring act_file_name_;

            std::set<std::wstring> tmp_dirs_;

        public:
            post_install_action(std::wstring _file_name);

            bool load();

            void delete_tmp_resources();
        };
    }
}
