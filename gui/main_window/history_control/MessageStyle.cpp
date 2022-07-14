#include "stdafx.h"

#include "MessageStyle.h"

#include "../../fonts.h"

#include "../../utils/utils.h"

#include "../../utils/InterConnector.h"

#include "styles/ThemeParameters.h"
#include "reactions/MessageReactions.h"
#include "main_window/history_control/FileStatus.h"

namespace
{
    constexpr auto senderFontSize() { return 14; }

    constexpr auto textFontSize() { return platform::is_linux() ? 16 : 15; }
}

namespace Ui::MessageStyle
{
    QFont getTextFont()
    {
        constexpr int size = textFontSize();
        auto font = Fonts::adjustedAppFont(size);

        if constexpr (platform::is_apple())
            font.setLetterSpacing(QFont::AbsoluteSpacing, -0.25);

        return font;
    }

    int getTextLineSpacing() noexcept
    {
        if constexpr (platform::is_apple())
            return Utils::scale_value(3);
        else
            return 0;
    }

    QFont getTextMonospaceFont()
    {
        return Fonts::adjustedAppFont(textFontSize() - 1, Fonts::monospaceAppFontFamily());
    }

    QFont getImagePreviewLinkFont()
    {
        return getTextFont();
    }

    Styling::ThemeColorKey getTextColorKey()
    {
        return Styling::ThemeColorKey{ Styling::StyleVariable::TEXT_SOLID };
    }

    QColor getTextColor()
    {
        return Styling::getColor(getTextColorKey());
    }

    Styling::ThemeColorKey getLinkColorKey()
    {
        return Styling::ThemeColorKey{ Styling::StyleVariable::TEXT_PRIMARY };
    }

    QColor getLinkColor()
    {
        return Styling::getColor(getLinkColorKey());
    }

    QColor getSelectionFontColor()
    {
        return Styling::getParameters().getColor(Styling::StyleVariable::TEXT_SOLID);
    }

    Styling::ThemeColorKey getTextSelectionColorKey(const QString& _aimId)
    {
        return Styling::ThemeColorKey{ Styling::StyleVariable::PRIMARY_BRIGHT_HOVER, _aimId };
    }

    QColor getTextSelectionColor(const QString& _aimId)
    {
        return Styling::getColor(getTextSelectionColorKey(_aimId));
    }

    Styling::ThemeColorKey getSelectionColorKey()
    {
        return Styling::ThemeColorKey{ Styling::StyleVariable::PRIMARY, 0.3 };
    }

    QColor getSelectionColor()
    {
        return Styling::getColor(getSelectionColorKey());
    }

    Styling::ThemeColorKey getHighlightColorKey()
    {
        return Styling::ThemeColorKey{ Styling::StyleVariable::SECONDARY_RAINBOW_WARNING };
    }

    Styling::ThemeColorKey getHighlightTextColorKey()
    {
        return Styling::ThemeColorKey{ Styling::StyleVariable::GHOST_PRIMARY };
    }

    QFont getSenderFont()
    {
        static const auto fontWeight = platform::is_apple() ? Fonts::FontWeight::SemiBold : Fonts::FontWeight::Normal;
        return Fonts::adjustedAppFont(senderFontSize(), fontWeight);
    }

    QFont getQuoteSenderFont()
    {
        static const auto fontWeight = platform::is_apple() ? Fonts::FontWeight::Medium : Fonts::FontWeight::Normal;
        return Fonts::adjustedAppFont(senderFontSize(), fontWeight);
    }

    QFont getQuoteSenderFontMarkdown()
    {
        return Fonts::adjustedAppFont(senderFontSize(), Fonts::FontWeight::SemiBold);
    }

    QFont getButtonsFont()
    {
        constexpr auto fontWeight = platform::is_apple() ? Fonts::FontWeight::Medium : Fonts::FontWeight::Normal;
        return Fonts::adjustedAppFont(14, fontWeight);
    }

    QColor getBorderColor()
    {
        return Styling::getParameters().getColor(Styling::StyleVariable::GHOST_SECONDARY);

    }
    int getBorderWidth() noexcept
    {
        return Utils::scale_value(1);
    }

