#pragma once

#include "GenericBlock.h"
#include "../../../types/link_metadata.h"

namespace Ui
{
    namespace TextRendering
    {
        class TextUnit;
    }

    class MediaControls;
    class DialogPlayer;
}

namespace Utils
{
    class MediaLoader;
}

UI_COMPLEX_MESSAGE_NS_BEGIN

class SnippetContent;
class SnippetBlockLayout;

//////////////////////////////////////////////////////////////////////////
// SnippetBlock
//////////////////////////////////////////////////////////////////////////

class SnippetBlock : public GenericBlock
{
    Q_OBJECT
public:
    enum class EstimatedType
    {
        Image,
        Video,
        GIF,
        Article,
        Geo
    };

    SnippetBlock(ComplexMessageItem* _parent, const QString& _link, const bool _hasLinkInMessage, EstimatedType _estimatedType);

    IItemBlockLayout* getBlockLayout() const override; // returns nullptr, since there is no layout

    Data::FormattedString getSelectedText(const bool _isFullSelect = false, const TextDestination _dest = TextDestination::selection) const override;
    QString getTextForCopy() const override;
    bool updateFriendly(const QString& _aimId, const QString& _friendly) override { return true; }

    void selectByPos(const QPoint& from, const QPoint& to, bool topToBottom) override;
    bool isAllSelected() const override { return isSelected(); }

    QRect setBlockGeometry(const QRect& _rect) override;
    QRect getBlockGeometry() const override;

    int desiredWidth(int _width = 0) const override;
    int getMaxWidth() const override;

    bool clicked(const QPoint& _p) override;
    bool pressed(const QPoint& _p) override;

    bool isBubbleRequired() const override;
    bool isMarginRequired() const override;
    bool isLinkMediaPreview() const override;

    MediaType getMediaType() const override;
    ContentType getContentType() const override;

    void onVisibleRectChanged(const QRect& _visibleRect) override;
    bool hasLeadLines() const override;
    void highlight(const highlightsV& _hl) override;
    void removeHighlight() override;

    PinPlaceholderType getPinPlaceholder() const override;
    void requestPinPreview() override;

    QString formatRecentsText() const override;

    bool isBlockVisible() const;

    bool managesTime() const override;

    void shiftHorizontally(const int _shift) override;
    bool needStretchToOthers() const override;
    bool needStretchByShift() const override;
    void resizeBlock(const QSize& _size) override;

    bool canStretchWithSender() const override;

    const QString& getLink() const override;

    const QPixmap& getPreviewImage() const;

protected:
    void drawBlock(QPainter& _p, const QRect& _rect, const QColor& _quoteColor) override;
    void initialize() override;

    void mouseMoveEvent(QMouseEvent* _event) override;
    void leaveEvent(QEvent* _event) override;

    void onMenuCopyLink() override;
    void onMenuCopyFile() override;
    void onMenuSaveFileAs() override;
    void onMenuOpenInBrowser() override;

    bool drag() override;

    MenuFlags getMenuFlags(QPoint p) const override;

private Q_SLOTS:
    void onLinkMetainfoMetaDownloaded(int64_t _seq, bool _success, Data::LinkMetadata _meta);
    void onContentLoaded();

private:
    void updateStyle() override;
    void updateFonts() override;

    void initWithMeta(const Data::LinkMetadata& _meta);

    std::unique_ptr<SnippetContent> createContent(const Data::LinkMetadata& _meta);
    std::unique_ptr<SnippetContent> createPreloader();

    bool shouldCopySource() const;

    // current content
    std::unique_ptr<SnippetContent> content_;

    // content that is waiting to be completely loaded
    std::unique_ptr<SnippetContent> loadingContent_;

    QString link_;
    bool hasLinkInMessage_;
    bool visible_;
    EstimatedType estimatedType_;
    QRect contentRect_;
    int64_t seq_;
};


//////////////////////////////////////////////////////////////////////////
// SnippetContent
//////////////////////////////////////////////////////////////////////////

class SnippetContent : public QObject
{
    Q_OBJECT
public:
    SnippetContent(SnippetBlock* _snippetBlock, const Data::LinkMetadata& _meta, const QString& _link);

    virtual QSize setContentSize(const QSize& _available) = 0;
    virtual void draw(QPainter& _p, const QRect& _rect) = 0;

    virtual void onBlockSizeChanged() {}

    virtual bool clicked() { return false; }
    virtual bool pressed() { return false; }

    virtual bool isBubbleRequired() const { return false; }
    virtual bool isMarginRequired() const { return false; }

    virtual MediaType mediaType() const;

    virtual int desiredWidth(int _availableWidth) const;
    virtual int maxWidth() const;

    virtual void onVisibilityChanged(const bool _visible) {}

