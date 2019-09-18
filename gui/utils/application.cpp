#include "stdafx.h"
#include "application.h"

#include "InterConnector.h"
#include "launch.h"
#include "utils.h"
#include "log/log.h"
#include "animation/animation.h"
#include "../core_dispatcher.h"
#include "../gui_settings.h"
#include "../cache/emoji/Emoji.h"
#include "../cache/stickers/stickers.h"
#include "../main_window/MainWindow.h"
#ifdef _WIN32
#include "../main_window/win32/WinNativeWindow.h"
#endif
#include "../main_window/contact_list/ContactListModel.h"
#include "../main_window/contact_list/RecentsModel.h"
#include "../main_window/contact_list/UnknownsModel.h"
#include "../main_window/friendly/FriendlyContainer.h"
#include "../my_info.h"
#include "../../common.shared/version_info.h"
#include "../../gui.shared/local_peer/local_peer.h"

#include "../main_window/mplayer/FFMpegPlayer.h"
#include "../main_window/sounds/SoundsManager.h"
#include "NativeEventFilter.h"
#include "service/ServiceUrlsManager.h"
#include "main_window/LocalPIN.h"
#include "utils/UrlCommand.h"

#include "gui_metrics.h"

#ifdef __linux__
#include <sys/file.h>
#endif //__linux__


namespace
{
    const int SHADOW_WIDTH = 10;
}

namespace Utils
{
#ifdef _WIN32
    AppGuard::AppGuard()
        : Mutex_(nullptr)
        , Exist_(false)
    {
        Mutex_ = ::CreateSemaphore(NULL, 0, 1, Utils::get_crossprocess_mutex_name());
        Exist_ = GetLastError() == ERROR_ALREADY_EXISTS;
    }

    AppGuard::~AppGuard()
    {
        if (Mutex_)
            CloseHandle(Mutex_);
    }

    bool AppGuard::succeeded() const
    {
        return !Exist_;
    }

    /**
     * To fix crash in voip on shutting down PC,
     * We process WM_QUERYENDSESSION/WM_ENDSESSION and Windows will wait our deinitialization.
     */
    class WindowsEventFilter : public QAbstractNativeEventFilter
    {
    public:
        WindowsEventFilter() {}

        virtual bool nativeEventFilter(const QByteArray& eventType, void* message, long* result) override
        {
            MSG* msg = reinterpret_cast<MSG*>(message);
            if (msg->message == WM_QUERYENDSESSION || msg->message == WM_ENDSESSION)
            {
                if (!Ui::GetDispatcher()->getVoipController().isVoipKilled())
                {
                    Ui::GetDispatcher()->getVoipController().voipReset();

                    // Under XP we cannot return true, because it will stop shutting down of PC
                    // https://msdn.microsoft.com/en-us/library/ms700677(v=vs.85).aspx
                    if (QSysInfo().windowsVersion() != QSysInfo::WV_XP)
                    {
                        // Do not shutting down us immediately, we need time to terminate
                        if (result)
                            *result = 0;

                        return true;
                    }
                    else
                    {
                        // Give voip time to release.
                        // Also we can show own message box
                        // while voip is shutting down.
                        Sleep(300);
                    }
                }
            }

            return false;
        }
    };
#endif //_WIN32

