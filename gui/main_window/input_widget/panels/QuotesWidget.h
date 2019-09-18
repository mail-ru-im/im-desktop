#pragma once

#include "../InputWidgetUtils.h"
#include "controls/ClickWidget.h"
#include "controls/TextUnit.h"
#include "types/message.h"

namespace Ui
{
    class CustomButton;
    class ScrollAreaWithTrScrollBar;

    class QuoteRow
    {
    public:
        QuoteRow(const Data::Quote& _quote);

        void draw(QPainter& _painter, const QRect& _rect) const;
        void elide(const int _width);
        void updateAvatar();

        const QString& getAimId() const { return aimId_; }
        qint64 getMsgId() const noexcept { return msgId_; }

        void setUnderMouse(const bool _value) { underMouse_ = _value; }
        bool isUnderMouse() const noexcept { return underMouse_; }

    private:
        QString getName() const;

    private:
        bool underMouse_ = false;
        qint64 msgId_ = -1;

        QString aimId_;
        QPixmap avatar_;
        TextRendering::TextUnitPtr textUnit_;
    };

    class QuotesContainer : public ClickableWidget
    {
        Q_OBJECT

    Q_SIGNALS:
        void rowClicked(const int _idx, QPrivateSignal) const;

    public:
        QuotesContainer(QWidget* _parent);
        bool setQuotes(const Data::QuotesVec& _quotes);
        void adjustHeight();

        int rowCount() const;
        QRect getRowRect(const int _idx) const;
        void updateRowHover();
        void clear();

    protected:
        void paintEvent(QPaintEvent* _e) override;
        void resizeEvent(QResizeEvent*) override;
        void mouseMoveEvent(QMouseEvent* _e) override;
        void wheelEvent(QWheelEvent* _e) override;

    private:
        void onClicked();
        void onHoverChanged(const bool _isHovered);
        void onAvatarChanged(const QString& _aimid);
        void addQuote(const Data::Quote& _quote);
        int rowWidth() const;
        void hoverRowUnderMouse(const QPoint& _pos);

    private:
        std::vector<QuoteRow> rows_;
    };


    class QuotesWidget : public QWidget, public StyledInputElement
    {
        Q_OBJECT

    Q_SIGNALS:
        void cancelClicked(QPrivateSignal) const;
        void needScrollToLastQuote(QPrivateSignal) const;
        void quoteCountChanged(QPrivateSignal) const;

    public:
        QuotesWidget(QWidget* _parent);

        void setQuotes(const Data::QuotesVec& _quotes);
        void clearQuotes();
        bool isMaxHeight() const;

    protected:
        void updateStyleImpl(const InputStyleMode _mode) override;

    private:
        void adjustHeight();
        void scrollToItem(const int _idx);
        void scrollToLastQuote();

    private:
        ScrollAreaWithTrScrollBar* scrollArea_;
        QuotesContainer* container_;
        CustomButton* buttonCancel_;
    };
}