    QColor getBodyColor(const bool _isOutgoing, const QString& _aimId)
    {
        return Styling::getParameters(_aimId).getColor(_isOutgoing ? Styling::StyleVariable::CHAT_SECONDARY : Styling::StyleVariable::CHAT_PRIMARY);
    }

    QBrush getBodyBrush(const bool _isOutgoing, const QString& _aimId)
    {
        auto color = getBodyColor(_isOutgoing, _aimId);
        if (Utils::InterConnector::instance().isMultiselect(_aimId))
            color.setAlpha(255 * 0.95);

        return color;
    }

    int32_t getMinBubbleHeight() noexcept
    {
        return Utils::scale_value(36);
    }

    int32_t getBorderRadius() noexcept
    {
        return Utils::scale_value(12);
    }

    int32_t getBorderRadiusSmall() noexcept
    {
        return Utils::scale_value(4);
    }

    int32_t getTopMargin(const bool hasTopMargin) noexcept
    {
        return Utils::scale_value(hasTopMargin ? 8 : 2);
    }

    int32_t getRightBubbleMargin(bool _isMultiselected)
    {
        const auto added = _isMultiselected
            ? (Utils::InterConnector::instance().multiselectAnimationCurrent() * Utils::scale_value(8))
            : 0;
        return Utils::scale_value(32) + added;
    }

    int32_t getLeftBubbleMargin() noexcept
    {
        return Utils::scale_value(54);
    }

    int getHeadsWidth(int _numHeads) noexcept
    {
        return Utils::scale_value(getHeadsWidthUnscaled(_numHeads));
    }

    struct WidthBreakpoint
    {
        int numHeads_;
        int width_;
        int margin_;
        qreal factor_;
    };

    //! Return -1 if not within breakpoints interval
    template <typename T, typename Func>
    int32_t calcMarginFromBreakpoints(const T& _breakpoints, int _width, Func _minMarginFunc)
    {
        for (const auto& [_, minWidth, margin, factor] : _breakpoints)
        {
            if (const auto diff = _width - Utils::scale_value(minWidth); diff >= 0)
                return Utils::scale_value(margin) + qRound(factor * diff);
        }
        return _minMarginFunc();
    }

    int32_t getLeftMargin(const bool _isOutgoing, const int _width)
    {
        if (!_isOutgoing)
            return getLeftBubbleMargin();

        constexpr auto breakpoints = std::array{
            WidthBreakpoint{5, getHeadsWidthUnscaled(5), 136                                   , 0.},
            WidthBreakpoint{4, getHeadsWidthUnscaled(4), 136 -     getSingleHeadWidthUnscaled(), 1.},
            WidthBreakpoint{3, getHeadsWidthUnscaled(3), 136 - 2 * getSingleHeadWidthUnscaled(), 1.},
            WidthBreakpoint{0, 440,  56, 2.},
        };
        return calcMarginFromBreakpoints(breakpoints, _width, getLeftBubbleMargin);
    }