    Application::Application(int& _argc, char* _argv[])
        : QObject(nullptr)
    {
        if constexpr (platform::is_apple())
        {
#ifndef ICQ_QT_STATIC
    QDir dir(_argv[0]);
    dir.cdUp();
    dir.cdUp();
    dir.cd(qsl("PlugIns"));
    QCoreApplication::setLibraryPaths(QStringList(dir.absolutePath()));
#endif
        }
        else if constexpr (platform::is_linux())
        {
            std::string appDir(_argv[0]);
            appDir = appDir.substr(0, appDir.rfind('/') + 1);
            appDir += "plugins";
            QCoreApplication::setLibraryPaths(QStringList(QString::fromStdString(appDir)));
        }

        app_ = std::make_unique<QApplication>(_argc, _argv);
        app_->setProperty("startupTime", QDateTime::currentMSecsSinceEpoch());

        nativeEvFilter_ = std::make_unique<NativeEventFilter>();
        app_->installNativeEventFilter(nativeEvFilter_.get());

#ifndef _WIN32
        // Fix float separator: force use . (dot).
        // http://doc.qt.io/qt-5/qcoreapplication.html#locale-settings
        setlocale(LC_NUMERIC, "C");

        if constexpr (platform::is_linux())
        {
            try
            {
                std::locale("");
            }
            catch (const std::runtime_error&)
            {
                qWarning() << "Invalid locale, defaulting to C";

                setlocale(LC_ALL, "C");
                setenv("LC_ALL", "C", 1);
            }
        }
#endif // _WIN32

#ifdef _WIN32
        guard_ = std::make_unique<Utils::AppGuard>();
#endif //_WIN32

        peer_ = std::make_unique<LocalPeer>(nullptr, isMainInstance());
        connect(peer_.get(), &LocalPeer::schemeUrl, &Utils::InterConnector::instance(), &Utils::InterConnector::schemeUrlClicked);
        connect(&Utils::InterConnector::instance(), &Utils::InterConnector::schemeUrlClicked, this, &Application::receiveUrlCommand);

        connect(app_.get(), &QGuiApplication::applicationStateChanged, this, &Application::applicationStateChanged, Qt::DirectConnection);

        serviceMgr_ = std::make_unique<ServiceUrlsManager>();

        connect(&Utils::InterConnector::instance(), &Utils::InterConnector::gotServiceUrls,
                serviceMgr_.get(), &ServiceUrlsManager::processNewServiceURls);
    }

    Application::~Application()
    {
#ifdef _WIN32
        std::unique_ptr<Ui::WinNativeWindow> nativeWindow;
        if (mainWindow_)
            nativeWindow = mainWindow_->takeNativeWindow();
#endif
        mainWindow_.reset();
#ifdef _WIN32
        nativeWindow.reset();
#endif
        anim::AnimationManager::stopManager();

        Emoji::Cleanup();

        Ui::ResetMediaContainer();

        Logic::ResetRecentsModel();
        Logic::ResetUnknownsModel();
        Logic::ResetContactListModel();
        Logic::ResetFriendlyContainer();
        Ui::ResetMyInfo();
        Ui::ResetSoundsManager();
    }


    int Application::exec()
    {
#ifdef _WIN32
        WindowsEventFilter eventFilter;
        qApp->installNativeEventFilter(&eventFilter);
#endif

        int res = app_->exec();
#ifdef _WIN32
        qApp->removeNativeEventFilter(&eventFilter);
#endif

        return res;
    }

    bool Application::init(launch::CommandLineParser& _cmdParser)
    {
        Ui::createDispatcher();

        init_win7_features();

        app_->setAttribute(Qt::AA_SynthesizeMouseForUnhandledTouchEvents, false);

        QPixmapCache::setCacheLimit(0);

        QObject::connect(Ui::GetDispatcher(), &Ui::core_dispatcher::coreLogins, this, &Application::coreLogins, Qt::DirectConnection);
        QObject::connect(Ui::GetDispatcher(), &Ui::core_dispatcher::guiSettings, this, &Application::guiSettings, Qt::DirectConnection);

        if (_cmdParser.isUrlCommand())
            urlCommand_ = _cmdParser.getUrlCommand();

        return true;
    }

    void Application::init_win7_features()
    {
        Utils::groupTaskbarIcon(true);
    }

    bool Application::isMainInstance()
    {
#if defined _WIN32
        return guard_->succeeded();
#elif defined __linux__
        int fd;
        struct flock fl;
        fd = open(qsl("/tmp/%1.pid").arg(build::is_agent() ? qsl("magent") : build::ProductNameShort()).toStdString().c_str(), O_CREAT | O_RDWR, 0666);
        if(fd == -1)
        {
            return true;
        }

        fl.l_type   = F_WRLCK;
        fl.l_whence = SEEK_SET;
        fl.l_start  = 0;
        fl.l_len    = 0;
        fl.l_pid    = QCoreApplication::applicationPid();

        if( fcntl(fd, F_SETLK, &fl) == -1)
        {
            if( errno == EACCES || errno == EAGAIN)
            {
                 return false;
            }
        }
        return true;
#else
        return true;
#endif //_WIN32
    }

