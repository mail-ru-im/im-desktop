#include "stdafx.h"
#include "worker.h"

#include "tools.h"

#include "tasks/copy_files.h"
#include "tasks/start_process.h"
#include "tasks/terminate_process.h"
#include "tasks/write_registry.h"
#include "tasks/create_links.h"

namespace installer
{
    namespace logic
    {
        worker current_worker;

        worker* get_worker()
        {
            return &current_worker;
        }

        worker::worker()
            :   progress_(0)
        {
        }

        worker::~worker()
        {
        }

        task_callback dummy_callback = [](const installer::error&){};

        void worker::reset_watcher()
        {
            current_task_watcher_ = std::make_unique< QFutureWatcher<installer::error> >();
        }

        void worker::final_install()
        {
            run_async_function<installer::error>(start_process, [this](const installer::error& /*_err*/)
            {
                emit finish();

            }, 100);
        }

        void worker::install()
        {
            progress_ = 5;
            emit progress(progress_);

            run_async_function<installer::error>(terminate_process, [this](const installer::error& _err)
            {
                if (!_err.is_ok())
                {
                    emit error(_err);
                    return;
                }

                run_async_function<installer::error>(copy_files, [this](const installer::error& _err)
                {
                    if (!_err.is_ok())
                    {
                        emit error(_err);
                        return;
                    }

                    if (get_install_config().is_appx())
                    {
                        run_async_function<installer::error>(create_links, [this](const installer::error& _err)
                        {
                            if (!_err.is_ok())
                            {
                                emit error(_err);
                                return;
                            }

                            emit finish();

                        }, 100);
                    }
                    else
                    {
                        run_async_function<installer::error>(write_registry, [this](const installer::error& _err)
                        {
                            if (!_err.is_ok())
                            {
                                emit error(_err);
                                return;
                            }

                            run_async_function<installer::error>(create_links, [this](const installer::error& _err)
                            {
                                if (!_err.is_ok())
                                {
                                    emit error(_err);
                                    return;
                                }

                                final_install();

                            }, 85);
                        }, 60);
                    }
                }, 40);
            }, 30);
        }

        void worker::uninstall()
        {
            progress_ = 10;
            emit progress(progress_);

            run_async_function<installer::error>(terminate_process, [this](const installer::error&)
            {
                run_async_function<installer::error>(clear_registry, [this](const installer::error&)
                {
                    run_async_function<installer::error>(delete_links, [this](const installer::error&)
                    {
                        run_async_function<installer::error>(delete_files, [this](const installer::error&)
                        {
                            run_async_function<installer::error>(delete_self_and_product_folder, [this](const installer::error&)
                            {
                                emit finish();
                            }, 100);
                        }, 80);
                    }, 60);
                }, 40);
            }, 20);
        }

        void worker::uninstalltmp()
        {
            QString folder_for_delete = get_product_folder();

            if (!get_install_config().get_folder_for_delete().isEmpty())
            {
                folder_for_delete = get_install_config().get_folder_for_delete();
            }

            QDir delete_dir(folder_for_delete);

            for (int i = 0; i < 50; i++)
            {
                if (delete_dir.removeRecursively())
                    return;

                ::Sleep(100);
            }
        }

        void worker::clear_updates()
        {
            QDir dir_updates(get_updates_folder());

            for (int i = 0; i < 50; i++)
            {
                if (dir_updates.removeRecursively())
                    return;

                ::Sleep(100);
            }
        }

        void worker::update()
        {
            progress_ = 5;
            emit progress(progress_);

            run_async_function<installer::error>(copy_files_to_updates, [this](const installer::error& _err)
            {
                if (!_err.is_ok())
                {
                    emit error(_err);
                    return;
                }

                run_async_function<installer::error>(write_update_version, [this](const installer::error&)
                {
                    emit finish();
                }, 100);

            }, 95);
        }

        void worker::update_final(CHandle& _mutex)
        {
            progress_ = 5;
            emit progress(progress_);

            run_async_function<installer::error>(wait_and_kill_prev_icq, [this, &_mutex](const installer::error& _err)
            {
                if (!_err.is_ok())
                {
                    emit error(_err);
                    return;
                }

                run_async_function<installer::error>(copy_files_from_updates, [this, &_mutex](const installer::error& _err)
                {
                    if (!_err.is_ok())
                    {
                        emit error(_err);
                        return;
                    }

                    run_async_function<installer::error>(write_to_uninstall_key, [this, &_mutex](const installer::error& _err)
                    {
                        if (!_err.is_ok())
                        {
                            emit error(_err);
                            return;
                        }

                        _mutex.Close();

                        run_async_function<installer::error>(start_process, [this](const installer::error& _err)
                        {
                            if (!_err.is_ok())
                            {
                                emit error(_err);
                                return;
                            }

                            run_async_function<installer::error>(copy_self_from_updates, [this](const installer::error& _err)
                            {
                                if (!_err.is_ok())
                                {
                                    emit error(_err);
                                    return;
                                }

                                run_async_function<installer::error>(delete_updates, [this](const installer::error& _err)
                                {
                                    if (!_err.is_ok())
                                    {
                                        emit error(_err);
                                        return;
                                    }

                                    emit finish();

                                }, 100);
                            }, 90);
                        }, 80);
                    }, 75);
                }, 70);
            }, 50);
        }
    }
}