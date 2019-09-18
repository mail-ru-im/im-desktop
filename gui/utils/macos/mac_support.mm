//
//  mac_updater.m
//  ICQ
//
//  Created by Vladimir Kubyshev on 03/12/15.
//  Copyright © 2015 Mail.RU. All rights reserved.
//

#import <Cocoa/Cocoa.h>
#import <Carbon/Carbon.h>
#import <Foundation/Foundation.h>
#import <Sparkle/Sparkle.h>
#import <HockeySDK/HockeySDK.h>
#import <Quartz/Quartz.h>
#import <MetalKit/MetalKit.h>

#include "stdafx.h"
#include "../../main_window/MainWindow.h"
#include "../../main_window/MainPage.h"
#include "../../main_window/ContactDialog.h"
#include "../../main_window/contact_list/ContactListModel.h"
#include "../../main_window/contact_list/RecentsModel.h"

#include "../utils.h"
#include "../launch.h"
#include "../InterConnector.h"

#include "main_window/contact_list/AddContactDialogs.h"

#include "../gui/utils/gui_metrics.h"
#include "../gui/gui_settings.h"

#include "core_dispatcher.h"

#import "mac_support.h"
#import "mac_translations.h"
#import "mac_titlebar.h"

#include <objc/objc.h>
#include <objc/message.h>
#include <objc/runtime.h>

#import <SystemConfiguration/SystemConfiguration.h>

enum
{
    edit_undo = 100,
    edit_redo,
    edit_cut,
    edit_copy,
    edit_paste,
    edit_pasteAsQuote,
    edit_pasteEmoji,
    contact_addBuddy,
    contact_profile,
    contact_close,
    view_fullScreen,
    view_unreadMessage,
    window_nextChat,
    window_prevChat,
    window_minimizeWindow,
    window_maximizeWindow,
    window_mainWindow,
    window_bringToFront,
    window_switchMainWindow,
    window_switchVoipWindow,
    global_about,
    global_settings,
    global_update,
    // TODO: add other items when needed.
};


static QMap<int, QAction *> menuItems_;
static Ui::MainWindow * mainWindow_ = nil;
static auto closeAct_ = Ui::ShortcutsCloseAction::Default;

static QString toQString(NSString * src)
{
    return QString::fromCFString((__bridge CFStringRef)src);
}

static NSString * fromQString(const QString & src)
{
    return (NSString *)CFBridgingRelease(src.toCFString());
}

@interface AppDelegate: NSObject<NSApplicationDelegate>
- (void)handleURLEvent:(NSAppleEventDescriptor *)event withReplyEvent:(NSAppleEventDescriptor *)replyEvent;
//- (void)applicationDidFinishLaunching:(NSNotification *)notification;
- (NSApplicationTerminateReply)applicationShouldTerminate:(NSApplication *)sender;
@end

@implementation AppDelegate

- (void)handleURLEvent:(NSAppleEventDescriptor*)event withReplyEvent:(NSAppleEventDescriptor*)replyEvent
{
    NSString* url = [[event paramDescriptorForKeyword:keyDirectObject]
                     stringValue];

    emit Utils::InterConnector::instance().schemeUrlClicked(QString::fromNSString(url));
}

//- (void)applicationDidFinishLaunching:(NSNotification *)notification
//{
////    auto args = [[NSProcessInfo processInfo] arguments];
////    char *argv[(const int)args.count];
////    int i = 0;
////    for (NSString *arg: args)
////    {
////        argv[i++] = const_cast<char *>(arg.UTF8String);
////    }
////    launch::main((int)args.count, &argv[0]);

////    launch::main()

//    NSAppleEventDescriptor* event = [[NSAppleEventManager sharedAppleEventManager] currentAppleEvent];
//    if ([event eventID] == kAEOpenApplication &&
//        [[event paramDescriptorForKeyword:keyAEPropData] enumCodeValue] == keyAELaunchedAsLogInItem)
//    {
//        // you were launched as a login item
//        statistic::getGuiMetrics().eventAppStartAutostart();
//    }
//}

- (NSApplicationTerminateReply)applicationShouldTerminate:(NSApplication *)sender
{
    if (mainWindow_ != nil)
    {
        mainWindow_->exit();
    }
    return NSTerminateCancel;
}

@end

@interface LinkPreviewItem : NSObject <QLPreviewItem>
@property (readonly) NSURL *previewItemURL;
@property (readonly) NSString *previewItemURLString;
@property (readonly) NSString *previewItemTitle;
@property (readonly) CGPoint point;

- (id)initWithPath:(NSString *)path andPoint:(CGPoint)point;

@end

@implementation LinkPreviewItem

- (id)initWithPath:(NSString *)path andPoint:(CGPoint)point
{
    self = [super init];
    if (self)
    {
        if ([[NSFileManager defaultManager] fileExistsAtPath:path])
        {
            _previewItemURL = [NSURL fileURLWithPath:path];
        }
        else
        {
            _previewItemURL = [NSURL URLWithString:path];
        }
        _previewItemURLString = [[NSString alloc] initWithString:path];
        _previewItemTitle = [[NSString alloc] initWithString:_previewItemURL.lastPathComponent];
        _point = point;
    }
    return self;
}

- (void)dealloc
{
    [_previewItemURL release];
    _previewItemURL = nil;

    [_previewItemURLString release];
    _previewItemURLString = nil;

    [_previewItemTitle release];
    _previewItemTitle = nil;

    [super dealloc];
}

@end

@interface MacPreviewProxy : NSViewController<QLPreviewPanelDelegate, QLPreviewPanelDataSource>

@property (strong) LinkPreviewItem *previewItem;

- (void)showPreviewPopupWithPath:(NSString *)path atPos:(CGPoint)point;

@end

@implementation MacPreviewProxy

- (instancetype)initInWindow:(NSWindow *)window
{
    if (self = [super init])
    {
        self.view = [[[NSView alloc] initWithFrame:NSMakeRect(0, 0, 1, 1)] autorelease];

        [window.contentView addSubview:self.view];

        NSResponder * aNextResponder = [window nextResponder];

        [window setNextResponder:self];
        [self setNextResponder:aNextResponder];

//        [QLPreviewPanel sharedPreviewPanel].delegate = self;
    }

    return self;
}

