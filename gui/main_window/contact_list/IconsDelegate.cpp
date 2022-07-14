#include "stdafx.h"
#include "IconsDelegate.h"

namespace Logic
{
    bool IconsDelegate::isInDraftIconRect(QPoint _posCursor) const
    {
        return !draftIconRect_.isEmpty() && draftIconRect_.contains(_posCursor);
    }
}