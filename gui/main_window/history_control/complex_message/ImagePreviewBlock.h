#pragma once

#include "../../../types/link_metadata.h"

#include "GenericBlock.h"
#include "../../mplayer/VideoPlayer.h"
#include "../../../utils/LoadMovieFromFileTask.h"

UI_NS_BEGIN
class ActionButtonWidget;
namespace TextRendering
{
    class TextUnit;
}
UI_NS_END

UI_COMPLEX_MESSAGE_NS_BEGIN

class ImagePreviewBlockLayout;

class ImagePreviewBlock final : public GenericBlock
{
    friend class ImagePreviewBlockLayout;

    Q_OBJECT

public:
    ImagePreviewBlock(ComplexMessageItem *parent, const QString &imageUri, const QString &imageType);

    virtual ~ImagePreviewBlock() override;

    void clearSelection() override;

    QPoint getActionButtonLogicalCenter() const;

    QSize getActionButtonSize() const;

    virtual IItemBlockLayout* getBlockLayout() const override;

    QSize getPreviewSize() const;
    QSize getSmallPreviewSize() const;
    QSize getMinPreviewSize() const;

    QString getSelectedText(const bool _isFullSelect = false, const TextDestination _dest = TextDestination::selection) const override;

    bool hasActionButton() const;

    void hideActionButton();

    bool hasPreview() const;

    bool isBubbleRequired() const override;

    bool isSelected() const override;

    bool isAllSelected() const override { return isSelected(); }

    void onVisibilityChanged(const bool isVisible) override;

    void onDistanceToViewportChanged(const QRect& _widgetAbsGeometry, const QRect& _viewportVisibilityAbsRect) override;

    void setMaxPreviewWidth(int width) override;

    int getMaxPreviewWidth() const override;

    void showActionButton(const QRect &pos);

    virtual ContentType getContentType() const override { return ContentType::Link; }

    Ui::MediaType getMediaType() const override;

    PinPlaceholderType getPinPlaceholder() const override;

    void setQuoteSelection() override;

    int desiredWidth(int _width = 0) const override;

    void requestPinPreview() override;

    QString formatRecentsText() const override;

    bool updateFriendly(const QString& _aimId, const QString& _friendly) override;

    bool isLinkMediaPreview() const override { return true; }

    bool clicked(const QPoint& _p) override;

    void resetClipping() override;

protected:
    void drawBlock(QPainter &p, const QRect& _rect, const QColor& _quoteColor) override;

    void drawEmptyBubble(QPainter &p, const QRect &bubbleRect);

    void initialize() override;

    void mouseMoveEvent(QMouseEvent *event) override;

    void onMenuCopyFile() override;

    void onMenuCopyLink() override;

    void onMenuSaveFileAs() override;

    void onMenuOpenInBrowser() override;

    bool drag() override;

    void resizeEvent(QResizeEvent * event) override;

    void selectByPos(const QPoint& from, const QPoint& to, const BlockSelectionType selection) override;

public:
    const QPixmap& getPreviewImage() const;

private:

    void connectSignals();

    void downloadFullImage(const QString &destination);

    QPainterPath evaluateClippingPath(const QRect &imageRect) const;

    void initializeActionButton();

    bool isFullImageDownloaded() const;

    bool isFullImageDownloading() const;

    bool isGifPreview() const;

    bool isOverImage(const QPoint &pos) const;

    bool isVideoPreview() const;

    void onFullImageDownloaded(QPixmap image, const QString &localPath);

    void onGifLeftMouseClick();

    void onGifVisibilityChanged(const bool isVisible);

    void onLeftMouseClick(const QPoint &_pos);

    void onPreviewImageDownloaded(QPixmap image, const QString &localPath);

    void openGif(const QString &localPath, const bool _byUser, const bool _play = true);

    void preloadFullImageIfNeeded();

    bool shouldDisplayProgressAnimation() const;

    bool isSingle() const;

    void stopDownloadingAnimation();

    void onChangeLoadState(const bool isVisible);

    bool isInPreloadRange(const QRect& _widgetAbsGeometry, const QRect& _viewportVisibilityAbsRect);

    void stopDownloading();

    bool isAutoPlay() const;

    bool isSmallPreview() const override;

    void cancelRequests() override;

    void updateStyle() override;
    void updateFonts() override;

    DialogPlayer* createPlayer();

    ImagePreviewBlockLayout *Layout_;

    QString ImageUri_;

    QString ImageType_;

    QString FullImageLocalPath_;

    int64_t PreviewDownloadSeq_;

    QPixmap Preview_;

    std::unique_ptr<TextRendering::TextUnit> link_;

    QPainterPath PreviewClippingPath_;

    QPainterPath RelativePreviewClippingPath_;

    QRect PreviewClippingPathRect_;

    QString PreviewLocalPath_;

    int64_t FullImageDownloadSeq_;

    std::unique_ptr<Ui::DialogPlayer> videoPlayer_;

    bool isSmallPreview_;

    bool IsSelected_;

    BlockSelectionType Selection_;

    ActionButtonWidget *ActionButton_;

    bool CopyFile_;

    QString DownloadUri_;

    QString metaContentType_;

    int MaxPreviewWidth_;

    bool IsVisible_;

    bool IsInPreloadDistance_;

    std::shared_ptr<bool> ref_;

    QSize originSize_;

private Q_SLOTS:
    void onGifFrameUpdated(int frameNumber);

    void onImageDownloadError(qint64 seq, QString rawUri);

    void onImageDownloaded(int64_t seq, QString, QPixmap image, QString localPath);

    void onImageDownloadingProgress(qint64 seq, int64_t bytesTotal, int64_t bytesTransferred, int32_t pctTransferred);

    void onImageMetaDownloaded(int64_t seq, Data::LinkMetadata meta);

    void onActionButtonClicked(const QPoint& _globalPos);
};

UI_COMPLEX_MESSAGE_NS_END
