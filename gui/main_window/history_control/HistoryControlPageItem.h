#pragma once

#include "QuoteColorAnimation.h"
#include "../../types/chat.h"
#include "../../types/chatheads.h"
#include "types/reactions.h"
#include "history/Message.h"
#include "reactions/MessageReactions.h"

namespace Ui
{
    class LastStatusAnimation;
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

    public:
        explicit HistoryControlPageItem(QWidget *parent);

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

        virtual void setSender(const QString& _sender);
        virtual void updateFriendly(const QString& _aimId, const QString& _friendly);

        virtual void setDeliveredToServer(const bool _delivered);

        virtual qint64 getId() const;
        virtual qint64 getPrevId() const;
        virtual const QString& getInternalId() const;
        virtual QString getSourceText() const;
        QString idImpl() const { return QString::number(getId()); }

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

        virtual void callEditing() {}

        virtual void setNextHasSenderName(bool _nextHasSenderName);

        void setBuddy(const Data::MessageBuddy& _msg);
        const Data::MessageBuddy& buddy() const;
        Data::MessageBuddy& buddy();

        bool nextHasSenderName() const;

        virtual void setNextIsOutgoing(bool _nextIsOutgoing);
        virtual bool nextIsOutgoing() const;

        virtual QRect messageRect() const;

        void setReactions(const Data::Reactions& _reactions);

        void onSizeChanged();

        virtual ReactionsPlateType reactionsPlateType() const;
        bool hasReactions() const;
        QRect reactionsPlateRect() const;

        virtual void setSpellErrorsVisible(bool _visible) {}
        virtual QRect avatarRect() const { return QRect(); }

    protected:

        virtual void drawLastStatusIcon(QPainter& _p, LastStatus _lastStatus, const QString& _aimid, const QString& _friendly, int _rightPadding);

        void mouseMoveEvent(QMouseEvent*) override;
        void mousePressEvent(QMouseEvent*) override;
        void mouseReleaseEvent(QMouseEvent*) override;
        void enterEvent(QEvent*) override;
        void leaveEvent(QEvent*) override;

        void showMessageStatus();
        void hideMessageStatus();

        virtual void drawSelection(QPainter& _p, const QRect& _rect);
        virtual void drawHeads(QPainter& _p) const;

        virtual bool showHeadsTooltip(const QRect& _rc, const QPoint& _pos);
        virtual bool clickHeads(const QRect& _rc, const QPoint& _pos, bool _left);

        virtual void onChainsChanged() {}

        virtual bool handleSelectByPos(const QPoint& from, const QPoint& to, const QPoint& areaFrom, const QPoint& areaTo);

        virtual void initialize();

        virtual bool supportsReactions() const { return true; }


        QuoteColorAnimation QuoteAnimation_;
        bool isChat_;

    private Q_SLOTS:
        void avatarChanged(const QString& _aimid);
        void onReactionsEnabledChanged();

    private:
        void drawLastStatusIconImpl(QPainter& _p, int _rightPadding, int _bottomPadding);
        int maxHeadsCount() const;
        void ensureAddAnimationInitialized();
        void ensureRemoveAnimationInitialized();
        void ensureHeightAnimationInitialized();

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
