#pragma once

namespace Ui::MessageStyle
{
    QFont getTextFont();
    int getTextLineSpacing();
    QFont getMarkdownFont();
    QFont getImagePreviewLinkFont();

    QColor getTextColor();
    QColor getLinkColor(bool _isOutgoing = false);
    QColor getSelectionFontColor();
    QColor getTextSelectionColor(const QString& _aimId = QString());
    QColor getSelectionColor();
    QColor getHighlightColor();

    QFont getSenderFont();

    QFont getQuoteSenderFont();

    QFont getQuoteSenderFontMarkdown();

    QColor getBorderColor();

    int getBorderWidth();

    QBrush getBodyBrush(const bool isOutgoing, const bool isSelected, const QString& _aimId);

    int32_t getMinBubbleHeight();

    int32_t getBorderRadius();

    int32_t getBorderRadiusSmall();

    int32_t getTopMargin(const bool hasTopMargin);

    int32_t getRightBubbleMargin();

    int32_t getLeftBubbleMargin();

    int32_t getLeftMargin(const bool _isOutgoing, const int _width);

    int32_t getRightMargin(const bool _isOutgoing, const int _width);

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

    int getSharingButtonOverlap();

    int getSharingButtonAnimationShift(const bool _isOutgoing);

    int32_t getProgressTextRectHMargin();

    int32_t getProgressTextRectVMargin();

    int fiveHeadsWidth();
    int fourHeadsWidth();
    int threeHeadsWidth();

    int32_t getBlocksSeparatorVertMargins();
    int getDragDistance();
    const QMargins& getDefaultBlockBubbleMargins();
    bool isBlocksGridEnabled();

    int32_t getHiddenControlsShift();
    std::chrono::milliseconds getHiddenControlsAnimationTime();

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
    }

    namespace Snippet
    {
        QSize getFaviconPlaceholderSize();
        QSize getFaviconSizeUnscaled();
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
        QColor getFileSizeColor(bool _isOutgoing = false);
        QFont getFileSizeFont();
        QFont getShowInDirLinkFont();
        int32_t getCtrlIconLeftMargin();
        int32_t getFilenameLeftMargin();
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
}
