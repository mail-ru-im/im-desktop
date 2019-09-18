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

class MacTitlebar;
class MacMenuBlocker;

class MacSupport
{
friend class MacMenuBlocker;

private:
    QMenuBar *mainMenu_;
    std::vector<QMenu *> extendedMenus_;
    std::vector<QAction *> extendedActions_;
    std::unique_ptr<MacTitlebar> title_;

    QPointer<QMenu> editMenu_;
    QPointer<QMenu> windowMenu_;

public:
    MacSupport(Ui::MainWindow * mainWindow);
    virtual ~MacSupport();

    void enableMacUpdater();
    void enableMacCrashReport();
    void enableMacPreview(WId wid);

    void postCrashStatsIfNedded();

    MacTitlebar* windowTitle();

    void listenSleepAwakeEvents();

    void runMacUpdater();
    void cleanMacUpdater();

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

    static QString currentTheme();

    static QString settingsPath();

    static QString bundleName();

    static QString defaultDownloadsPath();

    static void log(QString logString);

    static void getPossibleStrings(const QString& text, std::vector<std::vector<QString>> & result, unsigned& _count);

    static bool nativeEventFilter(const QByteArray &data, void *message, long *result);

    static void replacePasteboard(const QString & text);

    void createMenuBar(bool simple);
    void updateMainMenu();

    void activateWindow(unsigned long long view = 0);

    void registerAppDelegate();

    static void showEmojiPanel();

    static void saveFileName(const QString &caption, const QString &dir, const QString &filter, std::function<void (const QString& _filename, const QString& _directory)> _callback, QString _ext, std::function<void ()> _cancel_callback = std::function<void ()>());

    static bool isDoNotDisturbOn();

    static void showNSPopUpMenuWindowLevel(QWidget *w);

    static void showNSModalPanelWindowLevel(QWidget *w);

    static void showOverAll(QWidget *w);

    static bool isMetalSupported();

private:
    void setupDockClickHandler();
};

class MacMenuBlocker
{
public:
    MacMenuBlocker();
    ~MacMenuBlocker();

    using MenuStateContainer = std::vector<std::pair<QAction*, bool /* old state */>>;

    MenuStateContainer& menuState();

private:
    MenuStateContainer macMenuState_;
    std::unique_ptr<QObject> receiver_;
};

