#pragma once

#ifdef __APPLE__
class MacSupport;
class MacMigrationManager;
#endif
#include "../voip/secureCallWnd.h"

class QApplication;
class QDockWidget;

namespace Data
{
    struct Image;
}

namespace Previewer
{
    class GalleryWidget;
}

namespace Utils
{
    struct ProxySettings;
    struct CloseWindowInfo;
    struct SidebarVisibilityParams;

    namespace AddContactDialogs
    {
        struct Initiator;
    }
}

namespace Ui
{
    class MainPage;
    class LoginPage;
#ifdef __APPLE__
    class AccountsPage;
#endif
    class TrayIcon;
    class HistoryControlPage;
    class MainStackedWidget;
    class DialogPlayer;
    class MainWindow;
    class CustomButton;
    class CallQualityStatsMgr;
    class ConnectionStateWatcher;
    class LocalPINWidget;
#ifdef _WIN32
    class WinNativeWindow;
#endif
    void memberAddFailed(const int _error);

    class ShadowWindow : public QWidget
    {
        Q_OBJECT

    public:

        ShadowWindow(const int _shadowWidth);
        void setActive(const bool _value);

    protected:
        void paintEvent(QPaintEvent* _e) override;

    private:
        void setGradientColor(QGradient& _gradient);

    private:
        int ShadowWidth_;
        bool IsActive_;
    };

    class TitleWidgetEventFilter : public QObject
    {
        Q_OBJECT

    Q_SIGNALS:
        void doubleClick();
        void logoDoubleClick();
        void moveRequest(QPoint);
        void checkPosition();

    public:
        TitleWidgetEventFilter(QObject* _parent);
        void addIgnoredWidget(QWidget* _widget);

    private:
        void showContextMenu(QPoint _pos);
        void showLogoContextMenu();

    protected:
        bool eventFilter(QObject* _obj, QEvent* _event) override;

    private:
        bool dragging_;
        QPoint clickPos_;
        MainWindow* mainWindow_;
        std::set<QWidget*> ignoredWidgets_;
        QWidget* windowIcon_;
        QTimer* iconTimer_;
    };

    class MainWindow : public QMainWindow, QAbstractNativeEventFilter
    {
        Q_OBJECT

    public:
        enum class ActivateReason
        {
            Unknown = 0,
            ByNotificationClick = 1
        };

    Q_SIGNALS:
        void needActivate();
        void needActivateWith(ActivateReason _reason, QPrivateSignal);
        void upKeyPressed(QPrivateSignal);
        void downKeyPressed(QPrivateSignal);
        void enterKeyPressed(QPrivateSignal);
        void mouseReleased(QPrivateSignal);
        void mousePressed(QPrivateSignal);

        void windowHide(QPrivateSignal);
        void windowClose(QPrivateSignal);

        void galeryClosed();

    public Q_SLOTS:
        void showLoginPage(const bool _is_auth_error);
        void showMainPage();
        void showMigrateAccountPage(const QString& _accountId);
        void checkForUpdates();
        void showIconInTaskbar(bool);
        void activateWithReason(ActivateReason _reason);
        void activate();
        void bringToFront();
        void activateFromEventLoop(ActivateReason _reason = ActivateReason::Unknown);
        void updateMainMenu();
        void exit();

        void maximize();
        void moveRequest(QPoint);
        void minimize();
        void guiSettingsChanged(const QString&);
        void onVoipResetComplete();
        void hideWindow();
        void copy();
        void cut();
        void paste();
        void undo();
        void redo();
        void activateAbout();
        void activateProfile();
        void activateSettings();
        void closeCurrent();
        void activateNextUnread();
        void activateNextChat();
        void activatePrevChat();
        void toggleFullScreen();
        void pasteEmoji();
        void checkPosition();

        void onOpenChat(const std::string& _contact);
        void onShowVideoWindow();

        void checkNewVersion();
        void stateChanged(const Qt::WindowState& _state);
        void userActivityTimer();
        void onShowAddContactDialog(const Utils::AddContactDialogs::Initiator& _initiator);
        void gotoSleep();
        void gotoWakeup();

        void activateShortcutsClose();

        void onThemeChanged();
        void zoomWindow();

    public:
        MainWindow(QApplication* _app, const bool _has_valid_login, const bool _locked);
        ~MainWindow();

        void show();

        void openGallery(const QString &_aimId, const QString &_link, int64_t _msgId, DialogPlayer* _attachedPlayer = nullptr);
        void showHideGallery();
        void openAvatar(const QString &_aimId);
        void closeGallery();

