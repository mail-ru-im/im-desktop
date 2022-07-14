#include "stdafx.h"
#include "PersonTooltip.h"

#include "controls/TooltipWidget.h"
#include "utils/utils.h"
#include "../main_window/contact_list/ServiceContacts.h"

namespace
{
    constexpr auto tooltipMode = Tooltip::TooltipMode::Multiline;
}

namespace Utils
{
    PersonTooltip::PersonTooltip(QObject* _parent) : QObject(_parent)
    {
    }

    void PersonTooltip::initTimer()
    {
        if (!tooltipTimer_)
        {
            tooltipTimer_ = new QTimer(this);
            tooltipTimer_->setInterval(Tooltip::getDefaultShowDelay());
            tooltipTimer_->setSingleShot(true);
        }
        else
        {
            disableTimer();
        }
    }

    void PersonTooltip::show(PersonTooltipType _type,
        Data::FString _textOrAimId,
        const QRect& _rect,
        Tooltip::ArrowDirection _arrowDir,
        Tooltip::ArrowPointPos _arrowPos,
        Ui::TextRendering::HorAligment _align)
    {
        const auto isText = _type == PersonTooltipType::Text;
        if (isText)
            Tooltip::hide();

        initTimer();

        connect(tooltipTimer_, &QTimer::timeout, this, [textOrAimId = std::move(_textOrAimId), _rect, _arrowDir, _arrowPos, _align, isText]()
            {
                if (isText)
                    Tooltip::show(textOrAimId, _rect, {}, _arrowDir, _arrowPos, _align, {}, Tooltip::TooltipMode::Multiline);
                else if(!Utils::isChat(textOrAimId.string()) && !ServiceContacts::isServiceContact(textOrAimId.string()))
                    Tooltip::showMention(textOrAimId.string(), _rect, {}, _arrowDir, _arrowPos, {}, tooltipMode);
            });
        tooltipTimer_->start();
    }

    void PersonTooltip::hide()
    {
        if (preparedToShow())
            disableTimer();
        else
            Tooltip::hide(); 
    }

    bool PersonTooltip::preparedToShow() const
    {
        return tooltipTimer_ && tooltipTimer_->isActive();
    }

    void PersonTooltip::disableTimer()
    {
        tooltipTimer_->stop();
        tooltipTimer_->disconnect();
    }
}