- (BOOL)hidePreviewPopup
{
    if ([QLPreviewPanel sharedPreviewPanelExists] && [[QLPreviewPanel sharedPreviewPanel] isVisible])
    {
        [[QLPreviewPanel sharedPreviewPanel] orderOut:nil];
        self.previewItem = nil;

        return YES;
    }
    return NO;
}

- (void)showPreviewPopupWithPath:(NSString *)path atPos:(CGPoint)point
{
    if (![self hidePreviewPopup])
    {
        self.previewItem = [[LinkPreviewItem alloc] initWithPath:path andPoint:point];
        [[QLPreviewPanel sharedPreviewPanel] makeKeyAndOrderFront:nil];
    }
}

- (void)beginPreviewPanelControl:(QLPreviewPanel *)panel
{
    panel.delegate = self;
    panel.dataSource = self;
}

- (BOOL)acceptsPreviewPanelControl:(QLPreviewPanel *)panel;
{
    return YES;
}

- (NSInteger)numberOfPreviewItemsInPreviewPanel:(QLPreviewPanel *)panel
{
    return (self.previewItem) ? 1 : 0;
}

- (CGRect)previewPanel:(QLPreviewPanel *)panel sourceFrameOnScreenForPreviewItem:(id<QLPreviewItem>)item
{
    LinkPreviewItem * link = item;

    CGPoint point = link.point;

    if (link.point.x == -1 ||
        link.point.y == -1)
    {
        return NSZeroRect;
    }

    CGRect rect = CGRectMake(0, 0, 30, 20);
    rect.origin = point;

    return rect;
}

- (id <QLPreviewItem>)previewPanel:(QLPreviewPanel *)panel previewItemAtIndex:(NSInteger)index
{
    return self.previewItem;
}

- (void)endPreviewPanelControl:(QLPreviewPanel *)panel
{
    panel.delegate = nil;
    panel.dataSource = nil;

    self.previewItem = nil;
}

- (void)windowDidBecomeKey:(NSNotification *)notification
{
    if (!self.previewItem)
    {
        [self hidePreviewPopup];
    }
}

@end

// Events catcher

BOOL isNetworkAvailable()
{
    BOOL connected;
    BOOL isConnected;
    const char *host = "www.icq.com";
    SCNetworkReachabilityRef reachability = SCNetworkReachabilityCreateWithName(NULL, host);
    SCNetworkReachabilityFlags flags;
    connected = SCNetworkReachabilityGetFlags(reachability, &flags);
    isConnected = NO;
    isConnected = connected && (flags & kSCNetworkFlagsReachable) && !(flags & kSCNetworkFlagsConnectionRequired);
    CFRelease(reachability);
    return isConnected;
}

@interface EventsCatcher : NSObject

@property (nonatomic, copy) void(^sleep)(void);
@property (nonatomic, copy) void(^wake)(void);

- (instancetype)initWithSleepLambda:(void(^)(void))sleep andWakeLambda:(void(^)(void))wake;
- (void)receiveSleepEvent:(NSNotification *)notification;
- (void)receiveWakeEvent:(NSNotification *)notification;

@end

@implementation EventsCatcher

- (instancetype)initWithSleepLambda:(void (^)())sleep andWakeLambda:(void (^)())wake
{
    self = [super init];
    if (self)
    {
        self.sleep = sleep;
        self.wake = wake;
        [[[NSWorkspace sharedWorkspace] notificationCenter] addObserver:self
                                                               selector:@selector(receiveSleepEvent:)
                                                                   name:NSWorkspaceWillSleepNotification object:nil];
        [[[NSWorkspace sharedWorkspace] notificationCenter] addObserver:self
                                                               selector:@selector(receiveWakeEvent:)
                                                                   name:NSWorkspaceDidWakeNotification object:nil];
        [[NSDistributedNotificationCenter defaultCenter] addObserver:self
                                                            selector:@selector(receiveSleepEvent:)
                                                                name:@"com.apple.screenIsLocked"
                                                              object:nil];
        [[NSDistributedNotificationCenter defaultCenter] addObserver:self
                                                            selector:@selector(receiveWakeEvent:)
                                                                name:@"com.apple.screenIsUnlocked"
                                                              object:nil];
        [[NSDistributedNotificationCenter defaultCenter] addObserver:self
                                                            selector:@selector(receiveSleepEvent:)
                                                                name:@"com.apple.sessionDidMoveOffConsole"
                                                              object:nil];
        [[NSDistributedNotificationCenter defaultCenter] addObserver:self
                                                            selector:@selector(receiveWakeEvent:)
                                                                name:@"com.apple.sessionDidMoveOnConsole"
                                                              object:nil];
    }
    return self;
}

- (void)receiveSleepEvent:(NSNotification *)notification
{
    statistic::getGuiMetrics().eventAppSleep();
    self.sleep();
}

- (void)receiveWakeEvent:(NSNotification *)notification
{
    dispatch_async(dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_DEFAULT, 0), ^{

        dispatch_async(dispatch_get_main_queue(), ^{
            statistic::getGuiMetrics().eventAppWakeFromSuspend();
        });

        Ui::GetDispatcher()->post_message_to_core("connection/reset", nullptr);

        dispatch_async(dispatch_get_main_queue(), ^{
            //
            self.wake();
            //
        });
        //
    });
}

@end

// force override method with custom method
void Swizzle(Class _c, SEL _orig, SEL _new)
{
    Method origMethod = class_getInstanceMethod(_c, _orig);
    Method newMethod = class_getInstanceMethod(_c, _new);
    if(class_addMethod(_c, _orig, method_getImplementation(newMethod), method_getTypeEncoding(newMethod)))
        class_replaceMethod(_c, _new, method_getImplementation(origMethod), method_getTypeEncoding(origMethod));
    else
        method_exchangeImplementations(origMethod, newMethod);
}

@interface NSApplication (NSFullKeyboardAccess)
- (BOOL)overrideIsFullKeyboardAccessEnabled;
@end

@implementation NSApplication (NSFullKeyboardAccess)
- (BOOL)overrideIsFullKeyboardAccessEnabled { return YES; }
@end

// ! Events catcher

static SUUpdater * sparkleUpdater_ = nil;
static MacPreviewProxy * macPreviewProxy_ = nil;

MacSupport::MacSupport(Ui::MainWindow * mainWindow):
    mainMenu_(nullptr),
    title_(std::make_unique<MacTitlebar>(mainWindow))
{
    sparkleUpdater_ = nil;
    mainWindow_ = mainWindow;
    registerAppDelegate();
    setupDockClickHandler();

    Swizzle(NSApplication.class, @selector(isFullKeyboardAccessEnabled), @selector(overrideIsFullKeyboardAccessEnabled));
}

