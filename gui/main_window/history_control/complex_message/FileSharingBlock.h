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
        ComplexMessageItem* _parent,
        const QString& _link,
        const core::file_sharing_content_type _type);

    FileSharingBlock(
        ComplexMessageItem* _parent,
        const HistoryControl::FileSharingInfoSptr& _fsInfo,
        const core::file_sharing_content_type _type);

    virtual ~FileSharingBlock() override;

    QSize getCtrlButtonSize() const;

    QSize originSizeScaled() const;

    static QPixmap scaledSticker(const QPixmap&);
    static QImage scaledSticker(const QImage&);

    bool isPreviewReady() const;

    bool isFilenameElided() const;

    bool isSharingEnabled() const override;

    void onVisibleRectChanged(const QRect& _visibleRect) override;

    void onSelectionStateChanged(const bool _isSelected) override;

    void onDistanceToViewportChanged(const QRect& _widgetAbsGeometry, const QRect& _viewportVisibilityAbsRect) override;

    void setCtrlButtonGeometry(const QRect& _rect);

    void setQuoteSelection() override;

    int desiredWidth(int _width = 0) const override;

    int getMaxWidth() const override;

    bool isSizeLimited() const override;

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

    QString getProgressText() const override;

    void markTrustRequired() override;

protected:
    void drawBlock(QPainter& p, const QRect& _rect, const QColor& _quoteColor) override;

    void initializeFileSharingBlock() override;

    void onMetainfoDownloaded() override;

    virtual void onMenuOpenFolder() final override;

    bool drag() override;

    void resizeEvent(QResizeEvent* _event) override;

    MenuFlags getMenuFlags(QPoint _p) const override;

    bool isSmallPreview() const override;

    void mouseMoveEvent(QMouseEvent* _event) override;
    void leaveEvent(QEvent* _event) override;

public:
    const QPixmap& getPreview() const;

    const QPixmap& getBackground() const;

private:
    void init();

    void applyClippingPath(QPainter& _p, const QRect& _previewRect);

    void connectImageSignals();

    void drawPlainFileBlock(QPainter& _p, const QRect& _frameRect, const QColor& _quote_color);
    void drawPlainFileFrame(QPainter& _p, const QRect& _frameRect);
    void drawPlainFileName(QPainter& _p);
    void drawPlainFileProgress(QPainter& _p);
    void drawPlainFileShowInDirLink(QPainter& _p);
    void drawPlainFileStatus(QPainter& _p) const;

    void drawPreview(QPainter& _p, const QRect& _previewRect, const QColor& _quoteColor);
    void drawPreviewableBlock(QPainter& _p, const QRect& _previewRect, const QColor& _quoteColor);

    void initPlainFile();

    void initPreview();

    void initPreviewButton(const ActionButtonResource::ResourceSet& _resourceSet);

    bool isGifOrVideoPlaying() const;

    void onDataTransferStarted() override;

    void onDataTransferStopped() override;

    void onDownloaded() override;

    void onDownloadedAction() override;

    void onDataTransfer(const int64_t _bytesTransferred, const int64_t _bytesTotal, bool _showBytes = true) override;

    void onDownloadingFailed(const int64_t _requestId) override;

    void onGifImageVisibilityChanged(const bool _isVisible);

    void onLocalCopyInfoReady(const bool _isCopyExists) override;

    bool onLeftMouseClick(const QPoint& _pos);

    void onPreviewMetainfoDownloaded() override;

    Data::StickerId getStickerId() const override { return Data::StickerId(getFileSharingId()); }

    ContentType getContentType() const override;

    void parseLink();

    void requestPreview(const QString& _uri);

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

    void updateFonts() override;

    bool isProgressVisible() const;

    void onSticker(qint32 _error, const Utils::FileSharingId& _fsId);

    QRect getFileStatusRect() const;
    QRect getPlainButtonRect() const;

    void elidePlainFileName();
    void setPlainFileName(const QString& _name);
    void updatePlainButtonBlockReason(bool _forcePlaceholder);
    bool isPlaceholder() const;

#ifndef STRIP_AV_MEDIA
    std::unique_ptr<Ui::DialogPlayer> videoplayer_;
#endif // !STRIP_AV_MEDIA

    QRect LastContentRect_;

    QSize OriginalPreviewSize_;

    QPainterPath PreviewClippingPath_;
    QPainterPath RelativePreviewClippingPath_;

    QPixmap Background_;
    qint64 seq_ = -1;

    int64_t PreviewRequestId_ = -1;

    ActionButtonWidget* previewButton_ = nullptr;
    FileSharingIcon* plainButton_ = nullptr;

    TextRendering::TextUnitPtr nameLabel_;
    TextRendering::TextUnitPtr sizeProgressLabel_;
    TextRendering::TextUnitPtr showInFolderLabel_;

    bool IsVisible_ = false;
    bool IsInPreloadDistance_ = true;
    bool needInitFromLocal_ = false;

    bool antivirusCheckEnabled_ = false;

private Q_SLOTS:
    void onImageDownloadError(qint64 _seq, QString _rawUri);

    void onImageDownloaded(int64_t _seq, QString _uri, QPixmap _image);

    void onImageDownloadingProgress(qint64 _seq, int64_t _bytesTotal, int64_t _bytesTransferred, int32_t _pctTransferred);

    void onImageMetaDownloaded(int64_t _seq, Data::LinkMetadata _meta);

    void onAntivirusCheckResult(const Utils::FileSharingId& _fileHash, core::antivirus::check::result _result);

    void onStartClicked(const QPoint& _globalPos);
    void onStopClicked(const QPoint&);
    void onPlainFileIconClicked();

    void localPreviewLoaded(QPixmap _pixmap, const QSize _originalSize);
    void localDurationLoaded(qint64 _duration);
    void localGotAudioLoaded(bool _gotAudio);
    void multiselectChanged();
    void prepareBackground(const QPixmap& _result, qint64 _srcCacheKey);

    void onConfigChanged();
};

UI_COMPLEX_MESSAGE_NS_END
