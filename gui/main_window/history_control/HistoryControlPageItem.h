#pragma once

#include "QuoteColorAnimation.h"
#include "../../types/chat.h"
#include "../../types/chatheads.h"
#include "../../animation/animation.h"
#include "history/Message.h"

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

        virtual void clearSelection(bool _keepSingleSelection);

        virtual bool isOutgoing() const = 0;

        virtual bool nextIsOutgoing() const = 0;

        virtual QString formatRecentsText() const = 0;

        virtual MediaType getMediaType(MediaRequestMode _mode  = MediaRequestMode::Chat) const = 0;

        bool hasAvatar() const;

        bool hasSenderName() const;

        bool hasTopMargin() const;

        virtual void selectByPos(const QPoint& from, const QPoint& to, const QPoint& areaFrom, const QPoint& areaTo);

        virtual bool isSelected() const;

        virtual void onActivityChanged(const bool isActive);

        virtual void onVisibilityChanged(const bool isVisible);

        virtual void onDistanceToViewportChanged(const QRect& _widgetAbsGeometry, const QRect& _viewportVisibilityAbsRect);

        virtual void setHasAvatar(const bool value);

        virtual void setHasSenderName(const bool _hasSender);

        virtual void setChainedToPrev(const bool _isChained);
        virtual void setChainedToNext(const bool _isChained);
        virtual void setPrev(const Logic::MessageKey& _key);
        virtual void setNext(const Logic::MessageKey& _next);

        virtual void setSelected(const bool _isSelected);

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

        virtual void setQuoteSelection() = 0;
        virtual void highlightText(const highlightsV& _highlights) {}
        virtual void resetHighlight() {}

        virtual int bottomOffset() const;

        virtual void setHeads(const Data::HeadsVector& _heads);

        virtual void addHeads(const Data::HeadsVector& _heads);

        virtual void removeHeads(const Data::HeadsVector& _heads);

        virtual bool areTheSameHeads(const QVector<Data::ChatHead>& _heads) const;

        virtual void updateSize();

        virtual bool hasPictureContent() const { return false; };

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

        virtual void updateStyle() = 0;
        virtual void updateFonts() = 0;

        void setSelectionCenter(int _center);

    protected:

        virtual void drawLastStatusIcon(QPainter& _p, LastStatus _lastStatus, const QString& _aimid, const QString& _friendly, int _rightPadding);

        virtual void mouseMoveEvent(QMouseEvent*) override;

        virtual void mousePressEvent(QMouseEvent*) override;

        virtual void mouseReleaseEvent(QMouseEvent*) override;

        virtual void enterEvent(QEvent*) override;

        void showMessageStatus();
        void hideMessageStatus();

        virtual void drawSelection(QPainter& _p, const QRect& _rect);

        virtual void drawHeads(QPainter& _p) const;

        virtual bool showHeadsTooltip(const QRect& _rc, const QPoint& _pos);

        virtual bool clickHeads(const QRect& _rc, const QPoint& _pos, bool _left);

        virtual void onChainsChanged() {}

        virtual bool handleSelectByPos(const QPoint& from, const QPoint& to, const QPoint& areaFrom, const QPoint& areaTo);

    private:
        void drawLastStatusIconImpl(QPainter& _p, int _rightPadding, int _bottomPadding);
        int maxHeadsCount() const;

    private Q_SLOTS:
        void avatarChanged(const QString& _aimid);

    private:
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

        anim::Animation addAnimation_;
        anim::Animation removeAnimation_;
        anim::Animation heightAnimation_;

        QPoint pressPoint;
        QRect selectionMarkRect_;

        Logic::MessageKey prev_;
        Logic::MessageKey next_;

        QPoint prevTo_;
        QPoint prevFrom_;
        bool intersected_;
        bool wasSelected_;

    protected:
        QuoteColorAnimation QuoteAnimation_;
        bool isChat_;
    };
}
