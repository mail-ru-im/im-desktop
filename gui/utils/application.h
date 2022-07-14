#pragma once

#include "../main_window/contact_list/Common.h"

namespace Ui
{
    class MainWindow;
}

namespace launch
{
    class CommandLineParser;
}

namespace Utils
{
    class LocalPeer;
    class ServiceUrlsManager;
    class NativeEventFilter;

#ifdef _WIN32
    class AppGuard
    {
    public:
        AppGuard();
        ~AppGuard();

        bool succeeded() const;

    private:
        const HANDLE mutex_;
        const bool exist_;
    };
#endif //_WIN32

    class Application : public QObject
    {
        Q_OBJECT

    private Q_SLOTS:
        void applicationStateChanged(Qt::ApplicationState _state);
        void coreLogins(const bool _hasValidLogin, const bool _locked, const QString& _validOrFirstLogin);
        void guiSettings();

        void receiveUrlCommand(const QString& _urlCommand);
        void onHwndReceived(const unsigned int _hwnd);
        void getHwndAndActivate();

    public:
        Application(int& _argc, char* _argv[]);
        ~Application();

        int exec();
        bool init(const launch::CommandLineParser& _cmdParser);

        bool isMainInstance();
        int switchInstance(const launch::CommandLineParser& _cmdParser);

        enum class MainWindowMode
        {
            Normal,
            Minimized
        };

        static bool updating(MainWindowMode _mode);
        void parseUrlCommand(const QString& _urlCommand);

    private:
        void initMainWindow(const bool _hasValidLogin, const bool _locked, const QString& _validOrFirstLogin);
        void init_win7_features();
        void initFonts();

        std::unique_ptr<Ui::MainWindow> mainWindow_;
        std::unique_ptr<LocalPeer> peer_;
        std::unique_ptr<QApplication> app_;
        std::unique_ptr<ServiceUrlsManager> serviceMgr_;
        std::unique_ptr<NativeEventFilter> nativeEvFilter_;
        QString urlCommand_;

#ifdef _WIN32
        std::unique_ptr<AppGuard> guard_;
#endif //_WIN32
    };
}