MacSupport::~MacSupport()
{
    menuItems_.clear();

    mainWindow_ = nil;

    cleanMacUpdater();

    [macPreviewProxy_ hidePreviewPopup];
    [macPreviewProxy_ release];
    macPreviewProxy_ = nil;
}

void MacSupport::enableMacUpdater()
{
#ifdef UPDATES
    if (sparkleUpdater_ != nil)
        return;

    NSDictionary *plist = [[NSBundle mainBundle] infoDictionary];

    BOOL automaticChecks = [plist[@"SUEnableAutomaticChecks"] boolValue];
    NSURL * updateFeed = [NSURL URLWithString:plist[@"SUFeedURL"]];

    sparkleUpdater_ = [[SUUpdater alloc] init];

    [sparkleUpdater_ setAutomaticallyChecksForUpdates:automaticChecks];

    if constexpr (build::is_debug())
        updateFeed = [NSURL URLWithString:@"http://testmra.mail.ru/icq_mac_update/icq_update.xml"];

    if (updateFeed)
    {
        [sparkleUpdater_ setFeedURL:updateFeed];
        [sparkleUpdater_ setUpdateCheckInterval:86400];

//        [sparkleUpdater_ checkForUpdates:nil];
    }
#endif
}

void MacSupport::enableMacCrashReport()
{
    if constexpr (build::is_release())
    {
        [[BITHockeyManager sharedHockeyManager] configureWithIdentifier:HOCKEY_APPID];
        [BITHockeyManager sharedHockeyManager].disableMetricsManager = YES;
        [[BITHockeyManager sharedHockeyManager] startManager];
    }
}

void MacSupport::listenSleepAwakeEvents()
{
    static EventsCatcher *eventsCatcher = nil;
    static dispatch_once_t onceToken;
    dispatch_once(&onceToken, ^{
        //
        eventsCatcher = [[EventsCatcher alloc] initWithSleepLambda:^{

            if (mainWindow_)
            {
                mainWindow_->gotoSleep();
            }

        } andWakeLambda:^{
            if (mainWindow_)
            {
                mainWindow_->gotoWakeup();
            }
        }];
        //
    });
}

void MacSupport::runMacUpdater()
{
    if (sparkleUpdater_)
        [sparkleUpdater_ checkForUpdates:nil];
}

void MacSupport::cleanMacUpdater()
{
    [sparkleUpdater_ release];
    sparkleUpdater_ = nil;
}

void MacSupport::forceEnglishInputSource()
{
    NSArray *sources = CFBridgingRelease(TISCreateInputSourceList((__bridge CFDictionaryRef)@{ (__bridge NSString*)kTISPropertyInputSourceID:@"com.apple.keylayout.US" }, FALSE));
    TISInputSourceRef source = (__bridge TISInputSourceRef)sources[0];
    TISSelectInputSource(source);
}

void MacSupport::enableMacPreview(WId wid)
{
    void *pntr = (void *)wid;
    NSView *view = (__bridge NSView *)pntr;

    macPreviewProxy_ = [[MacPreviewProxy alloc] initInWindow:view.window];
}

void MacSupport::postCrashStatsIfNedded()
{
    if ([[BITHockeyManager sharedHockeyManager].crashManager didCrashInLastSession])
    {
        core::stats::event_props_type props;
        props.emplace_back("time", std::to_string(0));
        Ui::GetDispatcher()->post_im_stats_to_core(core::stats::im_stat_event_names::crash, props);
    }
}

MacTitlebar* MacSupport::windowTitle()
{
    return title_.get();
}

void MacSupport::minimizeWindow(WId wid)
{
    void *pntr = (void *)wid;
    NSView *view = (__bridge NSView *)pntr;
    [view.window miniaturize:nil];
}

void MacSupport::closeWindow(WId wid)
{
    void *pntr = (void *)wid;
    NSView *view = (__bridge NSView *)pntr;
    [view.window close];
}

bool MacSupport::isFullScreen()
{
    return (Utils::InterConnector::instance().getMainWindow() && Utils::InterConnector::instance().getMainWindow()->isFullScreen());
}

void MacSupport::toggleFullScreen()
{
    if (auto mainWindow = Utils::InterConnector::instance().getMainWindow())
    {
        emit Utils::InterConnector::instance().closeAnyPopupMenu();
        emit Utils::InterConnector::instance().closeAnyPopupWindow(Utils::CloseWindowInfo());
        void *pntr = (void *)mainWindow->winId();
        NSView *view = (__bridge NSView *)pntr;
        [view.window toggleFullScreen:nil];
    }
}

void MacSupport::showPreview(QString previewPath, QSize imageSize)
{
    showPreview(previewPath, -1, -1);
}

void MacSupport::showPreview(QString previewPath, int x, int y)
{
    if ([[[macPreviewProxy_ view] window] isKeyWindow])
    {
        NSString *path = fromQString(previewPath);
        if (y != -1)
        {
            y = [NSScreen mainScreen].visibleFrame.size.height - y;
        }
        [macPreviewProxy_ showPreviewPopupWithPath:path atPos:NSMakePoint(x, y)];
    }
}

bool MacSupport::previewIsShown()
{
    return ([QLPreviewPanel sharedPreviewPanelExists] && [[QLPreviewPanel sharedPreviewPanel] isVisible]);
}

void MacSupport::openFinder(QString previewPath)
{
    NSString * previewPath_ = fromQString(previewPath);
    NSURL * previewUrl = [NSURL fileURLWithPath:previewPath_];

    [[NSWorkspace sharedWorkspace] activateFileViewerSelectingURLs:@[previewUrl]];
}

void MacSupport::openFile(QString path)
{
    NSString *aFilePath = fromQString(path);
    [[NSWorkspace sharedWorkspace] openFile:aFilePath];
}

void MacSupport::openLink(QString link)
{
    NSString * nsLink = fromQString(link);

    NSURL * url;
    if ([nsLink.lowercaseString hasPrefix:@"http://"]) {
        url = [NSURL URLWithString:nsLink];
    } else {
        url = [NSURL URLWithString:[NSString stringWithFormat:@"http://%@",nsLink]];
    }

    [[NSWorkspace sharedWorkspace] openURL:url];
}

