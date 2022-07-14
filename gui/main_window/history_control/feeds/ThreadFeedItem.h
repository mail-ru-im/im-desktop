#pragma once

#include "types/message.h"
#include "types/thread_feed_data.h"

namespace Utils
{
    struct FileSharingId;
}

namespace hist
{
    class DateInserter;
}

namespace Ui
{
    class HistoryControlPageItem;
    class ThreadFeedItem_p;
    class InputWidget;
    enum class MessagesBuddiesOpt;
    class ThreadRepliesItem;

    class ThreadFeedItem : public QWidget
    {
    Q_OBJECT
    public:
        ThreadFeedItem(const Data::ThreadFeedItemData& _data, QWidget* _parent, QWidget* _parentForTooltips);
        ~ThreadFeedItem();

        bool containsMessage(const Logic::MessageKey& _key);
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

    public Q_SLOTS:
        void onNavigationKeyPressed(int _key, Qt::KeyboardModifiers _modifiers);
        void edit(const Data::MessageBuddySptr& _msg, MediaType _mediaType);
        void copy();
        void onForward(const Data::QuotesVec& _q);
        void addToFavorites(const Data::QuotesVec& _q);

    Q_SIGNALS:
        void focusedOnInput();
        void sizeUpdated();
        void inputHeightChanged();
        void quote(const Data::QuotesVec&);
        void editing();
        void selected();
        void activated();

        void createTask(const Data::FString& _text, const Data::MentionMap& _mentions, const QString& _assignee, const bool isThreadFeedMessage);

    protected:
        void resizeEvent(QResizeEvent* _event) override;
        void mousePressEvent(QMouseEvent* _event) override;
        void mouseReleaseEvent(QMouseEvent* _event) override;
        void mouseMoveEvent(QMouseEvent* _event) override;
        void keyPressEvent(QKeyEvent* _event) override;
        void paintEvent(QPaintEvent* _event) override;
        void dragEnterEvent(QDragEnterEvent* _event) override;
        void dragLeaveEvent(QDragLeaveEvent* _event) override;
        void dropEvent(QDropEvent* _event) override;

    private Q_SLOTS:
        void onLoadMore();
        void onMessageBuddies(const Data::MessageBuddies&_messages, const QString&_aimId, Ui::MessagesBuddiesOpt, bool _havePending, qint64 _seq, int64_t _lastMsgId);
        void onMessagesDeleted(const QString&, const Data::MessageBuddies&);
        void onRemovePending(const QString& _aimId, const Logic::MessageKey& _key);
        void onMessageIdsFromServer(const QVector<qint64>& _ids, const QString& _aimId, qint64 _seq);
        void onHeaderClicked();
        void onGroupSubscribeResult(const int64_t _seq, int _error, int _resubscribeIn);
        void onSuggestShow(const QString& _text, const QPoint& _pos);
        void onSuggestHide();
        void onSuggestedStickerSelected(const Utils::FileSharingId& _stickerId);
        void onHideMentionCompleter();
        void selectionStateChanged(const QString& _aimid, qint64, const QString&, bool);
        void onReactions(const QString& _contact, const std::vector<Data::Reactions> _reactionsData);

    private:
        void addMessages(const Data::MessageBuddies& _messages, bool _front = false);
        void readLastMessage(bool _force = false);
        void showStickersSuggest(const QString& _text, const QPoint& _pos);

        std::unique_ptr<ThreadFeedItem_p> d;

        struct MessagesBuddiesCache
        {
            Data::MessageBuddies _messages;
            QString _aimId;
            MessagesBuddiesOpt _type;
            bool _havePending;
            qint64 _seq;
            int64_t _lastMsgId;
        } _messageBuddiesCache;
    };
}
