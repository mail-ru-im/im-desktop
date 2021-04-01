#include "stdafx.h"
#include "application.h"

#include "InterConnector.h"
#include "launch.h"
#include "utils.h"
#include "log/log.h"
#include "../core_dispatcher.h"
#include "../gui_settings.h"
#include "../cache/emoji/Emoji.h"
#include "../cache/stickers/stickers.h"
#include "../main_window/MainWindow.h"
#include "../main_window/contact_list/ContactListModel.h"
#include "../main_window/contact_list/RecentsModel.h"
#include "../main_window/contact_list/UnknownsModel.h"
#include "../main_window/containers/FriendlyContainer.h"
#include "../main_window/containers/LastseenContainer.h"
#include "../main_window/containers/StatusContainer.h"

#include "../my_info.h"
#include "../../common.shared/version_info.h"
#include "../../common.shared/config/config.h"
#include "../../gui.shared/local_peer/local_peer.h"

#include "../accessibility/AccessibilityFactory.h"

#ifndef STRIP_AV_MEDIA
#include "../main_window/mplayer/FFMpegPlayer.h"
#include "../main_window/sounds/SoundsManager.h"
#endif // !STRIP_AV_MEDIA

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

    void addFont(const QString& _fontName)
    {
        int code = 0;
        code = QFontDatabase::addApplicationFont(_fontName);
        im_assert(code != -1);
    }

    template<typename ...Args>
    void addFonts(Args&&... _args)
    {
        (addFont(_args), ...);
    }
}

namespace Utils
{
#ifdef _WIN32
    AppGuard::AppGuard()
        : Mutex_(nullptr)
        , Exist_(false)
    {
        Mutex_ = ::CreateSemaphoreA(NULL, 0, 1, std::string(Utils::get_crossprocess_mutex_name()).c_str());
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
#ifndef STRIP_VOIP
            MSG* msg = reinterpret_cast<MSG*>(message);
            if (msg->message == WM_QUERYENDSESSION || msg->message == WM_ENDSESSION)
            {
                if (!Ui::GetDispatcher()->getVoipController().isVoipKilled())
                {
                    Ui::GetDispatcher()->getVoipController().voipReset();
                    // Do not shutting down us immediately, we need time to terminate
                    if (result)
                        *result = 0;

                    return true;
                }
            }
#endif

            return false;
        }
    };
