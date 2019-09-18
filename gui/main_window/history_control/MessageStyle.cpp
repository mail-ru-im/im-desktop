#include "stdafx.h"

#include "MessageStyle.h"

#include "../../fonts.h"

#include "../../utils/utils.h"

#include "styles/ThemeParameters.h"

namespace
{
    constexpr auto senderFontSize() { return 14; }
}


namespace Ui::MessageStyle
{
    QFont getTextFont()
    {
        constexpr int size = platform::is_linux() ? 16 : 15;
        auto font = Fonts::adjustedAppFont(size);

        if constexpr (platform::is_apple())
            font.setLetterSpacing(QFont::AbsoluteSpacing, -0.25);

        return font;
    }

    int getTextLineSpacing()
    {
        if constexpr (platform::is_apple())
            return Utils::scale_value(3);
        else
            return 0;
    }

    QFont getMarkdownFont()
    {
        constexpr int size = platform::is_apple() ? 16 : 15;
        return Fonts::adjustedAppFont(size, Fonts::FontFamily::ROBOTO_MONO);
    }

    QFont getImagePreviewLinkFont()
    {
        return getTextFont();
    }

    QColor getTextColor()
    {
        return Styling::getParameters().getColor(Styling::StyleVariable::TEXT_SOLID);
    }

    QColor getLinkColor(bool _isOutgoing)
    {
        return Styling::getParameters().getColor(_isOutgoing ? Styling::StyleVariable::PRIMARY_INVERSE : Styling::StyleVariable::TEXT_PRIMARY);
    }

    QColor getSelectionFontColor()
    {
        return Styling::getParameters().getColor(Styling::StyleVariable::TEXT_SOLID);
    }

    QColor getTextSelectionColor(const QString& _aimId)
    {
        return Styling::getParameters(_aimId).getColor(Styling::StyleVariable::PRIMARY_BRIGHT_HOVER);
    }

    QColor getSelectionColor()
    {
        QColor color(Styling::getParameters().getColor(Styling::StyleVariable::PRIMARY));
        color.setAlphaF(0.3);
        return color;
    }

    QColor getHighlightColor()
    {
        return Styling::getParameters().getColor(Styling::StyleVariable::SECONDARY_RAINBOW_WARNING);
    }

    QFont getSenderFont()
    {
        if constexpr (platform::is_apple())
            return Fonts::adjustedAppFont(senderFontSize(), Fonts::FontWeight::SemiBold);
        else
            return Fonts::adjustedAppFont(senderFontSize(), Fonts::FontWeight::Normal);
    }

    QFont getQuoteSenderFont()
    {
        if constexpr (platform::is_apple())
            return Fonts::adjustedAppFont(senderFontSize(), Fonts::FontWeight::Medium);
        else
            return Fonts::adjustedAppFont(senderFontSize(), Fonts::FontWeight::Normal);
    }

    QFont getQuoteSenderFontMarkdown()
    {
        return Fonts::adjustedAppFont(senderFontSize(), Fonts::FontWeight::SemiBold);
    }

    QColor getBorderColor()
    {
        return Styling::getParameters().getColor(Styling::StyleVariable::GHOST_SECONDARY);

    }
    int getBorderWidth()
    {
        return Utils::scale_value(1);
    }

    QBrush getBodyBrush(const bool _isOutgoing, const bool _isSelected, const QString& _aimId)
    {
        return Styling::getParameters(_aimId).getColor(_isSelected ? Styling::StyleVariable::PRIMARY : (_isOutgoing ? Styling::StyleVariable::CHAT_SECONDARY : Styling::StyleVariable::CHAT_PRIMARY));
    }

    int32_t getMinBubbleHeight()
    {
        return Utils::scale_value(36);
    }

    int32_t getBorderRadius()
    {
        return Utils::scale_value(12);
    }

    int32_t getBorderRadiusSmall()
    {
        return Utils::scale_value(4);
    }

    int32_t getTopMargin(const bool hasTopMargin)
    {
        return Utils::scale_value(hasTopMargin ? 8 : 2);
    }

    int32_t getRightBubbleMargin()
    {
        return Utils::scale_value(32);
    }

    int32_t getLeftBubbleMargin()
    {
        return Utils::scale_value(56);
    }

