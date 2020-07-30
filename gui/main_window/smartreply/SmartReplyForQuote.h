#pragma once

namespace Data
{
    class SmartreplySuggest;
}

namespace Ui
{
    class SmartReplyWidget;
    class TooltipWidget;

    class SmartReplyForQuote : public QWidget
    {
        Q_OBJECT

    Q_SIGNALS:
        void sendSmartreply(const Data::SmartreplySuggest& _suggest, QPrivateSignal) const;

    public:
        SmartReplyForQuote(QWidget* _parent);

        void clear();
        void setSmartReplies(const std::vector<Data::SmartreplySuggest>& _suggests);

        void showAnimated(const QPoint& _p, const QSize& _maxSize, const QRect& _rect);
        void hideAnimated();
        void hideForce();

        bool isTooltipVisible() const;
        void adjustTooltip(const QPoint& _p, const QSize& _maxSize, const QRect& _rect);

    private:
        SmartReplyWidget* widget_;
        TooltipWidget* tooltip_;
    };
}