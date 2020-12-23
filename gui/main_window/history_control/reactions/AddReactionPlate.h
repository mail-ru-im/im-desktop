#pragma once

#include "cache/emoji/EmojiCode.h"
#include "types/reactions.h"

namespace Ui
{

    class AddReactionPlate_p;

    //////////////////////////////////////////////////////////////////////////
    // AddReactionPlate
    //////////////////////////////////////////////////////////////////////////

    class AddReactionPlate : public QWidget
    {
        Q_OBJECT

        friend class AddReactionPlate_p;

    public:
        AddReactionPlate(const Data::Reactions& _messageReactions);
        ~AddReactionPlate();

        void setOutgoingPosition(bool _outgoingPosition);
        void setMsgId(int64_t _msgId);
        void setChatId(const QString& _chatId);
        void showOverButton(const QPoint& _buttonTopCenterGlobal);

    protected:
        void mousePressEvent(QMouseEvent* _event) override;
        void mouseReleaseEvent(QMouseEvent* _event) override;
        void keyPressEvent(QKeyEvent* _event) override;
        void wheelEvent(QWheelEvent* _event) override;
        void leaveEvent(QEvent* _event) override;
        void enterEvent(QEvent* _event) override;

    private Q_SLOTS:
        void onLeaveTimer();
        void onAnimationTimer();
        void onItemClicked(const QString& _reaction);
        void onItemHovered(bool _hovered);
        void onMultiselectChanged();

    Q_SIGNALS:
        void itemHovered(bool _hovered);
        void plateShown();
        void plateCloseStarted();
        void plateCloseFinished();
        void addReactionClicked(const QString& _reaction);
        void removeReactionClicked();

    private:
        std::unique_ptr<AddReactionPlate_p> d;
    };

    class EmojiWidget_p;

    //////////////////////////////////////////////////////////////////////////
    // EmojiWidget
    //////////////////////////////////////////////////////////////////////////

    class ReactionEmojiWidget : public QWidget
    {
        Q_OBJECT

    public:
        ReactionEmojiWidget(const QString& _reaction, const QString& _tooltip, QWidget* _parent);
        ~ReactionEmojiWidget();

        void setMyReaction(bool _myReaction);
        void setStartGeometry(const QRect& _rect);

        void showAnimated();
        void hideAnimated();

        QString reaction() const;

    Q_SIGNALS:
        void clicked(const QString& _reaction);
        void hovered(bool _hovered);

    public Q_SLOTS:
        void onOtherItemHovered(bool _hovered);

    private Q_SLOTS:
        void onTooltipTimer();

    protected:
        void mouseMoveEvent(QMouseEvent* _event) override;
        void mousePressEvent(QMouseEvent* _event) override;
        void mouseReleaseEvent(QMouseEvent* _event) override;
        void leaveEvent(QEvent* _event) override;
        void enterEvent(QEvent* _event) override;
        void paintEvent(QPaintEvent* _event) override;
    private:
        std::unique_ptr<EmojiWidget_p> d;
    };

    class PlateWithShadow_p;

    //////////////////////////////////////////////////////////////////////////
    // PlateWithShadow
    //////////////////////////////////////////////////////////////////////////

    class PlateWithShadow : public QWidget
    {
        Q_OBJECT

    public:
        PlateWithShadow(QWidget* _parent);
        ~PlateWithShadow();

        void showAnimated();
        void hideAnimated();

    Q_SIGNALS:
        void plateShown();
        void plateClosed();
        void hideFinished();

    protected:
        void paintEvent(QPaintEvent* _event);

    private:
        std::unique_ptr<PlateWithShadow_p> d;
    };

}