    int32_t getLeftMargin(const bool _isOutgoing, const int _width)
    {
        if (!_isOutgoing)
            return getLeftBubbleMargin();

        if (_width >= fiveHeadsWidth())
            return Utils::scale_value(136);

        if (_width >= fourHeadsWidth())
            return Utils::scale_value(116);

        if (_width >= threeHeadsWidth())
            return Utils::scale_value(96);

        return getLeftBubbleMargin();
    }

    int32_t getRightMargin(const bool _isOutgoing, const int _width)
    {
        if (_isOutgoing)
            return getRightBubbleMargin();

        if (_width >= fiveHeadsWidth())
            return Utils::scale_value(136);

        if (_width >= fourHeadsWidth())
            return Utils::scale_value(116);

        if (_width >= threeHeadsWidth())
            return Utils::scale_value(96);

        return getRightBubbleMargin();
    }

    int32_t getSenderTopPadding()
    {
        return getBubbleVerPadding();
    }

    int32_t getSenderBaseline()
    {
        return getSenderHeight() - Utils::scale_value(4);
    }

    int32_t getAvatarSize()
    {
        return Utils::scale_value(32);
    }

    int32_t getAvatarRightMargin()
    {
        return Utils::scale_value(6);
    }

    int32_t getBubbleHorPadding()
    {
        return Utils::scale_value(12);
    }

    int32_t getBubbleVerPadding()
    {
        return Utils::scale_value(8);
    }

    int32_t getLastReadAvatarSize()
    {
        return Utils::scale_value(16);
    }

    int32_t getLastReadAvatarBottomMargin()
    {
        return Utils::scale_value(2);
    }

    int32_t getLastReadAvatarOffset()
    {
        return Utils::scale_value(4);
    }

    QSize getLastStatusIconSize()
    {
        return Utils::scale_value(QSize(16, 16));
    }

    int32_t getLastReadAvatarMargin()
    {
        return Utils::scale_value(12);
    }

    int32_t getLastStatusLeftMargin()
    {
        return Utils::scale_value(4);
    }

    int32_t getHistoryWidgetMaxWidth()
    {
        return Utils::scale_value(1100);
    }

    int32_t getSenderHeight()
    {
        return Utils::scale_value(18);
    }

    int32_t getSenderStickerWidth()
    {
        return Utils::scale_value(360);
    }

    QFont getRotatingProgressBarTextFont()
    {
        return Fonts::adjustedAppFont(12, Fonts::FontWeight::SemiBold);
    }

    QPen getRotatingProgressBarTextPen()
    {
        const static QPen pen(Styling::getParameters().getColor(Styling::StyleVariable::TEXT_SOLID_PERMANENT));
        return pen;
    }

    int32_t getRotatingProgressBarTextTopMargin()
    {
        return Utils::scale_value(10);
    }

    int32_t getRotatingProgressBarPenWidth()
    {
        return Utils::scale_value(3);
    }

    QPen getRotatingProgressBarPen()
    {
        const static QPen pen(Styling::getParameters().getColor(Styling::StyleVariable::PRIMARY), getRotatingProgressBarPenWidth());
        return pen;
    }

    int32_t getSenderBottomMargin()
    {
        return Utils::scale_value(4);
    }

    int32_t getSenderBottomMarginLarge()
    {
        return Utils::scale_value(8);
    }

    int32_t getTextWidthStep()
    {
        return Utils::scale_value(20);
    }

    int32_t roundTextWidthDown(const int32_t width)
    {
        assert(width > 0);

        return ((width / getTextWidthStep()) * getTextWidthStep());
    }

    int32_t getSnippetMaxWidth()
    {
        return Utils::scale_value(420);
    }

    int32_t getHistoryBottomOffset()
    {
        return Utils::scale_value(12);
    }

    bool isShowLinksInImagePreview()
    {
        return false;
    }

    int getFileSharingMaxWidth()
    {
        return Utils::scale_value(580);
    }

    int getFileSharingDesiredWidth()
    {
        return Utils::scale_value(320);
    }

    int getQuoteBlockDesiredWidth()
    {
        return Utils::scale_value(108);
    }

    int getPreviewDesiredWidth()
    {
        return Utils::scale_value(360);
    }

    int getVoipDesiredWidth()
    {
        return Utils::scale_value(300);
    }

    int getProfileBlockDesiredWidth()
    {
        return Utils::scale_value(320);
    }

    int getShiftedTimeTopMargin()
    {
        return Utils::scale_value(4);
    }

    int getTimeLeftSpacing()
    {
        return Utils::scale_value(8);
    }

