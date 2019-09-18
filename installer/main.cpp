#include "stdafx.h"

#ifdef _WIN32
#include <comutil.h>
#include <comdef.h>

#include "logic/tools.h"
#include "logic/worker.h"
#include "ui/main_window/main_window.h"
#include "utils/styles.h"

#include "../common.shared/win32/crash_handler.h"
#include "../common.shared/win32/common_crash_sender.h"

Q_IMPORT_PLUGIN(QWindowsIntegrationPlugin);
Q_IMPORT_PLUGIN(QICOPlugin);
using namespace installer;
const wchar_t* installer_singlton_mutex_name_icq = L"{5686F5E0-121F-449B-ABB7-B01021D7DE65}";
const wchar_t* installer_singlton_mutex_name_agent = L"{6CE5AF68-83B7-477A-9101-D9F56C5F57B8}";
const wchar_t* installer_singlton_mutex_name_biz = L"{4301FF79-43C0-4182-A453-948D60C8BA56}";
const wchar_t* installer_singlton_mutex_name_dit = L"{9150781C-C201-4520-B8B7-0BF54BBCE1D4}";
const wchar_t* get_installer_singlton_mutex_name() { return build::get_product_variant(installer_singlton_mutex_name_icq, installer_singlton_mutex_name_agent, installer_singlton_mutex_name_biz, installer_singlton_mutex_name_dit);}
const wchar_t* old_version_launch_mutex_name = L"MRAICQLAUNCH";
#endif //_WIN32

int main(int _argc, char* _argv[])
{
#ifndef __linux__
    if (_argv[1] == send_dump_arg)
    {
        ::common_crash_sender::post_dump_to_hockey_app_and_delete();
        return 0;
    };
#endif //__linux__

    int res = 0;

#ifdef _WIN32
    const wchar_t* product_path = build::get_product_variant(product_path_icq_w, product_path_agent_w, product_path_biz_w, product_path_dit_w);
    core::dump::crash_handler chandler("icq.dekstop.installer", ::common::get_user_profile() + L"/" + product_path, false);
    chandler.set_process_exception_handlers();
    chandler.set_thread_exception_handlers();
    chandler.set_is_sending_after_crash(true);

    CHandle mutex(::CreateSemaphore(NULL, 0, 1, get_installer_singlton_mutex_name()));
    if (ERROR_ALREADY_EXISTS == ::GetLastError())
        return res;
#endif //_WIN32

#ifdef __linux__
    QCoreApplication app(_argc, _argv);
#else
    QApplication app(_argc, _argv);
#endif

#ifdef __linux__
    QString binary_path = app.applicationDirPath() + "/" + APP_TYPE;
    QFile::remove(binary_path);
#ifdef __x86_64__
    QFile::copy(QString(":/bin/Release64/%1").arg(APP_TYPE), binary_path);
#else
    QFile::copy(QString(":/bin/Release32/%1").arg(APP_TYPE), binary_path);
#endif
    std::string chmod("chmod 755 ");
    binary_path.replace(" ", "\\ ");
    chmod += binary_path.toStdString();
    system(chmod.c_str());
#else

    logic::get_translator()->init();

    std::unique_ptr<ui::main_window> wnd;
    bool autoInstall = false;
    {
        QStringList arguments = app.arguments();

        logic::install_config config;

        for (auto iter = arguments.cbegin(); iter != arguments.cend(); ++iter)
        {
            if (*iter == "-uninstalltmp")
            {
                config.set_uninstalltmp(true);

                ++iter;
                if (iter == arguments.cend())
                    break;

                config.set_folder_for_delete(*iter);
            }
            else if (*iter == "-uninstall")
            {
                config.set_uninstall(true);
            }
            else if (*iter == "-appx")
            {
                config.set_appx(true);
            }
            else if (*iter == "-update")
            {
                config.set_update(true);
            }
            else if (*iter == autoupdate_from_8x)
            {
                config.set_autoupdate_from_8x(true);
            }
            else if (*iter == update_final_command)
            {
                config.set_update_final(true);
            }
            else if (*iter == delete_updates_command)
            {
                config.set_delete_updates(true);
            }
            else if (*iter == nolaunch_from_8x)
            {
                config.set_nolaunch_from_8x(true);
            }
            else if (*iter == "-autoinstall")
            {
                autoInstall = true;
            }
            else if (*iter == "-installsilent")
            {
                config.set_install_silent_from_agent_5x(true);
            }
        }

        set_install_config(config);

        auto connect_to_events = [&wnd, &app, autoInstall]()
        {
            QObject::connect(logic::get_worker(), &logic::worker::finish, []()
            {
                QApplication::exit();
            });

            QObject::connect(logic::get_worker(), &logic::worker::error, [](installer::error _err)
            {
                QApplication::exit();
            });

            QObject::connect(logic::get_worker(), &logic::worker::select_account, [&wnd, &app, autoInstall]()
            {
                app.setStyleSheet(ui::styles::load_style(":/styles/styles.qss"));

                wnd = std::make_unique<ui::main_window>(ui::main_window_start_page::page_accounts, autoInstall);

                QIcon icon(build::get_product_variant(":/images/installer_icq_icon.ico", ":/images/installer_agent_icon.ico", ":/images/installer_agent_icon.ico", ":/images/installer_dit_icon.ico"));
                wnd->setWindowIcon(icon);

                wnd->show();
            });
        };

        if (logic::get_install_config().is_uninstalltmp())
        {
            logic::get_worker()->uninstalltmp();
        }
        else if (logic::get_install_config().is_delete_updates())
        {
            logic::get_worker()->clear_updates();
        }
        else if (logic::get_install_config().is_uninstall())
        {
            connect_to_events();

            logic::get_worker()->uninstall();

            res = app.exec();
        }
        else if (logic::get_install_config().is_update())
        {
            connect_to_events();

            logic::get_worker()->update();

            res = app.exec();
        }
        else if (logic::get_install_config().is_update_final())
        {
            const auto updater_singlton_mutex_name = build::get_product_variant(updater_singlton_mutex_name_icq, updater_singlton_mutex_name_agent, updater_singlton_mutex_name_biz, updater_singlton_mutex_name_dit);

            CHandle mutex(::CreateSemaphore(NULL, 0, 1, updater_singlton_mutex_name.c_str()));
            if (ERROR_ALREADY_EXISTS == ::GetLastError())
                return true;

            connect_to_events();

            logic::get_worker()->update_final(mutex);

            res = app.exec();
        }
        else
        {
            app.setStyleSheet(ui::styles::load_style(":/styles/styles.qss"));

            if (logic::get_install_config().is_appx())
            {
                connect_to_events();

                logic::get_worker()->install();
            }
            else
            {
                wnd = std::make_unique<ui::main_window>(ui::main_window_start_page::page_start, autoInstall);

                QIcon icon(build::get_product_variant(":/images/installer_icq_icon.ico", ":/images/installer_agent_icon.ico", ":/images/installer_biz_icon.ico", ":/images/installer_dit_icon.ico"));

                wnd->setWindowIcon(icon);

                wnd->show();
            }


            res = app.exec();
        }

    }
#endif //__linux__
    return res;
}
