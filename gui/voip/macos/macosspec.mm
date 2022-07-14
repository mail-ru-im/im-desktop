#include "stdafx.h"
#import "macosspec.h"
#import <Foundation/Foundation.h>
#import <AppKit/AppKit.h>
#import <IOKit/pwr_mgt/IOPMLib.h>
#include "render/VOIPRenderViewClasses.h"
#include "../../utils/utils.h"
#include "iTunes.h"
#include "styles/ThemeParameters.h"
#include "voip/CommonUI.h"

@interface WindowRect : NSObject
{
}

@property NSRect rect;

@end

@implementation WindowRect

-(instancetype)init
{
    self = [super init];
    if (!!self)
    {
        NSRect rc;
        rc.origin.x = 0.0f;
        rc.origin.y = 0.0f;
        rc.size.width  = 0.0f;
        rc.size.height = 0.0f;
        self.rect = rc;
    }
    return self;
}

@end

@interface WindowListApplierData : NSObject
{
}

@property (strong, nonatomic) NSMutableArray* outputArray;
@property int order;

@end

@implementation WindowListApplierData

-(instancetype)initWindowListData:(NSMutableArray *)array
 {
    self = [super init];
    if (!!self)
    {
        self.outputArray = array;
        self.order = 0;
    }
    return self;
}

-(void)dealloc
{
    if (!!self && !!self.outputArray)
    {
        for (id item in self.outputArray)
        {
            [item release];
        }
    }
    [super dealloc];
}

@end

@interface FullScreenDelegate : NSObject <NSWindowDelegate>
{
    platform_macos::FullScreenNotificaton* _notification;
    // Save prev delegate
    NSObject <NSWindowDelegate>* _oldDelegate;
}
@end

@implementation FullScreenDelegate

-(id)init: (platform_macos::FullScreenNotificaton*) notification
     withOldDelegate:(NSObject <NSWindowDelegate>*) oldDelegate
{
    if (![super init])
        return nil;
    _notification = notification;
    _oldDelegate  = oldDelegate;
    [self registerSpaceNotification];
    return self;
}

-(void)dealloc
{
    [self unregisterSpaceNotification];
    [super dealloc];
}

-(void)windowWillEnterFullScreen:(NSNotification *)notification
{
    if (_notification)
    {
        _notification->fullscreenAnimationStart();
        _notification->changeFullscreenState(true);
    }
}

-(void)windowDidEnterFullScreen:(NSNotification *)notification
{
    if (_notification)
        _notification->fullscreenAnimationFinish();
}

-(void)windowWillExitFullScreen:(NSNotification *)notification
{
    if (_notification)
    {
        _notification->fullscreenAnimationStart();
        _notification->changeFullscreenState(false);
    }
}

-(void)windowDidExitFullScreen:(NSNotification *)notification
{
    if (_notification)
        _notification->fullscreenAnimationFinish();
}

-(void)windowWillMiniaturize:(NSNotification *)notification
{
    if (_notification)
        _notification->minimizeAnimationStart();
}

-(void)windowDidMiniaturize:(NSNotification *)notification
{
    if (_notification)
       _notification->minimizeAnimationFinish();
}

-(void)windowDidDeminiaturize:(NSNotification *)notification
{
    if (_notification)
       _notification->minimizeAnimationFinish();
}



-(void)activeSpaceDidChange:(NSNotification *)notification
{
    if (_notification)
        _notification->activeSpaceDidChange();
}

-(void)registerSpaceNotification
{
    NSNotificationCenter *notificationCenter = [[NSWorkspace sharedWorkspace] notificationCenter];
    if (notificationCenter)
    {
        [notificationCenter addObserver:self
           selector:@selector(activeSpaceDidChange:)
               name:NSWorkspaceActiveSpaceDidChangeNotification
             object:[NSWorkspace sharedWorkspace]];
    }
}

-(void)unregisterSpaceNotification
{
    NSNotificationCenter *notificationCenter = [[NSWorkspace sharedWorkspace] notificationCenter];
    if (notificationCenter)
        [notificationCenter removeObserver:self];
}

