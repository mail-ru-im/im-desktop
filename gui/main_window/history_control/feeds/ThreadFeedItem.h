#pragma once

#include "types/message.h"
#include "types/thread_feed_data.h"
#include "controls/ClickWidget.h"

namespace Utils
{
    struct FileSharingId;
}

namespace Ui
{
    class HistoryControlPageItem;
    class ThreadFeedItem_p;
    class InputWidget;
    enum class MessagesBuddiesOpt;

    //////////////////////////////////////////////////////////////////////////
    // ThreadFeedItem
    //////////////////////////////////////////////////////////////////////////

    class ThreadFeedItem : public QWidget
    {
        Q_OBJECT
    public:
        ThreadFeedItem(const Data::ThreadFeedItemData& _data, QWidget* _parent, QWidget* _parentForTooltips);
        ~ThreadFeedItem();

        bool highlightQuote(int64_t _msgId);
        bool containsMessage(int64_t _msgId);
        int messageY(int64_t _msgId);
        int64_t parentMsgId() const;
        const QString& parentChatId() const;
        const QString& threadId() const;
        int64_t lastMsgId() const;
        void updateVisibleRects();
        QRect inputGeometry() const;
        InputWidget* inputWidget() const;
        bool isInputActive() const;

        void positionMentionCompleter(const QPoint& _atPos);
        void setInputVisible(bool _visible);
        void updateInputVisibilityAccordingToYourRoleInThread();
        void clearSelection();
        void setIsFirstInFeed(bool _isFirstInFeed);

    public Q_SLOTS:
        void onNavigationKeyPressed(int _key, Qt::KeyboardModifiers _modifiers);
        void edit(const Data::MessageBuddySptr& _msg, MediaType _mediaType);

    Q_SIGNALS:
        void focusedOnInput();
        void sizeUpdated();
        void inputHeightChanged();
        void quote(const Data::QuotesVec&);
        void selected();

    protected:
        void resizeEvent(QResizeEvent* _event) override;
        void mousePressEvent(QMouseEvent* _event) override;
        void mouseMoveEvent(QMouseEvent* _event) override;
        void keyPressEvent(QKeyEvent* _event) override;
        void paintEvent(QPaintEvent* _event) override;

    private Q_SLOTS:
        void onLoadMore();
        void onMessageBuddies(const Data::MessageBuddies&_messages, const QString&_aimId, Ui::MessagesBuddiesOpt, bool _havePending, qint64 _seq, int64_t _lastMsgId);
        void onMessagesDeleted(const QString&, const Data::MessageBuddies&);
        void onMessageIdsFromServer(const QVector<qint64>& _ids, const QString& _aimId, qint64 _seq);
        void onHeaderClicked();
        void onGroupSubscribeResult(const int64_t _seq, int _error, int _resubscribeIn);
        void onSuggestShow(const QString& _text, const QPoint& _pos);
        void onSuggestHide();
        void onSuggestedStickerSelected(const Utils::FileSharingId& _stickerId);
        void onHideMentionCompleter();
        void selectionStateChanged(const QString& _aimid, qint64, const QString&, bool);

    private:
        void addMessages(const Data::MessageBuddies& _messages, bool _front = false);
        void readLastMessage(bool _force = false);

        std::unique_ptr<ThreadFeedItem_p> d;
    };

    class ThreadFeedItemHeader_p;

    //////////////////////////////////////////////////////////////////////////
    // ThreadFeedItemHeader
    //////////////////////////////////////////////////////////////////////////

    class ThreadFeedItemHeader : public ClickableWidget
    {
        Q_OBJECT
    public:
        ThreadFeedItemHeader(const QString& _chatId, QWidget* _parent);
        ~ThreadFeedItemHeader();

    protected:
        void paintEvent(QPaintEvent* _event);
        void resizeEvent(QResizeEvent* _event);

    private:
        std::unique_ptr<ThreadFeedItemHeader_p> d;
    };
}
