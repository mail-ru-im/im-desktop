#pragma once

#include "../../../namespaces.h"
#include "../../../types/message.h"
#include "../../../controls/TextUnit.h"

#include "../MessageItemBase.h"

UI_NS_BEGIN

class ActionButtonWidget;
class ContextMenu;
class HistoryControlPage;
class MessageTimeWidget;
class CustomButton;
enum class MediaType;

UI_NS_END

UI_COMPLEX_MESSAGE_NS_BEGIN

enum class BlockSelectionType;
class ComplexMessageItemLayout;
class IItemBlock;
class GenericBlock;
enum class PinPlaceholderType;

typedef std::shared_ptr<const QPixmap> QPixmapSCptr;

typedef std::vector<IItemBlock*> IItemBlocksVec;

class ComplexMessageItem final : public MessageItemBase
{
    friend class ComplexMessageItemLayout;

    enum class MenuItemType
    {
        Copy,
        Quote,
        Forward,
    };

    Q_OBJECT

Q_SIGNALS:
    void copy(const QString& text);
    void quote(const Data::QuotesVec&);
    void forward(const Data::QuotesVec&);
    void avatarMenuRequest(const QString&);
    void pin(const QString& _chatId, const int64_t _msgId, const bool _isUnpin = false);

    void layoutChanged(QPrivateSignal) const;

    void editWithCaption(
        const int64_t _msgId,
        const QString& _internalId,
        const common::tools::patch_version& _patchVersion,
        const QString& _url,
        const QString& _description);

    void edit(
        const int64_t _msgId,
        const QString& _internalId,
        const common::tools::patch_version& _patchVersion,
        const QString& _text,
        const Data::MentionMap& _mentions,
        const Data::QuotesVec& _quotes,
        qint32 _time);

    void pinPreview(const QPixmap& _preview);

    void needUpdateRecentsText();

    void hoveredBlockChanged();
    void selectionChanged();
    void leave();
    void removeMe();

public:
    ComplexMessageItem(
        QWidget *parent,
        const int64_t id,
        const int64_t prevId,
        const QString &_internalId,
        const QDate date,
        const QString &chatAimid,
        const QString &senderAimid,
        const QString &senderFriendly,
        const QString &sourceText,
        const Data::MentionMap& _mentions,
        const bool isOutgoing);

    ~ComplexMessageItem();

    void clearSelection() override;

    QString formatRecentsText() const override;

    MediaType getMediaType() const override;

    void updateFriendly(const QString& _aimId, const QString& _friendly) override;

    PinPlaceholderType getPinPlaceholder() const;

    void requestPinPreview(PinPlaceholderType _type);

    QString getFirstLink() const;

    qint64 getId() const override;
    qint64 getPrevId() const override;
    const QString& getInternalId() const override;

    QString getQuoteHeader() const;

    QString getSelectedText(const bool isQuote) const;

    const QString& getChatAimid() const;

    QDate getDate() const;

    const QString& getSenderAimid() const;

    QString getSenderFriendly() const;

    bool isOutgoing() const override;
    bool isEditable() const override;

    int32_t getTime() const override;

    void setTime(const int32_t time);

    bool isSimple() const;
    bool isSingleSticker() const;

    void onHoveredBlockChanged(IItemBlock *newHoveredBlock);

    void updateStyle() override;
    void updateFonts() override;

    virtual void onActivityChanged(const bool isActive) override;

    virtual void onVisibilityChanged(const bool isVisible) override;

    virtual void onDistanceToViewportChanged(const QRect& _widgetAbsGeometry, const QRect& _viewportVisibilityAbsRect) override;

    void replaceBlockWithSourceText(IItemBlock *block);

    void removeBlock(IItemBlock *block);

    void cleanupBlock(IItemBlock *block);

    void selectByPos(const QPoint& from, const QPoint& to);

    void setHasAvatar(const bool value) override;
    void setHasSenderName(const bool value) override;

    void setItems(IItemBlocksVec blocks);

    void setMchatSender(const QString& sender);

    void setLastStatus(LastStatus _lastStatus) final override;

    QSize sizeHint() const override;

    bool hasPictureContent() const override;

    bool canBeUpdatedWith(const ComplexMessageItem& _other) const;

    void updateWith(ComplexMessageItem &update);

    bool isSelected() const override;

    bool isAllSelected() const;

    void setDrawFullBubbleSelected(const bool _selected);

    Data::QuotesVec getQuotes(const bool _selectedTextOnly = true, const bool _isForward = false) const;
    Data::QuotesVec getQuotesForEdit() const;

    void setSourceText(QString text);

    virtual void setQuoteSelection() override;

    void setDeliveredToServer(const bool _isDeliveredToServer) override;

    bool isQuoteAnimation() const;

    /// observe to resize when replaced
    bool isObserveToSize() const;

    virtual int getMaxWidth() const;

    virtual void setEdited(const bool _isEdited) override;

    void setUpdatePatchVersion(const common::tools::patch_version& _version);

