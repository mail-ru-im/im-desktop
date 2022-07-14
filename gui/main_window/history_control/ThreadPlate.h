#pragma once

#include "controls/ClickWidget.h"
#include "../../styles/StyleVariable.h"

namespace Data
{
    struct ThreadUpdate;
}

namespace TextRendering
{
    class TextUnit;
}

namespace Ui
{
    class HistoryControlPageItem;

    class ThreadPlate : public ClickableWidget
    {
        Q_OBJECT

    public:
        ThreadPlate(HistoryControlPageItem* _item);
        ThreadPlate(const QString& _caption, QWidget* _parent);
        void updateWith(const std::shared_ptr<Data::ThreadUpdate>& _update);
        void updatePosition();

        QSize sizeHint() const override;

        static ThreadPlate* plateForPopup(QWidget* _parent);

    protected:
        void paintEvent(QPaintEvent*) override;

    private:
        QColor getBackgroundColor() const;
        QColor getIconBgColor() const;
        Styling::StyleVariable getIconColorStyle() const;
        Styling::StyleVariable getTextColorStyle() const;
        bool isIconShadowNeeded() const;
        QString repliesCounterString() const;
        int repliesCounterWidth() const;

    private:
        QPointer<HistoryControlPageItem> item_;
        int repliesCount_ = 0;
        bool isOutgoing_ = false;
        bool hasMention_ = false;
        bool hasUnreads_ = false;
        bool isSubscriber_ = false;
    };
}
