#pragma once

#include "../../styles/StyleVariable.h"
#include "../../styles/ThemeParameters.h"

namespace Ui
{
    enum class ReactionsPlateType;
    enum class FileStatusType;
} // namespace Ui


namespace Ui::MessageStyle
{
    QFont getTextFont();
    int getTextLineSpacing() noexcept;
    QFont getTextMonospaceFont();
    QFont getImagePreviewLinkFont();

    Styling::ThemeColorKey getTextColorKey();
    QColor getTextColor();
    Styling::ThemeColorKey getLinkColorKey();
    QColor getLinkColor();
    QColor getSelectionFontColor();
    Styling::ThemeColorKey getTextSelectionColorKey(const QString& _aimId = QString());
    QColor getTextSelectionColor(const QString& _aimId = QString());
    Styling::ThemeColorKey getSelectionColorKey();
    QColor getSelectionColor();

    Styling::ThemeColorKey getHighlightColorKey();
    Styling::ThemeColorKey getHighlightTextColorKey();

    QFont getSenderFont();

    QFont getQuoteSenderFont();

    QFont getQuoteSenderFontMarkdown();

    QFont getButtonsFont();

    QColor getBorderColor();

    int getBorderWidth() noexcept;

    QColor getBodyColor(const bool _isOutgoing, const QString& _aimId);
    QBrush getBodyBrush(const bool _isOutgoing, const QString& _aimId);

    int32_t getMinBubbleHeight() noexcept;

    int32_t getBorderRadius() noexcept;

    int32_t getBorderRadiusSmall() noexcept;

    int32_t getTopMargin(const bool hasTopMargin) noexcept;

    int32_t getRightBubbleMargin(bool _isMutiselected);

    int32_t getLeftBubbleMargin() noexcept;

    int32_t getLeftMargin(const bool _isOutgoing, const int _width);

    int32_t getRightMargin(const bool _isOutgoing, const int _width, bool _isMultiselected);

    int32_t getSenderTopPadding();
    int32_t getSenderBaseline();

    int32_t getAvatarSize();

    int32_t getAvatarRightMargin();

    int32_t getBubbleHorPadding();
    int32_t getBubbleVerPadding();

    int32_t getLastReadAvatarSize();

    int32_t getLastReadAvatarBottomMargin();

    int32_t getLastReadAvatarOffset();

    QSize getLastStatusIconSize();

    int32_t getLastReadAvatarMargin();

    int32_t getLastStatusLeftMargin();

    int32_t getHistoryWidgetMaxWidth();

    QFont getRotatingProgressBarTextFont();

    QPen getRotatingProgressBarTextPen();

    int32_t getRotatingProgressBarTextTopMargin();

    int32_t getRotatingProgressBarPenWidth();

    QPen getRotatingProgressBarPen();

    int32_t getSenderBottomMargin();
    int32_t getSenderBottomMarginLarge();

    int32_t getSenderHeight();
    int32_t getSenderStickerWidth();

    int32_t getTextWidthStep();

    int32_t roundTextWidthDown(const int32_t width);

    int32_t getSnippetMaxWidth();

    int32_t getHistoryBottomOffset();

    bool isShowLinksInImagePreview();

    int getFileSharingMaxWidth();

    int getFileSharingDesiredWidth();

    int getQuoteBlockDesiredWidth();

    int getPreviewDesiredWidth();

    int getVoipDesiredWidth();

    int getProfileBlockDesiredWidth();

    int getShiftedTimeTopMargin();
    int getTimeLeftSpacing();

    QSize getSharingButtonSize();

    int getSharingButtonMargin();

    int getSharingButtonTopMargin();

    int getSharingButtonOverlap();

    int getSharingButtonAnimationShift(const bool _isOutgoing);

    int getButtonHeight();

    int getTwoLineButtonHeight();

    int getButtonHorOffset();

    int getUrlInButtonOffset();

    QSize getButtonsAnimationSize();

    int32_t getProgressTextRectHMargin();

    int32_t getProgressTextRectVMargin();

    constexpr int getSingleHeadWidthUnscaled() noexcept { return 20; }

    constexpr int getHeadsWidthUnscaled(int _numHeads) noexcept
    {
        if (_numHeads < 3 || _numHeads > 5)
            im_assert(!"number of heads are supposed to be within [3;5] range");

        return 400 + _numHeads * getSingleHeadWidthUnscaled();
    }

    int getHeadsWidth(int _numHeads) noexcept;

    int32_t getBlocksSeparatorVertMargins();
    int getDragDistance();
    const QMargins& getDefaultBlockBubbleMargins();
    bool isBlocksGridEnabled();

    int32_t getHiddenControlsShift();
    std::chrono::milliseconds getHiddenControlsAnimationTime();

