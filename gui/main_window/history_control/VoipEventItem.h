#pragma once

#include "MessageItemBase.h"
#include "../../controls/TextUnit.h"
#include "../../controls/ClickWidget.h"

namespace HistoryControl
{
    using VoipEventInfoSptr = std::shared_ptr<class VoipEventInfo>;
}

namespace Ui
{
    class CallButton : public ClickableTextWidget
    {
        Q_OBJECT

    public:
        CallButton(QWidget* _parent, const QString& _aimId, const bool _isOutgoing);
        void setSelected(const bool _isSelected);

        bool isPressed() const;

    protected:
        void paintEvent(QPaintEvent* _e) override;
        void mouseMoveEvent(QMouseEvent* _e) override;
        void mousePressEvent(QMouseEvent* _e) override;
        void mouseReleaseEvent(QMouseEvent* _e) override;

    private:
        bool isSelected_;
        bool isPressed_;
        bool isOutgoing_;
        QString aimId_;
    };

    class MessageTimeWidget;
    enum class MediaType;

    class VoipEventItem : public MessageItemBase
    {
        Q_OBJECT

    Q_SIGNALS:
        void quote(const Data::QuotesVec&);
        void copy(const QString&);
        void forward(const Data::QuotesVec&);

    private Q_SLOTS:
        void onAvatarChanged(const QString& _aimId);
        void onContactNameChanged(const QString& _aimId, const QString& _friendlyName);
        void onCallButtonClicked();
        void menu(QAction*);

    public:
        VoipEventItem(QWidget *parent, const ::HistoryControl::VoipEventInfoSptr& eventInfo);
        ~VoipEventItem();

        QString formatRecentsText() const override;
        QString formatCopyText() const;
        MediaType getMediaType() const override;
        Data::Quote getQuote() const;

        void setTopMargin(const bool value) override;
        void setHasAvatar(const bool value) override;

        void setLastStatus(LastStatus _lastStatus) override;

        bool isOutgoing() const override;
        int32_t getTime() const override;

        qint64 getId() const override;
        void setId(const qint64 _id, const QString& _internalId);

        void selectByPos(const QPoint& _from, const QPoint& _to);
        void clearSelection() override;

        void updateStyle() override;
        void updateFonts() override;

        void updateWith(VoipEventItem& _messageItem);

        void setQuoteSelection() override;
        void updateSize() override;

    protected:
        void mouseMoveEvent(QMouseEvent *) override;
        void mousePressEvent(QMouseEvent *) override;
        void mouseReleaseEvent(QMouseEvent *) override;
        void mouseDoubleClickEvent(QMouseEvent*) override;
        void leaveEvent(QEvent *) override;
        void paintEvent(QPaintEvent *) override;
        void resizeEvent(QResizeEvent *event) override;

        void onChainsChanged() override;

    private:
        void init();

        QRect getAvatarRect() const;
        bool isAvatarHovered(const QPoint &mousePos) const;

        enum class UpdateHeightOption
        {
            forceRecalcTextHeight,
            usual,
        };

        void updateHeight(const UpdateHeightOption _option = UpdateHeightOption::usual);
        void trackMenu(const QPoint& _pos);
        QColor getTextColor(const bool isHovered);

        void updateText();
        void updateTimePosition();
        void updateCallButton();
        void updateColors();

        int getTextWidth() const;
        int getTextTopMargin() const;
        int getTextBottomMargin() const;

        QBrush drawBubble(QPainter& _p);
        void drawConfMembers(QPainter& _p, const QBrush& _bodyBrush);

        void resetBubblePath();

        QPixmap avatar_;
        QPainterPath Bubble_;
        QRect BubbleRect_;

        ::HistoryControl::VoipEventInfoSptr EventInfo_;

        bool isAvatarHovered_;
        QPoint pressPoint_;

        qint64 id_;
        QString internalId_;
        QString friendlyName_;

        enum class SelectDirection
        {
            NONE = 0,
            DOWN,
            UP,
        };
        SelectDirection Direction_;
        int startSelectY_;
        bool isSelection_;

        std::map<QString, QPixmap> confMembers_;

        TextRendering::TextUnitPtr text_;
        TextRendering::TextUnitPtr duration_;
        CallButton* callButton_;
        MessageTimeWidget *timeWidget_;
    };

}