#pragma once

#include "history/History.h"
#include "history/Message.h"


namespace hist
{
    class DateInserter;
}

namespace Ui
{
    class MessagesScrollbar;
    class MessageItem;
    class HistoryControlPageItem;
    class MessagesScrollAreaLayout;

    struct InsertHistMessagesParams;

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

        MessagesScrollArea(QWidget *parent, QWidget *typingWidget, hist::DateInserter* dateInserter);

        void cancelSelection();

        void cancelWheelBufferReset();

        void enumerateWidgets(const WidgetVisitor& visitor, const bool reversed) const;

        QWidget* getItemByPos(const int32_t pos) const;

        QWidget* getItemByKey(const Logic::MessageKey &key) const;

        QWidget* extractItemByKey(const Logic::MessageKey& _key);

        QWidget* getPrevItem(QWidget* _w) const;

        int32_t getItemsCount() const;

        Logic::MessageKeyVector getItemsKeys() const;

        QString getSelectedText() const;

        Data::QuotesVec getQuotes() const;

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

        bool touchScrollInProgress() const;
        bool needFetchMoreToTop() const;
        bool needFetchMoreToBottom() const;

        void scrollToBottom();

        void updateItemKey(const Logic::MessageKey &key);
        void scrollTo(const Logic::MessageKey &key, hist::scroll_mode_type _scrollMode);
        void scrollContent(const int _dy);

        void updateScrollbar();

        bool isScrollAtBottom() const;

        void clearSelection();

        void pressedDestroyed();

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

    public slots:
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

    public Q_SLOTS:
        void onWheelEvent(QWheelEvent* e);

    private:
        bool isHidingScrollbar() const;
        void hideScrollbar();
        void setScrollbarVisible(bool _visible);

        int getSelectedCount() const;

        void onSelectionChanged(int _selectedCount);

    private:
        enum class ScrollingMode;

        bool IsSelecting_;

        bool TouchScrollInProgress_;

        int64_t LastAnimationMoment_;

        QPoint LastMouseGlobalPos_;

        QPoint SelectionBeginAbsPos_;

        QPoint SelectionEndAbsPos_;

        ScrollingMode Mode_;

        bool ScrollShowOnOpen_ = false;
        MessagesScrollbar *Scrollbar_;

        MessagesScrollAreaLayout *Layout_;

        QTimer ScrollAnimationTimer_;
        QTimer HideScrollbarTimer_;

        double ScrollDistance_;

        QPointF PrevTouchPoint_;

        bool IsUserActive_;

        std::deque<int32_t> WheelEventsBuffer_;

        QTimer WheelEventsBufferResetTimer_;

        void updateMode();

        void applySelection(const bool forShift = false);

        bool enqueWheelEvent(const int32_t delta);

        double evaluateScrollingVelocity() const;

        double evaluateScrollingStep(const int64_t now) const;

        int32_t evaluateWheelHistorySign() const;

        void scheduleWheelBufferReset();

        void startScrollAnimation(const ScrollingMode mode);

        void stopScrollAnimation();

        void eraseContact(QWidget* widget);

        int minViewPortHeightForUnload() const;

        std::set<QString> contacts_;

        bool IsSearching_;

        int scrollValue_;

        qint64 messageId_;
        bool recvLastMessage_;
    };

}
