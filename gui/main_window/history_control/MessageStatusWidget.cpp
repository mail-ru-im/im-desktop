#include "stdafx.h"
#include "MessageStatusWidget.h"
#include "MessageStyle.h"
#include "HistoryControlPageItem.h"

#include "../../styles/ThemeParameters.h"
#include "../../utils/utils.h"

#include "../../fonts.h"

namespace
{
    int getHeight(const bool _withUnderlay)
    {
        return _withUnderlay ? Utils::scale_value(20) : Utils::scale_value(14);
    }

    int getHorPadding()
    {
        return Utils::scale_value(8);
    }

    QFont getTimeFont()
    {
        return Fonts::adjustedAppFont(10, Fonts::FontWeight::Normal);
    }

    QFont getTimeFontLarge()
    {
        return Fonts::adjustedAppFont(11, Fonts::FontWeight::Normal);
    }
}

namespace Ui
{
    MessageTimeWidget::MessageTimeWidget(HistoryControlPageItem *messageItem)
        : QWidget(messageItem)
        , text_(nullptr)
        , isEdited_(false)
        , withUnderlay_(false)
        , isOutgoing_(false)
        , isSelected_(false)
    {
        setAttribute(Qt::WA_NoSystemBackground);
        setAttribute(Qt::WA_TransparentForMouseEvents);
    }

    void MessageTimeWidget::setTime(const int32_t _timestamp)
    {
        TimeText_ = QDateTime::fromTime_t(_timestamp).toString(qsl("HH:mm"));
        updateText();
    }

    void MessageTimeWidget::setContact(const QString& _aimId)
    {
        aimId_ = _aimId;
    }

    void MessageTimeWidget::setUnderlayVisible(const bool _visible)
    {
        if (withUnderlay_ == _visible)
            return;

        withUnderlay_ = _visible;
        updateText();
    }

    bool MessageTimeWidget::isUnderlayVisible() const noexcept
    {
        return withUnderlay_;
    }

    void MessageTimeWidget::setEdited(const bool _edited)
    {
        if (isEdited_ == _edited)
            return;

        isEdited_ = _edited;
        updateText();
    }

    bool MessageTimeWidget::isEdited() const noexcept
    {
        return isEdited_;
    }

    int MessageTimeWidget::getHorMargin() const noexcept
    {
        return isUnderlayVisible() ? Utils::scale_value(8) : Utils::scale_value(10);
    }

    int MessageTimeWidget::getVerMargin() const noexcept
    {
        return isUnderlayVisible() ? Utils::scale_value(8) : Utils::scale_value(6);
    }

    void MessageTimeWidget::setOutgoing(const bool _outgoing)
    {
        if (isOutgoing_ == _outgoing)
            return;

        isOutgoing_ = _outgoing;
        updateText();
    }

    bool MessageTimeWidget::isOutgoing() const noexcept
    {
        return isOutgoing_;
    }

    void MessageTimeWidget::setSelected(const bool _selected)
    {
        if (isSelected_ == _selected)
            return;

        isSelected_ = _selected;
        updateStyle();
    }

    bool MessageTimeWidget::isSelected() const noexcept
    {
        return isSelected_;
    }

    void MessageTimeWidget::paintEvent(QPaintEvent *)
    {
        if (!text_)
            return;

        QPainter p(this);

        if (withUnderlay_)
        {
            p.setRenderHint(QPainter::Antialiasing);

            const auto radius = rect().height() / 2;
            static const auto bgColor = QColor(0, 0, 0, 255 * 0.5);

            p.setBrush(bgColor);
            p.setPen(Qt::NoPen);
            p.drawRoundedRect(rect(), radius, radius);
        }

        text_->draw(p, TextRendering::VerPosition::MIDDLE);
    }

    void MessageTimeWidget::updateText()
    {
        if (!isEdited_ && TimeText_.isEmpty())
            return;

        text_ = TextRendering::MakeTextUnit(TimeText_);
        text_->init(withUnderlay_ ? getTimeFontLarge() : getTimeFont(), getTimeColor());

        if (isEdited_)
        {
            auto ed = TextRendering::MakeTextUnit(QT_TRANSLATE_NOOP("chat_page", "edited") % ql1c(' '));
            ed->init(getTimeFontLarge(), getTimeColor());

            ed->append(std::move(text_));
            text_ = std::move(ed);
        }

        const auto h = getHeight(withUnderlay_);
        text_->setOffsets(withUnderlay_ ? getHorPadding() : 0, h / 2);
        text_->evaluateDesiredSize();

        const auto w = text_->desiredWidth() + (withUnderlay_ ? getHorPadding() * 2 : 0);
        setFixedSize(w, h);
        update();
    }

    void MessageTimeWidget::updateStyle()
    {
        if (text_)
        {
            text_->setColor(getTimeColor());
            update();
        }
    }

    QColor MessageTimeWidget::getTimeColor() const
    {
        assert(!aimId_.isEmpty());

        auto var = Styling::StyleVariable::BASE_PRIMARY;
        if (withUnderlay_ || isSelected_)
            var = Styling::StyleVariable::TEXT_SOLID_PERMANENT;
        else if (isOutgoing())
            var = Styling::StyleVariable::PRIMARY_PASTEL;

        return Styling::getParameters(aimId_).getColor(var);
    }
}