    virtual void setPreview(const QPixmap& _preview);
    virtual void setFavicon(const QPixmap& _favicon);

    virtual void onMenuCopyFile() {}
    virtual void onMenuSaveFileAs() {}

    virtual void highlight(const highlightsV& _hl) {}
    virtual void removeHighlight() {}

    virtual void updateStyle() {}

    virtual IItemBlock::MenuFlags menuFlags() const { return IItemBlock::MenuFlagNone; };

    virtual bool needsPreload() const { return false; }

    virtual bool managesTime() const { return true; }

    virtual void showControls() {}
    virtual void hideControls() {}

    virtual bool needStretchToOthers() const;
    virtual bool needStretchByShift() const;

    virtual bool canStretchWithSender() const { return true; }

    const QPixmap& preview() const { return preview_; }

    void loadPreview();
    void loadFavicon();

    virtual bool isOverLink(const QPoint&) const { return false; }
    virtual bool needsTooltip() const { return false; }
    virtual QRect tooltipRect() const { return QRect(); }

Q_SIGNALS:
    void loaded();

protected Q_SLOTS:
    void onImageLoaded(qint64 _seq, const QString& _rawUri, const QPixmap& _image);

protected:
    QRect contentRect() const;

    SnippetBlock* snippetBlock_;
    Data::LinkMetadata meta_;
    QPixmap preview_;
    QPixmap favicon_;
    QString link_;

    int64_t previewSeq_;
    int64_t faviconSeq_;
};

//////////////////////////////////////////////////////////////////////////
// ArticleContent
//////////////////////////////////////////////////////////////////////////

class ArticleContent : public SnippetContent
{
    Q_OBJECT
public:
    ArticleContent(SnippetBlock* _snippetBlock, const Data::LinkMetadata& _meta, const QString& _link);
    ~ArticleContent();

    QSize setContentSize(const QSize& _available) override;
    void draw(QPainter& _p, const QRect& _rect) override;

    void onBlockSizeChanged() override;

    bool clicked() override;
    bool pressed() override;

    bool isBubbleRequired() const override;
    bool isMarginRequired() const override;

    void setPreview(const QPixmap& _preview) override;

    void highlight(const highlightsV& _hl) override;
    void removeHighlight() override;

    void updateStyle() override;

    IItemBlock::MenuFlags menuFlags() const override;

    void showControls() override;
    void hideControls() override;

    bool isOverLink(const QPoint& _p) const override;
    bool needsTooltip() const override;
    QRect tooltipRect() const override;

private:
    bool isGifArticle() const;
    bool isVideoArticle() const;

    void createControls();
    void initTextUnits();

    bool isHorizontalMode() const;

    std::unique_ptr<TextRendering::TextUnit> titleUnit_;
    std::unique_ptr<TextRendering::TextUnit> descriptionUnit_;
    std::unique_ptr<TextRendering::TextUnit> linkUnit_;

    QPointer<MediaControls> controls_;

    QRect previewRect_;
    QRect faviconRect_;
};

//////////////////////////////////////////////////////////////////////////
// MediaContent
//////////////////////////////////////////////////////////////////////////

class MediaContent : public SnippetContent
{
    Q_OBJECT
public:
    MediaContent(SnippetBlock* _snippetBlock, const Data::LinkMetadata& _meta, const QString& _link);
    ~MediaContent();

    QSize setContentSize(const QSize& _available) override;
    void draw(QPainter& _p, const QRect& _rect) override;

    void onBlockSizeChanged() override;

    bool clicked() override;
    bool pressed() override;

    bool isBubbleRequired() const override;
    bool isMarginRequired() const override;

    MediaType mediaType() const override;

    int desiredWidth(int _availableWidth) const override;
    int maxWidth() const override;

    void onVisibilityChanged(const bool _visible) override;

    void setPreview(const QPixmap& _preview) override;
    void setFavicon(const QPixmap& _favicon) override;

    void onMenuCopyFile() override;
    void onMenuSaveFileAs() override;

    IItemBlock::MenuFlags menuFlags() const override;

    bool needsPreload() const override { return true; }

    bool managesTime() const override;

    void showControls() override;
    void hideControls() override;

    bool needStretchToOthers() const override;
    bool needStretchByShift() const override;

    bool canStretchWithSender() const override;

private Q_SLOTS:
    void onProgress(int64_t _bytesTotal, int64_t _bytesTransferred);
    void onLoaded(const QString& _path);
    void onError();
    void onFilePath(int64_t _seq, const QString& _path);
    void prepareBackground(const QPixmap& _result, qint64 _srcCacheKey);

private:
    QSize calcContentSize(int _availableWidth) const;
    int minControlsWidth() const;
    QSize originSizeScaled() const;
    void updateButtonGeometry();
#ifndef STRIP_AV_MEDIA
    DialogPlayer* createPlayer();
#endif // !STRIP_AV_MEDIA
    void createControls();
    void initPlayableMedia();
    void loadMedia();
    void stopLoading();

