#pragma once

namespace Utils
{

    enum RenderBubbleFlags
    {
        LeftTopRounded      = (1 << 0),
        RightTopRounded     = (1 << 1),
        RightBottomRounded  = (1 << 2),
        LeftBottomRounded   = (1 << 3),

        LeftSideRounded     = LeftTopRounded | LeftBottomRounded,
        RightSideRounded    = RightTopRounded | RightBottomRounded,
        TopSideRounded      = LeftTopRounded | RightTopRounded,
        BottomSideRounded   = LeftBottomRounded | RightBottomRounded,
        AllRounded          = TopSideRounded | BottomSideRounded,

        LeftTopSmall        = (1 << /*0),*/4),
        RightTopSmall       = (1 << /*1),*/5),
        RightBottomSmall    = (1 << /*2),*/6),
        LeftBottomSmall     = (1 << /*3),*/7),

        LeftSideSmall       = LeftTopSmall | LeftBottomSmall,
        RightSideSmall      = RightTopSmall | RightBottomSmall,
    };

    QPainterPath renderMessageBubble(
        const QRect& _rect,
        const int32_t _borderRadius,
        const int32_t _borderRadiusSmall = 0,
        const RenderBubbleFlags _flags = RenderBubbleFlags::AllRounded);
}