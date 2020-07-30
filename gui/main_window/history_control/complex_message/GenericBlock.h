#pragma once

#include "IItemBlock.h"
#include "../QuoteColorAnimation.h"

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
        ComplexMessageItem *parent,
        const QString &sourceText,
        const MenuFlags menuFlags,
        const bool isResourcesUnloadingEnabled);

    using IItemBlock::clicked;

    virtual ~GenericBlock() = 0;

    virtual QSize blockSizeForMaxWidth(const int32_t maxWidth) override;

    virtual void deleteLater() final override;

    virtual QString formatRecentsText() const override;

    Ui::MediaType getMediaType() const override;

    virtual bool standaloneText() const override { return false; }

    virtual ComplexMessageItem* getParentComplexMessage() const final override;

    qint64 getId() const;

    QString getSenderAimid() const override;

    QString getSenderFriendly() const;

    void setSourceText(const QString& _text);

    const QString& getChatAimid() const;

    QString getSourceText() const override;

    QString getTextInstantEdit() const override;

    QString getPlaceholderText() const override;

    const QString& getLink() const override;

    QString getTextForCopy() const override;

    bool isBubbleRequired() const override;

    bool isMarginRequired() const override;

    void setBubbleRequired(bool required);

    bool isOutgoing() const;

    virtual bool isDraggable() const override;

    virtual bool isSharingEnabled() const override;

    bool isStandalone() const;

    bool isSenderVisible() const;

    MenuFlags getMenuFlags(QPoint p) const override;

    bool onMenuItemTriggered(const QVariantMap &params) override;

    void onActivityChanged(const bool isActive) override;

    void onVisibilityChanged(const bool isVisible) override;

    void onDistanceToViewportChanged(const QRect& _widgetAbsGeometry, const QRect& _viewportVisibilityAbsRect) override;

    QRect setBlockGeometry(const QRect &ltr) override;

    QSize sizeHint() const override;

    virtual void setMaxPreviewWidth(int width) { }

    virtual int getMaxPreviewWidth() const { return 0; }

    void setQuoteSelection() override;

    void hideBlock() final override;

    bool isHasLinkInMessage() const override;

    virtual bool isOverLink(const QPoint& _mousePosGlobal) const { return false; }

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
    virtual Data::FilesPlaceholderMap getFilePlaceholders() override;

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

    static void showFileCopiedToast();

    static void showToast(const QString& _text);

    bool isEdited() const;

protected:
    virtual bool drag() override;

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

    virtual void enterEvent(QEvent *) override;

    virtual void paintEvent(QPaintEvent *e) final override;

    virtual void mouseMoveEvent(QMouseEvent *e) override;

    virtual void mousePressEvent(QMouseEvent *e) override;

    virtual void mouseReleaseEvent(QMouseEvent *e) override;

    virtual QPoint getShareButtonPos(const bool _isBubbleRequired, const QRect& _bubbleRect) const override;

    QPoint mapFromParent(const QPoint& _point, const QRect& _blockGeometry) const;

    void resizeBlock(const QSize& _size) override;

    void onBlockSizeChanged(const QSize& _size) override;

    QuoteColorAnimation QuoteAnimation_;

private:
    GenericBlock();

    void startResourcesUnloadTimer();

    void stopResourcesUnloadTimer();

    static QPoint getToastPos(const QSize& _dialogSize);

    bool Initialized_;

    bool IsResourcesUnloadingEnabled_;

    MenuFlags MenuFlags_;

    ComplexMessageItem *Parent_;

    QTimer *ResourcesUnloadingTimer_;

    QString SourceText_;

    bool IsBubbleRequired_;

    QPoint mousePos_;

    QPoint mouseClickPos_;

    qint64 galleryId_;

    bool IsInsideQuote_;

    bool IsInsideForward_;

    bool IsSelected_;

private Q_SLOTS:
    void onResourceUnloadingTimeout();
};

UI_COMPLEX_MESSAGE_NS_END
