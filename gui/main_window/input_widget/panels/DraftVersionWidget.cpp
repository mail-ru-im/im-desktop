#include "stdafx.h"

#include "DraftVersionWidget.h"
#include "controls/TooltipWidget.h"
#include "controls/CustomButton.h"
#include "styles/ThemeParameters.h"
#include "../InputWidgetUtils.h"
#include "fonts.h"

namespace
{

constexpr auto tooltipInterval() { return std::chrono::milliseconds(150); }

QFont headerLabelFont()
{
    return Fonts::appFontScaled(14, Fonts::FontWeight::SemiBold);
}

QFont messageTextFont()
{
    return Fonts::appFontScaled(13);
}

auto widgetHeight()
{
    return Utils::scale_value(48);
}

auto textWidgetHeight()
{
    return Utils::scale_value(40);
}

class DraftTextWidget : public QWidget
{
public:
    DraftTextWidget(QWidget* _parent)
        : QWidget(_parent)
    {
        setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
        setFixedHeight(textWidgetHeight());
        label_ = Ui::TextRendering::MakeTextUnit(QT_TRANSLATE_NOOP("draft", "Apply another draft version?"));
        Ui::TextRendering::TextUnit::InitializeParameters params{ headerLabelFont(), Styling::ThemeColorKey{ Styling::StyleVariable::PRIMARY_INVERSE } };
        params.maxLinesCount_ = 1;
        label_->init(params);
        label_->setOffsets(0, Utils::scale_value(15));
        message_ = Ui::TextRendering::MakeTextUnit(QString(), Data::MentionMap(), Ui::TextRendering::LinksVisible::DONT_SHOW_LINKS);
        params.setFonts(messageTextFont());
        params.color_ = params.linkColor_ = Styling::ThemeColorKey{ Styling::StyleVariable::TEXT_SOLID };
        message_->init(params);
        message_->setOffsets(0, Utils::scale_value(32));
    }

    void setMessage(const Data::MessageBuddy& _msg)
    {
        message_->setText(_msg.GetText());
        message_->setMentions(_msg.Mentions_);
        updateSize(size());
    }

protected:
    void resizeEvent(QResizeEvent* _event) override
    {
        updateSize(_event->size());

        QWidget::resizeEvent(_event);
    }
    void paintEvent(QPaintEvent* _event) override
    {
        QPainter p(this);
        label_->draw(p, Ui::TextRendering::VerPosition::BASELINE);
        message_->draw(p, Ui::TextRendering::VerPosition::BASELINE);

        QWidget::paintEvent(_event);
    }
    void enterEvent(QEvent* _event) override
    {
        startTooltipTimer();
    }
    void leaveEvent(QEvent* _event) override
    {
        stopTooltipTimer();
    }

private:
    void updateSize(const QSize& _size)
    {
        auto availableWidth = _size.width() - Utils::scale_value(8);
        label_->getHeight(availableWidth);
        needsTooltip_ = message_->desiredWidth() > availableWidth;

        update();
    }
    void showTooltip()
    {
        if (!needsTooltip_)
            return;

        const auto r = rect();
        Tooltip::show(message_->getText(), QRect(mapToGlobal(r.topLeft()), r.size()), {0, 0}, Tooltip::ArrowDirection::Down, Tooltip::ArrowPointPos::Top, Ui::TextRendering::HorAligment::LEFT, {}, Tooltip::TooltipMode::Multiline);
    }

    void initTooltipTimer()
    {
        tooltipTimer_ = new QTimer(this);
        tooltipTimer_->setInterval(tooltipInterval());
        tooltipTimer_->setSingleShot(true);
        connect(tooltipTimer_, &QTimer::timeout, this, &DraftTextWidget::showTooltip);
    }

    void startTooltipTimer()
    {
        if (!tooltipTimer_)
            initTooltipTimer();

        tooltipTimer_->start();
    }

    void stopTooltipTimer()
    {
        if (tooltipTimer_)
            tooltipTimer_->stop();
    }


    Ui::TextRendering::TextUnitPtr label_;
    Ui::TextRendering::TextUnitPtr message_;
    QTimer* tooltipTimer_ = nullptr;
    bool needsTooltip_ = false;
};

}

namespace Ui
{


class DraftVersionWidget_p
{
public:
    DraftTextWidget* textWidget_ = nullptr;
};

DraftVersionWidget::DraftVersionWidget(QWidget* _parent)
    : QWidget(_parent)
    , d(std::make_unique<DraftVersionWidget_p>())
{
    setFixedHeight(widgetHeight());

    auto layout = Utils::emptyHLayout(this);
    setContentsMargins(0, 0, 0, Utils::scale_value(8));

    d->textWidget_ = new DraftTextWidget(this);

    auto acceptButton = new CustomButton(this, qsl(":/apply_edit"), QSize(20, 20), Styling::ThemeColorKey{ Styling::StyleVariable::PRIMARY });
    acceptButton->setCustomToolTip(QT_TRANSLATE_NOOP("draft", "Apply draft"));
    Testing::setAccessibleName(acceptButton, qsl("AS ChatInput applyDraft"));
    auto cancelButton = new CustomButton(this, qsl(":/controls/close_icon"), getCancelBtnIconSize());
    cancelButton->setCustomToolTip(QT_TRANSLATE_NOOP("draft", "Cancel"));
    Testing::setAccessibleName(cancelButton, qsl("AS ChatInput cancelDraft"));

    layout->addSpacing(Utils::scale_value(20));
    layout->addWidget(d->textWidget_);
    layout->addWidget(acceptButton);
    layout->addSpacing(Utils::scale_value(12));
    layout->addWidget(cancelButton);
    layout->addSpacing(Utils::scale_value(16));

    connect(acceptButton, &CustomButton::clicked, this, [this](){ Q_EMIT accept(QPrivateSignal()); });
    connect(cancelButton, &CustomButton::clicked, this, [this](){ Q_EMIT cancel(QPrivateSignal()); });
}

DraftVersionWidget::~DraftVersionWidget() = default;

void DraftVersionWidget::setDraft(const Data::Draft& _draft)
{
    d->textWidget_->setMessage(_draft.message_);
}

}