    int Application::switchInstance(launch::CommandLineParser& _cmdParser)
    {
        QTimer::singleShot(0, this, [&_cmdParser, this]()
        {
            connect(peer_.get(), &LocalPeer::error, this, [this]()
            {
                qDebug() << "switchInstance: LocalPeer error, exiting...";
                app_->quit();
            });

            if (_cmdParser.isUrlCommand())
            {
                connect(peer_.get(), &LocalPeer::commandSent, this, &Application::getHwndAndActivate);
                peer_->sendUrlCommand(_cmdParser.getUrlCommand());
            }
            else
            {
                getHwndAndActivate();
            }
        });

        return app_->exec();
    }

    void Application::getHwndAndActivate()
    {
        connect(peer_.get(), &LocalPeer::hwndReceived, this, &Application::onHwndReceived);
        peer_->getHwndAndActivate();
    }

    void Application::onHwndReceived(const unsigned int _hwnd)
    {
#ifdef _WIN32
        if (_hwnd)
            ::SetForegroundWindow((HWND)_hwnd);
#endif //_WIN32
        app_->quit();
    }

    void Application::coreLogins(const bool _has_valid_login, const bool _locked)
    {
        initMainWindow(_has_valid_login, _locked);
    }

    void Application::guiSettings()
    {
        Ui::convertOldDownloadPath();
    }

    void Application::initMainWindow(const bool _has_valid_login, const bool _locked)
    {
        anim::AnimationManager::startManager();
        Logic::GetFriendlyContainer();

        double dpi = app_->primaryScreen()->logicalDotsPerInchX();

        if constexpr (platform::is_apple())
        {
            dpi = 96;
            Utils::set_mac_retina(app_->primaryScreen()->devicePixelRatio() == 2);
            if (Utils::is_mac_retina())
                app_->setAttribute(Qt::AA_UseHighDpiPixmaps);
        }

        QFontDatabase::addApplicationFont(qsl(":/fonts/SourceSansPro_Light"));
        QFontDatabase::addApplicationFont(qsl(":/fonts/SourceSansPro_Regular"));
        QFontDatabase::addApplicationFont(qsl(":/fonts/SourceSansPro_SemiBold"));
        QFontDatabase::addApplicationFont(qsl(":/fonts/SourceSansPro_Bold"));

        QFontDatabase::addApplicationFont(qsl(":/fonts/RoundedMplus_Bold"));

        QFontDatabase::addApplicationFont(qsl(":/fonts/RobotoMono_Regular"));
        QFontDatabase::addApplicationFont(qsl(":/fonts/RobotoMono_Medium"));

        if constexpr (platform::is_apple())
        {
            QFontDatabase::addApplicationFont(qsl(":/fonts/SFProText_Regular"));
            QFontDatabase::addApplicationFont(qsl(":/fonts/SFProText_Medium"));
            QFontDatabase::addApplicationFont(qsl(":/fonts/SFProText_SemiBold"));
            QFontDatabase::addApplicationFont(qsl(":/fonts/SFProText_Bold"));
        }

        const auto guiScaleCoefficient = std::min(dpi / 96.0, 2.0);

        Utils::initBasicScaleCoefficient(guiScaleCoefficient);

        Utils::setScaleCoefficient(Ui::get_gui_settings()->get_value<double>(settings_scale_coefficient, Utils::getBasicScaleCoefficient()));
        app_->setFont(Fonts::appFontFamilyNameQss(Fonts::defaultAppFontFamily(), Fonts::FontWeight::Normal));

        Emoji::InitializeSubsystem();

        Ui::get_gui_settings()->set_shadow_width(Utils::scale_value(SHADOW_WIDTH));

        Utils::GetTranslator()->init();
        mainWindow_ = std::make_unique<Ui::MainWindow>(app_.get(), _has_valid_login, _locked);

        Ui::GetDispatcher()->updateRecentAvatarsSize();

        bool needToShow = true;
        bool isAutoStart = false;

#ifdef _WIN32
        const auto arguments = app_->arguments();
        const auto isStartMinimized = Ui::get_gui_settings()->get_value<bool>(settings_start_minimazed, true);

        for (const auto& argument : arguments)
        {
            if (argument == ql1s("/startup") && isStartMinimized)
            {
                isAutoStart = true;
                needToShow = false;
                mainWindow_->minimize();
                break;
            }
        }
#endif //_WIN32

        if (isAutoStart)
            statistic::getGuiMetrics().eventAppStartAutostart();
        else
            statistic::getGuiMetrics().eventAppStartByIcon();

        statistic::getGuiMetrics().eventUiReady(needToShow);

        if (needToShow)
        {
            mainWindow_->show();
            mainWindow_->raise();

            mainWindow_->activateWindow();
        }

        connect(peer_.get(), &LocalPeer::needActivate, mainWindow_.get(), &Ui::MainWindow::needActivate, Qt::DirectConnection);
        peer_->setMainWindow(mainWindow_.get());

        if (!urlCommand_.isEmpty())
        {
            connect(Ui::GetDispatcher(), &Ui::core_dispatcher::im_created, this, [this]()
            {
                parseUrlCommand(urlCommand_);
            });
        }
    }