-(BOOL)windowShouldClose:(id)sender
{
    if (_oldDelegate)
        return [_oldDelegate windowShouldClose: sender];
    return YES;
}

-(NSObject<NSWindowDelegate>*)getDelegate
{
    return _oldDelegate;
}

@end

NSString *kAppNameKey      = @"applicationName"; // Application Name & PID
NSString *kWindowOriginKey = @"windowOrigin";    // Window Origin as a string
NSString *kWindowSizeKey   = @"windowSize";      // Window Size as a string
NSString *kWindowRectKey   = @"windowRect";      // The overall front-to-back ordering of the windows as returned by the window server
NSString *kWindowIDKey     = @"windowId";

inline uint32_t ChangeBits(uint32_t currentBits, uint32_t flagsToChange, BOOL setFlags)
{
    if (setFlags)
        return currentBits | flagsToChange;
    else
        return currentBits & ~flagsToChange;
}

void WindowListApplierFunction(const void *inputDictionary, void *context)
{
    im_assert(!!inputDictionary && !!context);
    if (!inputDictionary || !context)
        return;

    NSDictionary* entry         = (__bridge NSDictionary*)inputDictionary;
    WindowListApplierData* data = (__bridge WindowListApplierData*)context;

    int sharingState = [entry[(id)kCGWindowSharingState] intValue];
    if (sharingState != kCGWindowSharingNone)
    {
        NSMutableDictionary *outputEntry = [NSMutableDictionary dictionary];
        im_assert(!!outputEntry);
        if (!outputEntry)
            return;

        NSString* applicationName = entry[(id)kCGWindowOwnerName];
        NSString* windowName = entry[(id)kCGWindowName];
        if(applicationName != NULL && windowName != NULL)
        {
            if ([applicationName compare:@"dock" options:NSCaseInsensitiveSearch] == NSOrderedSame)
                return;
            if ([applicationName compare:@"screenshot" options:NSCaseInsensitiveSearch] == NSOrderedSame)
                return;
            if ([windowName length] == 0)
                return;

#ifdef _DEBUG
            NSString *nameAndPID = [NSString stringWithFormat:@"%@ (%@)", applicationName, entry[(id)kCGWindowOwnerPID]];
            outputEntry[kAppNameKey] = nameAndPID;
#endif
        } else
            return;

        CGRect bounds;
        CGRectMakeWithDictionaryRepresentation((CFDictionaryRef)entry[(id)kCGWindowBounds], &bounds);
#ifdef _DEBUG
        NSString *originString = [NSString stringWithFormat:@"%.0f/%.0f", bounds.origin.x, bounds.origin.y];
        outputEntry[kWindowOriginKey] = originString;
        NSString *sizeString = [NSString stringWithFormat:@"%.0f*%.0f", bounds.size.width, bounds.size.height];
        outputEntry[kWindowSizeKey] = sizeString;
#endif
        outputEntry[kWindowIDKey] = entry[(id)kCGWindowNumber];

        WindowRect* wr = [[WindowRect alloc] init];
        wr.rect = bounds;
        outputEntry[kWindowRectKey] = wr;

        [data.outputArray addObject:outputEntry];
    }
}

void platform_macos::fadeIn(QWidget* wnd)
{
    im_assert(!!wnd);
    if (!wnd)
        return;

    NSView* view = (NSView*)wnd->effectiveWinId();
    im_assert(view);

    NSWindow* window = [view window];
    im_assert(window);

    if ([[window animator] alphaValue] < 0.01f)
        [[window animator] setAlphaValue:1.0f];
}

void platform_macos::fadeOut(QWidget* wnd)
{
    im_assert(!!wnd);
    if (!wnd)
        return;

    NSView* view = (NSView*)wnd->effectiveWinId();
    im_assert(view);

    NSWindow* window = [view window];
    im_assert(window);

    if ([[window animator] alphaValue] > 0.9f)
        [[window animator] setAlphaValue:0.0f];
}

