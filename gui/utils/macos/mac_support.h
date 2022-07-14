//
//  mac_support.h
//  ICQ
//
//  Created by Vladimir Kubyshev on 03/12/15.
//  Copyright © 2015 Mail.RU. All rights reserved.
//

#pragma once

namespace Ui
{
    class MainWindow;
}

namespace Utils
{
    void enableCloseButton(QWidget* _w, bool _on);
    void disableZoomButton(QWidget* _w);
    int getTitlebarHeight(QWidget* _w);

    // window can be shown in the same space with the full-screen window
    void setFullScreenAuxiliaryBehavior(QWidget* _w);
}

class MacTitlebar;
class MacMenuBlocker;

class MacSupport
{
friend class MacMenuBlocker;

private:
    QMenuBar *mainMenu_;
    std::vector<QPointer<QMenu>> extendedMenus_;
    std::vector<QPointer<QAction>> extendedActions_;
    std::unique_ptr<MacTitlebar> title_;

    QPointer<QMenu> editMenu_;
    QPointer<QMenu> windowMenu_;

    bool updateRequested_;

public:
    MacSupport(Ui::MainWindow * mainWindow);
    virtual ~MacSupport();

    void enableMacUpdater();
    void enableMacPreview(WId wid);

    MacTitlebar* windowTitle();

    void listenSystemEvents();

    void checkForUpdates();
    void checkForUpdatesInBackground();
    void runMacUpdater();
    void installUpdates();
    void cleanMacUpdater();
    bool isUpdateRequested() const { return updateRequested_; }

    void forceEnglishInputSource();

    static void minimizeWindow(WId wid);
    static void zoomWindow(WId wid);

    static void closeWindow(WId wid);
    static bool isFullScreen();
    static void toggleFullScreen();
    static void showPreview(QString previewPath, QSize imageSize);
    static void showPreview(QString previewPath, int x, int y);
    static bool previewIsShown();
    static void openFinder(QString previewPath);
    static void openFile(QString path);
    static void openLink(QString link);

    static QString currentRegion();

    enum class ThemeType
    {
        Dark,
        White
    };

    static ThemeType currentTheme();

    static ThemeType currentThemeForTray();

    static QString themeTypeToString(ThemeType);

    static bool isBigSurOrGreater();

    static QString settingsPath();

    static QString bundleName();
    static QString bundleDir();

    static QString defaultDownloadsPath();

    static void log(QString logString);

    static void getPossibleStrings(const QString& text, std::vector<std::vector<QString>> & result, unsigned& _count);

    static std::vector<QString> getKeyboardLanguages();

    static bool nativeEventFilter(const QByteArray &data, void *message, long *result);

    void createMenuBar(bool simple);
    void updateMainMenu();

    void activateWindow(unsigned long long view = 0);

    void registerAppDelegate();

    static void showEmojiPanel();

    static void saveFileName(const QString &caption, const QString &dir, const QString &filter, std::function<void (const QString& _filename, const QString& _directory, bool _exportAsPng)> _callback, QString _ext, std::function<void ()> _cancel_callback = std::function<void ()>());

    static bool isDoNotDisturbOn();

    static void showNSPopUpMenuWindowLevel(QWidget *w);

    static void showNSModalPanelWindowLevel(QWidget *w);

    static void showNSFloatingWindowLevel(QWidget *w);

    enum class WindowOrder
    {
        None,
        FrontRegardless
    };
    static void showOverAll(QWidget* _w, WindowOrder _order = WindowOrder::None);

    static void showInAllWorkspaces(QWidget *w);

    static bool isMetalSupported();

    static int getWidgetHeaderHeight(const QWidget& widget);

    static QRectF screenGeometryByPoint(const QPoint& _point);

private:
    void setupDockClickHandler();
    bool setFeedUrl();

    std::unique_ptr<QTimer> updateFeedUrl_;
};

class MacMenuBlocker
{
public:
    MacMenuBlocker();
    ~MacMenuBlocker();

    void block();
    void unblock();

    using MenuStateContainer = std::vector<std::pair<QPointer<QAction>, bool /* old state */>>;

private:
    MenuStateContainer macMenuState_;
    std::unique_ptr<QObject> receiver_;
    bool isBlocked_;
};


