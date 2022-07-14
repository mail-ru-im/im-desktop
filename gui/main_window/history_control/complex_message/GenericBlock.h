#pragma once

#include "IItemBlock.h"
#include "../QuoteColorAnimation.h"
#include "../gui/utils/PersonTooltip.h"

namespace Utils
{
    enum class OpenAt;
}

UI_NS_BEGIN

class ActionButtonWidget;
enum class MediaType;

namespace TextRendering
{
    enum class EmojiSizeType;
}

UI_NS_END

UI_COMPLEX_MESSAGE_NS_BEGIN

class ComplexMessageItem;

class GenericBlock :
    public QWidget,
    public IItemBlock
{
    Q_OBJECT

    Q_SIGNALS:
        void clicked();
        void setQuoteAnimation();
        void removeMe();

public:
    GenericBlock(
        ComplexMessageItem* parent,
        const Data::FString& _sourceText,
        const MenuFlags _menuFlags,
        const bool _isResourcesUnloadingEnabled);

    GenericBlock(
        ComplexMessageItem* parent,
        Data::FStringView _sourceText,
        const MenuFlags _menuFlags,
        const bool _isResourcesUnloadingEnabled);

    using IItemBlock::clicked;

    virtual ~GenericBlock() = 0;

    void setParentMessage(ComplexMessageItem* _parent) override;

    QSize blockSizeForMaxWidth(const int32_t maxWidth) override;

    void deleteLater() final override;

    QString formatRecentsText() const override;

    Ui::MediaType getMediaType() const override;

    bool standaloneText() const override { return false; }

    ComplexMessageItem* getParentComplexMessage() const final override;

    qint64 getId() const;

    QString getSenderAimid() const override;

    QString getSenderFriendly() const;

    void setSourceText(const Data::FString& _text);

    const QString& getChatAimid() const;

    Data::FString getSourceText() const override;

    Data::FString getTextInstantEdit() const override;

    QString getPlaceholderText() const override;

    const QString& getLink() const override;

    QStringList messageLinks() const override;

    QString getTextForCopy() const override;

    bool isBubbleRequired() const override;

    bool isMarginRequired() const override;

    void setBubbleRequired(bool required);

    bool isOutgoing() const;

    bool isDraggable() const override;

    bool isSharingEnabled() const override;

    bool isStandalone() const;

    bool isSenderVisible() const;

    MenuFlags getMenuFlags(QPoint p) const override;

    bool onMenuItemTriggered(const QVariantMap &params) override;

    void onActivityChanged(const bool isActive) override;

    void onVisibleRectChanged(const QRect& _visibleRect) override;

    void onDistanceToViewportChanged(const QRect& _widgetAbsGeometry, const QRect& _viewportVisibilityAbsRect) override;

    QRect setBlockGeometry(const QRect &ltr) override;

    QSize sizeHint() const override;

    virtual void setMaxPreviewWidth(int width) { }

    virtual int getMaxPreviewWidth() const { return 0; }

    void setQuoteSelection() override;

    void hideBlock() final override;

    bool isHasLinkInMessage() const override;

    void shiftHorizontally(const int _shift) override;

    void setGalleryId(qint64 _msgId);

    virtual void setEmojiSizeType(const TextRendering::EmojiSizeType _emojiSizeType) {}

    void setInsideQuote(bool _inside);

    void setInsideForward(bool _inside);

    bool isInsideQuote() const;

    bool isInsideForward() const;

    void selectByPos(const QPoint& from, const QPoint& to, bool _topToBottom) override;

    void selectAll() override;

    bool isSelected() const override;

    void setSelected(bool _selected) override;

    void clearSelection() override;
    Data::FilesPlaceholderMap getFilePlaceholders() override;

    void updateWith(IItemBlock* _other) override;

    bool needStretchToOthers() const override;

    void stretchToWidth(const int _width) override;

    QRect getBlockGeometry() const override;

    qint64 getGalleryId() const;

    void notifyBlockContentsChanged();

    bool hasLeadLines() const override;

    void markDirty() override;

    static void showFileSavedToast(const QString& _path);

    static void showErrorToast();

    static void showToast(const QString& _text);

    bool isEdited() const;

protected:
    bool drag() override;

    virtual void drawBlock(QPainter &p, const QRect& _rect, const QColor& _quoteColor) = 0;

    virtual void initialize() = 0;

    bool isInitialized() const;

    virtual void onMenuCopyLink();

    virtual void onMenuCopyFile();

    virtual void onMenuSaveFileAs();

    virtual void onMenuOpenInBrowser();

    virtual void onMenuOpenFolder();

    virtual void showFileInDir(const Utils::OpenAt _mode);

    virtual void onRestoreResources();

    virtual void onUnloadResources();

    void enterEvent(QEvent*) override;

    void paintEvent(QPaintEvent* _e) final override;

    void mouseMoveEvent(QMouseEvent* _e) override;

    void mousePressEvent(QMouseEvent* _e) override;

    void mouseReleaseEvent(QMouseEvent* _e) override;

    void wheelEvent(QWheelEvent* _e) override;

    QPoint getShareButtonPos(const bool _isBubbleRequired, const QRect& _bubbleRect) const override;

    QPoint mapFromParent(const QPoint& _point, const QRect& _blockGeometry) const;

    void resizeBlock(const QSize& _size) override;

    void onBlockSizeChanged(const QSize& _size) override;

    bool isTooltipActivated() const;
    void showTooltip(QString _text, QRect _rect, Tooltip::ArrowDirection _arrowDir, Tooltip::ArrowPointPos _arrowPos);
    void showMentionTooltip(QString _text, QString _name, QRect _rect, Tooltip::ArrowDirection _arrowDir, Tooltip::ArrowPointPos _arrowPos);
    void hideTooltip();

    QPointer<QuoteColorAnimation> QuoteAnimation_;

private:
    GenericBlock();

    void startResourcesUnloadTimer();

    void stopResourcesUnloadTimer();


    MenuFlags MenuFlags_;

    ComplexMessageItem* Parent_;

    QTimer* ResourcesUnloadingTimer_ = nullptr;
    QTimer* tooltipTimer_ = nullptr;
    Utils::PersonTooltip* personTooltip_ = nullptr;

    Data::FString SourceText_;

    QPoint mousePos_;

    QPoint mouseClickPos_;

    qint64 galleryId_ = -1;

    bool IsBubbleRequired_ = true;
    bool Initialized_ = false;
    bool IsResourcesUnloadingEnabled_;
    bool IsInsideQuote_ = false;
    bool IsInsideForward_ = false;
    bool IsSelected_ = false;
    bool tooltipActivated_ = false;

private Q_SLOTS:
    void onResourceUnloadingTimeout();
};

UI_COMPLEX_MESSAGE_NS_END