void platform_macos::setPanelAttachedAsChild(bool attach, QWidget* parent, QWidget* child)
{
    NSView* parentView = (NSView*)parent->effectiveWinId();
    im_assert(parentView);
    if (!parentView)
        return;

    NSWindow* parentWindow = [parentView window];
    im_assert(parentWindow);

    NSView* childView = (NSView*)child->effectiveWinId();
    im_assert(childView);
    if (!childView)
        return;

    NSWindow* childWindow = [childView window];
    im_assert(childWindow);
    if (!childWindow)
        return;

    if (attach)
        [parentWindow addChildWindow:childWindow ordered:NSWindowAbove];
    else
        [parentWindow removeChildWindow:childWindow];
}

void platform_macos::setWindowPosition(QWidget& widget, const QRect& widgetRect)
{
    NSView* view = (NSView*)widget.effectiveWinId();
    im_assert(view);

    NSWindow* window = [view window];
    im_assert(window);

    NSRect rect;
    rect.size.width  = widgetRect.width();
    rect.size.height = widgetRect.height();
    rect.origin.x    = widgetRect.left();
    rect.origin.y    = widgetRect.top();

    [window setFrame:rect display:YES];
}

QRect platform_macos::getWidgetRect(const QWidget& widget)
{
    NSRect rect;
    {
        // get parent window rect
        NSView* view = (NSView*)widget.effectiveWinId();
        im_assert(view);
        if (!view)
            return QRect();

        NSWindow* window = [view window];
        im_assert(window);
        if (!window)
            return QRect();

        rect = [window frame];
    }
    return QRect(rect.origin.x, rect.origin.y, rect.size.width, rect.size.height);
}

void setAspectRatioForWindow(QWidget& wnd, float w, float h)
{
    NSView* view = (NSView*)wnd.effectiveWinId();
    im_assert(view);
    if (view)
    {
        NSWindow* window = [view window];
        im_assert(window);
        if (window)
        {
            if (w > 0.0f)
                [window setContentAspectRatio: NSMakeSize(w, h)];
            else
                [window setResizeIncrements: NSMakeSize(1.0, 1.0)];
        }
    }
}

void platform_macos::setAspectRatioForWindow(QWidget& wnd, float aspectRatio)
{
    setAspectRatioForWindow(wnd, 10.0f * aspectRatio, 10.0f);
}

void platform_macos::unsetAspectRatioForWindow(QWidget& wnd)
{
    setAspectRatioForWindow(wnd, 0.0f, 0.0f);
}

bool platform_macos::windowIsOnActiveSpace(QWidget* _widget)
{
    if (_widget)
        if (auto wnd = [reinterpret_cast<NSView *>(_widget->effectiveWinId()) window])
            return [wnd isOnActiveSpace];

    return false;
}

bool platform_macos::windowIsOverlapped(QWidget* _frame)
{
    im_assert(!!_frame);
    if (!_frame)
        return false;

    NSView* view = (NSView*)_frame->effectiveWinId();
    im_assert(!!view);
    if (!view)
        return false;

    NSWindow* window = [view window];
    im_assert(!!window);
    if (!window)
        return false;

    QRect screenGeometry = QDesktopWidget().availableGeometry(_frame);

    const CGWindowID windowID = (CGWindowID)[window windowNumber];
    CGWindowListOption listOptions = kCGWindowListOptionOnScreenAboveWindow;
    listOptions = ChangeBits(listOptions, kCGWindowListExcludeDesktopElements, YES);

    CFArrayRef windowList = CGWindowListCopyWindowInfo(listOptions, windowID);
    NSMutableArray* prunedWindowList = [NSMutableArray array];
    WindowListApplierData* windowListData = [[WindowListApplierData alloc] initWindowListData:prunedWindowList];

    CFArrayApplyFunction(windowList, CFRangeMake(0, CFArrayGetCount(windowList)), &WindowListApplierFunction, (__bridge void *)(windowListData));

    // Flip y coord.
    const QRect frameRect([window frame].origin.x, screenGeometry.height() - int([window frame].origin.y + [window frame].size.height), [window frame].size.width, [window frame].size.height);
    const int originalSquare = frameRect.width() * frameRect.height();

    QRegion selfRegion(frameRect);
    for (NSMutableDictionary* params in windowListData.outputArray)
    {
        WindowRect* wr = params[kWindowRectKey];
        QRect rc(wr.rect.origin.x, wr.rect.origin.y, wr.rect.size.width, wr.rect.size.height);
        QRegion wrRegion(rc);
        selfRegion -= wrRegion;
    }

    int remainsSquare = 0;
    const auto remainsRects = selfRegion.rects();
    for (unsigned ix = 0; ix < remainsRects.count(); ++ix)
    {
        const QRect& rc = remainsRects.at(ix);
        remainsSquare += rc.width() * rc.height();
    }

    [windowListData release];
    CFRelease(windowList);

    return remainsSquare < originalSquare*0.6f;
}

