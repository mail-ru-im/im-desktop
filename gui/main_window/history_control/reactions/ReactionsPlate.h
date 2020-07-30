#pragma once

#include "types/reactions.h"

namespace Ui
{
    class HistoryControlPageItem;
    class ReactionsPlate_p;

    //////////////////////////////////////////////////////////////////////////
    // ReactionsPlate
    //////////////////////////////////////////////////////////////////////////

    class ReactionsPlate : public QWidget
    {
        Q_OBJECT
    public:
        ReactionsPlate(HistoryControlPageItem* _item);
        ~ReactionsPlate();

        void setReactions(const Data::Reactions& _reactions);
        void setOutgoingPosition(bool _outgoingPosition);

        void onMessageSizeChanged();

    Q_SIGNALS:
        void reactionClicked(const QString& _reaction);
        void platePositionChanged();

    protected:
        void paintEvent(QPaintEvent* _event) override;
        void showEvent(QShowEvent* _event) override;
        void mouseMoveEvent(QMouseEvent* _event) override;
        void mousePressEvent(QMouseEvent* _event) override;
        void mouseReleaseEvent(QMouseEvent* _event) override;
        void leaveEvent(QEvent* _event) override;

    private Q_SLOTS:
        void onMultiselectChanged();

    private:
        std::unique_ptr<ReactionsPlate_p> d;
    };

}