QString MacSupport::currentRegion()
{
    NSLocale * locale = [NSLocale currentLocale];

    return toQString(locale.localeIdentifier);
}

QString MacSupport::currentTheme()
{
    NSDictionary *dict = [[NSUserDefaults standardUserDefaults] persistentDomainForName:NSGlobalDomain];
    id style = [dict objectForKey:@"AppleInterfaceStyle"];
    BOOL darkModeOn = ( style && [style isKindOfClass:[NSString class]] && NSOrderedSame == [style caseInsensitiveCompare:@"dark"] );

    return darkModeOn ? qsl("black") : qsl("white");
}

QString MacSupport::settingsPath()
{
    NSArray *paths = NSSearchPathForDirectoriesInDomains(NSApplicationSupportDirectory, NSUserDomainMask, YES);
    NSString *applicationSupportDirectory = [paths firstObject];
    return toQString([applicationSupportDirectory stringByAppendingPathComponent:[NSString stringWithUTF8String:(build::is_icq() ? product_path_icq_a : product_path_agent_mac_a)]]);
}

QString MacSupport::bundleName()
{
    return QString::fromUtf8([[[NSBundle mainBundle] bundleIdentifier] UTF8String]);
}

QString MacSupport::defaultDownloadsPath()
{
    NSArray *paths = NSSearchPathForDirectoriesInDomains(NSLibraryDirectory, NSUserDomainMask, YES);
    NSString *path = [paths objectAtIndex:0];
    path = [path stringByAppendingPathComponent:[NSString stringWithUTF8String:(build::is_icq() ? product_path_icq_a : product_path_agent_mac_a)]];
    path = [path stringByAppendingPathComponent:@"Downloads"];
    return QString::fromUtf8([path UTF8String]);

    /*
    const auto bundleName = [[NSBundle mainBundle] bundleIdentifier];
    NSArray *paths = NSSearchPathForDirectoriesInDomains(NSLibraryDirectory, NSUserDomainMask, YES);
    NSString *path = [paths objectAtIndex:0];
    path = [path stringByAppendingPathComponent:@"Containers"];
    path = [path stringByAppendingPathComponent:bundleName];
    path = [path stringByAppendingPathComponent:@"Downloads"];
    return [path UTF8String];
    */
}

template<typename MenuT, typename Object, typename Method>
QAction * createAction(MenuT * menu, const QString& title, const QString& shortcut, const Object * receiver, Method method)
{
    return menu->addAction(title, receiver, std::move(method), QKeySequence(shortcut));
}

