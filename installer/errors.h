#pragma once

namespace installer
{
    enum errorcode
    {
        ok = 0,
        open_files_archive = 1,
        create_product_folder = 2,
        open_file_for_write = 3,
        write_file = 4,
        start_exe = 5,
        open_registry_key = 7,
        set_registry_value = 8,
        get_registry_value = 9,
        create_registry_key = 10,
        copy_installer = 11,
        get_special_folder = 12,
        write_to_uninstall = 13,
        copy_installer_to_temp = 14,
        get_temp_path = 15,
        get_temp_file_name = 16,
        get_module_name = 17,
        start_installer_from_temp = 18,
        invalid_installer_pack = 19,
        create_exported_account_folder = 20,
        create_exported_settings_folder = 21,

        terminate_connect_timeout = 22,
        terminate_procid_rcvd = 23,
        terminate_open_proc_failed = 24,
        terminate_shutdown_sent = 25,

        terminate_abnormal_open_proc_failed = 26,
        terminate_abnormal_wait_timeout = 27,
    };

    class error
    {
        errorcode	error_;
        QString		error_text_;

    public:

        error(errorcode _error = errorcode::ok, const QString& _error_text = "")
            : error_(_error),
            error_text_(_error_text)
        {

        }

        bool is_ok() const
        {
            return (error_ == errorcode::ok);
        }

        void show() const
        {
            std::wstringstream ss_err;
            ss_err << L"installation error, code = " << error_ << L", (" << (const wchar_t*)error_text_.utf16() << L").";

            ::MessageBox(0, ss_err.str().c_str(), L"ICQ installer", MB_ICONERROR);
        }

        QString get_code_string()
        {
            return QString::number(error_);
        }
    };
}
