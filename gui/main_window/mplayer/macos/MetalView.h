#pragma once

#import <Cocoa/Cocoa.h>
#import <MetalKit/MetalKit.h>

@interface MetalView : MTKView

- (instancetype)initWithFrame:(CGRect)frame;

- (void) setFrameImage : (CGImageRef) image;

- (void) setFrameWithSize : (CGImageRef) image size : (NSSize) size;

@end