void MacSupport::createMenuBar(bool simple)
{
    if (!mainMenu_)
    {
        mainMenu_ = new QMenuBar();
        mainWindow_->setMenuBar(mainMenu_);

        QAction *action = nullptr;;

        auto menu = mainMenu_->addMenu(Utils::Translations::Get(qsl("&Edit")));
        menuItems_.insert(edit_undo, createAction(menu, Utils::Translations::Get(qsl("Undo")), qsl("Ctrl+Z"), mainWindow_, &Ui::MainWindow::undo));
        menuItems_.insert(edit_redo, createAction(menu, Utils::Translations::Get(qsl("Redo")), qsl("Shift+Ctrl+Z"), mainWindow_, &Ui::MainWindow::redo));
        menu->addSeparator();
        menuItems_.insert(edit_cut, createAction(menu, Utils::Translations::Get(qsl("Cut")), qsl("Ctrl+X"), mainWindow_, &Ui::MainWindow::cut));
        menuItems_.insert(edit_copy, createAction(menu, Utils::Translations::Get(qsl("Copy")), qsl("Ctrl+C"), mainWindow_, &Ui::MainWindow::copy));
        menuItems_.insert(edit_paste, createAction(menu, Utils::Translations::Get(qsl("Paste")), qsl("Ctrl+V"), mainWindow_, &Ui::MainWindow::paste));
        //menuItems_.insert(edit_pasteAsQuote, createAction(menu, Utils::Translations::Get("Paste as Quote"), qsl("Alt+Ctrl+V"), mainWindow_, &Ui::MainWindow::quote));
        menu->addSeparator();
        menuItems_.insert(edit_pasteEmoji, createAction(menu, Utils::Translations::Get(qsl("Emoji && Symbols")), qsl("Meta+Ctrl+Space"), mainWindow_, &Ui::MainWindow::pasteEmoji));
        extendedMenus_.push_back(menu);
        editMenu_ = menu;
        QObject::connect(menu, &QMenu::aboutToShow, menu, []() { emit Utils::InterConnector::instance().closeAnyPopupMenu(); });

        menu = mainMenu_->addMenu(Utils::Translations::Get(qsl("Contact")));

        if (!build::is_dit() && !build::is_biz())
        {
            menuItems_.insert(contact_addBuddy, createAction(menu, Utils::Translations::Get(qsl("Add Buddy")), qsl("Ctrl+N"), mainWindow_, [](){
                if (mainWindow_)
                    mainWindow_->onShowAddContactDialog({ Utils::AddContactDialogs::Initiator::From::HotKey });
            }));
        }
        
        menuItems_.insert(contact_profile, createAction(menu, Utils::Translations::Get(qsl("Profile")), qsl("Ctrl+I"), mainWindow_, &Ui::MainWindow::activateProfile));

        auto code = Ui::get_gui_settings()->get_value<int>(settings_shortcuts_close_action, static_cast<int>(Ui::ShortcutsCloseAction::Default));
        closeAct_ = static_cast<Ui::ShortcutsCloseAction>(code);
        menuItems_.insert(contact_close, createAction(menu, Utils::getShortcutsCloseActionName(closeAct_), qsl("Ctrl+W"), mainWindow_, &Ui::MainWindow::activateShortcutsClose));

        extendedMenus_.push_back(menu);
        QObject::connect(menu, &QMenu::aboutToShow, menu, []() { emit Utils::InterConnector::instance().closeAnyPopupMenu(); });

        menu = mainMenu_->addMenu(Utils::Translations::Get(qsl("View")));
        menuItems_.insert(view_unreadMessage, createAction(menu, Utils::Translations::Get(qsl("Next Unread Message")), qsl("Ctrl+]"), mainWindow_, &Ui::MainWindow::activateNextUnread));
        menuItems_.insert(view_fullScreen, createAction(menu, Utils::Translations::Get(qsl("Enter Full Screen")), qsl("Meta+Ctrl+F"), mainWindow_, &Ui::MainWindow::toggleFullScreen));
        extendedMenus_.push_back(menu);
        QObject::connect(menu, &QMenu::aboutToShow, menu, []() { emit Utils::InterConnector::instance().closeAnyPopupMenu(); });

        menu = mainMenu_->addMenu(Utils::Translations::Get(qsl("Window")));
        menuItems_.insert(window_minimizeWindow, createAction(menu, Utils::Translations::Get(qsl("Minimize")), qsl("Ctrl+M"), mainWindow_, &Ui::MainWindow::minimize));
        menuItems_.insert(window_maximizeWindow, createAction(menu, Utils::Translations::Get(qsl("Zoom")), QString(), mainWindow_, &Ui::MainWindow::zoomWindow));
        menu->addSeparator();
        menuItems_.insert(window_prevChat, createAction(menu, Utils::Translations::Get(qsl("Select Previous Chat")), qsl("Meta+Shift+Tab"), mainWindow_, &Ui::MainWindow::activatePrevChat));
        menuItems_.insert(window_nextChat, createAction(menu, Utils::Translations::Get(qsl("Select Next Chat")), qsl("Meta+Tab"), mainWindow_, &Ui::MainWindow::activateNextChat));
        menu->addSeparator();
        // in QMenu are no tags to make difference between items with same title, so ql1c(' ') is added as mark
        menuItems_.insert(window_mainWindow, createAction(menu, Utils::getAppTitle() % ql1c(' '), qsl("Ctrl+0"), mainWindow_, &Ui::MainWindow::activate));
        menu->addSeparator();
        menuItems_.insert(window_bringToFront, createAction(menu, Utils::Translations::Get(qsl("Bring All To Front")), QString(), mainWindow_, &Ui::MainWindow::bringToFront));
        menu->addSeparator();
        menuItems_.insert(window_switchMainWindow, createAction(menu, Utils::getAppTitle(), QString(), mainWindow_, &Ui::MainWindow::activate));

        if (const auto& mainPage = mainWindow_->getMainPage())
            menuItems_[window_switchVoipWindow] = createAction(menu, Utils::Translations::Get(qsl("ICQ VOIP")), QString(), mainPage, &Ui::MainPage::showVideoWindow);

        windowMenu_ = menu;
        extendedMenus_.push_back(menu);
        QObject::connect(menu, &QMenu::aboutToShow, menu, []() { emit Utils::InterConnector::instance().closeAnyPopupMenu(); });

        if (false)
        {
            action = createAction(menu, Utils::Translations::Get(qsl("About app")), QString(), mainWindow_, &Ui::MainWindow::activateAbout);
            action->setMenuRole(QAction::MenuRole::ApplicationSpecificRole);
            menuItems_.insert(global_about, action);
            extendedActions_.push_back(action);

            action = createAction(menu, Utils::Translations::Get(qsl("Settings")), QString(), mainWindow_, &Ui::MainWindow::activateSettings);
            action->setMenuRole(QAction::MenuRole::ApplicationSpecificRole);
            menuItems_.insert(global_settings, action);
            extendedActions_.push_back(action);
        }
        else
        {
            action = createAction(menu, qsl("about.*"), QString(), mainWindow_, &Ui::MainWindow::activateAbout);
            menuItems_.insert(global_about, action);
            extendedActions_.push_back(action);

            action = createAction(menu, qsl("settings"), QString(), mainWindow_, &Ui::MainWindow::activateSettings);
            menuItems_.insert(global_settings, action);
            extendedActions_.push_back(action);
        }

#ifdef UPDATES
        action = createAction(menu, Utils::Translations::Get(qsl("Check For Updates...")), QString(), mainWindow_, &Ui::MainWindow::checkForUpdates);
        action->setMenuRole(QAction::MenuRole::ApplicationSpecificRole);
        menuItems_.insert(global_update, action);
        extendedActions_.push_back(action);
#endif

        mainMenu_->addAction(qsl("quit"), mainWindow_, SLOT(exit())); // use qt5 style connect since qt5.11

        mainWindow_->setMenuBar(mainMenu_);
    }
    else
    {
        if (windowMenu_)
        {
            if (const auto& mainPage = mainWindow_->getMainPage())
                menuItems_[window_switchVoipWindow] = createAction((QMenu*)windowMenu_, Utils::Translations::Get(qsl("ICQ VOIP")), QString(), mainPage, &Ui::MainPage::showVideoWindow);
        }
    }

    for (auto menu: extendedMenus_)
        menu->setEnabled(!simple);

    windowMenu_->setEnabled(true);

    for (auto action: extendedActions_)
        action->setEnabled(!simple);

    updateMainMenu();

//    Edit
//    Undo cmd+Z
//    Redo shift+cmd+Z
//    Cut cmd+X
//    Copy cmd+C
//    Paste cmd+V
//    Paste as quote alt+cmd+V
//    Delete
//    Select all cmd+A

//    Юзер залогинен
//    Меню состоит из пунктов:
//    ICQ
//    About
//    Preferences cmd+,
//    Logout username (userid)
//    Hide cmd+H
//    Hide other alt+cmd+H
//    Quit

//    Contact
//    Add buddy cmd+N
//    Profile cmd+I
//    Close cmd+W

//    View
//    Next unread messsage cmd+]
//    Enter Full Screen ^cmd+F

//    Window
//    Select next chat shift+cmd+]
//    Select previous chat shift+cmd+[
//    Main window cmd+1
}

void MacSupport::showEmojiPanel()
{
    [NSApp orderFrontCharacterPalette:nil];
}

