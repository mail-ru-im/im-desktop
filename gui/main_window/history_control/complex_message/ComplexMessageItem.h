#pragma once

#include "../../../namespaces.h"
#include "../../../types/message.h"
#include "../../../types/link_metadata.h"
#include "../../../controls/TextUnit.h"
#include "../../../animation/animation.h"
#include "SnippetBlock.h"

#include "../HistoryControlPageItem.h"

UI_NS_BEGIN

class ActionButtonWidget;
class ContextMenu;
class HistoryControlPage;
class MessageTimeWidget;
class CustomButton;
enum class MediaType;


enum class ReplaceReason
{
    NoMeta,
    OtherReason,
};

UI_NS_END

UI_COMPLEX_MESSAGE_NS_BEGIN

enum class BlockSelectionType;
class ComplexMessageItemLayout;
class IItemBlock;
class GenericBlock;
enum class PinPlaceholderType;

typedef std::shared_ptr<const QPixmap> QPixmapSCptr;

typedef std::vector<IItemBlock*> IItemBlocksVec;

struct MessageButton
{
    void clearProgress();
    void initButton();

    Data::ButtonData data_;
    QRect rect_;
    bool hovered_ = false;
    bool pressed_ = false;
    bool animating_ = false;
    bool isExternalUrl_ = false;
    bool isTwoLines_ = false;

    qint64 seq_ = -1;
    QString reqId_;
    QDateTime pressTime_;
    QString originalText_;
};

class ComplexMessageItem final : public HistoryControlPageItem
{
    friend class ComplexMessageItemLayout;

    enum class MenuItemType
    {
        Copy,
        Quote,
        Forward
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
        const QString& _description,
        const Data::MentionMap& _mentions,
        const Data::QuotesVec& _quotes,
        qint32 _time,
        const Data::FilesPlaceholderMap& _files,
        MediaType _mediaType,
        bool instantEdit);

    void edit(
        const int64_t _msgId,
        const QString& _internalId,
        const common::tools::patch_version& _patchVersion,
        const QString& _text,
        const Data::MentionMap& _mentions,
        const Data::QuotesVec& _quotes,
        qint32 _time,
        const Data::FilesPlaceholderMap& _files,
        MediaType _mediaType,
        bool instantEdit);

    void pinPreview(const QPixmap& _preview);

    void needUpdateRecentsText();

    void hoveredBlockChanged();
    void selectionChanged();
    void leave();
    void removeMe();
    void addToFavorites(const Data::QuotesVec& _quotes);

    void readOnlyUser(const QString& _aimId, bool _isReadOnly, QPrivateSignal) const;
    void blockUser(const QString& _aimId, bool _isBlock, QPrivateSignal) const;

public:
    ComplexMessageItem(QWidget *parent, const Data::MessageBuddy& _msg);

    ~ComplexMessageItem();

    void clearSelection(bool _keepSingleSelection) override;

    QString formatRecentsText() const override;

    MediaType getMediaType(MediaRequestMode _mode = MediaRequestMode::Chat) const override;

    void updateFriendly(const QString& _aimId, const QString& _friendly) override;

    PinPlaceholderType getPinPlaceholder() const;

    void requestPinPreview(PinPlaceholderType _type);

    QString getFirstLink() const;
    Data::StickerId getFirstStickerId() const;

    qint64 getId() const override;
    qint64 getPrevId() const override;
    const QString& getInternalId() const override;

    QString getQuoteHeader() const;

    enum class TextFormat
    {
        Raw,
        Formatted
    };

    QString getSelectedText(const bool _isQuote, TextFormat _format = TextFormat::Formatted) const;

    const QString& getChatAimid() const;

    QDate getDate() const;

    const QString& getSenderAimid() const;

    QString getSenderFriendly() const;

    bool isOutgoing() const override;
    bool isEditable() const override;
    bool isMediaOnly() const;

    int32_t getTime() const override;

    void setTime(const int32_t time);

    bool isSimple() const;
    bool isSingleSticker() const;

    void onHoveredBlockChanged(IItemBlock *newHoveredBlock);
    void onSharingBlockHoverChanged(IItemBlock *newHoveredBlock);

    void updateStyle() override;
    void updateFonts() override;

    void startSpellChecking() override;

    virtual void onActivityChanged(const bool isActive) override;

    virtual void onVisibilityChanged(const bool isVisible) override;

    virtual void onDistanceToViewportChanged(const QRect& _widgetAbsGeometry, const QRect& _viewportVisibilityAbsRect) override;

