#pragma once

#include "QuoteColorAnimation.h"
#include "../../types/chat.h"
#include "../../types/chatheads.h"
#include "../../types/filesharing_meta.h"
#include "../../types/thread.h"
#include "types/reactions.h"
#include "history/Message.h"
#include "reactions/MessageReactions.h"
#include "complex_message/FileSharingUtils.h"

namespace Ui
{
    class ThreadPlate;
    class LastStatusAnimation;
    class CornerMenu;
    enum class MenuButtonType;
    enum class MediaType;

    using HistoryControlPageItemUptr = std::unique_ptr<class HistoryControlPageItem>;
    using highlightsV = std::vector<QString>;

    enum class LastStatus {
        None,
        Pending,
        DeliveredToServer,
        DeliveredToPeer,
        Read
    };

    enum class MediaRequestMode
    {
        Chat,
        Recents
    };

    class HistoryControlPageItem : public QWidget
    {
        Q_OBJECT

    Q_SIGNALS:
        void mention(const QString&, const QString&) const;
        void selectionChanged() const;
        void edit(const Data::MessageBuddySptr& _msg, MediaType _mediaType);
        void quote(const Data::QuotesVec&);

    public:
        explicit HistoryControlPageItem(QWidget *parent);
        ~HistoryControlPageItem();

        virtual bool isOutgoing() const = 0;
        bool isOutgoingPosition() const;
        virtual QString formatRecentsText() const = 0;
        virtual MediaType getMediaType(MediaRequestMode _mode  = MediaRequestMode::Chat) const = 0;
        virtual void setQuoteSelection() = 0;
        virtual void updateStyle() = 0;
        virtual void updateFonts() = 0;
        virtual int32_t getTime() const = 0;

        bool hasAvatar() const;
        bool hasSenderName() const;
        bool hasTopMargin() const;

        virtual bool needsAvatar() const { return false; }

        virtual void clearSelection(bool _keepSingleSelection);
        virtual void selectByPos(const QPoint& from, const QPoint& to, const QPoint& areaFrom, const QPoint& areaTo);
        virtual void setSelected(const bool _isSelected);
        virtual bool isSelected() const;

        virtual void onActivityChanged(const bool isActive);
        virtual void onVisibleRectChanged(const QRect& _visibleRect);
        virtual void onDistanceToViewportChanged(const QRect& _widgetAbsGeometry, const QRect& _viewportVisibilityAbsRect);

        virtual void setHasAvatar(const bool value);
        virtual void setHasSenderName(const bool _hasSender);

        virtual void setChainedToPrev(const bool _isChained);
        virtual void setChainedToNext(const bool _isChained);
        virtual void setPrev(const Logic::MessageKey& _key);
        virtual void setNext(const Logic::MessageKey& _next);

        virtual void setTopMargin(const bool value);

        virtual void setContact(const QString& _aimId);
        const QString& getContact() const { return aimId_; }
        const QString& getPageId() const;

        virtual void setSender(const QString& _sender);
        virtual void updateFriendly(const QString& _aimId, const QString& _friendly);

        virtual void setDeliveredToServer(const bool _delivered);

        virtual qint64 getId() const;
        virtual qint64 getPrevId() const;
        virtual const QString& getInternalId() const;
        virtual QString getSourceText() const;

        void setDeleted(const bool _isDeleted);
        bool isDeleted() const;

        virtual void setEdited(const bool _isEdited);
        virtual bool isEdited() const;

        virtual void setLastStatus(LastStatus _lastStatus);
        virtual LastStatus getLastStatus() const;

        virtual void highlightText(const highlightsV& _highlights) {}
        virtual void resetHighlight() {}

        virtual int bottomOffset() const;

        virtual void setHeads(const Data::HeadsVector& _heads);
        virtual void addHeads(const Data::HeadsVector& _heads);
        virtual void removeHeads(const Data::HeadsVector& _heads);
        virtual bool areTheSameHeads(const QVector<Data::ChatHead>& _heads) const;

        virtual void initSize();
        virtual void updateSize();

        virtual bool hasPictureContent() const { return false; };

        virtual void startSpellChecking();

        void setIsChat(bool _isChat);
        bool isChat() const { return isChat_; }

        bool hasHeads() const noexcept;
        bool headsAtBottom() const;

        virtual void cancelRequests() {}

        static QMap<QString, QVariant> makeData(const QString& _command, const QString& _arg = QString());

        bool isChainedToPrevMessage() const;
        bool isChainedToNextMessage() const;

        void setContextMenuEnabled(const bool _isEnabled) { isContextMenuEnabled_ = _isEnabled; }
        void setMultiselectEnabled(const bool _isEnabled) { isMultiselectEnabled_ = _isEnabled; }
        bool isContextMenuEnabled() const noexcept { return  isContextMenuEnabled_; }
        bool isMultiselectEnabled() const noexcept { return  isMultiselectEnabled_; }
        bool isContextMenuReplyOnly() const noexcept;

        void setSelectionCenter(int _center);

        void setIsUnsupported(bool _unsupported);
        bool isUnsupported() const;

        virtual bool isEditable() const;
        virtual bool isUpdateable() const { return true; }
        virtual bool isSingleMedia() const { return false; }

        virtual void callEditing() {}

        virtual void setNextHasSenderName(bool _nextHasSenderName);

        void setBuddy(const Data::MessageBuddy& _msg);
        const Data::MessageBuddy& buddy() const;
        Data::MessageBuddy& buddy();

        bool nextHasSenderName() const;