void MacSupport::updateMainMenu()
{
    if (editMenu_)
    {
        const auto contDialog = Utils::InterConnector::instance().getContactDialog();
        const bool isPttActive = contDialog && contDialog->isRecordingPtt();
        editMenu_->setEnabled(!isPttActive);
    }

    auto actions = mainWindow_->menuBar()->actions();

    QAction *nextChat = menuItems_[window_nextChat];
    QAction *prevChat = menuItems_[window_prevChat];
    QAction *fullScreen = menuItems_[view_fullScreen];
    QAction *contactClose = menuItems_[contact_close];
    QAction *windowMaximize = menuItems_[window_maximizeWindow];
    QAction *windowMinimize = menuItems_[window_minimizeWindow];
    QAction *windowVoip = menuItems_[window_switchVoipWindow];
    QAction *windowICQ = menuItems_[window_switchMainWindow];
    QAction *bringToFront = menuItems_[window_bringToFront];

    const auto mainPage = mainWindow_ ? mainWindow_->getMainPage() : nullptr;
    bool mainWindowActive = mainWindow_ && mainWindow_->isActive();

    bool isCall = mainPage && mainPage->isVideoWindowOn();
    bool voipWindowActive = mainPage && mainPage->isVideoWindowActive();
    bool voipWindowMinimized = mainPage && mainPage->isVideoWindowMinimized();
    bool voipWindowFullscreen = !isCall || (mainPage && mainPage->isVideoWindowFullscreen());

    auto code = Ui::get_gui_settings()->get_value<int>(settings_shortcuts_close_action, static_cast<int>(Ui::ShortcutsCloseAction::Default));
    auto act = static_cast<Ui::ShortcutsCloseAction>(code);
    if (closeAct_ != act && contactClose)
    {
        closeAct_ = act;
        contactClose->setText(Utils::getShortcutsCloseActionName(closeAct_));
    }

    auto isFullScreen = false;
    auto inDock = false;

    if (mainWindow_)
    {
        void *pntr = (void *) mainWindow_->winId();
        NSView *view = (__bridge NSView *) pntr;
        inDock = ![view.window isVisible];
        isFullScreen = (([view.window styleMask] & NSFullScreenWindowMask) == NSFullScreenWindowMask);

        if (fullScreen)
        {
            if (isFullScreen)
                fullScreen->setText(Utils::Translations::Get(qsl("Exit Full Screen")));
            else
                fullScreen->setText(Utils::Translations::Get(qsl("Enter Full Screen")));
        }
    }

    if (nextChat)
        nextChat->setEnabled(false);
    if (prevChat)
        prevChat->setEnabled(false);

    if (mainWindow_ && mainWindow_->getMainPage() && mainWindow_->getMainPage()->getContactDialog() && nextChat && prevChat)
    {
        const QString &aimId = Logic::getContactListModel()->selectedContact();
        const auto enabled = !aimId.isEmpty() && !mainWindow_->isMinimized() && !inDock;
        const auto hasPrev = enabled && !Logic::getRecentsModel()->prevAimId(aimId).isEmpty();
        const auto hasNext = enabled && !Logic::getRecentsModel()->nextAimId(aimId).isEmpty();
        nextChat->setEnabled(hasNext && !voipWindowActive);
        prevChat->setEnabled(hasPrev && !voipWindowActive);
    }

    const auto notMiniAndFull = (!isCall || mainWindowActive) && !mainWindow_->isMinimized() && !isFullScreen && !inDock;
    if (windowMaximize)
        windowMaximize->setEnabled(mainWindow_ && (notMiniAndFull || (!voipWindowFullscreen && !voipWindowMinimized)));
    if (windowMinimize)
        windowMinimize->setEnabled(mainWindow_ && (notMiniAndFull || (!voipWindowFullscreen && !voipWindowMinimized)));

    if (windowVoip)
    {
        windowVoip->setVisible(isCall);
        windowVoip->setCheckable(true);
        windowVoip->setChecked(voipWindowActive);
    }

    if (windowICQ)
    {
        windowICQ->setCheckable(true);
        windowICQ->setChecked(mainWindow_ && mainWindowActive);
    }

    if (bringToFront)
        bringToFront->setEnabled(isCall);

    @autoreleasepool
    {
        QPixmap diamond(Utils::scale_bitmap_with_value(QSize(5, 9)));
        diamond.fill(Qt::transparent);
        {
            QPainter p(&diamond);
            p.setRenderHint(QPainter::Antialiasing);
            p.setRenderHint(QPainter::HighQualityAntialiasing);
            p.setPen(Qt::black);
            p.setBrush(Qt::black);
            p.setFont(Fonts::appFontScaled(18));
            p.drawText(diamond.rect(), Qt::AlignCenter, QChar(0x2666)); // diamond in unicode
        }
        Utils::check_pixel_ratio(diamond);

        QByteArray bArray;
        QBuffer buffer(&bArray);
        buffer.open(QIODevice::WriteOnly);
        diamond.save(&buffer, "TIFF");

        NSData *data = [NSData dataWithBytes:bArray.constData() length:bArray.size()];
        NSImage *ava = [[[NSImage alloc] initWithData:data] autorelease];

        NSImage *composedImage = [[[NSImage alloc] initWithSize:ava.size] autorelease];
        [composedImage lockFocus];
        NSRect imageFrame = NSMakeRect(0.f, 0.f, ava.size.width, ava.size.height);
        [ava drawInRect:imageFrame fromRect:NSZeroRect operation:NSCompositeSourceOver fraction:1.f];
        [composedImage unlockFocus];

        auto menuWindow = std::find_if(extendedMenus_.begin(), extendedMenus_.end(), [](const auto& _item)
        {
            return (_item->title() == Utils::Translations::Get(qsl("Window")));
        });

        if (menuWindow != extendedMenus_.end())
        {
            auto nsmenu = (*menuWindow)->toNSMenu();

            NSMenuItem* winMainItem = [nsmenu itemWithTitle: Utils::getAppTitle().toNSString()];
            NSMenuItem* winCallItem = [nsmenu itemWithTitle: Utils::Translations::Get(qsl("ICQ VOIP")).toNSString()];

            [winMainItem setMixedStateImage: composedImage];
            [winCallItem setMixedStateImage: composedImage];


            if (mainWindow_ && mainWindow_->isMinimized())
                [winMainItem setState: NSMixedState];

            if (mainPage && mainPage->isVideoWindowMinimized())
                [winCallItem setState: NSMixedState];
        }
    }
}

void MacSupport::log(QString logString)
{
    NSString * logString_ = fromQString(logString);

    NSLog(@"%@", logString_);
}

void MacSupport::replacePasteboard(const QString &text)
{
    NSPasteboard * pb = [NSPasteboard generalPasteboard];

    [pb clearContents];
    [pb declareTypes:[NSArray arrayWithObject:NSPasteboardTypeString] owner:nil];
    [pb setString:fromQString(text) forType:NSPasteboardTypeString];
}

typedef const UCKeyboardLayout * LayoutsPtr;

