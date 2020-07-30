#include "stdafx.h"

#include "Emoji_mac.h"
#include "../EmojiDb.h"

#import <AppKit/AppKit.h>

#include <QtMacExtras/QtMac>

namespace Emoji
{
namespace mac
{

static std::unordered_multiset<int> glyphs;

static int getGlyphIndex(const EmojiCode& code)
{
    @autoreleasepool {
        NSString *string = EmojiCode::toQString(code).toNSString();
        CFAttributedStringRef richText = (CFAttributedStringRef)[[[NSAttributedString alloc]initWithString:string] autorelease];
        CTLineRef line = CTLineCreateWithAttributedString(richText);
        if (CTLineGetGlyphCount(line) == 1)
        {
            CFArrayRef glyphArray = CTLineGetGlyphRuns(line);
            if (glyphArray != NULL)
            {
                CFIndex size = CFArrayGetCount(glyphArray);
                assert (size == 1);

                for (CFIndex i = 0; i < size; ++i)
                {
                    CTRunRef glyphRun = (CTRunRef)CFArrayGetValueAtIndex(glyphArray, i);
                    CFRange runRange = CTRunGetStringRange(glyphRun);
                    CGGlyph glyphBuffer[runRange.length];
                    const CGGlyph *tmpGlyphs = CTRunGetGlyphsPtr(glyphRun);
                    if (!tmpGlyphs)
                        CTRunGetGlyphs(glyphRun, runRange, glyphBuffer);
                    const auto res = tmpGlyphs ? tmpGlyphs : glyphBuffer;
                    return *res;
                }
            }
        }
        return 0;
    }
}

void setEmojiVector(const EmojiRecordVec& vector)
{
    glyphs.clear();
    for (const auto& record : vector)
    {
        const auto g = getGlyphIndex(record.fullCodePoints);
        if (g)
            glyphs.insert(g);
    }
}

namespace
{
    constexpr bool isBlackListed(int g) noexcept
    {
        switch (g)
        {
        case 0:
        case 4:
            return true;

        default:
            return false;
        }
    }
}

bool supportEmoji(const EmojiCode& code)
{
    static std::unordered_map<EmojiCode, std::optional<bool>, EmojiCodeHasher> supportCache;

    auto& x = supportCache[code];
    if (x)
        return *x;

    const auto g = getGlyphIndex(code);
    const auto res = g && !isBlackListed(g);
    x = std::make_optional<bool>(res);
    return res;
}

QImage getEmoji(const EmojiCode& code, int rectSize)
{
@autoreleasepool {

      NSString *string = EmojiCode::toQString(code).toNSString();

      const auto fontSize = rectSize - std::pow(rectSize, 0.5); // hack

      NSFont *font = [NSFont fontWithName:@"AppleColorEmoji" size: fontSize];

      NSMutableParagraphStyle* style = [[[NSMutableParagraphStyle alloc] init] autorelease];
      [style setAlignment:NSCenterTextAlignment];

      NSDictionary *attributes = @{ NSFontAttributeName : font,
                                    NSForegroundColorAttributeName : NSColor.blackColor,
                                    NSParagraphStyleAttributeName : style };

      auto emojiSize = [string sizeWithAttributes : attributes];
      auto xOffset = (rectSize - emojiSize.width) / 2;
      auto yOffset = 0;

      const auto macos10_15 = (NSOperatingSystemVersion) {10, 15, 0};
      if ([[NSProcessInfo processInfo] isOperatingSystemAtLeastVersion:macos10_15])
      {
          auto yShift = [string boundingRectWithSize:NSZeroSize options:0 attributes:attributes].origin.y;
          auto yShiftDevice = [string boundingRectWithSize:NSZeroSize options:NSStringDrawingUsesDeviceMetrics attributes:attributes].origin.y;
          yOffset = yShiftDevice;
          if (yOffset > 0)
              yOffset = yShift;
      }
      else
      {
          const auto ascent = [font ascender];
          const auto descent = std::abs([font descender]);
          yOffset =  -(((ascent + descent) / 2 - descent) / 2);
      }

      NSBitmapImageRep *rep = [[[NSBitmapImageRep alloc]
                                        initWithBitmapDataPlanes: NULL
                                        pixelsWide: rectSize
                                        pixelsHigh: rectSize
                                        bitsPerSample: 8
                                        samplesPerPixel: 4
                                        hasAlpha: YES
                                        isPlanar: NO
                                        colorSpaceName: NSDeviceRGBColorSpace
                                        bytesPerRow: rectSize * 4
                                        bitsPerPixel: 32] autorelease];
      NSGraphicsContext *context = [NSGraphicsContext graphicsContextWithBitmapImageRep: rep];

      [NSGraphicsContext saveGraphicsState];
      [NSGraphicsContext setCurrentContext: context];
      [string drawAtPoint:NSMakePoint(xOffset, yOffset) withAttributes: attributes];
      [context flushGraphics];
      [NSGraphicsContext restoreGraphicsState];

      //qDebug() << qApp->applicationFilePath();
      //const auto res = QtMac::fromCGImageRef(cgRef).save(ql1s("/Users/antonkudravcev/work/") % str % ql1s(".png"));

      return QtMac::fromCGImageRef(rep.CGImage).toImage();
    }
}
}
}
