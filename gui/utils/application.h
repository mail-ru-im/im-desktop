#pragma once

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
        HANDLE Mutex_;
        bool Exist_;
    };
#endif //_WIN32

    class Application : public QObject
    {
        Q_OBJECT

    private Q_SLOTS:
        void applicationStateChanged(Qt::ApplicationState state);
        void coreLogins(const bool _has_valid_login, const bool _locked);
        void guiSettings();

        void receiveUrlCommand(const QString& _urlCommand);
        void onHwndReceived(const unsigned int _hwnd);
        void getHwndAndActivate();

    public:
        Application(int& _argc, char* _argv[]);
        ~Application();

        int exec();
        bool init(launch::CommandLineParser& _cmdParser);

        bool isMainInstance();
        int switchInstance(launch::CommandLineParser& _cmdParser);

        static bool updating();
        void parseUrlCommand(const QString& _urlCommand);

    private:
        void initMainWindow(const bool _has_valid_login, const bool _locked);
        void init_win7_features();

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
