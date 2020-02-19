#pragma once

#include "GenericBlock.h"

#include "../../../types/link_metadata.h"

UI_NS_BEGIN

class ActionButtonWidget;

namespace TextRendering
{
    class TextUnit;
}

UI_NS_END

UI_COMPLEX_MESSAGE_NS_BEGIN

class EmbeddedPreviewWidgetBase;
class ILinkPreviewBlockLayout;
class YoutubeLinkPreviewBlockLayout;

class LinkPreviewBlock : public GenericBlock
{
    friend class LinkPreviewBlockLayout;

    Q_OBJECT

public:
    LinkPreviewBlock(ComplexMessageItem *parent, const QString &uri, const bool _hasLinkInMessage);

    virtual ~LinkPreviewBlock() override;

    IItemBlockLayout* getBlockLayout() const override;

    QSize getFaviconSizeUnscaled() const;

    QSize getPreviewImageSize() const;

    QString getSelectedText(const bool _isFullSelect = false, const TextDestination _dest = TextDestination::selection) const override;

    QString getTextForCopy() const override;

    bool updateFriendly(const QString& _aimId, const QString& _friendly) override;

    const QString& getSiteName() const;

    bool hasTitle() const;

    bool isInPreloadingState() const;

    bool isAllSelected() const override { return isSelected(); }

    void onVisibilityChanged(const bool isVisible) override;

    void onDistanceToViewportChanged(const QRect& _widgetAbsGeometry, const QRect& _viewportVisibilityAbsRect) override;

    void selectByPos(const QPoint& from, const QPoint& to, bool) override;

    void setMaxPreviewWidth(int width) override;

    int getMaxPreviewWidth() const override;

    ContentType getContentType() const override { return ContentType::Link; }

    PinPlaceholderType getPinPlaceholder() const override { return PinPlaceholderType::Link; }

    void setQuoteSelection() override;

    bool isHasLinkInMessage() const override;

    int getMaxWidth() const override;

    QPoint getShareButtonPos(const bool _isBubbleRequired, const QRect& _bubbleRect) const override;

    int desiredWidth(int _width = 0) const override;

    int getTitleHeight(int _titleWidth);

    void setTitleOffsets(int _x, int _y);

    void requestPinPreview() override;

    QString formatRecentsText() const override;

    bool pressed(const QPoint& _p) override;
    bool clicked(const QPoint& _p) override;

    virtual void cancelRequests() override;

    void highlight(const highlightsV& _hl) override;
    void removeHighlight() override;

    const QPixmap& getPreviewImage() const;

protected:
    void drawBlock(QPainter &p, const QRect& _rect, const QColor& _quoteColor) override;

    void initialize() override;

    void mouseMoveEvent(QMouseEvent *event) override;

    void showEvent(QShowEvent *event) override;

    void onMenuCopyLink() override;

    bool drag() override;

    virtual void drawFavicon(QPainter &p);

    virtual void drawSiteName(QPainter &p);

    virtual std::unique_ptr<YoutubeLinkPreviewBlockLayout> createLayout();

    virtual QString getTitle() const;

private Q_SLOTS:
    void onLinkMetainfoMetaDownloaded(int64_t seq, bool success, Data::LinkMetadata meta);

    void onLinkMetainfoFaviconDownloaded(int64_t seq, bool success, QPixmap image);

    void onLinkMetainfoImageDownloaded(int64_t seq, bool success, QPixmap image);

    void setPreviewImage(const QPixmap& _image);

private:
    Q_PROPERTY(int PreloadingTicker READ getPreloadingTickerValue WRITE setPreloadingTickerValue);

    void createTextControls(const QRect &blockGeometry);

    int getPreloadingTickerValue() const;

    void setPreloadingTickerValue(const int32_t _val);

    void updateStyle() override;
    void updateFonts() override;

    std::unique_ptr<TextRendering::TextUnit> Title_;
    std::unique_ptr<TextRendering::TextUnit> domain_;

    QPixmap FavIcon_;

    //QString SiteName_;

    QString Uri_;

    int64_t RequestId_;

    std::unique_ptr<ILinkPreviewBlockLayout> Layout_;

    QPixmap PreviewImage_;

    QSize PreviewSize_;

    QRect previewClippingPathRect_;

    QPainterPath previewClippingPath_;

    QPropertyAnimation *PreloadingTickerAnimation_;

    int32_t PreloadingTickerValue_;

    QPainterPath Bubble_;

    int32_t Time_;

    bool MetaDownloaded_;

    bool ImageDownloaded_;

    bool FaviconDownloaded_;

    QString ContentType_;

    Data::LinkMetadata Meta_;

    int MaxPreviewWidth_;

    const bool hasLinkInMessage_;

    std::vector<QMetaObject::Connection> connections_;

    void connectSignals(const bool _isConnected);

    enum class TextOptions
    {
        PlainText,
        ClickableLinks
    };

    void drawPreloader(QPainter &p);

    void drawPreview(QPainter &p);

    QPainterPath evaluateClippingPath(const QRect &imageRect) const;

    bool isOverSiteLink(const QPoint& p) const;

    bool isTitleClickable() const;

    void scalePreview(QPixmap &image);

    QSize scalePreviewSize(const QSize &size) const;

    void updateRequestId();
};

UI_COMPLEX_MESSAGE_NS_END
