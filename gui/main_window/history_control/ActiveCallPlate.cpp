#include "stdafx.h"

#include "main_window/MainPage.h"
#include "controls/DialogButton.h"
#include "controls/ClickWidget.h"
#include "controls/TextUnit.h"
#include "styles/ThemeParameters.h"
#include "previewer/toast.h"
#include "core_dispatcher.h"
#include "utils/utils.h"
#include "fonts.h"

#include "ActiveCallPlate.h"

namespace
{
constexpr auto offsetTopName() noexcept { return 5; }
constexpr auto offsetTopText() noexcept { return 24; }

int callTextTopOffset()
{
    return Utils::scale_value(5);
}

int participantsCountTextTopOffset()
{
    return Utils::scale_value(24);
}

int textLeftOffset()
{
    return Utils::scale_value(20);
}

QColor callTextColor()
{
    return Styling::getParameters().getColor(Styling::StyleVariable::TEXT_PRIMARY);
}

QColor participantsCountTextColor()
{
    return Styling::getParameters().getColor(Styling::StyleVariable::BASE_PRIMARY);
}

}

namespace Ui
{
    class ActiveCallPlate_p
    {
    public:
        int widthAvailableForText() const
        {
            return button_->geometry().left() - 2 * textLeftOffset();
        }

        QString chatId_;
        DialogButton* button_ = nullptr;
        int participantsCount_ = 0;
        TextRendering::TextUnitPtr callText_;
        TextRendering::TextUnitPtr participantsCountText_;
    };

    ActiveCallPlate::ActiveCallPlate(const QString& _chatId, QWidget* _parent)
        : QWidget(_parent),
          d(std::make_unique<ActiveCallPlate_p>())
    {
        setFixedHeight(Utils::scale_value(48));
        QHBoxLayout* layout = Utils::emptyHLayout(this);
        layout->setAlignment(Qt::AlignRight | Qt::AlignVCenter);

        d->chatId_ = _chatId;

        d->button_ = new DialogButton(this, QT_TRANSLATE_NOOP("active_call_plate", "Join"));
        layout->addWidget(d->button_);
        layout->addSpacing(Utils::scale_value(16));
        connect(d->button_, &DialogButton::clicked, this, &ActiveCallPlate::onJoinClicked);

        d->callText_ = TextRendering::MakeTextUnit(QT_TRANSLATE_NOOP("active_call_plate", "Active call"));
        d->callText_->init(Fonts::appFontScaled(14, Fonts::FontWeight::SemiBold), callTextColor(), QColor(), QColor(), QColor(), TextRendering::HorAligment::LEFT, 1);
        d->callText_->setOffsets(textLeftOffset(), callTextTopOffset());
        d->callText_->getHeight(d->callText_->desiredWidth());

        d->participantsCountText_ = TextRendering::MakeTextUnit(QString());
        d->participantsCountText_->init(Fonts::appFontScaled(14, Fonts::FontWeight::Normal), participantsCountTextColor(), QColor(), QColor(), QColor(), TextRendering::HorAligment::LEFT, 1);
        d->participantsCountText_->setOffsets(textLeftOffset(), participantsCountTextTopOffset());
    }

    ActiveCallPlate::~ActiveCallPlate() = default;

    void ActiveCallPlate::setParticipantsCount(int _count)
    {
        const QString text = QString::number(_count) % ql1c(' ') % Utils::GetTranslator()->getNumberString(
                    _count,
                    QT_TRANSLATE_NOOP3("active_call_plate", "participant", "1"),
                    QT_TRANSLATE_NOOP3("active_call_plate", "participants", "2"),
                    QT_TRANSLATE_NOOP3("active_call_plate", "participants", "5"),
                    QT_TRANSLATE_NOOP3("active_call_plate", "participants", "21"));

        d->participantsCountText_->setText(text);
        d->participantsCountText_->getHeight(d->widthAvailableForText());
        d->participantsCount_ = _count;

        update();
    }

    int ActiveCallPlate::participantsCount() const
    {
        return d->participantsCount_;
    }

    void ActiveCallPlate::paintEvent(QPaintEvent* _event)
    {
        QPainter p(this);

        static const auto bg = Styling::getParameters().getColor(Styling::StyleVariable::BASE_GLOBALWHITE);
        static const auto border = Styling::getParameters().getColor(Styling::StyleVariable::BASE_BRIGHT);

        Utils::drawBackgroundWithBorders(p, rect(), bg, border, Qt::AlignBottom);

        d->callText_->draw(p);
        d->participantsCountText_->draw(p);

        QWidget::paintEvent(_event);
    }

    void ActiveCallPlate::resizeEvent(QResizeEvent* _event)
    {
        const auto availableWidth = d->widthAvailableForText();
        d->callText_->elide(availableWidth);
        d->participantsCountText_->elide(availableWidth);

        QWidget::resizeEvent(_event);
    }

    void ActiveCallPlate::onJoinClicked()
    {
#ifndef STRIP_VOIP
        auto joined = Ui::GetDispatcher()->getVoipController().setStartChatRoomCall(d->chatId_);

        auto mainPage = MainPage::instance();
        if (mainPage && joined)
            mainPage->raiseVideoWindow(voip_call_type::audio_call);
#endif
    }

}
