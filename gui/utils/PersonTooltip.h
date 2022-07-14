#pragma once
#include "controls/textrendering/TextRendering.h"
#include "controls/TooltipWidget.h"

namespace Tooltip
{
    enum class ArrowDirection;
    enum class ArrowPointPos;
}

namespace Utils
{
    enum class PersonTooltipType
    {
        Text,
        Person,
    };

    class PersonTooltip : public QObject
    {
        Q_OBJECT

    public:
        PersonTooltip(QObject* _parent);
        void show(PersonTooltipType _type,
            Data::FString _textOrAimId,
            const QRect& _rect,
            Tooltip::ArrowDirection _arrowDir,
            Tooltip::ArrowPointPos _arrowPos,
            Ui::TextRendering::HorAligment _align = Ui::TextRendering::HorAligment::LEFT);

        void hide();
        bool preparedToShow() const;

    private:

        void initTimer();
        void disableTimer();

        QTimer* tooltipTimer_ = nullptr;
    };
}