    void replaceBlockWithSourceText(IItemBlock *block, ReplaceReason _reason = ReplaceReason::OtherReason);

    void removeBlock(IItemBlock *block);

    void cleanupBlock(IItemBlock *block);

    void selectByPos(const QPoint& from, const QPoint& to, const QPoint& areaFrom, const QPoint& areaTo) override;

    void setHasAvatar(const bool value) override;
    void setHasSenderName(const bool value) override;

    void setItems(IItemBlocksVec blocks);

    void setMchatSender(const QString& sender);

    void setLastStatus(LastStatus _lastStatus) final override;

    QSize sizeHint() const override;

    bool hasPictureContent() const override;

    bool canBeUpdatedWith(const ComplexMessageItem& _other) const;

    void updateWith(ComplexMessageItem &update);

    void getUpdatableInfoFrom(ComplexMessageItem &update);

    bool isSelected() const override;

    bool isAllSelected() const;

    Data::QuotesVec getQuotes(const bool _selectedTextOnly = true, const bool _isForward = false) const;
    Data::QuotesVec getQuotesForEdit() const;

    void setSourceText(QString text);

    void setQuoteSelection() override;
    void highlightText(const highlightsV& _highlights) override;
    void resetHighlight() override;

    void setDeliveredToServer(const bool _isDeliveredToServer) override;

    bool isQuoteAnimation() const;

    /// observe to resize when replaced
    bool isObserveToSize() const;

    int getMaxWidth() const;

    virtual void setEdited(const bool _isEdited) override;

    void setUpdatePatchVersion(const common::tools::patch_version& _version);

    const Data::MentionMap& getMentions() const;

    const Data::FilesPlaceholderMap& getFilesPlaceholders() const;

    int desiredWidth() const;

    int maxEffectiveBLockWidth() const;

    int buttonsDesiredWidth() const;

    int calcButtonsWidth(int _availableWidth);

    int buttonsHeight() const;

    void updateSize() override;

    void callEditing() override;

    void setHasTrailingLink(const bool _hasLink);

    void setHasLinkInText(const bool _hasLinkInText);

    void setButtons(const std::vector<std::vector<Data::ButtonData>>& _buttons);

    void updateButtonsFromOther(const std::vector<std::vector<MessageButton>>& _buttons);

    void setHideEdit(bool _hideEdit);

    int getBlockCount() const;

    IItemBlock* getHoveredBlock() const;
    void resetHover();

    int getSharingAdditionalWidth() const;

    bool isFirstBlockDebug() const;

    bool hasButtons() const;

    // NOTE: Debug purposes only
    const IItemBlocksVec& getBlocks() const;
    std::vector<Ui::ComplexMessage::GenericBlock*> getBlockWidgets() const;

    bool isHeadless() const;
    bool isLastBlock(const IItemBlock* _block) const;
    bool isHeaderRequired() const;
    bool isSmallPreview() const;
    bool canStretchWithSender() const;
    bool containsShareableBlocks() const;

    MessageTimeWidget* getTimeWidget() const;

    virtual void cancelRequests() override;

    void setUrlAndDescription(const QString& _url, const QString& _description);

    bool hasCaption() const { return !Description_.isEmpty(); }

    int getSenderDesiredWidth() const noexcept;

    bool isSenderVisible() const;

    void setCustomBubbleHorPadding(const int32_t _val);

    bool hasSharedContact() const;

    GenericBlock* addSnippetBlock(const QString& _link, const bool _linkInText, SnippetBlock::EstimatedType _estimatedType, size_t _insertAt, bool _quoteOrForward = false);

    virtual void setSelected(bool _selected) override;

    bool containsText() const;

    void updateTimeWidgetUnderlay();

    static bool isSpellCheckIsOn();

    QRect getBubbleGeometry() const;

    QRect messageRect() const override;

    ReactionsPlateType reactionsPlateType() const override;

    void setSpellErrorsVisible(bool _visible) override;

protected:

