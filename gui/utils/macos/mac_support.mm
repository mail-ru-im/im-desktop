#import <Cocoa/Cocoa.h>
#import <Carbon/Carbon.h>
#import <Foundation/Foundation.h>
#import <Sparkle/Sparkle.h>
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
#include "../features.h"

#include "../gui/app_config.h"
#include "../gui/url_config.h"
#include "../libomicron/include/omicron/omicron.h"
#include "../common.shared/version_info.h"
#include "../common.shared/omicron_keys.h"
#include "../common.shared/config/config.h"

#include "core_dispatcher.h"

#import "mac_support.h"
#import "mac_translations.h"
#import "mac_titlebar.h"

#include <objc/objc.h>
#include <objc/message.h>
#include <objc/runtime.h>

#include "../../app_config.h"

#import <SystemConfiguration/SystemConfiguration.h>

namespace
{
    std::chrono::seconds checkUpdateInterval()
    {
        return std::chrono::seconds(Ui::GetAppConfig().AppUpdateIntervalSecs());
    }
}

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


static std::map<int, QAction *> menuItems_;
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

void setEditMenuEnabled(bool _enabled)
{
    for (auto &[key, action] : menuItems_)
    {
        if (key >= edit_undo && key <= edit_pasteEmoji)
            action->setEnabled(_enabled);
    }
}

@interface StatusItemKVO : NSObject
-(void)observeValueForKeyPath:(NSString *)keyPath ofObject:(id)object change:(NSDictionary<NSKeyValueChangeKey,id> *)change context:(void *)context;
@end
@implementation StatusItemKVO
-(void)observeValueForKeyPath:(NSString *)keyPath ofObject:(id)object change:(NSDictionary<NSKeyValueChangeKey,id> *)change context:(void *)context
{
    if ([keyPath isEqualToString:@"button.effectiveAppearance"])
       Q_EMIT Utils::InterConnector::instance().trayIconThemeChanged();
}
@end

static NSStatusItem* statusItemForTheme_ = nil;
static StatusItemKVO* statusItemKVO_ = nil;

@interface AppDelegate: NSObject<NSApplicationDelegate>
- (void)handleURLEvent:(NSAppleEventDescriptor *)event withReplyEvent:(NSAppleEventDescriptor *)replyEvent;
//- (void)applicationDidFinishLaunching:(NSNotification *)notification;
- (NSApplicationTerminateReply)applicationShouldTerminate:(NSApplication *)sender;
- (void)activateMainWindow;
@end

@implementation AppDelegate

- (void)handleURLEvent:(NSAppleEventDescriptor*)event withReplyEvent:(NSAppleEventDescriptor*)replyEvent
{
    NSString* url = [[event paramDescriptorForKeyword:keyDirectObject]
                     stringValue];

    Q_EMIT Utils::InterConnector::instance().schemeUrlClicked(QString::fromNSString(url));
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
        mainWindow_->exit();
    if (statusItemKVO_ != nil)
    {
        if (statusItemForTheme_ != nil)
            [statusItemForTheme_ removeObserver:statusItemKVO_ forKeyPath:@"button.effectiveAppearance"];

        [statusItemKVO_ release];
    }
    if (statusItemForTheme_ != nil)
    {
        [[NSStatusBar systemStatusBar] removeStatusItem:statusItemForTheme_];
        [statusItemForTheme_ release];
        statusItemForTheme_ = nil;
    }
    [[NSNotificationCenter defaultCenter] postNotificationName:@"NSApplicationWillTerminateNotification" object:nil];
    return NSTerminateCancel;
}

