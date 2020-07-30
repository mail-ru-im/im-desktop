#include "stdafx.h"

#include "ComplexMessageUtils.h"
#include "../../../utils/utils.h"

UI_COMPLEX_MESSAGE_NS_BEGIN

QSize limitSize(const QSize& _size, const QSize& _maxSize, const QSize& _minSize)
{
    assert(!_size.isEmpty());
    assert(_maxSize.width() > 0 || _maxSize.height() > 0);

    if (_size.isEmpty())
        return _size;

    if (_size.width() > _maxSize.width() || _size.height() > _maxSize.height())
    {
        if (Utils::isPanoramic(_size) && _minSize.isValid())
        {
            const auto scale = [&_maxSize, &_size](const auto _imgSide, const int _preSide)
            {
                const auto ratio = double(std::max(_imgSide, _preSide)) / std::min(_imgSide, _preSide);
                if (_imgSide > _preSide)
                    return (_size / ratio).boundedTo(_maxSize);
                else
                    return (_size * ratio).boundedTo(_maxSize);
            };

            if (_size.height() > _size.width())
                return scale(_size.width(), _minSize.width());
            else
                return scale(_size.height(), _minSize.height());
        }
        else
        {
            return _size.scaled(_maxSize, Qt::KeepAspectRatio);
        }
    }

    return _size;
}

UI_COMPLEX_MESSAGE_NS_END