    const Data::MentionMap& getMentions() const;

    QString getEditableText() const;

    int desiredWidth() const;

    void updateSize() override;

    void callEditing() override;

    void setHasTrailingLink(const bool _hasLink);

    int getBlockCount() const;

    IItemBlock* getHoveredBlock() const;
    void resetHover();

    int getSharingAdditionalWidth() const;

    bool isFirstBlockDebug() const;

    // NOTE: Debug purposes only
    const IItemBlocksVec& getBlocks() const;
    std::vector<Ui::ComplexMessage::GenericBlock*> getBlockWidgets() const;

    bool isHeadless() const;
    bool isLastBlock(const IItemBlock* _block) const;
    bool isHeaderRequired() const;
    bool isSmallPreview() const;
    bool containsShareableBlocks() const;

    MessageTimeWidget* getTimeWidget() const;

    virtual void cancelRequests() override;

    void setUrlAndDescription(const QString& _url, const QString& _description);

    bool hasCaption() const { return !Description_.isEmpty(); }

    int getSenderDesiredWidth() const noexcept;

    bool isSenderVisible() const;

    void setCustomBubbleHorPadding(const int32_t _val);

    bool hasSharedContact() const;

protected:

    void leaveEvent(QEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void mouseDoubleClickEvent(QMouseEvent *event) override;
    void paintEvent(QPaintEvent *event) override;

    void renderDebugInfo();

    void onChainsChanged() override;

private Q_SLOTS:
    void onAvatarChanged(const QString& aimId);
    void onMenuItemTriggered(QAction *action);

public Q_SLOTS:
    void trackMenu(const QPoint &globalPos);

    void setQuoteAnimation();
    void onObserveToSize();

private:

    void cleanupMenu();

    void connectSignals();

    void drawAvatar(QPainter &p);

    void drawBubble(QPainter &p, const QColor& quote_color);

    QString getBlocksText(const IItemBlocksVec& _items, const bool _isSelected) const;

    void drawGrid(QPainter &p);

    IItemBlock* findBlockUnder(const QPoint &pos) const;

    void initialize();
    void initializeShareButton();

    void initSender();

    bool isBubbleRequired() const;

    bool isOverAvatar(const QPoint &pos) const;
    bool isOverSender(const QPoint &pos) const;

    void loadAvatar();

    void onCopyMenuItem(MenuItemType type);

    bool onDeveloperMenuItemTriggered(const QStringRef &cmd);

    void onShareButtonClicked();

    void updateSenderControlColor();

    void updateShareButtonGeometry();
    void updateTimeAnimated();

    void showShareButtonAnimated();
    void hideShareButtonAnimated();

    void showTimeAnimated();
    void hideTimeAnimated();
    int getTimeAnimationDistance() const;

    enum class WidgetAnimationType
    {
        show,
        hide
    };
    void animateShareButton(const int _startPosX, const int _endPosX, const WidgetAnimationType _anim);
    void animateTime(const int _startPosY, const int _endPosY, const WidgetAnimationType _anim);

    Data::Quote getQuoteFromBlock(IItemBlock* _block, const bool _selectedTextOnly) const;

    void updateTimeWidgetUnderlay();
    void setTimeWidgetVisible(const bool _visible);
    bool isNeedAvatar() const;

    QPixmapSCptr Avatar_;

    IItemBlocksVec Blocks_;

    QPainterPath Bubble_;

    QRect BubbleGeometry_;

    QString ChatAimid_;

    QDate Date_;

    BlockSelectionType FullSelectionType_;

    IItemBlock* hoveredBlock_;
    IItemBlock* hoveredSharingBlock_;

    int64_t Id_;
    int64_t PrevId_;
    QString internalId_;
    common::tools::patch_version version_;

    bool Initialized_;

    bool IsOutgoing_;

    bool IsDeliveredToServer_;

    ComplexMessageItemLayout *Layout_;

    IItemBlock *MenuBlock_;

    bool MouseLeftPressedOverAvatar_;
    bool MouseLeftPressedOverSender_;

    bool MouseRightPressedOverItem_;

    TextRendering::TextUnitPtr Sender_;
    int desiredSenderWidth_;

    QString SenderAimid_;

    QString SenderFriendly_;

    CustomButton* shareButton_;
    anim::Animation shareButtonAnimation_;
    QGraphicsOpacityEffect* shareButtonOpacityEffect_;

    QString SourceText_;

    MessageTimeWidget* TimeWidget_;
    anim::Animation timeAnimation_;
    QGraphicsOpacityEffect* timeOpacityEffect_;

    int32_t Time_;

    bool bQuoteAnimation_;
    bool bObserveToSize_;
    bool bubbleHovered_;
    bool hasTrailingLink_;

    Data::MentionMap mentions_;

    QPoint PressPoint_;
    bool isDrawFullBubbleSelected_;
    QString Url_;
    QString Description_;
};

UI_COMPLEX_MESSAGE_NS_END