- (void)activateMainWindow
{
    if (mainWindow_ != nil)
        mainWindow_->activate();
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

// sparkle

#ifdef UPDATES

@interface SparkleUpdaterDelegate : NSObject <SUUpdaterDelegate>
@end

@implementation SparkleUpdaterDelegate
- (void)updaterDidNotFindUpdate:(SUUpdater *)__unused updater
{
    Q_EMIT Utils::InterConnector::instance().onMacUpdateInfo(Utils::MacUpdateState::NotFound);
}

- (void)updater:(SUUpdater *)__unused updater failedToDownloadUpdate:(SUAppcastItem *)__unused item error:(NSError *)__unused error
{
    Q_EMIT Utils::InterConnector::instance().onMacUpdateInfo(Utils::MacUpdateState::LoadError);
}

- (void)updater:(SUUpdater *)__unused updater didExtractUpdate:(SUAppcastItem *)__unused item
{
    Q_EMIT Utils::InterConnector::instance().onMacUpdateInfo(Utils::MacUpdateState::Ready);
}

@end
// end of sparkle
#endif

@interface EventsCatcher : NSObject

@property (nonatomic, copy) void(^sleepAction)(void);
@property (nonatomic, copy) void(^wakeAction)(void);
@property (nonatomic, copy) void(^changeWorkspaceAction)(void);

- (instancetype)init;

- (void)setSleep:(void(^)(void))action;
- (void)setWake:(void(^)(void))action;
- (void)setChangeWorkspace:(void(^)(void))action;

- (void)receiveSleepEvent:(NSNotification *)notification;
- (void)receiveWakeEvent:(NSNotification *)notification;
- (void)receiveWorkspaceChangeEvent:(NSNotification *)notification;

@end

@implementation EventsCatcher

- (instancetype)init
{
    self = [super init];
    if (self)
    {
        self.sleepAction = nil;
        self.wakeAction = nil;
        self.changeWorkspaceAction = nil;

        [[[NSWorkspace sharedWorkspace] notificationCenter] addObserver:self
                                                               selector:@selector(receiveSleepEvent:)
                                                                   name:NSWorkspaceWillSleepNotification object:nil];
        [[[NSWorkspace sharedWorkspace] notificationCenter] addObserver:self
                                                               selector:@selector(receiveWakeEvent:)
                                                                   name:NSWorkspaceDidWakeNotification object:nil];
        [[[NSWorkspace sharedWorkspace] notificationCenter] addObserver:self
                                                               selector:@selector(receiveWorkspaceChangeEvent:)
                                                                   name:NSWorkspaceActiveSpaceDidChangeNotification object:nil];
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

- (void)setSleep:(void(^)(void))action
{
    if (self)
        self.sleepAction = action;
}

- (void)setWake:(void(^)(void))action
{
    if (self)
        self.wakeAction = action;
}

- (void)setChangeWorkspace:(void(^)(void))action
{
    if (self)
        self.changeWorkspaceAction = action;
}

- (void)receiveSleepEvent:(NSNotification *)notification
{
    statistic::getGuiMetrics().eventAppSleep();
    if (self.sleepAction)
        self.sleepAction();
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
            if (self.wakeAction)
                self.wakeAction();
            //
        });
        //
    });
}