LayoutsPtr layoutFromSource(TISInputSourceRef source)
{
    CFDataRef keyboardLayoutUchr = (CFDataRef)TISGetInputSourceProperty(source, kTISPropertyUnicodeKeyLayoutData);
    if (keyboardLayoutUchr == nil)
    {
        return nil;
    }
    return (LayoutsPtr)CFDataGetBytePtr(keyboardLayoutUchr);
}

void setupLayouts(QList<LayoutsPtr> & layouts)
{
    NSDictionary *filter = @{(NSString *)kTISPropertyInputSourceCategory: (NSString *)kTISCategoryKeyboardInputSource};

    CFArrayRef inputSourcesListRef = TISCreateInputSourceList((CFDictionaryRef)filter, false);

    NSArray * sources = (NSArray *)inputSourcesListRef;

    NSMutableArray * usingSources = [[NSMutableArray alloc] init];

    NSInteger availableSourcesNumber = [sources count];
    for (NSInteger j = 0; j < availableSourcesNumber; ++j)
    {
        LayoutsPtr layout = layoutFromSource((TISInputSourceRef)sources[j]);
        if(layout)
        {
            layouts.append(layout);
        }
    }

    TISInputSourceRef currentKeyboard = TISCopyCurrentKeyboardInputSource();
    LayoutsPtr currentLayout = layoutFromSource(currentKeyboard);

    for (int i = 0; i < layouts.size(); i++)
    {
        LayoutsPtr l = layouts[i];
        if (l == currentLayout)
        {
            layouts.removeAt(i);
            layouts.insert(0, l);
            break;
        }
    }

    CFRelease(inputSourcesListRef);
    CFRelease(currentKeyboard);

    [usingSources release];
}

UniChar stringForKey(CGKeyCode keyCode, const UCKeyboardLayout * layout)
{
    UInt32 keysDown = 0;
    UniChar chars[4];
    UniCharCount realLength = 0;
    UInt32	 modifierFlags = 0;

    OSStatus status = UCKeyTranslate(layout,
                                     keyCode,
                                     kUCKeyActionDisplay,
                                     modifierFlags,
                                     LMGetKbdType(),
                                     kUCKeyTranslateNoDeadKeysBit,
                                     &keysDown,
                                     sizeof(chars) / sizeof(chars[0]),
                                     &realLength,
                                     chars);
    if( (status != noErr) || (realLength == 0) )
    {
        return 0;
    }
    return chars[0];
}

CGKeyCode keyCodeForChar(unichar letter, UCKeyboardLayout const * layout)
{
    for (int i =0; i<52; ++i)
    {
        if (letter == stringForKey(i, layout))
        {
            return i;
        }
    }

    return UINT16_MAX;
}

NSString * translitedString(NSString * sourceString, LayoutsPtr sourceLayout, LayoutsPtr targetLayout)
{
    NSMutableString *possibleString = [NSMutableString string];

    for (NSInteger i = 0; i < sourceString.length; ++i)
    {
        unichar letter = [sourceString characterAtIndex:i];

        unichar translit = letter;
        CGKeyCode keyCode = UINT16_MAX;
        keyCode = keyCodeForChar(letter, sourceLayout);
        if(keyCode != UINT16_MAX)
        {
            translit = stringForKey(keyCode, targetLayout);
        }

        [possibleString appendString:[NSString stringWithCharacters:&translit length:1]];
    }

    return possibleString;
}

void MacSupport::getPossibleStrings(const QString& text, std::vector<std::vector<QString>>& result, unsigned& _count)
{
    QList<LayoutsPtr> layouts;

    setupLayouts(layouts);

    for (int i = 0; i < text.length(); ++i)
    {
        result.push_back(std::vector<QString>());
        result[i].push_back(text.at(i));
    }

    if (layouts.isEmpty())
    {
        _count = 1;
        return;
    }

    NSString * sourceString = fromQString(text);

    LayoutsPtr sourceLayout = layouts[0];

    _count = layouts.size();
    for (int i = 1; i < layouts.size(); i++)
    {
        LayoutsPtr targetLayout = layouts[i];

        if (sourceLayout != targetLayout)
        {
            NSString * translited = translitedString(sourceString, sourceLayout, targetLayout);

            if (translited.length)
            {
                auto translited_q = toQString(translited);
                for (int i = 0; i < text.length(); ++i)
                {
                    result[i].push_back(translited_q.at(i));
                }
            }
        }
    }
}

bool dockClickHandler(id self,SEL _cmd,...)
{
    Q_UNUSED(self)
    Q_UNUSED(_cmd)

    if (mainWindow_ && !mainWindow_->isFullScreen())// && (mainWindow_->isHidden() || mainWindow_->isMinimized()))
    {
        statistic::getGuiMetrics().eventAppIconClicked();
        mainWindow_->activate();
    }

    // Return NO (false) to suppress the default OS X actions
    return false;
}

void MacSupport::setupDockClickHandler()
{
    NSApplication * appInst = [NSApplication sharedApplication];

    if (appInst != NULL)
    {
        id<NSApplicationDelegate> delegate = appInst.delegate;
        Class delClass = delegate.class;
        SEL shouldHandle = sel_registerName("applicationShouldHandleReopen:hasVisibleWindows:");
        if (class_getInstanceMethod(delClass, shouldHandle))
        {
            if (class_replaceMethod(delClass, shouldHandle, (IMP)dockClickHandler, "B@:"))
                qDebug() << "Registered dock click handler (replaced original method)";
            else
                qWarning() << "Failed to replace method for dock click handler";
        }
        else
        {
            if (class_addMethod(delClass, shouldHandle, (IMP)dockClickHandler,"B@:"))
                qDebug() << "Registered dock click handler";
            else
                qWarning() << "Failed to register dock click handler";
        }
    }
}