    void leaveEvent(QEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void mouseDoubleClickEvent(QMouseEvent *event) override;
    void paintEvent(QPaintEvent *event) override;

    void renderDebugInfo();

    void onChainsChanged() override;

    void initialize() override;

private Q_SLOTS:
    void onAvatarChanged(const QString& aimId);
    void onMenuItemTriggered(QAction *action);
    void onLinkMetainfoMetaDownloaded(int64_t _seq, bool _success, Data::LinkMetadata _meta);
    void onBotCallbackAnswer(const int64_t _seq, const QString& _reqId, const QString& text, const QString& _url, bool _showAlert);
    void updateButtons();

public Q_SLOTS:
    void trackMenu(const QPoint &globalPos);

    void setQuoteAnimation();
    void onObserveToSize();

private:
    void fillFilesPlaceholderMap();
    void handleBotAction(const QString& _url, const QString& _text = QString(), bool _showAlert = false);
    void initButtonsTimerIfNeeded();
    void startButtonsTimer(int _timeout);

    enum class InstantEdit
    {
        No,
        Yes
    };
    void callEditingImpl(InstantEdit _mode);
    QString getEditableText(InstantEdit _mode) const;

    void cleanupMenu();

    void connectSignals();

    void drawAvatar(QPainter &p);

    void drawBubble(QPainter &p, const QColor& quote_color);

    void drawButtons(QPainter &p, const QColor& quote_color);

    QString getBlocksText(const IItemBlocksVec& _items, const bool _isSelected, TextFormat _format = TextFormat::Formatted) const;

    void drawGrid(QPainter &p);

    enum class FindForSharing
    {
        No,
        Yes
    };

    IItemBlock* findBlockUnder(const QPoint& _pos, FindForSharing _findFor = FindForSharing::No) const;

    void initializeShareButton();

    void initSender();
    void initButtonsLabel();

    bool isBubbleRequired() const;
    bool isMarginRequired() const;

    bool isOverAvatar(const QPoint &pos) const;
    bool isOverSender(const QPoint &pos) const;

    void loadAvatar();

    void onCopyMenuItem(MenuItemType type);

    bool onDeveloperMenuItemTriggered(QStringView cmd);

    void onShareButtonClicked();

    void updateSenderControlColor();

    void updateShareButtonGeometry();
    void updateTimeAnimated();

    void showShareButtonAnimated();
    void hideShareButtonAnimated();

    void showTimeAnimated();
    void hideTimeAnimated();
    int getTimeAnimationDistance() const;

    void clearBlockSelection();

    enum class WidgetAnimationType
    {
        show,
        hide
    };
    void animateShareButton(const int _startPosX, const int _endPosX, const WidgetAnimationType _anim);
    void animateTime(const int _startPosY, const int _endPosY, const WidgetAnimationType _anim);

    Data::Quote getQuoteFromBlock(IItemBlock* _block, const bool _selectedTextOnly) const;

    void setTimeWidgetVisible(const bool _visible);
    bool isNeedAvatar() const;

    void loadSnippetsMetaInfo();

    QPixmapSCptr Avatar_;

    IItemBlocksVec Blocks_;

    QPainterPath Bubble_;

    QRect BubbleGeometry_;

    QString ChatAimid_;

    QDate Date_;

    IItemBlock* hoveredBlock_;
    IItemBlock* hoveredSharingBlock_;

    int64_t Id_;
    int64_t PrevId_;
    QString internalId_;
    common::tools::patch_version version_;

    bool IsOutgoing_;

    bool deliveredToServer_;

    ComplexMessageItemLayout *Layout_;

    IItemBlock *MenuBlock_;

    bool MouseLeftPressedOverAvatar_;
    bool MouseLeftPressedOverSender_;

    bool MouseRightPressedOverItem_;

    TextRendering::TextUnitPtr Sender_;
    int desiredSenderWidth_;

    QString SenderAimid_;

    QString SenderAimidForDisplay_;

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
    bool hasLinkInText_;
    bool hideEdit_;

    Data::MentionMap mentions_;
    Data::FilesPlaceholderMap files_;

    QPoint PressPoint_;
    QString Url_;
    QString Description_;
    bool isMediaOnly_;

    struct SnippetData
    {
        QString link_;
        bool linkInText_;
        IItemBlock* textBlock_;
        SnippetBlock::EstimatedType estimatedType_;
        size_t insertAt_;
    };

    std::vector<SnippetData> snippetsWaitingForInitialization_;
    std::map<int64_t, SnippetData> snippetsWaitingForMeta_;

    std::vector<std::vector<MessageButton>> buttons_;
    std::vector<int> buttonLineHeights_;
    TextRendering::TextUnitPtr buttonLabel_;

    QTimer* buttonsUpdateTimer_;
    anim::Animation buttonsAnimation_;
};

UI_COMPLEX_MESSAGE_NS_END
