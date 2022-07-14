#include "stdafx.h"
#include "PolygonRounder.h"

namespace Utils
{
    QPainterPath PolygonRounder::operator()(const QPolygonF& _polygon, double _radius) const
    {
        QPainterPath path;

        const int n = _polygon.size() - _polygon.isClosed();
        if (n < 3) // unable to rounding a polygon with less than 3 vertices
        {
            path.addPolygon(_polygon);
            return path;
        }

        _radius = std::max(0.0, _radius); // radius can't be negative

        //
        // Next piece of code is an optimized
        // version that eliminates modulo
        // operations and unnecessary computations,
        // so it looks a little-bit bizarre.
        //

        double r;
        QPointF pt1, pt2, curr, next;

        curr = _polygon[0];
        next = _polygon[1];

        // process first point
        r = distanceRadius(curr, next, _radius);
        pt1 = lineStart(curr, next, r);
        path.moveTo(pt1);
        pt2 = lineEnd(curr, next, r);
        path.lineTo(pt2);

        int i = 1;
        for (; i < (n - 1); ++i)
        {
            curr = _polygon[i];
            next = _polygon[i + 1];

            r = distanceRadius(curr, next, _radius);
            pt1 = lineStart(curr, next, r);
            path.quadTo(curr, pt1);
            pt2 = lineEnd(curr, next, r);
            path.lineTo(pt2);
        }

        // process last point
        curr = _polygon[i];
        next = _polygon[0];

        r = distanceRadius(curr, next, _radius);
        pt1 = lineStart(curr, next, r);
        path.quadTo(curr, pt1);
        pt2 = lineEnd(curr, next, r);
        path.lineTo(pt2);

        // close last corner
        curr = _polygon[0];
        next = _polygon[1];
        r = distanceRadius(curr, next, _radius);
        pt1 = lineStart(curr, next, r);
        path.quadTo(curr, pt1);

        return path;
    }

    QPainterPath PolygonRounder::operator()(const QPolygonF& _polygon, const double* _radiuses, size_t _length) const
    {
        QPainterPath path;
        const size_t n = static_cast<size_t>(_polygon.size() - _polygon.isClosed());
        _length = std::min(_length, n);
        im_assert(n >= 3);
        if (n < 3 || _length == 0 || std::all_of(_radiuses, _radiuses + _length, [](auto x) { return (x == 0.0); }))
        {
            // unable to rounding a polygon with less than 3 vertices
            // or if all radiuses is zero of if no radiuses was provided
            path.addPolygon(_polygon);
            return path;
        }

        double r, radius0 = std::max(_radiuses[0], 0.0);
        QPointF pt1, pt2, curr, next;

        curr = _polygon[0];
        next = _polygon[1];

        // process first point
        r = distanceRadius(curr, next, radius0);
        pt1 = lineStart(curr, next, r);
        path.moveTo(pt1);
        r = distanceRadius(curr, next, std::max(_radiuses[1], 0.0));
        pt2 = lineEnd(curr, next, r);
        path.lineTo(pt2);

        size_t i = 1;
        for (; i < (n - 1); ++i)
        {
            curr = _polygon[i];
            next = _polygon[i + 1];
            if (i >= _length)
            {
                path.lineTo(curr);
                path.lineTo(next);
                continue;
            }

            r = distanceRadius(curr, next, std::max(_radiuses[i], 0.0));
            pt1 = lineStart(curr, next, r);
            path.quadTo(curr, pt1);
            r = distanceRadius(curr, next, std::max(_radiuses[i + 1], 0.0));
            pt2 = lineEnd(curr, next, r);
            path.lineTo(pt2);
        }

        // process last point
        curr = _polygon[i];
        next = _polygon[0];

        r = distanceRadius(curr, next, std::max(_radiuses[i], 0.0));
        pt1 = lineStart(curr, next, r);
        path.quadTo(curr, pt1);
        r = distanceRadius(curr, next, radius0);
        pt2 = lineEnd(curr, next, r);
        path.lineTo(pt2);

        // close last corner
        curr = _polygon[0];
        next = _polygon[1];
        r = distanceRadius(curr, next, radius0);
        pt1 = lineStart(curr, next, r);
        path.quadTo(curr, pt1);

        return path;
    }

} // end namespace Utils