bool MacSupport::nativeEventFilter(const QByteArray &data, void *message, long *result)
{
    NSEvent *e = (NSEvent *)message;
    // NSLog(@"------------\n%@\n%@", data.toNSData(), e);

    if ([e type] == NSKeyDown && ([e modifierFlags] & (NSControlKeyMask | NSCommandKeyMask)))
    {
        if ([e modifierFlags] & NSCommandKeyMask)
        {
            static std::set<uint> possibles =
            {
                kVK_ANSI_Comma,
                kVK_ANSI_N,
                kVK_ANSI_I,
                kVK_ANSI_F,
                kVK_ANSI_LeftBracket,
                kVK_ANSI_RightBracket,
            };

            if (possibles.find(e.keyCode) != possibles.end())
            {
                emit Utils::InterConnector::instance().closeAnyPopupMenu();
                emit Utils::InterConnector::instance().closeAnyPopupWindow(Utils::CloseWindowInfo({ Utils::CloseWindowInfo::Initiator::MacEventFilter,
                                                                                                    Utils::CloseWindowInfo::Reason::MacEventFilter }));
            }
            else if (mainWindow_ && ([e modifierFlags] & NSCommandKeyMask) && e.keyCode == kVK_ANSI_1)
            {
                mainWindow_->activate();
                emit Utils::InterConnector::instance().applicationActivated();
            }
            else if (e.keyCode == kVK_ANSI_M)
            {
                emit Utils::InterConnector::instance().closeAnyPopupMenu();
            }
        }
    }
    else if ([e type] == NSAppKitDefined && [e subtype] == NSApplicationDeactivatedEventType)
    {
        emit Utils::InterConnector::instance().applicationDeactivated();
        emit Utils::InterConnector::instance().closeAnyPopupMenu();
    }
    else if ([e type] == NSAppKitDefined && [e subtype] == NSApplicationActivatedEventType)
    {
        emit Utils::InterConnector::instance().applicationActivated();
    }
    return false;
}

void MacSupport::activateWindow(unsigned long long view/* = 0*/)
{
    if (view)
    {
        auto p = (NSView *)view;
        if (p)
        {
            auto w = [p window];
            if (w)
            {
                [w setIsVisible:YES];
            }
        }
        updateMainMenu();
    }
    [NSApp activateIgnoringOtherApps:YES];
}

void MacSupport::registerAppDelegate()
{
    AppDelegate* delegate = [AppDelegate new];
    [NSApp setDelegate: delegate];

    [[NSAppleEventManager sharedAppleEventManager]
     setEventHandler:delegate
     andSelector:@selector(handleURLEvent:withReplyEvent:)
     forEventClass:kInternetEventClass
     andEventID:kAEGetURL];
}

// pass _ext by value to avoid UB in completionHandler
void MacSupport::saveFileName(const QString &caption, const QString &qdir, const QString &filter, std::function<void (const QString& _filename, const QString& _directory)> _callback, QString _ext, std::function<void ()> _cancel_callback)
{
    auto dir = qdir.toNSString().stringByDeletingLastPathComponent;
    auto filename = qdir.toNSString().lastPathComponent;

    NSSavePanel *panel = [NSSavePanel savePanel];
    [panel setTitle:caption.toNSString()];
    [panel setDirectoryURL:[NSURL URLWithString:dir]];
    [panel setShowsTagField:NO];
    [panel setNameFieldStringValue:filename];

    [panel beginSheetModalForWindow:[NSApp mainWindow] completionHandler:^(NSInteger result) {
        if (result == NSFileHandlingPanelOKButton)
        {
            const auto path = QString::fromNSString([[panel URL] path]);
            if (!path.isEmpty())
            {
                const QFileInfo info(path);
                const QString directory = info.dir().absolutePath();
                const QString inputExt = !_ext.isEmpty() && info.suffix().isEmpty() ? _ext : QString();
                const QString filename = info.fileName() % inputExt;
                if (_callback)
                    _callback(filename, directory);
            }
            else if (_cancel_callback)
            {
                _cancel_callback();
            }
        }
        else if (_cancel_callback)
        {
            _cancel_callback();
        }
    }];

    if (auto editor = [panel fieldEditor:NO forObject:nil])
    {
        auto nameFieldString = [panel nameFieldStringValue];
        if (auto nameFieldExtension = [nameFieldString pathExtension])
        {
            auto newLength = ([nameFieldString length] - [nameFieldExtension length] - 1);
            [editor setSelectedRange:NSMakeRange(0, newLength)];
        }
    }
    auto connection = QWidget::connect(&Utils::InterConnector::instance(), &Utils::InterConnector::closeAnyPopupMenu, [panel]()
    {
        [NSApp endSheet:panel];
    });

    QWidget::disconnect(connection);
}

bool MacSupport::isDoNotDisturbOn()
{
    NSUserDefaults* defaults = [[NSUserDefaults alloc] initWithSuiteName:@"com.apple.notificationcenterui"];
    return [defaults boolForKey:@"doNotDisturb"];
}

void MacSupport::showNSPopUpMenuWindowLevel(QWidget* w)
{
    w->show();
    NSWindow *wnd = [reinterpret_cast<NSView *>(w->winId()) window];
    [wnd setLevel:NSPopUpMenuWindowLevel];
}

void MacSupport::showNSModalPanelWindowLevel(QWidget* w)
{
    w->show();
    NSWindow *wnd = [reinterpret_cast<NSView *>(w->winId()) window];
    [wnd setLevel:NSModalPanelWindowLevel];
}

void MacSupport::showOverAll(QWidget* w)
{
    w->show();
    NSWindow *wnd = [reinterpret_cast<NSView *>(w->winId()) window];
    [wnd setLevel:NSScreenSaverWindowLevel];
}

bool MacSupport::isMetalSupported()
{
    static bool isMetalSupported = false;
    static bool checked = false;

    if (!checked)
    {
        auto device = MTLCreateSystemDefaultDevice();
        if (device)
            isMetalSupported = true;

        checked = true;
    }

    return isMetalSupported;
}

void MacSupport::zoomWindow(WId wid)
{
    void *pntr = (void *)wid;
    NSView *view = (__bridge NSView *)pntr;
    [view.window zoom:nil];
}

MacMenuBlocker::MacMenuBlocker()
    : receiver_(std::make_unique<QObject>())
{
    macMenuState_.reserve(menuItems_.size());

    for (auto it = menuItems_.begin(); it != menuItems_.end(); ++it)
    {
        if (!it.value())
            continue;

        macMenuState_.emplace_back(it.value(), it.value()->isEnabled());
        it.value()->setEnabled(false);

        auto index = macMenuState_.size() - 1;
        QObject::connect(it.value(), &QAction::changed, receiver_.get(), [index, this]{
            menuState()[index].first->setEnabled(false);
        });
    }
}

MacMenuBlocker::~MacMenuBlocker()
{
    for (auto [action, enabled]: macMenuState_)
    {
        QSignalBlocker blocker(action);
        action->setEnabled(enabled);
    }
}

MacMenuBlocker::MenuStateContainer& MacMenuBlocker::menuState()
{
    return macMenuState_;
}
