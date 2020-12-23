#pragma once

#include "types/reactions.h"

namespace Ui
{
    class HistoryControlPageItem;
    class ReactionsPlate_p;
    class AccessibleStatusesList;

    //////////////////////////////////////////////////////////////////////////
    // ReactionsPlate
    //////////////////////////////////////////////////////////////////////////

    class ReactionsPlate : public QWidget
    {
        friend class ReactionsPlate_p;
        friend class AccessibleReactionsPlate;

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
        void onReactionsPage(int64_t _seq, const Data::ReactionsPage& _page, bool _success);

    private:
        std::unique_ptr<ReactionsPlate_p> d;
    };

    class AccessibleReactionsPlate : public QAccessibleWidget
    {
    public:
        AccessibleReactionsPlate(ReactionsPlate* _plate) : QAccessibleWidget(_plate), plate_(_plate) {}

        int childCount() const override;
        QAccessibleInterface* child(int _index) const override;
        int indexOfChild(const QAccessibleInterface* _child) const override;
        QString	text(QAccessible::Text _type) const override;

    private:
        ReactionsPlate* plate_ = nullptr;
    };

}