    int getMessageMaxWidth() noexcept;
    int getTextCodeMessageMaxWidth() noexcept;

    QColor getBlockedFileIconBackground(bool _isOutgoing);
    QColor getBlockedFileIconColor(bool _isOutgoing);

    QColor getInfectedFileIconBackground(bool _isOutgoing);
    QColor getInfectedFileIconColor(bool _isOutgoing);

    QColor getFileStatusIconBackground(FileStatusType _status, bool _isOutgoing);
    Styling::StyleVariable getFileStatusIconColor(FileStatusType _status, bool _isOutgoing) noexcept;

    namespace Preview
    {
        int32_t getImageHeightMax();
        int32_t getImageWidthMax();
        int32_t getStickerMaxHeight();
        int32_t getStickerMaxWidth();
        QSize getImagePlaceholderSize();
        QBrush getImagePlaceholderBrush();

        enum class Option
        {
            video,
            image
        };
        QSize getMinPreviewSize(const Option _option);
        QSize getSmallPreviewSize(const Option _option);
        int32_t getSmallPreviewHorPadding();

        enum class BottomLeftControls
        {
            NoButtons,
            OneButton,
            TwoButtons
        };

        int32_t timeWidgetLeftMargin();
        int32_t showDownloadProgressThreshold();

        // if media block's width is less than the threshold its max width will be equal to the threshold value
        int32_t mediaBlockWidthStretchThreshold();

        int32_t mediaCropThreshold();

        int32_t getInternalBorderRadius();
        int32_t getBorderRadius(bool _isStandalone);
    }

    namespace Snippet
    {
        QSize getFaviconPlaceholderSize();
        QSize getFaviconSize();
        int32_t getLinkPreviewHeightMax();
        QSize getImagePreloaderSize();
        QColor getSiteNameColor();
        QFont getSiteNameFont();
        QSize getSiteNamePlaceholderSize();
        QSize getTitlePlaceholderSize();
        int32_t getTitlePlaceholderTopOffset();
        int32_t getTitleTopOffset();
        int32_t getFaviconTopPadding();
        int32_t getSiteNameLeftPadding();
        int32_t getSiteNameTopPadding();
        QFont getYoutubeTitleFont();
        QBrush getPreloaderBrush();

        int32_t getMaxWidth();
        int32_t getMinWidth();
        QFont getTitleFont();
        QFont getDescriptionFont();
        QFont getLinkFont();
        Styling::ThemeColorKey getTitleColorKey();
        Styling::ThemeColorKey getDescriptionColorKey();
        Styling::ThemeColorKey getLinkColorKey(bool _isOutgoing);

        int32_t getTitleTopPadding();
        int32_t getDescriptionTopPadding();
        int32_t getLinkTopPadding();
        int32_t getLinkLeftPadding();
        int32_t getHorizontalModeLinkTopPadding();
        int32_t getHorizontalModeImageRightMargin();
        int32_t getPreviewTopPadding();
    }

    namespace Quote
    {
        int32_t getLineWidth();
        int32_t getQuoteOffsetLeft();
        int32_t getQuoteOffsetRight();
        int32_t getQuoteSpacing();
        int32_t getSpacingAfterLastQuote();
        int32_t getMaxImageWidthInQuote();
        int32_t getQuoteBlockInsideSpacing();
        int32_t getFwdLeftOverflow();
        int32_t getFwdOffsetLeft();
        int32_t getSenderTopOffset();
        QColor getHoverColor(const bool _isOutgoing);
    }

    namespace Files
    {
        QSize getIconSize();
        int32_t getHorMargin();
        int32_t getVerMargin();
        int32_t getVerMarginWithSender();
        int32_t getSecondRowVerOffset();

        int32_t getFileBubbleHeight();
        QFont getFilenameFont();
        Styling::ThemeColorKey getFileSizeColorKey(bool _isOutgoing = false);
        QFont getFileSizeFont();
        QFont getShowInDirLinkFont();
        int32_t getCtrlIconLeftMargin();
        int32_t getFilenameLeftMargin();
        int32_t getLinkTopMargin();
    }

    namespace Ptt
    {
        int32_t getPttBlockHeight();
        int32_t getPttBlockMaxWidth();

        int32_t getTopMargin();
        int32_t getDecodedTextVertPadding();
        int32_t getTextButtonMarginRight();
        int32_t getPttProgressWidth();
        QFont getDurationFont();
    }

    namespace Plates
    {
        int32_t plateHeight();
        int32_t plateYOffset();
        int32_t mediaPlateYOffset();
        int32_t shadowHeight();
        QSize addReactionButtonSize();
        int32_t plateHeightWithShadow(Ui::ReactionsPlateType _type);
        int32_t plateOffsetY(Ui::ReactionsPlateType _type);
        int32_t plateOffsetX();
        int32_t borderRadius();
    }
}
