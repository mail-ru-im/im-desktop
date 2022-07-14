#pragma once

#include "history/History.h"
#include "history/Message.h"
#include "../../types/filesharing_meta.h"
#include "complex_message/FileSharingUtils.h"


namespace hist
{
    class DateInserter;
}

namespace Ui
{
    class SmartReplyWidget;
    class MessagesScrollbar;
    class MessageItem;
    class HistoryControlPageItem;
    class MessagesScrollAreaLayout;

    struct InsertHistMessagesParams;
    struct DeleteMessageInfo;

    struct SelectedMessageInfo
    {
        Data::FString text_;
        bool isOutgoing_ = false;
        bool isDisabled_ = false;
        Data::QuotesVec quotes_;
        QPoint from_;
        QPoint to_;
        Data::FilesPlaceholderMap filesPlaceholders_;
        Data::MentionMap mentions_;
    };

    enum class ScrollDirection
    {
        UP = 0,
        DOWN,
    };

    class ScrollAreaContainer : public QAbstractScrollArea
    {
        Q_OBJECT

    Q_SIGNALS:
        void scrollViewport(const int _dy, QPrivateSignal) const;

    public:
        ScrollAreaContainer(QWidget* _parent);

    protected:
        void wheelEvent(QWheelEvent* _e) override;
        void scrollContentsBy(const int _dx, const int _dy) override;
    };

    class MessagesScrollArea : public QWidget
    {
        Q_OBJECT

    Q_SIGNALS:
        void fetchRequestedEvent(bool _isMoveToBottomIfNeed = true);

        void scrollMovedToBottom();

        void messagesSelected();

        void messagesDeselected();

        void updateHistoryPosition(int32_t position, int32_t offset);

        void buttonDownClicked();

        void recreateAvatarRect();

        void itemRead(const qint64 _id, const bool _visible);

        void widgetRemoved();

    public:
        using MessageItemVisitor = std::function<bool(Ui::MessageItem*, const bool)>;
        using WidgetVisitor = std::function<bool(QWidget*, const bool)>;

        MessagesScrollArea(QWidget *parent, QWidget *typingWidget, hist::DateInserter* dateInserter, const QString& _aimid);

        void cancelSelection();

        void cancelWheelBufferReset();

        void enumerateWidgets(const WidgetVisitor& visitor, const bool reversed) const;

        QWidget* getItemByPos(const int32_t pos) const;

        QWidget* getItemByKey(const Logic::MessageKey &key) const;

        QWidget* extractItemByKey(const Logic::MessageKey& _key);

        QWidget* getPrevItem(QWidget* _w) const;

        int32_t getItemsCount() const;

        Logic::MessageKeyVector getItemsKeys() const;

        enum class TextFormat
        {
            Raw,
            Formatted
        };

        Data::FString getSelectedText(TextFormat _format = TextFormat::Formatted) const;
        Data::FilesPlaceholderMap getFilesPlaceholders() const;
        Data::MentionMap getMentions() const;

        enum class IsForward
        {
            No,
            Yes
        };

        Data::QuotesVec getQuotes(IsForward _isForward = IsForward::No) const;

        MessagesScrollAreaLayout* getLayout() const { return Layout_; }

        QVector<Logic::MessageKey> getKeysToUnloadTop() const;
        QVector<Logic::MessageKey> getKeysToUnloadBottom() const;

        void insertWidget(const Logic::MessageKey &key, std::unique_ptr<QWidget> widget);
        void insertWidgets(InsertHistMessagesParams&& _params);

        bool isScrolling() const;

        bool isSelecting() const;

        bool isViewportFull() const;

        bool containsWidget(QWidget *widget) const;

        void removeWidget(QWidget *widget);
        void cancelWidgetRequests(const QVector<Logic::MessageKey>&);
        void removeWidgets(const QVector<Logic::MessageKey>&);

        void removeAll();

        void replaceWidget(const Logic::MessageKey &key, std::unique_ptr<QWidget> widget);

        bool needFetchMoreToTop() const;
        bool needFetchMoreToBottom() const;

        void scrollToBottom();

        void updateItemKey(const Logic::MessageKey &key);
        void scrollTo(const Logic::MessageKey &key, hist::scroll_mode_type _scrollMode);
        void scrollContent(const int _dy);

        void updateScrollbar();

        bool isScrollAtBottom() const;

        void clearSelection(bool keepSingleSelection = false);

        void clearPartialSelection();

        void continueSelection(const QPoint& _pos);

        void scroll(ScrollDirection direction, int delta);

        bool contains(const QString& _aimId) const;

        void resumeVisibleItems();

        void suspendVisibleItems();

        void setIsSearch(bool _isSearch);
        bool getIsSearch() const;

        void setMessageId(qint64 _id);
        qint64 getMessageId() const;

        void setRecvLastMessage(bool _value);

        void updateItems();

        void enableViewportShifting(bool enable);
        void resetShiftingParams();

        bool isInitState() const noexcept;

