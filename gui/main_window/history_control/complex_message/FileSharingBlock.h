#pragma once

#include "../../../namespaces.h"
#include "../../../types/link_metadata.h"
#include "../../../types/StickerId.h"
#include "../../../controls/TextUnit.h"

#include "FileSharingBlockBase.h"

#include "../../../utils/LoadMovieFromFileTask.h"

#include "../../../gui/utils/utils.h"

Q_DECLARE_LOGGING_CATEGORY(fileSharingBlock)

UI_NS_BEGIN

class FileSharingIcon;
class ActionButtonWidget;
namespace ActionButtonResource
{
    struct ResourceSet;
}

UI_NS_END

UI_COMPLEX_MESSAGE_NS_BEGIN

class IFileSharingBlockLayout;

namespace MediaUtils
{
    enum class MediaType;
}

class FileSharingBlock final : public FileSharingBlockBase
{
    Q_OBJECT

public:
    FileSharingBlock(
        ComplexMessageItem *parent,
        const QString &link,
        const core::file_sharing_content_type type);

    FileSharingBlock(
        ComplexMessageItem *parent,
        const HistoryControl::FileSharingInfoSptr& fsInfo,
        const core::file_sharing_content_type type);

    virtual ~FileSharingBlock() override;

    QSize getCtrlButtonSize() const;

    QSize originSizeScaled() const;

    static QPixmap scaledSticker(const QPixmap&);
    static QImage scaledSticker(const QImage&);

    bool isPreviewReady() const;

    bool isFilenameElided() const;

    bool isSharingEnabled() const override;

    void onVisibleRectChanged(const QRect& _visibleRect) override;

    void onSelectionStateChanged(const bool isSelected) override;

    void onDistanceToViewportChanged(const QRect& _widgetAbsGeometry, const QRect& _viewportVisibilityAbsRect) override;

    void setCtrlButtonGeometry(const QRect &rect);

    void setQuoteSelection() override;

    int desiredWidth(int _width = 0) const override;

    int getMaxWidth() const override;

    bool isSizeLimited() const override;

    bool updateFriendly(const QString& _aimId, const QString& _friendly) override;

    int filenameDesiredWidth() const;

    int getMaxFilenameWidth() const;

    void requestPinPreview() override;

    QSize getImageMaxSize();

    bool clicked(const QPoint& _p) override;

    void resetClipping() override;

    bool isAutoplay();

    bool isSticker() const noexcept;

    MediaUtils::MediaType mediaType() const;

    bool needStretchToOthers() const override;
    bool needStretchByShift() const override;

    bool hasLeadLines() const override;

    bool isBubbleRequired() const override;

    bool canStretchWithSender() const override;

    int minControlsWidth() const;

    void updateWith(IItemBlock* _other) override;

    int effectiveBlockWidth() const override;

protected:
    void drawBlock(QPainter &p, const QRect& _rect, const QColor& _quoteColor) override;

    void initializeFileSharingBlock() override;

    void onMetainfoDownloaded() override;

    virtual void onMenuOpenFolder() final override;

    bool drag() override;

    void resizeEvent(QResizeEvent * _event) override;

    MenuFlags getMenuFlags(QPoint p) const override;

    void setSelected(const bool _isSelected) override;

    bool isSmallPreview() const override;

    void mouseMoveEvent(QMouseEvent* _event) override;
    void leaveEvent(QEvent* _event) override;

public:
    const QPixmap& getPreview() const;

    const QPixmap& getBackground() const;

private:
    void init();

    void applyClippingPath(QPainter &p, const QRect &previewRect);

    void connectImageSignals();

    void drawPlainFileBlock(QPainter &p, const QRect &frameRect, const QColor& quote_color);
    void drawPlainFileFrame(QPainter &p, const QRect &frameRect);
    void drawPlainFileName(QPainter &p);
    void drawPlainFileProgress(QPainter& _p);
    void drawPlainFileShowInDirLink(QPainter &p);

