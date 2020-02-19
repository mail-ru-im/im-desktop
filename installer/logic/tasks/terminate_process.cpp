#include "stdafx.h"
#include "terminate_process.h"
#include "../tools.h"

#ifdef _WIN32
#include <windows.h>
#include <psapi.h>
#endif //_WIN32

namespace
{
    constexpr std::chrono::milliseconds connectStepTimeout = std::chrono::seconds(10);
    constexpr std::chrono::milliseconds processWaitTimeout = std::chrono::seconds(10);
    constexpr std::chrono::milliseconds connectWaitTimeout = processWaitTimeout + std::chrono::seconds(2);
}

namespace installer
{
    namespace logic
    {
        errorcode connect_to_process_exit_it()
        {
            errorcode res = errorcode::terminate_connect_timeout;

            QEventLoop loop;

            QTimer t;
            t.setSingleShot(true);

            auto peer = std::make_unique<Utils::LocalPeer>(nullptr, false);

            QObject::connect(&t, &QTimer::timeout, &loop, &QEventLoop::quit);
            QObject::connect(peer.get(), &Utils::LocalPeer::error, &loop, &QEventLoop::quit);

            QObject::connect(peer.get(), &Utils::LocalPeer::processIdReceived, [&t, &loop, &res, &peer](unsigned int _processId)
            {
                res = errorcode::terminate_procid_rcvd;
#ifdef _WIN32
                t.start(connectStepTimeout.count());

                HANDLE process = ::OpenProcess(SYNCHRONIZE, FALSE, _processId);
                if (!process)
                {
                    res = errorcode::terminate_open_proc_failed;
                    loop.quit();
                    return;
                }

                QObject::connect(peer.get(), &Utils::LocalPeer::commandSent, [&t, &loop, &res, process]()
                {
                    res = errorcode::terminate_shutdown_sent;

                    t.start(connectWaitTimeout.count());
                    if (::WaitForSingleObject(process, processWaitTimeout.count()) != WAIT_TIMEOUT)
                    {
                        res = errorcode::ok;
                        loop.quit();
                    }
                });
#endif //_WIN32

                t.start(connectStepTimeout.count());
                peer->sendShutdown();
            });

            t.start(connectStepTimeout.count());
            peer->getProcessId();

            loop.exec();

            // process pending deleteLater from LocalPeer sockets
            QCoreApplication::sendPostedEvents(0, QEvent::DeferredDelete);

            return res;
        }

#ifdef _WIN32
        std::vector<DWORD> findProcessIds()
        {
            std::vector<DWORD> res;

            DWORD processes_ids[1024], needed, count_processes;

            QString path_installed = get_installed_product_path();

            if (::EnumProcesses(processes_ids, sizeof(processes_ids), &needed))
            {
                count_processes = needed / sizeof(DWORD);

                for (unsigned int i = 0; i < count_processes; ++i)
                {
                    if (processes_ids[i] != 0)
                    {
                        wchar_t process_name[1024] = L"<unknown>";

                        HANDLE process_handle = ::OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, processes_ids[i]);
                        if (process_handle)
                        {
                            HMODULE module = 0;
                            DWORD cbNeeded = 0;

                            if (::EnumProcessModules(process_handle, &module, sizeof(module), &cbNeeded))
                            {
                                if (::GetModuleFileNameEx(process_handle, module, process_name, sizeof(process_name) / sizeof(wchar_t)))
                                {
                                    QString process_path = QString::fromUtf16((const ushort*)process_name);

                                    if (process_path.toLower() == path_installed.toLower())
                                    {
                                        res.push_back(processes_ids[i]);
                                    }
                                }
                            }

                            ::CloseHandle(process_handle);
                        }
                    }
                }
            }
            return res;
        }
#endif

        errorcode abnormal_terminate()
        {
#ifdef _WIN32
            const auto procIds = findProcessIds();

            if (!procIds.empty())
            {
                errorcode res;
                for (const auto id : procIds)
                {
                    HANDLE process_handle = ::OpenProcess(PROCESS_TERMINATE, FALSE, id);
                    if (process_handle)
                    {
                        ::TerminateProcess(process_handle, 0);
                        if (::WaitForSingleObject(process_handle, processWaitTimeout.count()) == WAIT_TIMEOUT)
                            res = errorcode::terminate_abnormal_wait_timeout;

                        ::CloseHandle(process_handle);
                    }
                    else
                    {
                        res = errorcode::terminate_abnormal_open_proc_failed;
                    }
                }
            }
#endif
            return errorcode::ok;
        }

        installer::error terminate_process()
        {
            installer::error err;

#ifdef _WIN32
            HANDLE mutex = ::CreateSemaphoreA(NULL, 0, 1, std::string(get_crossprocess_mutex_name()).c_str());
            if (ERROR_ALREADY_EXISTS == ::GetLastError())
            {
                const auto connectErr = connect_to_process_exit_it();
                if (connectErr != errorcode::ok)
                {
                    err = installer::error(connectErr, qsl("connect to previous: can not connect to previous installation"));

                    const auto abnormalErr = abnormal_terminate();
                    err = installer::error(
                        abnormalErr,
                        abnormalErr != errorcode::ok ? qsl("exit previous: can not terminate previous installation") : QString()
                    );
                }
            }

            if (mutex)
                ::CloseHandle(mutex);
#endif //_WIN32

            return err;
        }

        installer::error wait_and_kill_prev_icq()
        {
            installer::error err = errorcode::ok;
#ifdef _WIN32

            DWORD processes_ids[1024], needed, count_processes;

            QString path_installed = get_installed_product_path();

            if (::EnumProcesses(processes_ids, sizeof(processes_ids), &needed))
            {
                count_processes = needed / sizeof(DWORD);

                for (unsigned int i = 0; i < count_processes; ++i)
                {
                    if (processes_ids[i] != 0)
                    {
                        wchar_t process_name[1024] = L"<unknown>";

                        HANDLE process_handle = ::OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ | PROCESS_TERMINATE | SYNCHRONIZE, FALSE, processes_ids[i]);
                        if (process_handle)
                        {
                            HMODULE module = 0;
                            DWORD cbNeeded = 0;

                            if (::EnumProcessModules(process_handle, &module, sizeof(module), &cbNeeded))
                            {
                                if (::GetModuleFileNameEx(process_handle, module, process_name, sizeof(process_name) / sizeof(wchar_t)))
                                {
                                    QString process_path = QString::fromUtf16((const ushort*)process_name);

                                    if (process_path.toLower() == path_installed.toLower())
                                    {
                                        if (WAIT_TIMEOUT == ::WaitForSingleObject(process_handle, processWaitTimeout.count()))
                                        {
                                            ::TerminateProcess(process_handle, 0);
                                            if (WAIT_TIMEOUT == ::WaitForSingleObject(process_handle, processWaitTimeout.count()))
                                                err = errorcode::terminate_abnormal_wait_timeout;

                                        }
                                    }
                                }
                            }

                            ::CloseHandle(process_handle);
                        }
                    }
                }
            }
#endif //_WIN32

            return err;
        }
    }
}
