#pragma once

namespace Ui
{

class HistoryControlPageItem;
class ReactionButton_p;
class ReactionButtonPrivate;

//////////////////////////////////////////////////////////////////////////
// ReactionsPlate
//////////////////////////////////////////////////////////////////////////

class AddReactionButton : public QWidget
{
    Q_OBJECT

public:
    AddReactionButton(HistoryControlPageItem* _item);
    ~AddReactionButton();

    void setOutgoingPosition(bool _outgoingPosition);

    void onMouseLeave();
    void onMouseMove(const QPoint& _pos, const QRegion& _hoverRegion = QRegion());
    void onMousePress(const QPoint& _pos);
    void onMouseRelease(const QPoint& _pos);
    void onMessageSizeChanged();

Q_SIGNALS:
    void update();
    void clicked();

protected:
    void mouseMoveEvent(QMouseEvent* _event) override;
    void mousePressEvent(QMouseEvent* _event) override;
    void mouseReleaseEvent(QMouseEvent* _event) override;
    void paintEvent(QPaintEvent* _event) override;
    void showEvent(QShowEvent* _event) override;
    void updateCursorShape(const QPoint& _pos);

public Q_SLOTS:
    void onAddReactionPlateShown();
    void onAddReactionPlateClosed();
    void onPlatePositionChanged();

private Q_SLOTS:
    void onMultiselectChanged();
    void onHoverTimer();

private:
    std::unique_ptr<ReactionButton_p> d;
};

}