    QSize getSharingButtonSize()
    {
        return Utils::scale_value(QSize(28, 28));
    }

    int getSharingButtonMargin()
    {
        return Utils::scale_value(8);
    }

    int getSharingButtonOverlap()
    {
        return Utils::scale_value(12);
    }

    int getSharingButtonAnimationShift(const bool _isOutgoing)
    {
        return (_isOutgoing ? 1 : -1) * (MessageStyle::getSharingButtonMargin() + MessageStyle::getSharingButtonOverlap());
    }

    int32_t getProgressTextRectHMargin()
    {
        return Utils::scale_value(4);
    }

    int32_t getProgressTextRectVMargin()
    {
        return Utils::scale_value(2);
    }

    int fiveHeadsWidth()
    {
        return Utils::scale_value(500);
    }

    int fourHeadsWidth()
    {
        return Utils::scale_value(480);
    }

    int threeHeadsWidth()
    {
        return Utils::scale_value(460);
    }

    int32_t getBlocksSeparatorVertMargins()
    {
        return Utils::scale_value(16);
    }

    const QMargins& getDefaultBlockBubbleMargins()
    {
        static const QMargins margins(
            MessageStyle::getBubbleHorPadding(),
            MessageStyle::getBubbleVerPadding(),
            MessageStyle::getBubbleHorPadding(),
            MessageStyle::getBubbleVerPadding());

        return margins;
    }

    bool isBlocksGridEnabled()
    {
        if (build::is_release())
            return false;

        return false;
    }

    int32_t getHiddenControlsShift()
    {
        return Utils::scale_value(8);
    }

    std::chrono::milliseconds getHiddenControlsAnimationTime()
    {
        return std::chrono::milliseconds(150);
    }

    int getDragDistance()
    {
        return Utils::scale_value(50);
    }

    namespace Preview
    {
        int32_t getImageHeightMax()
        {
            return Utils::scale_value(360);
        }

        int32_t getImageWidthMax()
        {
            return Utils::scale_value(360);
        }

        int32_t getStickerMaxHeight()
        {
            return Utils::scale_value(160);
        }

        int32_t getStickerMaxWidth()
        {
            return Utils::scale_value(160);
        }

        QBrush getImagePlaceholderBrush()
        {
            QColor imagePlaceholderColor(ql1s("#000000"));
            imagePlaceholderColor.setAlphaF(0.15);
            return imagePlaceholderColor;
        }

        QSize getImagePlaceholderSize()
        {
            return Utils::scale_value(QSize(320, 240));
        }

        QSize getMinPreviewSize(const Option _option)
        {
            const auto w = _option == Option::image ? 112 : 145;
            return Utils::scale_value(QSize(w, 112));
        }

        QSize getSmallPreviewSize(const Option _option)
        {
            const auto w = _option == Option::image ? 120 : 145;
            const auto side = Utils::scale_value(w) - 2 * getSmallPreviewHorPadding();
            return QSize(side, side);
        }

        int32_t getSmallPreviewHorPadding()
        {
            return Utils::scale_value(8);
        }
    }

    namespace Snippet
    {
        QSize getFaviconPlaceholderSize()
        {
            return QSize(12, 12);
        }

        QSize getFaviconSizeUnscaled()
        {
            return QSize(12, 12);
        }

        QSize getImagePreloaderSize()
        {
            return QSize(Preview::getImageWidthMax(), getLinkPreviewHeightMax());
        }

        int32_t getLinkPreviewHeightMax()
        {
            return Utils::scale_value(180);
        }

        QColor getSiteNameColor()
        {
            return Styling::getParameters().getColor(Styling::StyleVariable::PRIMARY_PASTEL);
        }

        QFont getSiteNameFont()
        {
            return adjustedAppFont(12, Fonts::FontWeight::SemiBold);
        }

        QSize getSiteNamePlaceholderSize()
        {
            return Utils::scale_value(QSize(64, 12));
        }

        QSize getTitlePlaceholderSize()
        {
            return Utils::scale_value(QSize(272, 20));
        }

        int32_t getTitlePlaceholderTopOffset()
        {
            return Utils::scale_value(8);
        }

        int32_t getTitleTopOffset()
        {
            return Utils::scale_value(12);
        }

        QFont getYoutubeTitleFont()
        {
            return adjustedAppFont(15, Fonts::FontWeight::SemiBold);
        }