    bool isPlayable() const;
    bool isAutoPlay() const;
    bool isVideo() const;
    bool isGif() const;

    bool isProgressVisible() const;

    std::unique_ptr<Utils::MediaLoader> loader_;
    QPointer<ActionButtonWidget> button_;
    QPointer<MediaControls> controls_;
#ifndef STRIP_AV_MEDIA
    QPointer<DialogPlayer> player_;
#endif // !STRIP_AV_MEDIA
    QPainterPath clipPath_;
    bool fileLoaded_;
    int64_t seq_ = 0;
    QPixmap background_;
    int64_t backgroundSeq_ = 0;
};

//////////////////////////////////////////////////////////////////////////
// FileContent
//////////////////////////////////////////////////////////////////////////

class FileContent : public SnippetContent
{
    Q_OBJECT
public:
    FileContent(SnippetBlock* _snippetBlock, const Data::LinkMetadata& _meta, const QString& _link);
    ~FileContent();

    QSize setContentSize(const QSize& _available) override;
    void draw(QPainter& _p, const QRect& _rect) override;

    bool clicked() override;
    bool pressed() override;

    bool isBubbleRequired() const override;
    bool isMarginRequired() const override;

    void highlight(const highlightsV& _hl) override;
    void removeHighlight() override;

    void updateStyle() override;

    IItemBlock::MenuFlags menuFlags() const override;

    bool isOverLink(const QPoint& _p) const override;
    bool needsTooltip() const override;
    QRect tooltipRect() const override;

private:
    void initTextUnits();

    std::unique_ptr<TextRendering::TextUnit> fileNameUnit_;
    std::unique_ptr<TextRendering::TextUnit> linkUnit_;

    QRect faviconRect_;
    QRect iconRect_;
};

//////////////////////////////////////////////////////////////////////////
// LocationContent
//////////////////////////////////////////////////////////////////////////

class LocationContent : public SnippetContent
{
    Q_OBJECT
public:
    LocationContent(SnippetBlock* _snippetBlock, const Data::LinkMetadata& _meta, const QString& _link);
    ~LocationContent();

    QSize setContentSize(const QSize& _available) override;
    void draw(QPainter& _p, const QRect& _rect) override;

    bool clicked() override;
    bool pressed() override;

    bool isBubbleRequired() const override;
    bool isMarginRequired() const override;

    MediaType mediaType() const override;

    int desiredWidth(int _availableWidth) const override;

    void updateStyle() override;

private:
    QSize calcPreviewSize(int _availableWidth) const;
    QSize originSizeScaled() const;
    bool isTimeInTitle() const;

    void initTextUnits();

    std::unique_ptr<TextRendering::TextUnit> titleUnit_;
    QRect previewRect_;
};

//////////////////////////////////////////////////////////////////////////
// ArticlePreloader
//////////////////////////////////////////////////////////////////////////

class ArticlePreloader : public SnippetContent
{
    Q_OBJECT
public:
    ArticlePreloader(SnippetBlock* _snippetBlock, const QString& _link);
    ~ArticlePreloader() = default;

    QSize setContentSize(const QSize& _available) override;
    void draw(QPainter& _p, const QRect& _rect) override;

    bool clicked() override;
    bool pressed() override;

    bool isBubbleRequired() const override;
    bool isMarginRequired() const override;

    void onVisibilityChanged(const bool _visible) override;

    IItemBlock::MenuFlags menuFlags() const override;

    bool isOverLink(const QPoint& _p) const override;
    bool needsTooltip() const override;
    QRect tooltipRect() const override;

private:
    void startAnimation();

    QVariantAnimation* animation_;
    QRect previewRect_;
    QRect titleRect_;
    QRect linkRect_;
    QRect faviconRect_;
};

//////////////////////////////////////////////////////////////////////////
// MediaPreloader
//////////////////////////////////////////////////////////////////////////

class MediaPreloader : public SnippetContent
{
    Q_OBJECT
public:
    MediaPreloader(SnippetBlock* _snippetBlock, const QString& _link);
    ~MediaPreloader() = default;

    QSize setContentSize(const QSize& _available) override;
    void draw(QPainter& _p, const QRect& _rect) override;

    void onBlockSizeChanged() override;

    bool clicked() override;
    bool pressed() override;

    int desiredWidth(int _availableWidth) const override;

private:
    QPainterPath clipPath_;
};


UI_COMPLEX_MESSAGE_NS_END