        bool isActive() const;
        bool isUIActive() const;

        bool isMainPage() const;

        int getScreen() const;
        QRect screenGeometry() const;
        void skipRead(); //skip next sending last read by window activation

        void closePopups(const Utils::CloseWindowInfo&);

        HistoryControlPage* getHistoryPage(const QString& _aimId) const;
        MainPage* getMainPage() const;
        QLabel* getWindowLogo() const;

        void showSidebar(const QString& _aimId);
        void showMembersInSidebar(const QString& _aimId);
        void setSidebarVisible(const Utils::SidebarVisibilityParams& _params);
        bool isSidebarVisible() const;
        void restoreSidebar();

        void showMenuBarIcon(bool _show);
        void setFocusOnInput();
        void onSendMessage(const QString&);

        int getTitleHeight() const;
        bool isMaximized() const;
        bool isMinimized() const;

        void resize(int w, int h);

        QWidget* getWidget();

        bool hasMemoryDockWidget() const;
        void addMemoryDockWidget(Qt::DockWidgetArea _area, QDockWidget *_dockWidget);
        bool removeMemoryDockWidget();

        void lock();

#ifdef _WIN32
        HWND getParentWindow() const;
        void hideParentWindow();

        std::unique_ptr<WinNativeWindow> takeNativeWindow();
#endif
        QRect nativeGeometry() const;

    private:
        void initSizes();
        void initSettings();
        void initStats();
        void showMaximized();
        void showNormal(bool _activate = true);
        void updateState();
        void upgradeStuff();
        void notifyWindowActive(const bool _active);
        void userActivity();
        void initGDPR();
        void updateMaximizeButton(bool _isMaximized);
        void needChangeState();

        void resetMainPage();
        void saveWindowSize(QSize size);

        void activateMainWindow(bool _activate = true);

    protected:
        bool nativeEventFilter(const QByteArray&, void* _message, long* _result) override;
        bool eventFilter(QObject *, QEvent *) override;

        void customEvent(QEvent* _event) override;
        void enterEvent(QEvent* _event) override;
        void resizeEvent(QResizeEvent* _event) override;
        void moveEvent(QMoveEvent* _event) override;
        void changeEvent(QEvent* _event) override;
        void closeEvent(QCloseEvent* _event) override;
        void hideEvent(QHideEvent* _event) override;
        void keyPressEvent(QKeyEvent* _event) override;
        void mouseReleaseEvent(QMouseEvent* _event) override;
        void hideTaskbarIcon();
        void showTaskbarIcon();
        void clear_global_objects();

    private:
        struct ShowingAgreementInfo
        {
            bool amShowingGDPR_ = false;
        };

    private:
        QPointer<Previewer::GalleryWidget> gallery_;
        MainPage* mainPage_;
        LoginPage* loginPage_;
        QApplication* app_;
        TitleWidgetEventFilter* eventFilter_;
        TrayIcon* trayIcon_;
        QPixmap backgroundPixmap_;
        QWidget *mainWidget_;
        QVBoxLayout *mainLayout_;
        QWidget *titleWidget_;
        QHBoxLayout *titleLayout_;
        QLabel *logo_;
        QLabel *title_;
        QSpacerItem *spacer_;
        CustomButton *hideButton_;
        CustomButton *maximizeButton_;
        CustomButton *closeButton_;
        MainStackedWidget *stackedWidget_;
        ShadowWindow* Shadow_;
        DialogPlayer* ffplayer_;
        CallQualityStatsMgr* callStatsMgr_;
        QDockWidget* memoryDockWidget_;
        ConnectionStateWatcher* connStateWatcher_;

        bool SkipRead_;
        bool TaskBarIconHidden_;
        bool isMaximized_;
        bool sleeping_;

        QTimer* userActivityTimer_;

#ifdef _WIN32
        HWND fake_parent_window_;

        std::unique_ptr<WinNativeWindow> parentNativeWindow_;
        HWND parentNativeWindowHandle_;
        bool ignoreActivateEvent_;
#endif //_WIN32
#ifdef __APPLE__
        MacSupport* getMacSupport();
        AccountsPage* accounts_page_;
        MacMigrationManager* migrationManager_;
#endif
        bool windowActive_;
        bool uiActive_;

        ShowingAgreementInfo showingAgreementInfo_;
        LocalPINWidget* localPINWidget_;
    };
}