    void drawPreview(QPainter &p, const QRect &previewRect, const QColor& quote_color);
    void drawPreviewableBlock(QPainter &p, const QRect &previewRect, const QColor& quote_color);

    void initPlainFile();

    void initPreview();

    void initPreviewButton(const ActionButtonResource::ResourceSet& _resourceSet);

    bool isGifOrVideoPlaying() const;

    void onDataTransferStarted() override;

    void onDataTransferStopped() override;

    void onDownloaded() override;

    void onDownloadedAction() override;

    void onDataTransfer(const int64_t _bytesTransferred, const int64_t _bytesTotal) override;

    void onDownloadingFailed(const int64_t requestId) override;

    void onGifImageVisibilityChanged(const bool isVisible);

    void onLocalCopyInfoReady(const bool isCopyExists) override;

    bool onLeftMouseClick(const QPoint &_pos);

    void onPreviewMetainfoDownloaded() override;

    Data::StickerId getStickerId() const override { return Data::StickerId(Id_); }

    ContentType getContentType() const override;

    void parseLink();

    void requestPreview(const QString &uri);

    void sendGenericMetainfoRequests();

    bool shouldDisplayProgressAnimation() const;

    void updateFileTypeIcon();

    bool isInPreloadRange(const QRect& _widgetAbsGeometry, const QRect& _viewportVisibilityAbsRect);

#ifndef STRIP_AV_MEDIA
    std::unique_ptr<Ui::DialogPlayer> createPlayer();
#endif // !STRIP_AV_MEDIA

    bool loadPreviewFromLocalFile();

    bool getLocalFileMetainfo();

    int getLabelLeftOffset();
    int getSecondRowVerOffset();
    int getVerMargin() const;
    bool isButtonVisible() const;

    void buttonStartAnimation();
    void buttonStopAnimation();

    void setPreview(QPixmap _preview, QPixmap _background = QPixmap());

    void updateStyle() override;
    void updateFonts() override;

    bool isProgressVisible() const;

    void onSticker(qint32 _error, const QString& _fsId);

#ifndef STRIP_AV_MEDIA
    std::unique_ptr<Ui::DialogPlayer> videoplayer_;
#endif // !STRIP_AV_MEDIA

    QString Id_;

    QRect LastContentRect_;

    bool IsVisible_;

    bool IsInPreloadDistance_;

    bool needInitFromLocal_ = false;

    QSize OriginalPreviewSize_;

    QPainterPath PreviewClippingPath_;
    QPainterPath RelativePreviewClippingPath_;

    QPixmap Background_;
    qint64 seq_;

    int64_t PreviewRequestId_;

    ActionButtonWidget* previewButton_;
    FileSharingIcon* plainButton_;

    TextRendering::TextUnitPtr nameLabel_;
    TextRendering::TextUnitPtr sizeProgressLabel_;
    TextRendering::TextUnitPtr showInFolderLabel_;

private Q_SLOTS:
    void onImageDownloadError(qint64 seq, QString rawUri);

    void onImageDownloaded(int64_t seq, QString uri, QPixmap image);

    void onImageDownloadingProgress(qint64 seq, int64_t bytesTotal, int64_t bytesTransferred, int32_t pctTransferred);

    void onImageMetaDownloaded(int64_t seq, Data::LinkMetadata meta);

    void onStartClicked(const QPoint& _globalPos);
    void onStopClicked(const QPoint&);
    void onPlainFileIconClicked();

    void localPreviewLoaded(QPixmap pixmap, const QSize _originalSize);
    void localDurationLoaded(qint64 _duration);
    void localGotAudioLoaded(bool _gotAudio);
    void multiselectChanged();
    void prepareBackground(const QPixmap& _result, qint64 _srcCacheKey);
};

UI_COMPLEX_MESSAGE_NS_END