    int32_t getRightMargin(const bool _isOutgoing, const int _width, bool _isMultiselected)
    {
        if (_isOutgoing)
            return getRightBubbleMargin(_isMultiselected);

        constexpr auto breakpoints = std::array{
            WidthBreakpoint{5, getHeadsWidthUnscaled(5), 136                                   , 0.},
            WidthBreakpoint{4, getHeadsWidthUnscaled(4), 136 -     getSingleHeadWidthUnscaled(), 1.},
            WidthBreakpoint{3, getHeadsWidthUnscaled(3), 136 - 2 * getSingleHeadWidthUnscaled(), 1.},
            WidthBreakpoint{0, 396,  32, 1.},
        };
        return calcMarginFromBreakpoints(breakpoints, _width, [&_isMultiselected]() { return getRightBubbleMargin(_isMultiselected); });
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
        im_assert(width > 0);

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

    int getSharingButtonTopMargin()
    {
        return Utils::scale_value(-4);
    }

    int getSharingButtonOverlap()
    {
        return Utils::scale_value(12);
    }

    int getSharingButtonAnimationShift(const bool _isOutgoing)
    {
        return (_isOutgoing ? 1 : -1) * (MessageStyle::getSharingButtonMargin() + MessageStyle::getSharingButtonOverlap());
    }

    int getButtonHeight()
    {
        return Utils::scale_value(36);
    }

    int getTwoLineButtonHeight()
    {
        return Utils::scale_value(48);
    }

    int getButtonHorOffset()
    {
        return Utils::scale_value(12);
    }

    int getUrlInButtonOffset()
    {
        return Utils::scale_value(4);
    }

    QSize getButtonsAnimationSize()
    {
        return QSize(Utils::scale_value(20), Utils::scale_value(20));
    }

    int32_t getProgressTextRectHMargin()
    {
        return Utils::scale_value(4);
    }

    int32_t getProgressTextRectVMargin()
    {
        return Utils::scale_value(2);
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
        if constexpr (build::is_release())
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

    int getMessageMaxWidth() noexcept
    {
        return Utils::scale_value(600);
    }

    int getTextCodeMessageMaxWidth() noexcept 
    {
        return Utils::scale_value(1100);
    }

    QColor getBlockedFileIconBackground(bool _isOutgoing)
    {
        const auto var = _isOutgoing
            ? Styling::StyleVariable::PRIMARY_BRIGHT
            : Styling::StyleVariable::GHOST_ULTRALIGHT_INVERSE;

        return Styling::getParameters().getColor(var);
    }

    QColor getBlockedFileIconColor(bool _isOutgoing)
    {
        const auto var = _isOutgoing
            ? Styling::StyleVariable::PRIMARY_PASTEL
            : Styling::StyleVariable::BASE_SECONDARY;

        return Styling::getParameters().getColor(var);
    }

    QColor getInfectedFileIconBackground(bool _isOutgoing)
    {
        const auto var = _isOutgoing
            ? Styling::StyleVariable::PRIMARY_BRIGHT
            : Styling::StyleVariable::GHOST_ULTRALIGHT_INVERSE;

        return Styling::getParameters().getColor(var);
    }

    QColor getInfectedFileIconColor(bool _isOutgoing)
    {
        const auto var = _isOutgoing
            ? Styling::StyleVariable::PRIMARY_PASTEL
            : Styling::StyleVariable::BASE_SECONDARY;

        return Styling::getParameters().getColor(var);
    }

    QColor getFileStatusIconBackground(FileStatusType _status, bool _isOutgoing)
    {
        if (_status == FileStatusType::AntivirusOk)
            return Styling::getParameters().getColor(Styling::StyleVariable::SECONDARY_RAINBOW_MINT, _isOutgoing ? 0.15 : 0.12);
        else if (_status == FileStatusType::VirusInfected)
            return Styling::getParameters().getColor(Styling::StyleVariable::SECONDARY_ATTENTION, _isOutgoing ? 0.15 : 0.12);

        return Styling::getParameters().getColor(_isOutgoing ? Styling::StyleVariable::PRIMARY_BRIGHT : Styling::StyleVariable::BASE_BRIGHT);
    }

    Styling::StyleVariable getFileStatusIconColor(FileStatusType _type, bool _isOutgoing) noexcept
    {
        if (_type == FileStatusType::AntivirusOk)
            return Styling::StyleVariable::SECONDARY_RAINBOW_MINT;
        else if (_type == FileStatusType::VirusInfected)
            return Styling::StyleVariable::SECONDARY_ATTENTION;

        return _isOutgoing ? Styling::StyleVariable::PRIMARY_PASTEL : Styling::StyleVariable::BASE_PRIMARY;
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
            QColor imagePlaceholderColor(u"#000000");
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

        int32_t showDownloadProgressThreshold()
        {
            return Utils::scale_value(168);
        }

        int32_t mediaBlockWidthStretchThreshold()
        {
            return Utils::scale_value(286);
        }

        int32_t timeWidgetLeftMargin()
        {
            return Utils::scale_value(8);
        }

        int32_t getBorderRadius(bool _isStandalone)
        {
            if (_isStandalone)
                return MessageStyle::getBorderRadius();
            else
                return getInternalBorderRadius();
        }

        int32_t getInternalBorderRadius()
        {
            return Utils::scale_value(8);
        }

        int32_t mediaCropThreshold()
        {
            return Utils::scale_value(24);
        }

    }

    namespace Snippet
    {
        QSize getFaviconPlaceholderSize()
        {
            return QSize(16, 16);
        }

        QSize getFaviconSize()
        {
            return Utils::scale_value(QSize(16, 16));
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
            return Fonts::adjustedAppFont(12, Fonts::FontWeight::SemiBold);
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
            return Fonts::adjustedAppFont(15, Fonts::FontWeight::SemiBold);
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

            QColor colorEdge(u"#000000");
            colorEdge.setAlphaF(0.07);
            grad.setColorAt(0, colorEdge);

            QColor colorCenter(u"#000000");
            colorCenter.setAlphaF(0.12);
            grad.setColorAt(0.5, colorCenter);

            grad.setColorAt(1, colorEdge);

            QBrush result(grad);
            result.setColor(Qt::transparent);

            return result;
        }

        int32_t getMaxWidth()
        {
            return Utils::scale_value(360);
        }

        int32_t getMinWidth()
        {
            return Utils::scale_value(268);
        }

        QFont getTitleFont()
        {
            return Fonts::adjustedAppFont(15, Fonts::FontWeight::SemiBold);
        }

        QFont getDescriptionFont()
        {
            return Fonts::adjustedAppFont(15, Fonts::FontWeight::Normal);
        }

        QFont getLinkFont()
        {
            return Fonts::adjustedAppFont(13, Fonts::FontWeight::Medium);
        }

        Styling::ThemeColorKey getTitleColorKey()
        {
            return Styling::ThemeColorKey{ Styling::StyleVariable::TEXT_SOLID };
        }

        Styling::ThemeColorKey getDescriptionColorKey()
        {
            return Styling::ThemeColorKey{ Styling::StyleVariable::GHOST_PRIMARY_INVERSE_ACTIVE };
        }

        Styling::ThemeColorKey getLinkColorKey(bool _isOutgoing)
        {
            return Styling::ThemeColorKey{ _isOutgoing ? Styling::StyleVariable::PRIMARY_PASTEL : Styling::StyleVariable::BASE_PRIMARY };
        }

        int32_t getTitleTopPadding()
        {
            if constexpr (platform::is_apple())
                return Utils::scale_value(10);
            else
                return Utils::scale_value(8);
        }

        int32_t getDescriptionTopPadding()
        {
            return Utils::scale_value(4);
        }

        int32_t getLinkTopPadding()
        {
            return Utils::scale_value(10);
        }

        int32_t getLinkLeftPadding()
        {
            return Utils::scale_value(8);
        }

        int32_t getHorizontalModeLinkTopPadding()
        {
            return Utils::scale_value(8);
        }

        int32_t getHorizontalModeImageRightMargin()
        {
            return Utils::scale_value(12);
        }

        int32_t getPreviewTopPadding()
        {
            return Utils::scale_value(4);
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

        Styling::ThemeColorKey getFileSizeColorKey(bool _isOutgoing)
        {
            return Styling::ThemeColorKey{ _isOutgoing ? Styling::StyleVariable::PRIMARY_PASTEL : Styling::StyleVariable::BASE_PRIMARY };
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

        int32_t getLinkTopMargin()
        {
            if constexpr (platform::is_apple())
                return Utils::scale_value(12);
            else
                return Utils::scale_value(8);
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

    namespace Plates
    {
        int32_t plateHeight()
        {
            return Utils::scale_value(24);
        }

        int32_t plateYOffset()
        {
            return Utils::scale_value(-8);
        }

        int32_t mediaPlateYOffset()
        {
            return Utils::scale_value(-4);
        }

        int32_t shadowHeight()
        {
            return Utils::scale_value(2);
        }

        QSize addReactionButtonSize()
        {
            return Utils::scale_value(QSize(28, 28));
        }

        int32_t plateHeightWithShadow(Ui::ReactionsPlateType _type)
        {
            auto height = plateHeight();
            if (_type == Ui::ReactionsPlateType::Regular)
                height += Ui::MessageStyle::Plates::shadowHeight();

            return height;
        }

        int32_t plateOffsetY(Ui::ReactionsPlateType _type)
        {
            return _type == Ui::ReactionsPlateType::Regular
                ? Ui::MessageStyle::Plates::plateYOffset()
                : Ui::MessageStyle::Plates::mediaPlateYOffset();
        }

        int32_t plateOffsetX()
        {
            return Utils::scale_value(4);
        }

        int32_t borderRadius()
        {
            return Utils::scale_value(12);
        }
    }

}
