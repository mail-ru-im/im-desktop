#include "stdafx.h"

#include "EditMessageWidget.h"

#include "../../history_control/MessageStyle.h"
#include "controls/CustomButton.h"
#include "controls/ClickWidget.h"
#include "utils/utils.h"
#include "styles/ThemeParameters.h"
#include "fonts.h"

namespace
{
    int getEditHeight()
    {
        return Utils::scale_value(48);
    }

    int selectVerPadding()
    {
        return Utils::scale_value(4);
    }
}

namespace Ui
{
    EditBlockText::EditBlockText(QWidget* _parent)
        : ClickableWidget(_parent)
    {
        caption_ = TextRendering::MakeTextUnit(QT_TRANSLATE_NOOP("input_widget", "Edit message"));
        caption_->init(Fonts::appFontScaled(14, Fonts::FontWeight::SemiBold), Styling::getParameters().getColor(Styling::StyleVariable::PRIMARY_INVERSE));
        caption_->setOffsets(getInputTextLeftMargin(), Utils::scale_value(15));

        text_ = TextRendering::MakeTextUnit(QString(), {}, TextRendering::LinksVisible::DONT_SHOW_LINKS, TextRendering::ProcessLineFeeds::REMOVE_LINE_FEEDS);
        text_->init(
            Fonts::appFontScaled(13, Fonts::FontWeight::Normal),
            Styling::getParameters().getColor(Styling::StyleVariable::TEXT_SOLID),
            Styling::getParameters().getColor(Styling::StyleVariable::TEXT_SOLID),
            QColor(), QColor(), TextRendering::HorAligment::LEFT, 1);
        text_->setOffsets(getInputTextLeftMargin(), Utils::scale_value(32));

        elide();

        connect(this, &EditBlockText::hoverChanged, this, Utils::QOverload<>::of(&EditBlockText::update));
        connect(this, &EditBlockText::pressChanged, this, Utils::QOverload<>::of(&EditBlockText::update));
    }

    void EditBlockText::setMessage(Data::MessageBuddySptr _message)
    {
        if (!_message)
            return;

        assert(!_message->GetText().isEmpty());
        text_->setTextAndMentions(_message->GetText(), _message->Mentions_);
        elide();

        update();
    }

    void EditBlockText::paintEvent(QPaintEvent* _event)
    {
        QPainter p(this);
        p.setRenderHint(QPainter::Antialiasing);

        if (isHovered() || isPressed())
        {
            p.setBrush(isPressed() ? Ui::getPopupPressedColor() : Ui::getPopupHoveredColor());
            p.setPen(Qt::NoPen);

            const auto radius = Utils::scale_value(6);
            p.drawRoundedRect(rect(), radius, radius);
        }

        caption_->draw(p, TextRendering::VerPosition::BASELINE);
        text_->draw(p, TextRendering::VerPosition::BASELINE);
    }

    void EditBlockText::resizeEvent(QResizeEvent* _event)
    {
        elide();
    }

    void EditBlockText::elide()
    {
        if (const auto w = width() - Utils::scale_value(8) - getInputTextLeftMargin(); w > 0)
        {
            caption_->getHeight(w);
            text_->getHeight(w);
            update();
        }
    }

    EditMessageWidget::EditMessageWidget(QWidget* _parent)
        : QWidget(_parent)
    {
        setFixedHeight(getEditHeight());

        editBlockText_ = new EditBlockText(this);
        editBlockText_->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
        Testing::setAccessibleName(editBlockText_, qsl("AS editBlockText_"));

        auto btnWidget = new QWidget(this);
        Testing::setAccessibleName(btnWidget, qsl("AS iw btnWidget"));
        btnWidget->setFixedWidth(getHorMargin());

        buttonCancel_ = new CustomButton(btnWidget, qsl(":/controls/close_icon"), getCancelBtnIconSize());
        buttonCancel_->setFixedSize(getCancelButtonSize());
        updateButtonColors(buttonCancel_, InputStyleMode::Default);
        Testing::setAccessibleName(buttonCancel_, qsl("AS iw cancel"));

        auto btnLayout = Utils::emptyVLayout(btnWidget);
        btnLayout->setAlignment(Qt::AlignCenter);
        btnLayout->addWidget(buttonCancel_);

        auto rootLayout = Utils::emptyHLayout(this);
        rootLayout->setContentsMargins(0, selectVerPadding(), 0, selectVerPadding());
        rootLayout->addSpacing(getEditHorMarginLeft());
        rootLayout->addWidget(editBlockText_);
        rootLayout->addWidget(btnWidget);

        connect(buttonCancel_, &CustomButton::clicked, this, [this]()
        {
            emit cancelClicked(QPrivateSignal());
        });
        connect(editBlockText_, &ClickableWidget::clicked, this, [this]()
        {
            emit messageClicked(QPrivateSignal());
        });
    }

    void EditMessageWidget::setMessage(Data::MessageBuddySptr _message)
    {
        editBlockText_->setMessage(_message);
    }

    void EditMessageWidget::updateStyleImpl(const InputStyleMode _mode)
    {
        updateButtonColors(buttonCancel_, _mode);
    }
}
