#pragma once

namespace Utils
{
    struct ManhattanDistance
    {
        // fastest, but less accurate than euclidian distance
        double operator()(const QPointF& _p1, const QPointF& _p2) const
        {
            return std::abs(_p1.x() - _p2.x()) + std::abs(_p1.y() - _p2.y());
        }
    };

    class PolygonRounder : ManhattanDistance
    {
    public:

        QPainterPath operator()(const QPolygonF& _polygon, double _radius) const;

        inline QPainterPath operator()(const QRect& _rect, double _radius) const
        {
            return operator()(QRectF(_rect), _radius);
        }

        inline QPainterPath operator()(const QPolygonF& _polygon, const std::vector<double>& _radiuses) const
        {
            return operator()(_polygon, _radiuses.data(), _radiuses.size());
        }
        template<int _Prealloc>
        inline QPainterPath operator()(const QPolygonF& _polygon, const QVarLengthArray<double, _Prealloc>& _radiuses) const
        {
            return operator()(_polygon, _radiuses.constData(), _radiuses.size());
        }

        template<size_t _Size>
        inline QPainterPath operator()(const QPolygonF& _rect, const double(&_radiuses)[_Size]) const
        {
            return operator()(_rect, _radiuses, _Size);
        }

        template<size_t _Size>
        inline QPainterPath operator()(const QRect& _rect, const double(&_radiuses)[_Size]) const
        {
            static_assert(_Size > 0 && _Size < 5, "rectangle has only 4 corners");
            return operator()(QRectF(_rect), _radiuses, _Size);
        }

    private:
        QPainterPath operator()(const QPolygonF& _polygon, const double* _radiuses, size_t _length) const;

        inline double distance(const QPointF& _curr, const QPointF& _next) const
        {
            return static_cast<const ManhattanDistance&>(*this)(_curr, _next);
        }

        inline double distanceRadius(const QPointF& _curr, const QPointF& _next, double _radius) const
        {
            return std::min(0.5, _radius / distance(_curr, _next));
        }

        inline QPointF lineStart(const QPointF& _curr, const QPointF& _next, double _r) const
        {
            QPointF pt;
            pt.setX((1.0 - _r) * _curr.x() + _r * _next.x());
            pt.setY((1.0 - _r) * _curr.y() + _r * _next.y());
            return pt;
        }

        inline QPointF lineEnd(const QPointF& _curr, const QPointF& _next, double _r) const
        {
            QPointF pt;
            pt.setX(_r * _curr.x() + (1.0 - _r) * _next.x());
            pt.setY(_r * _curr.y() + (1.0 - _r) * _next.y());
            return pt;
        }
    };
}