    void Application::parseUrlCommand(const QString& _urlCommand)
    {
        const auto activate = [this]()
        {
            if (mainWindow_)
            {
                mainWindow_->activate();
                mainWindow_->raise();
            }
        };

        if (!Utils::InterConnector::instance().getMainPage())
        {
            activate();
            return;
        }

        Ui::DeferredCallback action;

        auto command = UrlCommandBuilder::makeCommand(_urlCommand, UrlCommandBuilder::Context::External);

        if (command->isValid())
        {
            action = [activate, command]()
            {
                activate();
                command->execute();
            };
        }

        if (action)
        {
            if (!Ui::LocalPIN::instance()->locked())
                action();
            else
                Ui::LocalPIN::instance()->setDeferredAction(new Ui::DeferredAction(std::move(action)));
        }
    }

    void Application::receiveUrlCommand(const QString& _urlCommand)
    {
        parseUrlCommand(_urlCommand);
    }

    bool Application::updating()
    {
#ifdef _WIN32

        const auto updater_singlton_mutex_name = build::GetProductVariant(updater_singlton_mutex_name_icq, updater_singlton_mutex_name_agent, updater_singlton_mutex_name_biz, updater_singlton_mutex_name_dit);

        CHandle mutex(::CreateSemaphore(NULL, 0, 1, updater_singlton_mutex_name.c_str()));
        if (ERROR_ALREADY_EXISTS == ::GetLastError())
            return true;

        CRegKey keySoftware;

        if (ERROR_SUCCESS != keySoftware.Open(HKEY_CURRENT_USER, L"Software"))
            return false;

        CRegKey key_product;
        auto productName = getProductName();
        if (ERROR_SUCCESS != key_product.Create(keySoftware, (const wchar_t*) productName.utf16()))
            return false;

        wchar_t versionBuffer[1025];
        unsigned long len = 1024;
        if (ERROR_SUCCESS != key_product.QueryStringValue(L"update_version", versionBuffer, &len))
            return false;

        QString versionUpdate = QString::fromUtf16((const ushort*)versionBuffer);

        core::tools::version_info infoCurrent;
        core::tools::version_info infoUpdate(versionUpdate.toStdString());

        if (infoCurrent < infoUpdate)
        {
            wchar_t thisModuleName[1024];
            if (!::GetModuleFileName(0, thisModuleName, 1024))
                return false;

            QDir dir = QFileInfo(QString::fromUtf16((const ushort*)thisModuleName)).absoluteDir();
            if (!dir.cdUp())
                return false;

            const QString updateFolder = dir.absolutePath() % ql1c('/') % updates_folder_short  % ql1c('/') % versionUpdate;

            QDir updateDir(updateFolder);
            if (updateDir.exists())
            {
                const QString setupName = updateFolder % ql1c('/') % Utils::getInstallerName();
                QFileInfo setupInfo(setupName);
                if (!setupInfo.exists())
                    return false;

                mutex.Close();

                const QString command = ql1c('"') % QDir::toNativeSeparators(setupName) % ql1s("\" ") % update_final_command;
                QProcess::startDetached(command);

                return true;
            }
        }
#endif //_WIN32

        return false;
    }

    void Application::applicationStateChanged(Qt::ApplicationState state)
    {
        if (platform::is_apple() && state == Qt::ApplicationInactive)
        {
            emit Utils::InterConnector::instance().closeAnyPopupMenu();
            emit Utils::InterConnector::instance().closeAnyPopupWindow(Utils::CloseWindowInfo());
        }
    }
}
