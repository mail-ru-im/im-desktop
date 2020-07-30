#pragma once

#ifdef __APPLE__
class MacSupport;
#endif
#include "../voip/secureCallWnd.h"
#include "sidebar/Sidebar.h"

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
    class TermsPrivacyWidget;
    class TrayIcon;
    class HistoryControlPage;
    class MainStackedWidget;
    class DialogPlayer;
    class MainWindow;
    class CustomButton;
    class CallQualityStatsMgr;
    class ConnectionStateWatcher;
    class LocalPINWidget;

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
            ByNotificationClick = 1,
            ByMailClick = 2,
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
        void showGDPRPage();
        void checkForUpdates();
        void checkForUpdatesInBackground();
        void installUpdates();
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

        void openStatusPicker();

    public:
        MainWindow(QApplication* _app, const bool _hasValidLogin, const bool _locked, const QString& _validOrFirstLogin);
        ~MainWindow();

        void init(bool _needToShow);

        void openGallery(const QString &_aimId, const QString &_link, int64_t _msgId, DialogPlayer* _attachedPlayer = nullptr);
        void showHideGallery();
        void openAvatar(const QString &_aimId);
        void closeGallery();

        bool isActive() const;
        bool isUIActive() const;

        //! Same as isUIActive but considering only MainWindow
        bool isUIActiveMainWindow() const;

        bool isMainPage() const;

        int getScreen() const;
        QRect screenGeometry() const;
        QRect screenAvailableGeometry() const;
        QRect availableVirtualGeometry() const;

        int getMinWindowWidth();
        int getMinWindowHeight();
        QRect getDefaultWindowSize();

        void skipRead(); //skip next sending last read by window activation

        void closePopups(const Utils::CloseWindowInfo&);

        HistoryControlPage* getHistoryPage(const QString& _aimId) const;
        MainPage* getMainPage() const;
        QLabel* getWindowLogo() const;

        void showSidebar(const QString& _aimId);
        void showSidebarWithParams(const QString& _aimId, SidebarParams _params);
        void showMembersInSidebar(const QString& _aimId);
        void setSidebarVisible(const Utils::SidebarVisibilityParams& _params);
        bool isSidebarVisible() const;
        void restoreSidebar();

        void showMenuBarIcon(bool _show);
        void setFocusOnInput();
        void onSendMessage(const QString&);

        int getTitleHeight() const;
        bool isMaximized() const;

        void resize(int w, int h);

        QWidget* getWidget() const;

        void lock();
        void minimizeOnStartup();

        void updateWindowTitle();

    private:
        void initSizes();
        void initSettings(const bool _do_not_show);
        void initStats();
        void showMaximized();
        void showNormal();
        void showMinimized();
        void updateState();
        void notifyWindowActive(const bool _active);
        void userActivity();
        void updateMaximizeButton(bool _isMaximized);
        void needChangeState();

        void resetMainPage();

        void saveWindowGeometryAndState(QSize size);

        void sendStatus() const;
        void onEscPressed(const QString& _aimId, const bool _autorepeat);

    protected:
        bool nativeEventFilter(const QByteArray&, void* _message, long* _result) override;
        bool eventFilter(QObject *, QEvent *) override;

        void enterEvent(QEvent* _event) override;
        void resizeEvent(QResizeEvent* _event) override;
        void moveEvent(QMoveEvent* _event) override;
        void changeEvent(QEvent* _event) override;
        void closeEvent(QCloseEvent* _event) override;
        void hideEvent(QHideEvent* _event) override;
        void keyPressEvent(QKeyEvent* _event) override;
        void mouseReleaseEvent(QMouseEvent* _event) override;
        void showEvent(QShowEvent* _event) override;
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
        TermsPrivacyWidget* gdprPage_;
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
        ConnectionStateWatcher* connStateWatcher_;

        bool SkipRead_;
        bool TaskBarIconHidden_;
        bool isMaximized_;
        bool sleeping_;

        bool inCtor_ = true;

        QTimer* userActivityTimer_;
        QTimer* updateWhenUserInactiveTimer_;

#ifdef _WIN32
        HWND fake_parent_window_;
        bool isAeroEnabled_;
#endif //_WIN32
#ifdef __APPLE__
        MacSupport* getMacSupport();
#endif
        bool windowActive_;
        bool uiActive_;
        bool uiActiveMainWindow_;
        bool maximizeLater_;
        bool minimizeOnStartup_;

        ShowingAgreementInfo showingAgreementInfo_;
        LocalPINWidget* localPINWidget_;
    };
}