        virtual void setNextIsOutgoing(bool _nextIsOutgoing);
        virtual bool nextIsOutgoing() const;

        virtual QRect messageRect() const;
        virtual QRect cornerMenuContentRect() const;

        void setReactions(const Data::Reactions& _reactions);

        void onSizeChanged();

        virtual ReactionsPlateType reactionsPlateType() const;
        bool hasReactions() const;
        QRect reactionsPlateRect() const;
        QRect threadPlateRect() const;

        virtual void setSpellErrorsVisible(bool _visible) {}
        virtual QRect avatarRect() const { return QRect(); }

        virtual std::optional<Data::FileSharingMeta> getMeta(const Utils::FileSharingId& _id) const { return std::nullopt; }

        void applyThreadUpdate(const std::shared_ptr<Data::ThreadUpdate>& _update);

        bool hasThread() const;
        bool hasThreadWithReplies() const;

        const QString& getThreadId() const;
        void setThreadId(const QString& _threadId);

        bool isHeadless() const;

        bool isThreadFeedMessage() const;

    protected:

        virtual void drawLastStatusIcon(QPainter& _p, LastStatus _lastStatus, const QString& _aimid, const QString& _friendly, int _rightPadding);

        void mouseMoveEvent(QMouseEvent*) override;
        void mousePressEvent(QMouseEvent*) override;
        void mouseReleaseEvent(QMouseEvent*) override;
        void enterEvent(QEvent*) override;
        void leaveEvent(QEvent*) override;
        void wheelEvent(QWheelEvent*) override;
        void resizeEvent(QResizeEvent* _event) override;

        void moveEvent(QMoveEvent* _event) override;

        void showMessageStatus();
        void hideMessageStatus();

        void hideCornerMenuForce();
        void setCornerMenuVisible(bool _visible);
        void checkCornerMenuNeeded(const QPoint& _pos); // accepts global coord

        virtual void drawSelection(QPainter& _p, const QRect& _rect);
        virtual void drawHeads(QPainter& _p) const;

        virtual bool showHeadsTooltip(const QRect& _rc, const QPoint& _pos);
        virtual bool clickHeads(const QRect& _rc, const QPoint& _pos, bool _left);

        virtual void onChainsChanged() {}

        virtual bool handleSelectByPos(const QPoint& from, const QPoint& to, const QPoint& areaFrom, const QPoint& areaTo);

        virtual void initialize();

        virtual bool canBeThreadParent() const { return true; }
        virtual bool supportsOverlays() const { return true; }

        int64_t getThreadMsgId() const;

        void copyPlates(const HistoryControlPageItem* _other);
        std::shared_ptr<Data::ThreadUpdate> getthreadUpdate() const;

        QPointer<QuoteColorAnimation> QuoteAnimation_;
        bool isChat_;

    private Q_SLOTS:
        void avatarChanged(const QString& _aimid);
        void onConfigChanged();
        void openThread() const;

    private:
        void drawLastStatusIconImpl(QPainter& _p, int _rightPadding, int _bottomPadding);
        int maxHeadsCount() const;
        void ensureAddAnimationInitialized();
        void ensureRemoveAnimationInitialized();
        void ensureHeightAnimationInitialized();

        void updateThreadSubscription();
        void onThreadUpdates(const Data::ThreadUpdates& _updates);

        void initializeReactionsPlate();
        void initializeCornerMenu();

        bool hasReactionButton() const;
        bool hasThreadButton() const;

        QRect getCornerMenuRect() const;
        bool isInCornerMenuHoverArea(const QPoint& _pos) const;
        std::vector<MenuButtonType> getCornerMenuButtons() const;
        void positionCornerMenu();

        void onCornerMenuClicked(MenuButtonType _type);

        bool Selected_;
        bool HasTopMargin_;
        bool HasAvatar_;
        bool hasSenderName_;
        bool isChainedToPrev_;
        bool isChainedToNext_;
        bool HasAvatarSet_;
        bool isDeleted_;
        bool isEdited_;
        bool isContextMenuEnabled_;
        bool isMultiselectEnabled_;
        bool selectedTop_;
        bool selectedBottom_;
        bool hoveredTop_;
        bool hoveredBottom_;
        int selectionCenter_;

        LastStatus lastStatus_;
        QString aimId_;

        QPointer<LastStatusAnimation> lastStatusAnimation_;

        Data::HeadsVector heads_;
        std::unique_ptr<MessageReactions> reactions_;
        QPointer<CornerMenu> cornerMenu_;
        QTimer* cornerMenuHoverTimer_ = nullptr;

        QVariantAnimation* addAnimation_;
        QVariantAnimation* removeAnimation_;
        QVariantAnimation* heightAnimation_;

        QPoint pressPoint;
        QRect selectionMarkRect_;

        Logic::MessageKey prev_;
        Logic::MessageKey next_;

        QPoint prevTo_;
        QPoint prevFrom_;
        bool intersected_;
        bool wasSelected_;
        bool isUnsupported_;

        bool nextHasSenderName_;
        Data::MessageBuddy msg_;
        bool nextIsOutgoing_;
        bool initialized_;

        std::shared_ptr<Data::ThreadUpdate> threadUpdateData_;
        ThreadPlate* threadPlate_ = nullptr;
    };

    class AccessibleHistoryControlPageItem : public QAccessibleWidget
    {
    public:
        AccessibleHistoryControlPageItem(HistoryControlPageItem* _item) : QAccessibleWidget(_item), item_(_item) {}

        QString	text(QAccessible::Text _type) const override;

    private:
        HistoryControlPageItem* item_ = nullptr;
    };

}