#endif //_WIN32

    Application::Application(int& _argc, char* _argv[])
        : QObject(nullptr)
    {
        if constexpr (platform::is_linux())
        {
            const auto appFilePath = std::string_view(_argv[0]);
            const std::string appDir = su::concat(appFilePath.substr(0, appFilePath.rfind('/') + 1), "plugins");
            QCoreApplication::setLibraryPaths(QStringList(QString::fromStdString(appDir)));
        }

        app_ = std::make_unique<QApplication>(_argc, _argv);
        app_->setProperty("startupTime", QDateTime::currentMSecsSinceEpoch());

        initFonts();

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
        // use QueuedConnection since peer should not wait schemeUrlClicked
        connect(peer_.get(), &LocalPeer::schemeUrl, &Utils::InterConnector::instance(), &Utils::InterConnector::schemeUrlClicked, Qt::QueuedConnection);
        connect(&Utils::InterConnector::instance(), &Utils::InterConnector::schemeUrlClicked, this, &Application::receiveUrlCommand);

        connect(app_.get(), &QGuiApplication::applicationStateChanged, this, &Application::applicationStateChanged);

        serviceMgr_ = std::make_unique<ServiceUrlsManager>();

        connect(&Utils::InterConnector::instance(), &Utils::InterConnector::gotServiceUrls, serviceMgr_.get(), &ServiceUrlsManager::processNewServiceURls);
    }

    Application::~Application()
    {
        mainWindow_.reset();

        Emoji::Cleanup();

#ifndef STRIP_AV_MEDIA
        Ui::ResetMediaContainer();
#endif // !STRIP_AV_MEDIA

        Logic::ResetRecentsModel();
        Logic::ResetUnknownsModel();
        Logic::ResetContactListModel();
        Logic::ResetFriendlyContainer();
        Logic::ResetLastseenContainer();
        Logic::ResetStatusContainer();
        Ui::ResetMyInfo();
#ifndef STRIP_AV_MEDIA
        Ui::ResetSoundsManager();
#endif
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

        QObject::connect(Ui::GetDispatcher(), &Ui::core_dispatcher::guiSettings, this, &Application::guiSettings, Qt::DirectConnection);
        QObject::connect(Ui::GetDispatcher(), &Ui::core_dispatcher::coreLogins, this, &Application::coreLogins, Qt::DirectConnection);

        if (_cmdParser.isUrlCommand())
            urlCommand_ = _cmdParser.getUrlCommand();

        QAccessible::installFactory(Ui::Accessibility::customWidgetsAccessibilityFactory);

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
        const auto lockName = config::get().string(config::values::main_instance_mutex_linux);
        fd = open(ql1s("/tmp/%1.pid").arg(QString::fromUtf8(lockName.data(), lockName.size())).toStdString().c_str(), O_CREAT | O_RDWR, 0666);
        if (fd == -1)
            return true;

        fl.l_type   = F_WRLCK;
        fl.l_whence = SEEK_SET;
        fl.l_start  = 0;
        fl.l_len    = 0;
        fl.l_pid    = QCoreApplication::applicationPid();

        if (fcntl(fd, F_SETLK, &fl) == -1)
        {
            if (errno == EACCES || errno == EAGAIN)
                 return false;
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

    void Application::coreLogins(const bool _hasValidLogin, const bool _locked, const QString& _validOrFirstLogin)
    {
        if (Ui::get_gui_settings()->get_value<bool>(login_page_need_fill_profile, false))
        {
            Ui::get_gui_settings()->set_value(settings_feedback_email, QString());
            Ui::GetDispatcher()->post_message_to_core("logout", nullptr);
            Q_EMIT Utils::InterConnector::instance().logout();
        }

        initMainWindow(_hasValidLogin, _locked, _validOrFirstLogin);
    }

    void Application::guiSettings()
    {
        Ui::convertOldDownloadPath();
    }

    void Application::initMainWindow(const bool _hasValidLogin, const bool _locked, const QString& _validOrFirstLogin)
    {
        Logic::GetFriendlyContainer();
        Logic::GetLastseenContainer();

        double dpi = app_->primaryScreen()->logicalDotsPerInchX();

        if constexpr (platform::is_apple())
        {
            dpi = 96;
            Utils::set_mac_retina(qFuzzyCompare(app_->devicePixelRatio(), 2.0));
            if (Utils::is_mac_retina())
                app_->setAttribute(Qt::AA_UseHighDpiPixmaps);
        }

        const auto guiScaleCoefficient = std::min(dpi / 96.0, 3.0);

        Utils::initBasicScaleCoefficient(guiScaleCoefficient);

        Utils::setScaleCoefficient(Ui::get_gui_settings()->get_value<double>(settings_scale_coefficient, Utils::getBasicScaleCoefficient()));

        Emoji::InitializeSubsystem();

        Ui::get_gui_settings()->set_shadow_width(Utils::scale_value(SHADOW_WIDTH));

        Utils::GetTranslator()->init();

        bool needToShow = true;
        bool isAutoStart = false;

#ifdef _WIN32
        const auto arguments = app_->arguments();
        const auto isStartMinimized = Ui::get_gui_settings()->get_value<bool>(settings_start_minimazed, true);

        for (const auto& argument : arguments)
        {
            if (argument == arg_start_client_minimized() && isStartMinimized)
            {
                isAutoStart = true;
                needToShow = false;
                break;
            }
        }
#endif //_WIN32

        mainWindow_ = std::make_unique<Ui::MainWindow>(app_.get(), _hasValidLogin, _locked, _validOrFirstLogin);
        mainWindow_->init(needToShow);

        Ui::GetDispatcher()->updateRecentAvatarsSize();

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

    void Application::initFonts()
    {
        addFonts(
            qsl(":/fonts/SourceSansPro_Light")
          , qsl(":/fonts/SourceSansPro_Regular")
          , qsl(":/fonts/SourceSansPro_SemiBold")
          , qsl(":/fonts/SourceSansPro_Bold")
          , qsl(":/fonts/RoundedMplus_Bold")
        );

        if constexpr (platform::is_apple())
        {
            addFonts(
                qsl(":/fonts/SFProText_Regular")
              , qsl(":/fonts/SFProText_Medium")
              , qsl(":/fonts/SFProText_SemiBold")
              , qsl(":/fonts/SFProText_Bold")
              , qsl(":/fonts/SFProText_RegularItalic")
              , qsl(":/fonts/SFProText_MediumItalic")
              , qsl(":/fonts/SFProText_SemiBoldItalic")
              , qsl(":/fonts/SFProText_BoldItalic")
              , qsl(":/fonts/SFMono_Medium")
              , qsl(":/fonts/SFMono_MediumItalic")
              , qsl(":/fonts/SFMono_Bold")
              , qsl(":/fonts/SFMono_BoldItalic")
            );
        }
        else if constexpr (platform::is_windows())
        {
            addFonts(
                qsl(":/fonts/RobotoMono_Regular")
              , qsl(":/fonts/RobotoMono_Medium")
              , qsl(":/fonts/RobotoMono_Bold")
              , qsl(":/fonts/RobotoMono_Italic")
              , qsl(":/fonts/RobotoMono_MediumItalic")
              , qsl(":/fonts/RobotoMono_BoldItalic")
            );
        }
        app_->setFont(Fonts::appFontFamilyNameQss(Fonts::defaultAppFontFamily(), Fonts::FontWeight::Normal));
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

        if (!Utils::InterConnector::instance().getMessengerPage())
        {
            activate();
            return;
        }

        Ui::DeferredCallback action;
        if (auto command = UrlCommandBuilder::makeCommand(_urlCommand, UrlCommandBuilder::Context::External); command->isValid())
        {
            action = [activate, command = std::shared_ptr<UrlCommand>(std::move(command))]()
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

    bool Application::updating(MainWindowMode _mode)
    {
#ifdef _WIN32
        const auto updater_singlton_mutex_name = config::get().string(config::values::updater_main_instance_mutex_win);

        CHandle mutex(::CreateSemaphoreA(NULL, 0, 1, std::string(updater_singlton_mutex_name).c_str()));
        if (ERROR_ALREADY_EXISTS == ::GetLastError())
            return true;

        QSettings s(u"HKEY_CURRENT_USER\\Software\\" % getProductName(), QSettings::NativeFormat);
        const auto versionUpdate = s.value(qsl("update_version")).toString();
        if (versionUpdate.isEmpty())
            return false;

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

            const QString updateFolder = dir.absolutePath() % u'/' % updates_folder_short() % u'/' % versionUpdate;

            QDir updateDir(updateFolder);
            if (updateDir.exists())
            {
                const QString setupName = updateFolder % u'/' % Utils::getInstallerName();
                if (!QFileInfo::exists(setupName))
                    return false;

                mutex.Close();

                const QString command = u'"' % QDir::toNativeSeparators(setupName) % u'"';
                QStringList args = { update_final_command().toString() };

                if (_mode == MainWindowMode::Minimized)
                    args.push_back(installer_start_client_minimized().toString());

                QProcess::startDetached(command, args);

                return true;
            }
        }
#endif //_WIN32

        return false;
    }

    void Application::applicationStateChanged(Qt::ApplicationState _state)
    {
        if (platform::is_apple() && _state == Qt::ApplicationInactive)
        {
            Q_EMIT Utils::InterConnector::instance().closeAnyPopupMenu();
            Q_EMIT Utils::InterConnector::instance().closeAnyPopupWindow(Utils::CloseWindowInfo());
        }
    }
}