        int32_t getSiteNameLeftPadding()
        {
            return Utils::scale_value(4);
        }

        int32_t getSiteNameTopPadding()
        {
            return Utils::scale_value(18);
        }

        int32_t getFaviconTopPadding()
        {
            return Utils::scale_value(8);
        }

        QBrush getPreloaderBrush()
        {
            QLinearGradient grad(0, 0, 0.5, 0);
            grad.setCoordinateMode(QGradient::StretchToDeviceMode);
            grad.setSpread(QGradient::ReflectSpread);

            QColor colorEdge(ql1s("#000000"));
            colorEdge.setAlphaF(0.07);
            grad.setColorAt(0, colorEdge);

            QColor colorCenter(ql1s("#000000"));
            colorCenter.setAlphaF(0.12);
            grad.setColorAt(0.5, colorCenter);

            grad.setColorAt(1, colorEdge);

            QBrush result(grad);
            result.setColor(Qt::transparent);

            return result;
        }
    }

    namespace Quote
    {
        int32_t getLineWidth()
        {
            return Utils::scale_value(2);
        }

        int32_t getQuoteOffsetLeft() //offset to content for quotes
        {
            return getLineWidth() + Utils::scale_value(10);
        }

        int32_t getQuoteOffsetRight()
        {
            return Utils::scale_value(8);
        }

        int32_t getQuoteSpacing() // Spacing between quotes
        {
            return Utils::scale_value(8);
        }

        int32_t getSpacingAfterLastQuote() // Spacing after last quote
        {
            return Utils::scale_value(4);
        }

        int32_t getMaxImageWidthInQuote()
        {
            return Utils::scale_value(200);
        }

        int32_t getQuoteBlockInsideSpacing() //Vertical padding between content inside one quote
        {
            return Utils::scale_value(4);
        }

        int32_t getFwdLeftOverflow()
        {
            return Utils::scale_value(4);
        }

        int32_t getFwdOffsetLeft()
        {
            return Utils::scale_value(8);
        }

        int32_t getSenderTopOffset()
        {
            return Utils::scale_value(2);
        }

        QColor getHoverColor(const bool _isOutgoing)
        {
            return _isOutgoing ? QColor(0, 0, 0, 0.04 * 255) : QColor(120, 138, 187, 0.08 * 255);
        }
    }

    namespace Files
    {
        QSize getIconSize()
        {
            return Utils::scale_value(QSize(44, 44));
        }

        int32_t getHorMargin()
        {
            return Utils::scale_value(12);
        }

        int32_t getVerMargin()
        {
            return Utils::scale_value(8);
        }

        int32_t getVerMarginWithSender()
        {
            return Utils::scale_value(4);
        }

        int32_t getSecondRowVerOffset()
        {
            return Utils::scale_value(37);
        }

        int32_t getFileBubbleHeight()
        {
            return Utils::scale_value(44);
        }

        QFont getFilenameFont()
        {
            return Fonts::adjustedAppFont(16);
        }

        QColor getFileSizeColor(bool _isOutgoing)
        {
            return Styling::getParameters().getColor(_isOutgoing ? Styling::StyleVariable::PRIMARY_PASTEL : Styling::StyleVariable::BASE_PRIMARY);
        }

        QFont getFileSizeFont()
        {
            return Fonts::adjustedAppFont(13);
        }

        QFont getShowInDirLinkFont()
        {
            return Fonts::adjustedAppFont(13);
        }

        int32_t getCtrlIconLeftMargin()
        {
            return Utils::scale_value(12);
        }

        int32_t getFilenameLeftMargin()
        {
            return Utils::scale_value(12);
        }
    }

    namespace Ptt
    {
        int32_t getPttBlockHeight()
        {
            return Utils::scale_value(40);
        }

        int32_t getPttBlockMaxWidth()
        {
            return Utils::scale_value(340) - 2 * MessageStyle::getBubbleHorPadding();
        }

        int32_t getTopMargin()
        {
            return Utils::scale_value(4);
        }

        int32_t getDecodedTextVertPadding()
        {
            return Utils::scale_value(12);
        }

        int32_t getTextButtonMarginRight()
        {
            return Utils::scale_value(40) - MessageStyle::getBubbleHorPadding();
        }

        int32_t getPttProgressWidth()
        {
            return Utils::scale_value(2);
        }

        QFont getDurationFont()
        {
            return Fonts::adjustedAppFont(13);
        }
    }
}
