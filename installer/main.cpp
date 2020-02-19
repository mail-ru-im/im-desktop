#include "stdafx.h"

#ifdef _WIN32
#include <comutil.h>
#include <comdef.h>

#include "logic/tools.h"
#include "logic/worker.h"
#include "ui/main_window/main_window.h"
#include "utils/styles.h"
#include "utils/statistics.h"

#include "../common.shared/win32/crash_handler.h"
#include "../common.shared/win32/common_crash_sender.h"
#include "../common.shared/config/config.h"

Q_IMPORT_PLUGIN(QWindowsIntegrationPlugin);
Q_IMPORT_PLUGIN(QICOPlugin);
using namespace installer;

static void waitGlobalThreadPool()
{
    QThreadPool::globalInstance()->waitForDone(std::chrono::milliseconds(8000).count());
}
#endif //_WIN32

int main(int _argc, char* _argv[])
{
#ifndef __linux__
    if (_argc > 1 && QString::fromUtf8(_argv[1]) == send_dump_arg)
    {
        ::common_crash_sender::post_dump_and_delete();
        return 0;
    };
#endif //__linux__

    int res = 0;

#ifdef _WIN32
    const auto start_time = std::chrono::system_clock::now();
    const auto product_path = config::get().string(config::values::product_path);
    core::dump::crash_handler chandler("icq.dekstop.installer", ::common::get_user_profile() + L'/' + QString::fromUtf8(product_path.data(), product_path.size()).toStdWString(), false);
    chandler.set_process_exception_handlers();
    chandler.set_thread_exception_handlers();
    chandler.set_is_sending_after_crash(true);

    CHandle mutex(::CreateSemaphoreA(NULL, 0, 1, std::string(config::get().string(config::values::installer_main_instance_mutex_win)).c_str()));
    if (ERROR_ALREADY_EXISTS == ::GetLastError())
        return res;
#endif //_WIN32

#ifdef __linux__
    QCoreApplication app(_argc, _argv);
#else
    QApplication app(_argc, _argv);
#endif

#ifdef __linux__
    const QString app_name = QString::fromUtf8(APP_PROJECT_NAME);
    QString binary_path = app.applicationDirPath() % QLatin1Char('/') % app_name;
    QFile::remove(binary_path);
#ifdef __x86_64__
    QFile::copy(QStringLiteral(":/bin/Release64/%1").arg(app_name), binary_path);
#else
    QFile::copy(QStringLiteral(":/bin/Release32/%1").arg(app_name), binary_path);
#endif
    binary_path.replace(QLatin1Char(' '), QLatin1String("\\ "));
    const std::string chmod = "chmod 755 " + binary_path.toStdString();
    system(chmod.c_str());
#else

    logic::get_translator()->init();
    QFontDatabase::addApplicationFont(qsl(":/fonts/SourceSansPro_Regular"));
    QFontDatabase::addApplicationFont(qsl(":/fonts/SourceSansPro_Bold"));
    QFontDatabase::addApplicationFont(qsl(":/fonts/SFProText_Regular"));

    std::unique_ptr<ui::main_window> wnd;
    bool autoInstall = false;
    {
        const QStringList arguments = app.arguments();

        logic::install_config config;

        for (auto iter = arguments.cbegin(); iter != arguments.cend(); ++iter)
        {
            if (*iter == ql1s("-uninstalltmp"))
            {
                config.set_uninstalltmp(true);

                ++iter;
                if (iter == arguments.cend())
                    break;

                config.set_folder_for_delete(*iter);
            }
            else if (*iter == ql1s("-uninstall"))
            {
                config.set_uninstall(true);
            }
            else if (*iter == ql1s("-appx"))
            {
                config.set_appx(true);
            }
            else if (*iter == ql1s("-update"))
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
            else if (*iter == ql1s("-autoinstall"))
            {
                autoInstall = true;
            }
            else if (*iter == ql1s("-installsilent"))
            {
                config.set_install_silent_from_agent_5x(true);
            }
        }

        set_install_config(config);

        QObject::connect(logic::get_worker(), &logic::worker::error, [start_time](const installer::error& _err)
        {
            if (!_err.is_ok())
                QThreadPool::globalInstance()->start(new statisctics::SendTask(_err, start_time));
        });

        auto connect_to_events = [&wnd, &app, autoInstall]()
        {
            QObject::connect(logic::get_worker(), &logic::worker::finish, []()
            {
                QApplication::exit();
            });

            QObject::connect(logic::get_worker(), &logic::worker::error, [](const installer::error&)
            {
                QApplication::exit();
            });

            QObject::connect(logic::get_worker(), &logic::worker::select_account, [&wnd, &app, autoInstall]()
            {
                app.setStyleSheet(ui::styles::load_style(qsl(":/styles/styles.qss")) % ui::styles::load_style(qsl(":/product/styles.qss")));

                wnd = std::make_unique<ui::main_window>(ui::main_window_start_page::page_accounts, autoInstall);

                wnd->setWindowIcon(QIcon(qsl(":/product/installer.ico")));

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
            CHandle mutex(::CreateSemaphoreA(NULL, 0, 1, std::string(config::get().string(config::values::updater_main_instance_mutex_win)).c_str()));
            if (ERROR_ALREADY_EXISTS == ::GetLastError())
                return true;

            connect_to_events();

            logic::get_worker()->update_final(mutex);

            res = app.exec();
        }
        else
        {
            app.setStyleSheet(ui::styles::load_style(qsl(":/styles/styles.qss")) % ui::styles::load_style(qsl(":/product/styles.qss")));

            if (logic::get_install_config().is_appx())
            {
                connect_to_events();

                logic::get_worker()->install();
            }
            else
            {
                wnd = std::make_unique<ui::main_window>(ui::main_window_start_page::page_start, autoInstall);

                wnd->setWindowIcon(QIcon(qsl(":/product/installer.ico")));

                wnd->show();
            }

            res = app.exec();
        }

        logic::worker::clear_tmp_install_dir();

        waitGlobalThreadPool();
    }
#endif //__linux__
    return res;
}
