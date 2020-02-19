#import <Availability.h>
#import <TargetConditionals.h>

#if TARGET_OS_IPHONE
#import <UIKit/UIKit.h>
#else
#import <Cocoa/Cocoa.h>
#import <AppKit/AppKit.h>
#endif

@protocol VOIPRenderViewProtocol
- (CGFloat)renderScaleFactor;
- (void)updateRenderScaleFactor;
@end

#if TARGET_OS_IPHONE

@interface VOIPMTLRenderView : UIView <VOIPRenderViewProtocol>
@end

@interface VOIPRenderViewOGLES : UIView <VOIPRenderViewProtocol>
@end

#else

NS_AVAILABLE_MAC(10.11)
@interface VOIPMTLRenderView : NSView <VOIPRenderViewProtocol>
+ (BOOL)isSupported;
@end

@interface VOIPRenderViewOGL : NSOpenGLView <VOIPRenderViewProtocol>
@end

#endif