bool platform_macos::pauseiTunes()
{
    bool res = false;
    iTunesApplication* iTunes = [SBApplication applicationWithBundleIdentifier:@"com.apple.iTunes"];
    if ([iTunes isRunning])
    {
        // pause iTunes if it is currently playing.
        if (iTunesEPlSPlaying == [iTunes playerState])
        {
            [iTunes playpause];
            res = true;
        }
    }
    return res;
}

void platform_macos::playiTunes()
{
    iTunesApplication* iTunes = [SBApplication applicationWithBundleIdentifier:@"com.apple.iTunes"];
    if ([iTunes isRunning])
    {
        // play iTunes if it was paused.
        if (iTunesEPlSPaused == [iTunes playerState])
        {
            [iTunes playpause];
        }
    }
}

void platform_macos::moveAboveParentWindow(QWidget& parent, QWidget& child)
{
    NSView* parentView = (NSView*)parent.effectiveWinId();
    im_assert(parentView);

    NSWindow* window = [parentView window];
    im_assert(window);

    NSView* childView = (NSView*)child.effectiveWinId();
    im_assert(childView);
    if (!childView)
        return;

    NSWindow* childWnd = [childView window];
    im_assert(childWnd);
    if (!childWnd)
        return;

    [childWnd orderWindow: NSWindowAbove relativeTo: [window windowNumber]];
}

int platform_macos::doubleClickInterval()
{
    return [NSEvent doubleClickInterval] * 1000;
}

void platform_macos::showInWorkspace(QWidget* _w)
{
    auto wnd = [reinterpret_cast<NSView *>(_w->effectiveWinId()) window];
    [wnd setCollectionBehavior:NSWindowCollectionBehaviorMoveToActiveSpace];
}

QSize platform_macos::framebufferSize(QWidget* _w)
{
    NSView* view = (NSView*)_w->effectiveWinId();
    NSRect backingBounds = [view convertRectToBacking: [view bounds]];
    return QSize((int)backingBounds.size.width, (int)backingBounds.size.height);
}

NSWindow* getNSWindow(QWidget& parentWindow)
{
    NSWindow* res = nil;
    NSView* view = (NSView*)parentWindow.winId();
    im_assert(view);
    if (view)
        res = [view window];
    return res;
}

platform_macos::FullScreenNotificaton::FullScreenNotificaton (QWidget& parentWindow) : _delegate(nil), _parentWindow(parentWindow)
{
    NSWindow* window = getNSWindow(_parentWindow);
    im_assert(!!window);
    if (window)
    {
        FullScreenDelegate* delegate = [[FullScreenDelegate alloc] init: this withOldDelegate: [window delegate]];
        [window setDelegate: delegate];
        _delegate = delegate;
        //[delegate retain];
    }
}

platform_macos::FullScreenNotificaton::~FullScreenNotificaton ()
{
    NSWindow* window = getNSWindow(_parentWindow);
    if (window)
    {
        if (_delegate)
        {
            // return old delegate back.
            [window setDelegate: [(FullScreenDelegate*)_delegate getDelegate]];
        } else
        {
            [window setDelegate: nil];
        }
    }
    if (_delegate != nil)
        [(FullScreenDelegate*)_delegate release];
}