        bool tryEditLastMessage();

        void invalidateLayout();
        void checkVisibilityForRead();

        void countSelected(int& _forMe, int& _forAll) const;
        std::vector<DeleteMessageInfo> getSelectedMessagesForDelete() const;
        bool messagesCanBeDeleted() const;

        int getSelectedCount() const;
        int getSelectedUnsupportedCount() const;
        int getSelectedPlainFilesCount() const;

        void setSmartreplyWidget(SmartReplyWidget* _widget);
        void showSmartReplies();
        void hideSmartReplies();
        void setSmartreplyButton(QWidget* _button);
        void setSmartrepliesSemitransparent(bool _semi);
        void notifyQuotesResize();

        bool hasItems() const noexcept;
        QString getAimid() const;

        void clearPttProgress();
        void applySelection();


    public:
        enum State
        {
            None                     = 0,
            IsSelecting_             = 1 << 1,
            IsTouchScrollInProgress_ = 1 << 2,
            IsScrollShowOnOpen_      = 1 << 3,
            IsSearching_             = 1 << 4
        };
        Q_DECLARE_FLAGS (States, State)

    public Q_SLOTS:
        void notifySelectionChanges();

    protected:
        void mouseMoveEvent(QMouseEvent *e) override;
        void mousePressEvent(QMouseEvent *e) override;
        void mouseReleaseEvent(QMouseEvent *e) override;

        void enterEvent(QEvent *_event) override;
        void leaveEvent(QEvent *event) override;

        void wheelEvent(QWheelEvent *e) override;
        bool event(QEvent *e) override;
        void showEvent(QShowEvent *_event) override;
        void hideEvent(QHideEvent *_event) override;

    private Q_SLOTS:
        void onAnimationTimer();

        void onMessageHeightChanged(QSize, QSize);

        void onSliderMoved(int value);

        void onSliderPressed();

        void onSliderValue(int value);

        void onWheelEventResetTimer();
        void onScrollHoverChanged(bool _nowHovered);

        /// resend from MessagesScrollLayout to HistoryControlPage
        void onUpdateHistoryPosition(int32_t position, int32_t offset);
        void onHideScrollbarTimer();

        void multiSelectCurrentElementChanged();
        void multiSelectCurrentMessageUp(bool);
        void multiSelectCurrentMessageDown(bool);
        void messageSelected(qint64, const QString&);
        bool switchMessageSelection(Logic::MessageKey _key, std::optional<bool> _prevSelection = std::nullopt);

    public Q_SLOTS:
        void onWheelEvent(QWheelEvent* e);
        void updateSelected(const QString& _aimId, qint64, const QString&, bool);
        void updateSelectedCount(const QString& _aimId);
        void multiselectChanged();
        void updatePttProgress(qint64, const Utils::FileSharingId&, int);

    private:
        bool isHidingScrollbar() const;
        void hideScrollbar();
        void setScrollbarVisible(bool _visible);

        void onSelectionChanged(int _selectedCount);

        void updateMode();

        bool enqueWheelEvent(const int32_t delta);

        double evaluateScrollingVelocity() const;

        double evaluateScrollingStep(const int64_t now) const;

        int32_t evaluateWheelHistorySign() const;

        void scheduleWheelBufferReset();

        std::map<Logic::MessageKey, QWidget*> getWidgetsMapFromParams(const InsertHistMessagesParams& _params);

        enum class ScrollingMode;
        void startScrollAnimation(const ScrollingMode mode);

        void stopScrollAnimation();

        void eraseContact(QWidget* widget);

        int minViewPortHeightForUnload() const;

        void updateStateFlag(State _flag, bool _enabled);

    private:
        int64_t LastAnimationMoment_;

        QPoint LastMouseGlobalPos_;

        QPoint SelectionBeginAbsPos_;

        QPoint SelectionEndAbsPos_;

        ScrollingMode Mode_;

        MessagesScrollbar *Scrollbar_;

        MessagesScrollAreaLayout *Layout_;

        QTimer ScrollAnimationTimer_;
        QTimer HideScrollbarTimer_;

        double ScrollDistance_;

        QPointF PrevTouchPoint_;

        std::deque<int32_t> WheelEventsBuffer_;

        QTimer WheelEventsBufferResetTimer_;

        std::unordered_set<QString> contacts_;

        int scrollValue_;

        qint64 messageId_;

        Logic::MessageKey lastSelectedMessageId_;

        QString aimid_;

        std::map<Logic::MessageKey, SelectedMessageInfo> selected_;

        struct PttProgress
        {
            qint64 id_;
            Utils::FileSharingId fsId_;
            int progress_;
            PttProgress(qint64 _id, const Utils::FileSharingId& _fsId, int _progress) : id_(_id), fsId_(_fsId), progress_(_progress) {};
        };

        std::vector<PttProgress> pttProgress_;

        States areaState_ = State::None;
    };
}

Q_DECLARE_OPERATORS_FOR_FLAGS(Ui::MessagesScrollArea::States)

