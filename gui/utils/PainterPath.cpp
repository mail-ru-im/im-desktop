#include "stdafx.h"

#include "utils.h"

#include "PainterPath.h"

namespace Utils
{
    QPainterPath renderMessageBubble(
        const QRect& _rect,
        const int32_t _borderRadius,
        const int32_t _borderRadiusSmall,
        const RenderBubbleFlags _flags)
    {
        if (!_flags)
        {
            QPainterPath path;
            path.addRect(_rect);

            return path;
        }

        assert(_borderRadius > 0);

        const auto right = _rect.right() + 1;
        const auto bottom = _rect.bottom() + 1;

        const auto getParams = [_flags, _borderRadius, _borderRadiusSmall](const auto _corner) -> std::pair<int, int>
        {
            int radius = _borderRadius;
            switch (_corner)
            {
            case RenderBubbleFlags::LeftTopRounded:
                radius = (_flags & RenderBubbleFlags::LeftTopSmall) ? _borderRadiusSmall : _borderRadius;
                break;

            case RenderBubbleFlags::LeftBottomRounded:
                radius = (_flags & RenderBubbleFlags::LeftBottomSmall) ? _borderRadiusSmall : _borderRadius;
                break;

            case RenderBubbleFlags::RightTopRounded:
                radius = (_flags & RenderBubbleFlags::RightTopSmall) ? _borderRadiusSmall : _borderRadius;
                break;

            case RenderBubbleFlags::RightBottomRounded:
                radius = (_flags & RenderBubbleFlags::RightBottomSmall) ? _borderRadiusSmall : _borderRadius;
                break;

            default:
                assert(!"only one corner allowed");
                break;
            }

            return { radius, radius * 2 };
        };

        QPainterPath clipPath;
        auto x = _rect.left();
        auto y = _rect.top();

        if (_flags & RenderBubbleFlags::LeftTopRounded)
        {
            const auto [radius, diameter] = getParams(_flags & RenderBubbleFlags::LeftTopRounded);
            x += radius;

            clipPath.moveTo(x, y);
            clipPath.arcTo(_rect.left(), _rect.top(), diameter, diameter, 90, 90);
        }
        else
        {
            clipPath.lineTo(x, y);
        }

        if (_flags & RenderBubbleFlags::LeftBottomRounded)
        {
            const auto [radius, diameter] = getParams(_flags & RenderBubbleFlags::LeftBottomRounded);
            x = _rect.left();
            y = bottom - radius;
            clipPath.lineTo(x, y);

            y = bottom - diameter;
            clipPath.arcTo(x, y, diameter, diameter, 180, 90);
        }
        else
        {
            x = _rect.left();
            y = bottom;
            clipPath.lineTo(x, y);
        }

        if (_flags & RenderBubbleFlags::RightBottomRounded)
        {
            const auto [radius, diameter] = getParams(_flags & RenderBubbleFlags::RightBottomRounded);
            x = right - radius;
            y = bottom;
            clipPath.lineTo(x, y);

            x = right - diameter;
            y = bottom - diameter;
            clipPath.arcTo(x, y, diameter, diameter, 270, 90);
        }
        else
        {
            clipPath.lineTo(right, bottom);
        }

        if (_flags & RenderBubbleFlags::RightTopRounded)
        {
            const auto [radius, diameter] = getParams(_flags & RenderBubbleFlags::RightTopRounded);
            x = right;
            y = _rect.top() + radius;
            clipPath.lineTo(x, y);

            x = right - diameter;
            y = _rect.top();
            clipPath.arcTo(x, y, diameter, diameter, 0, 90);
        }
        else
        {
            x = right;
            y = _rect.top();
            clipPath.lineTo(x, y);
        }

        // the final closure
        if (_flags & RenderBubbleFlags::LeftTopRounded)
        {
            const auto radius = (_flags & RenderBubbleFlags::LeftTopSmall) ? _borderRadiusSmall : _borderRadius;
            x = _rect.left() + radius;
            y = _rect.top();
            clipPath.lineTo(x, y);
        }
        else
        {
            clipPath.lineTo(_rect.topLeft());
        }

        return clipPath;
    }

}