- (void)receiveWorkspaceChangeEvent:(NSNotification *)notification;
{
    if (self.changeWorkspaceAction)
        self.changeWorkspaceAction();
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
    title_(std::make_unique<MacTitlebar>(mainWindow)),
    updateRequested_(false)
{
    sparkleUpdater_ = nil;
    mainWindow_ = mainWindow;
    registerAppDelegate();
    setupDockClickHandler();

    Swizzle(NSApplication.class, @selector(isFullKeyboardAccessEnabled), @selector(overrideIsFullKeyboardAccessEnabled));
}

MacSupport::~MacSupport()
{
    updateFeedUrl_.reset();
    menuItems_.clear();

    mainWindow_ = nil;

    cleanMacUpdater();

    [macPreviewProxy_ hidePreviewPopup];
    [macPreviewProxy_ release];
    macPreviewProxy_ = nil;
}

namespace
{
    std::string feedUrl()
    {
        std::string feed_url;
        feed_url += "https://";
        if (!Features::isUpdateFromBackendEnabled())
        {
            if constexpr (environment::is_alpha())
                feed_url += Ui::GetAppConfig().getMacUpdateAlpha();
            else if (Ui::GetDispatcher()->isInstallBetaUpdatesEnabled())
                feed_url += Ui::GetAppConfig().getMacUpdateBeta();
            else
                feed_url += Ui::GetAppConfig().getMacUpdateRelease();

            feed_url += '/';

            std::string current_build_version = core::tools::version_info().get_version();
            if constexpr (environment::is_alpha())
                feed_url += omicronlib::_o(omicron::keys::update_alpha_version, current_build_version);
            else if (Ui::GetDispatcher()->isInstallBetaUpdatesEnabled())
                feed_url += omicronlib::_o(omicron::keys::update_beta_version, current_build_version);
            else
                feed_url += omicronlib::_o(omicron::keys::update_release_version, current_build_version);
        }
        else
        {
            feed_url += Ui::getUrlConfig().getUrlAppUpdate().toStdString();
        }
        feed_url += "/version.xml";
        return feed_url;
    };
}

void MacSupport::enableMacUpdater()
{
#ifdef UPDATES
    if (!Utils::supportUpdates())
        return;

    if (sparkleUpdater_ != nil)
        return;

    sparkleUpdater_ = [[SUUpdater alloc] init];
    [sparkleUpdater_ setAutomaticallyDownloadsUpdates:YES];

    SparkleUpdaterDelegate* delegate = [SparkleUpdaterDelegate new];
    [sparkleUpdater_ setDelegate: delegate];

    if (!updateFeedUrl_)
    {
        updateFeedUrl_ = std::make_unique<QTimer>();
        updateFeedUrl_->setInterval(checkUpdateInterval());
        QObject::connect(updateFeedUrl_.get(), &QTimer::timeout, updateFeedUrl_.get(), [this]()
        {
            checkForUpdates();
        });
        updateFeedUrl_->start();
    }

    checkForUpdates();
#endif
}

bool MacSupport::setFeedUrl()
{
    if (sparkleUpdater_)
    {
        std::string feed_url = feedUrl();

        NSString* su_feed_url = [NSString stringWithUTF8String:feed_url.c_str()];
        NSURL * updateFeed = [NSURL URLWithString:su_feed_url];

        if (updateFeed)
        {
            [sparkleUpdater_ setFeedURL:updateFeed];
            return true;
        }
    }
    return false;
}

void MacSupport::listenSystemEvents()
{
    static EventsCatcher *eventsCatcher = nil;
    static dispatch_once_t onceToken;
    dispatch_once(&onceToken, ^{
        //
        eventsCatcher = [[EventsCatcher alloc] init];
        [eventsCatcher setSleep:^{
            if (mainWindow_)
                mainWindow_->gotoSleep();
        }];
        [eventsCatcher setWake:^{
            if (mainWindow_)
                mainWindow_->gotoWakeup();
        }];
        [eventsCatcher setChangeWorkspace:^{
            Q_EMIT Utils::InterConnector::instance().workspaceChanged();
        }];
        //
    });
}

void MacSupport::checkForUpdates()
{
    if (isDoNotDisturbOn() && !Features::forceCheckMacUpdates())
        return;
    checkForUpdatesInBackground();
}

void MacSupport::checkForUpdatesInBackground()
{
    if (sparkleUpdater_ && setFeedUrl())
    {
        [sparkleUpdater_ checkForUpdatesInBackground];
        updateRequested_ = true;
    }
}

void MacSupport::runMacUpdater()
{
    if (sparkleUpdater_ && setFeedUrl())
        [sparkleUpdater_ checkForUpdates:nil];
}

void MacSupport::installUpdates()
{
    if (sparkleUpdater_ && setFeedUrl())
        [sparkleUpdater_ installUpdatesIfAvailable];
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
        Q_EMIT Utils::InterConnector::instance().closeAnyPopupMenu();
        Q_EMIT Utils::InterConnector::instance().closeAnyPopupWindow(Utils::CloseWindowInfo());
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

MacSupport::ThemeType MacSupport::currentTheme()
{
    NSDictionary *dict = [[NSUserDefaults standardUserDefaults] persistentDomainForName:NSGlobalDomain];
    id style = [dict objectForKey:@"AppleInterfaceStyle"];
    BOOL darkModeOn = ( style && [style isKindOfClass:[NSString class]] && NSOrderedSame == [style caseInsensitiveCompare:@"dark"] );

    return darkModeOn ? ThemeType::Dark : ThemeType::White;
}

static MacSupport::ThemeType getTheme(NSAppearance *appearance)
{
    NSString *appearanceName = (NSString*)(appearance.name);
    if ([[appearanceName lowercaseString] containsString:@"dark"])
        return MacSupport::ThemeType::Dark;
    return MacSupport::ThemeType::White;
}

MacSupport::ThemeType MacSupport::currentThemeForTray()
{
    if (isBigSurOrGreater())
    {
        if (statusItemForTheme_ == nil)
        {
            statusItemForTheme_ = [[[NSStatusBar systemStatusBar] statusItemWithLength:NSSquareStatusItemLength] retain];
            statusItemForTheme_.visible = NO;
            statusItemKVO_ = [[StatusItemKVO alloc] init];
            [statusItemForTheme_ addObserver:statusItemKVO_ forKeyPath:@"button.effectiveAppearance" options:NSKeyValueObservingOptionNew context:nil];
        }
        return getTheme(statusItemForTheme_.button.effectiveAppearance);
    }
    return currentTheme();
}

QString MacSupport::themeTypeToString(ThemeType _type)
{
    return _type == ThemeType::Dark ? qsl("black") : qsl("white");
}

bool MacSupport::isBigSurOrGreater()
{
    if (@available(macOS 11.0, *))
        return true;
    return false;
}

QString MacSupport::settingsPath()
{
    NSArray *paths = NSSearchPathForDirectoriesInDomains(NSApplicationSupportDirectory, NSUserDomainMask, YES);
    NSString *applicationSupportDirectory = [paths firstObject];
    const auto productPath = config::get().string(config::values::product_path_mac);
    return toQString([applicationSupportDirectory stringByAppendingPathComponent:[NSString stringWithUTF8String:productPath.data()]]);
}

QString MacSupport::bundleName()
{
    return QString::fromUtf8([[[NSBundle mainBundle] bundleIdentifier] UTF8String]);
}

QString MacSupport::bundleDir()
{
    return QString::fromUtf8([[[NSBundle mainBundle] bundlePath] UTF8String]);
}

QString MacSupport::defaultDownloadsPath()
{
    NSArray *paths = NSSearchPathForDirectoriesInDomains(NSLibraryDirectory, NSUserDomainMask, YES);
    NSString *path = [paths objectAtIndex:0];
    const auto productPath = config::get().string(config::values::product_path_mac);
    path = [path stringByAppendingPathComponent:[NSString stringWithUTF8String:(productPath.data())]];
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
QAction* createAction(MenuT* menu, const QString& title, const QString& shortcut, const Object* receiver, Method method)
{
    return menu->addAction(title, receiver, std::move(method), QKeySequence(shortcut));
}

void MacSupport::createMenuBar(bool simple)
{
    if (!mainMenu_)
    {
        mainMenu_ = new QMenuBar();

        auto menu = mainMenu_->addMenu(Utils::Translations::Get(qsl("&Edit")));
        menuItems_.insert({edit_undo, createAction(menu, Utils::Translations::Get(qsl("Undo")), qsl("Ctrl+Z"), mainWindow_, &Ui::MainWindow::undo)});
        menuItems_.insert({edit_redo, createAction(menu, Utils::Translations::Get(qsl("Redo")), qsl("Shift+Ctrl+Z"), mainWindow_, &Ui::MainWindow::redo)});
        menu->addSeparator();
        menuItems_.insert({edit_cut, createAction(menu, Utils::Translations::Get(qsl("Cut")), qsl("Ctrl+X"), mainWindow_, &Ui::MainWindow::cut)});
        menuItems_.insert({edit_copy, createAction(menu, Utils::Translations::Get(qsl("Copy")), qsl("Ctrl+C"), mainWindow_, &Ui::MainWindow::copy)});
        menuItems_.insert({edit_paste, createAction(menu, Utils::Translations::Get(qsl("Paste")), qsl("Ctrl+V"), mainWindow_, &Ui::MainWindow::paste)});
        //menuItems_.insert({edit_pasteAsQuote, createAction(menu, Utils::Translations::Get("Paste as Quote"), qsl("Alt+Ctrl+V"), mainWindow_, &Ui::MainWindow::quote)});
        menu->addSeparator();
        menuItems_.insert({edit_pasteEmoji, createAction(menu, Utils::Translations::Get(qsl("Emoji && Symbols")), qsl("Meta+Ctrl+Space"), mainWindow_, &Ui::MainWindow::pasteEmoji)});
        extendedMenus_.push_back(menu);
        editMenu_ = menu;
        QObject::connect(menu, &QMenu::aboutToShow, menu, []() { Q_EMIT Utils::InterConnector::instance().closeAnyPopupMenu(); });

        menu = mainMenu_->addMenu(Utils::Translations::Get(qsl("Contact")));

        if (config::get().is_on(config::features::add_contact))
        {
            menuItems_.insert({contact_addBuddy, createAction(menu, Utils::Translations::Get(qsl("Add Buddy")), qsl("Ctrl+N"), mainWindow_, [](){
                if (mainWindow_)
                    mainWindow_->onShowAddContactDialog({ Utils::AddContactDialogs::Initiator::From::HotKey });
            })});
        }

        menuItems_.insert({contact_profile, createAction(menu, Utils::Translations::Get(qsl("Profile")), qsl("Ctrl+I"), mainWindow_, &Ui::MainWindow::activateProfile)});

        auto code = Ui::get_gui_settings()->get_value<int>(settings_shortcuts_close_action, static_cast<int>(Ui::ShortcutsCloseAction::Default));
        closeAct_ = static_cast<Ui::ShortcutsCloseAction>(code);
        menuItems_.insert({contact_close, createAction(menu, Utils::getShortcutsCloseActionName(closeAct_), qsl("Ctrl+W"), mainWindow_, &Ui::MainWindow::activateShortcutsClose)});

        extendedMenus_.push_back(menu);
        QObject::connect(menu, &QMenu::aboutToShow, menu, []() { Q_EMIT Utils::InterConnector::instance().closeAnyPopupMenu(); });

        menu = mainMenu_->addMenu(Utils::Translations::Get(qsl("View")));
        menuItems_.insert({view_unreadMessage, createAction(menu, Utils::Translations::Get(qsl("Next Unread Message")), qsl("Ctrl+]"), mainWindow_, &Ui::MainWindow::activateNextUnread)});
        menuItems_.insert({view_fullScreen, createAction(menu, Utils::Translations::Get(qsl("Enter Full Screen")), qsl("Meta+Ctrl+F"), mainWindow_, &Ui::MainWindow::toggleFullScreen)});
        extendedMenus_.push_back(menu);
        QObject::connect(menu, &QMenu::aboutToShow, menu, []() { Q_EMIT Utils::InterConnector::instance().closeAnyPopupMenu(); });

        menu = mainMenu_->addMenu(Utils::Translations::Get(qsl("Window")));
        menuItems_.insert({window_minimizeWindow, createAction(menu, Utils::Translations::Get(qsl("Minimize")), qsl("Ctrl+M"), mainWindow_, &Ui::MainWindow::minimize)});
        menuItems_.insert({window_maximizeWindow, createAction(menu, Utils::Translations::Get(qsl("Zoom")), QString(), mainWindow_, &Ui::MainWindow::zoomWindow)});
        menu->addSeparator();
        menuItems_.insert({window_prevChat, createAction(menu, Utils::Translations::Get(qsl("Select Previous Chat")), qsl("Meta+Shift+Tab"), mainWindow_, &Ui::MainWindow::activatePrevChat)});
        menuItems_.insert({window_nextChat, createAction(menu, Utils::Translations::Get(qsl("Select Next Chat")), qsl("Meta+Tab"), mainWindow_, &Ui::MainWindow::activateNextChat)});
        menu->addSeparator();
        // in QMenu are no tags to make difference between items with same title, so ql1c(' ') is added as mark
        menuItems_.insert({window_mainWindow, createAction(menu, Utils::getAppTitle() % ql1c(' '), qsl("Ctrl+0"), mainWindow_, &Ui::MainWindow::activate)});
        menu->addSeparator();
        menuItems_.insert({window_bringToFront, createAction(menu, Utils::Translations::Get(qsl("Bring All To Front")), QString(), mainWindow_, &Ui::MainWindow::bringToFront)});
        menu->addSeparator();
        menuItems_.insert({window_switchMainWindow, createAction(menu, Utils::getAppTitle(), QString(), mainWindow_, &Ui::MainWindow::activate)});

        if (const auto& mainPage = mainWindow_->getMessengerPage())
            menuItems_[window_switchVoipWindow] = createAction(menu, Utils::Translations::Get(qsl("ICQ VOIP")), QString(), mainPage, &Ui::MainPage::showVideoWindow);

        windowMenu_ = menu;
        extendedMenus_.push_back(menu);
        QObject::connect(menu, &QMenu::aboutToShow, menu, []() { Q_EMIT Utils::InterConnector::instance().closeAnyPopupMenu(); });

        auto action = createAction(menu, qsl("about.*"), QString(), mainWindow_, &Ui::MainWindow::activateAbout);
        menuItems_.insert({global_about, action});
        extendedActions_.push_back(action);

        action = createAction(menu, qsl("settings"), QString(), mainWindow_, &Ui::MainWindow::activateSettings);
        menuItems_.insert({global_settings, action});
        extendedActions_.push_back(action);

        if (Utils::supportUpdates())
        {
            action = createAction(menu, Utils::Translations::Get(qsl("Check For Updates...")), QString(), mainWindow_, &Ui::MainWindow::checkForUpdates);
            action->setMenuRole(QAction::MenuRole::ApplicationSpecificRole);
            menuItems_.insert({global_update, action});
            extendedActions_.push_back(action);
        }

        mainMenu_->addAction(qsl("quit"), mainWindow_, &Ui::MainWindow::exit);

        mainWindow_->setMenuBar(mainMenu_);
    }
    else
    {
        if (windowMenu_)
        {
            if (const auto& mainPage = mainWindow_->getMessengerPage())
                menuItems_[window_switchVoipWindow] = createAction((QMenu*)windowMenu_, Utils::Translations::Get(qsl("ICQ VOIP")), QString(), mainPage, &Ui::MainPage::showVideoWindow);
        }
    }

    for (auto menu : extendedMenus_)
        menu->setEnabled(!simple);

    windowMenu_->setEnabled(true);

    for (auto action: extendedActions_)
        action->setEnabled(!simple);

    if (mainWindow_)
    {
        QObject::connect(&Utils::InterConnector::instance(), &Utils::InterConnector::omicronUpdated, mainWindow_, [this]() { updateMainMenu(); });
        QObject::connect(Ui::GetDispatcher(), &Ui::core_dispatcher::externalUrlConfigUpdated, mainWindow_, [this]() { updateMainMenu(); });
    }
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
        const bool isPttActive = Utils::InterConnector::instance().isRecordingPtt();
        editMenu_->setEnabled(!isPttActive);
    }

    if (Utils::supportUpdates())
        menuItems_[global_update]->setVisible(!Features::isUpdateFromBackendEnabled() || (Features::isUpdateFromBackendEnabled() && !Ui::getUrlConfig().getUrlAppUpdate().isEmpty()));

    QAction *nextChat = menuItems_[window_nextChat];
    QAction *prevChat = menuItems_[window_prevChat];
    QAction *fullScreen = menuItems_[view_fullScreen];
    QAction *contactClose = menuItems_[contact_close];
    QAction *windowMaximize = menuItems_[window_maximizeWindow];
    QAction *windowMinimize = menuItems_[window_minimizeWindow];
    QAction *windowVoip = menuItems_[window_switchVoipWindow];
    QAction *windowICQ = menuItems_[window_switchMainWindow];
    QAction *bringToFront = menuItems_[window_bringToFront];
    QAction *viewNextUnread = menuItems_[view_unreadMessage];

    menuItems_[window_mainWindow]->setEnabled(true);

    const auto mainPage = mainWindow_ ? mainWindow_->getMessengerPage() : nullptr;
    bool mainWindowActive = mainWindow_ && mainWindow_->isActive();
    const bool messengerPageDisplayed = mainWindow_ && mainWindow_->isMessengerPage();

    bool isCall = mainPage && mainPage->isVideoWindowOn();
    bool voipWindowActive = mainPage && mainPage->isVideoWindowActive();
    bool voipWindowMinimized = mainPage && mainPage->isVideoWindowMinimized();
    bool voipWindowFullscreen = !isCall || (mainPage && mainPage->isVideoWindowFullscreen());

    if (viewNextUnread)
        viewNextUnread->setEnabled(messengerPageDisplayed);

    auto code = Ui::get_gui_settings()->get_value<int>(settings_shortcuts_close_action, static_cast<int>(Ui::ShortcutsCloseAction::Default));
    auto act = static_cast<Ui::ShortcutsCloseAction>(code);
    if (closeAct_ != act && contactClose)
    {
        closeAct_ = act;
        contactClose->setText(Utils::getShortcutsCloseActionName(closeAct_));
    }
    if (contactClose)
        contactClose->setEnabled(messengerPageDisplayed || act != Ui::ShortcutsCloseAction::CloseChat);

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

    if (mainWindow_ && nextChat && prevChat && messengerPageDisplayed)
    {
        const QString &aimId = Logic::getContactListModel()->selectedContact();
        const auto enabled = !aimId.isEmpty() && !mainWindow_->isMinimized() && !inDock;
        const auto hasPrev = enabled && !Logic::getRecentsModel()->prevAimId(aimId).isEmpty();
        const auto hasNext = enabled && !Logic::getRecentsModel()->nextAimId(aimId).isEmpty();
        nextChat->setEnabled(hasNext && !voipWindowActive);
        prevChat->setEnabled(hasPrev && !voipWindowActive);
    }

    const auto notMiniAndFull = (!isCall || mainWindowActive) && !mainWindow_->isMinimized() && !isFullScreen && !inDock;
    const auto isFullResizeEnabled = mainWindow_ && (notMiniAndFull || (!voipWindowFullscreen && !voipWindowMinimized));
    if (windowMaximize)
        windowMaximize->setEnabled(isFullResizeEnabled);
    if (windowMinimize)
        windowMinimize->setEnabled(isFullResizeEnabled);

    if (windowVoip)
    {
        windowVoip->setVisible(isCall);
        windowVoip->setCheckable(true);
        windowVoip->setChecked(voipWindowActive);
    }

    if (windowICQ)
    {
        windowICQ->setCheckable(true);
        windowICQ->setEnabled(true);
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

            const QString appTitle = Utils::getAppTitle();
            NSMenuItem* winMainItem = [nsmenu itemWithTitle:appTitle.toNSString()];
            NSMenuItem* winCallItem = [nsmenu itemWithTitle:Utils::Translations::Get(qsl("ICQ VOIP")).toNSString()];

            [winMainItem setMixedStateImage:composedImage];
            [winCallItem setMixedStateImage:composedImage];

            if (mainWindow_ && mainWindow_->isMinimized())
                [winMainItem setState:NSMixedState];

            if (mainPage && mainPage->isVideoWindowMinimized())
                [winCallItem setState:NSMixedState];

            // workaround to prevent two items with the application name from being disabled in the Window menu
            const QString appTitleWithSpace = appTitle % ql1c(' ');
            auto winMainWithSpaceItem = [nsmenu itemWithTitle:appTitleWithSpace.toNSString()];
            [winMainWithSpaceItem setAction:@selector(activateMainWindow)];
            [winMainItem setAction:@selector(activateMainWindow)];
        }
    }
}

void MacSupport::log(QString logString)
{
    NSString * logString_ = fromQString(logString);

    NSLog(@"%@", logString_);
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
            if (e.keyCode == kVK_ANSI_Q && mainWindow_)
            {
                mainWindow_->exit();
                return true;
            }

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
                Q_EMIT Utils::InterConnector::instance().closeAnyPopupMenu();
                Q_EMIT Utils::InterConnector::instance().closeAnyPopupWindow(Utils::CloseWindowInfo({ Utils::CloseWindowInfo::Initiator::MacEventFilter,
                                                                                                    Utils::CloseWindowInfo::Reason::MacEventFilter }));
            }
            else if (mainWindow_ && ([e modifierFlags] & NSCommandKeyMask) && e.keyCode == kVK_ANSI_1)
            {
                mainWindow_->activate();
                Q_EMIT Utils::InterConnector::instance().applicationActivated();
            }
            else if (e.keyCode == kVK_ANSI_M)
            {
                Q_EMIT Utils::InterConnector::instance().closeAnyPopupMenu();
            }
        }
    }
    else if ([e type] == NSAppKitDefined && [e subtype] == NSApplicationDeactivatedEventType)
    {
        Q_EMIT Utils::InterConnector::instance().applicationDeactivated();
        Q_EMIT Utils::InterConnector::instance().closeAnyPopupMenu();
    }
    else if ([e type] == NSAppKitDefined && [e subtype] == NSApplicationActivatedEventType)
    {
        Q_EMIT Utils::InterConnector::instance().applicationActivated();
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
void MacSupport::saveFileName(const QString &caption, const QString &qdir, const QString &filter, std::function<void (const QString& _filename, const QString& _directory, bool _exportAsPng)> _callback, QString _ext, std::function<void ()> _cancel_callback)
{
    auto dir = qdir.toNSString().stringByDeletingLastPathComponent;
    auto filename = qdir.toNSString().lastPathComponent;

    NSSavePanel *panel = [NSSavePanel savePanel];
    [panel setTitle:caption.toNSString()];
    [panel setDirectoryURL:[NSURL URLWithString:dir]];
    [panel setShowsTagField:NO];
    [panel setNameFieldStringValue:filename];

    setEditMenuEnabled(false);

    [panel beginSheetModalForWindow:[NSApp mainWindow] completionHandler:^(NSInteger result) {
        if (result == NSFileHandlingPanelOKButton)
        {
            const auto path = QString::fromNSString([[panel URL] path]);
            if (!path.isEmpty())
            {
                const QFileInfo info(path);
                const QString directory = info.dir().absolutePath();
                const QString inputExt = !_ext.isEmpty() && info.suffix().isEmpty() ? _ext : QString();
                const auto isWebp = _ext.compare(u".webp", Qt::CaseInsensitive) == 0;
                const bool exportAsPng = isWebp && info.suffix().compare(u"png", Qt::CaseInsensitive) == 0;
                const QString filename = info.fileName() % inputExt;
                if (_callback)
                    _callback(filename, directory, exportAsPng);
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

        setEditMenuEnabled(true);
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
    [wnd setHidesOnDeactivate:NO];
}

void MacSupport::showNSModalPanelWindowLevel(QWidget* w)
{
    w->show();
    NSWindow *wnd = [reinterpret_cast<NSView *>(w->winId()) window];
    [wnd setLevel:NSModalPanelWindowLevel];
}

void MacSupport::showNSFloatingWindowLevel(QWidget* w)
{
    w->show();
    NSWindow *wnd = [reinterpret_cast<NSView *>(w->winId()) window];
    [wnd setLevel:NSFloatingWindowLevel];
}

void MacSupport::showOverAll(QWidget* w)
{
    w->show();
    NSWindow *wnd = [reinterpret_cast<NSView *>(w->winId()) window];
    [wnd setLevel:NSScreenSaverWindowLevel];
    [wnd setHidesOnDeactivate:NO];
}

void MacSupport::showInAllWorkspaces(QWidget *w)
{
    auto wnd = [reinterpret_cast<NSView *>(w->winId()) window];
    [wnd setLevel:NSScreenSaverWindowLevel];
    [wnd setCollectionBehavior:NSWindowCollectionBehaviorCanJoinAllSpaces | NSWindowCollectionBehaviorFullScreenAuxiliary
                               | NSWindowCollectionBehaviorStationary];
    [wnd setHidesOnDeactivate:NO];
    [wnd makeKeyAndOrderFront:nil];

    // Keep mini-window position
    NSRect frame = [wnd frame];
    const auto geo = w->geometry();
    frame.origin.x = geo.left();
    frame.origin.y = geo.top();
    [wnd setFrame:frame display:NO];
    w->setGeometry(geo);

    w->show();
}

bool MacSupport::isMetalSupported()
{
    static bool isMetalSupported = false;
    static bool checked = false;

    if (!checked)
    {
        NSOperatingSystemVersion minimumSupportedOSVersion = { .majorVersion = 10, .minorVersion = 12, .patchVersion = 0 };
        const auto isAtLeastSierra = [NSProcessInfo.processInfo isOperatingSystemAtLeastVersion:minimumSupportedOSVersion];

        if (isAtLeastSierra)
        {
            NSArray<id<MTLDevice>>* devices = MTLCopyAllDevices();

            for (id<MTLDevice> device in devices)
            {
                if (device)
                {
                    isMetalSupported = true;
                    break;
                }
            }
        }

        checked = true;
    }

    return isMetalSupported;
}

int MacSupport::getWidgetHeaderHeight(const QWidget& widget)
{
    // get parent window rect
    auto view = reinterpret_cast<NSView*>(widget.winId());
    im_assert(view);
    if (!view)
        return 0;

    auto window = [view window];
    im_assert(window);
    if (!window)
        return 0;

    const auto contentHeight = [window contentRectForFrameRect: window.frame].size.height;
    return static_cast<int>(window.frame.size.height - contentHeight);
}

void MacSupport::zoomWindow(WId wid)
{
    void *pntr = (void *)wid;
    NSView *view = (__bridge NSView *)pntr;
    [view.window zoom:nil];
}

MacMenuBlocker::MacMenuBlocker()
    : receiver_(std::make_unique<QObject>())
    , isBlocked_(false)
{
}

MacMenuBlocker::~MacMenuBlocker()
{
    unblock();
}

void MacMenuBlocker::block()
{
    if (isBlocked_)
        return;

    if (!macMenuState_.empty())
        macMenuState_.clear();

    macMenuState_.reserve(menuItems_.size());

    for (auto &[key, action] : menuItems_)
    {
        if (!action)
            continue;

        if (key == window_mainWindow || key == window_bringToFront
            || key == window_maximizeWindow || key == window_minimizeWindow
            || key == window_switchMainWindow)
        {
            continue;
        }

        macMenuState_.emplace_back(action, action->isEnabled());
        action->setEnabled(false);
    }
    isBlocked_ = true;
}

void MacMenuBlocker::unblock()
{
    for (auto [action, enabled]: macMenuState_)
    {
        QSignalBlocker blocker(action);
        action->setEnabled(enabled);
    }
    isBlocked_ = false;
}

void Utils::disableCloseButton(QWidget *_w)
{
    NSWindow *wnd = [reinterpret_cast<NSView *>(_w->winId()) window];
    NSButton *closeButton = [wnd standardWindowButton:NSWindowCloseButton];
    [closeButton setEnabled:NO];
}
