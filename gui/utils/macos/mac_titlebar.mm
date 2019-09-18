#include "stdafx.h"

#import <AppKit/AppKit.h>

#import "mac_titlebar.h"

#include "../../main_window/MainWindow.h"
#include "../../utils/InterConnector.h"
#include "../../styles/ThemeParameters.h"

struct MacTitlebarPrivate
{
    NSTextView* windowTitle_ = nil;
    NSWindow* nativeWindow_ = nil;
};

MacTitlebar::MacTitlebar(Ui::MainWindow* _mainWindow)
    : mainWindow_(_mainWindow)
    , d(std::make_unique<MacTitlebarPrivate>())
{
    void * pntr = (void*)mainWindow_->winId();
    NSView * view = (__bridge NSView *)pntr;
    d->nativeWindow_ = view.window;
}

MacTitlebar::~MacTitlebar()
{
    mainWindow_ = nullptr;
}

void MacTitlebar::setup()
{
    if (!mainWindow_ || !d->nativeWindow_)
        return;

    bool yosemiteOrLater = @available(macOS 10.10, *); //osx 10.10+

    auto gp = mainWindow_->geometry();
    if (yosemiteOrLater)
        [d->nativeWindow_ setTitleVisibility: NSWindowTitleHidden];

    auto g = mainWindow_->geometry();
    auto shift = (g.bottom() - gp.bottom()) - (g.top() - gp.top());
    g.setHeight(g.height() - shift);
    mainWindow_->setGeometry(g);

    if (yosemiteOrLater)
        initWindowTitle();
}

void MacTitlebar::initWindowTitle()
{
    if (d->windowTitle_ || !d->nativeWindow_)
        return;

    d->nativeWindow_.titlebarAppearsTransparent = YES;
    updateTitleBgColor();

    d->windowTitle_ = [[NSTextView alloc] initWithFrame:CGRectMake(0, 0, 64, 22)];
    [d->windowTitle_ setSelectable: NO];
    [d->windowTitle_ setEditable: NO];
    d->windowTitle_.drawsBackground = NO;
    d->windowTitle_.rulerVisible = NO;
    [d->windowTitle_ setAlignment:NSCenterTextAlignment];
    d->windowTitle_.font = [NSFont titleBarFontOfSize:14.0];
    d->windowTitle_.string = @"???";
    [d->windowTitle_ setAutoresizingMask:NSViewWidthSizable|NSViewMinYMargin];
    [d->windowTitle_ setTextContainerInset:NSMakeSize(0, 3)];//magic number

    updateTitleTextColor();

    NSView *themeFrame = [[d->nativeWindow_ contentView] superview];
    NSRect c = [themeFrame frame];	// c for "container"
    NSRect aV = [d->windowTitle_ frame];     // aV for "accessory view"
    NSRect newFrame = NSMakeRect(
                                 0,
                                 c.size.height - aV.size.height,	// y position
                                 c.size.width,	// width
                                 22.0);	// height
    [d->windowTitle_ setFrame:newFrame];
    [themeFrame addSubview: d->windowTitle_ positioned:NSWindowBelow relativeTo:nil];
}

void MacTitlebar::updateTitleTextColor()
{
    if (d->windowTitle_)
    {
        const auto color = Styling::getParameters().getColor(Styling::StyleVariable::TEXT_SOLID);
        const auto isWindowActive = !Utils::InterConnector::instance().isInBackground();
        const auto opacity = color.alphaF() * (isWindowActive ? 1.0 : 0.5);

        d->windowTitle_.textColor = [NSColor
                colorWithCalibratedRed: color.redF()
                green: color.greenF()
                blue: color.blueF()
                alpha: opacity];

        [d->windowTitle_ setNeedsDisplay:YES];
    }
}

void MacTitlebar::updateTitleBgColor()
{
    if (d->nativeWindow_)
    {
        const auto color = Styling::getParameters().getColor(Styling::StyleVariable::APP_PRIMARY);

        if (!@available(macOS 10.12, *))
            [d->nativeWindow_ setStyleMask:[d->nativeWindow_ styleMask] | NSTexturedBackgroundWindowMask];

        d->nativeWindow_.backgroundColor = [NSColor
                colorWithCalibratedRed: color.redF()
                green: color.greenF()
                blue: color.blueF()
                alpha: color.alphaF()];
    }
}

void MacTitlebar::setTitleText(const QString& _text)
{
    if(d->windowTitle_)
        d->windowTitle_.string = _text.toNSString();
}
