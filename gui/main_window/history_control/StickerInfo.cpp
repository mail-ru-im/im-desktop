#include "stdafx.h"

#include "../../../corelib/collection_helper.h"

#include "StickerInfo.h"

namespace HistoryControl
{

    StickerInfoSptr StickerInfo::Make(const core::coll_helper& _coll)
    {
        const auto setId = _coll.get_value_as_uint("set_id");
        im_assert(setId > 0);

        const auto stickerId = _coll.get_value_as_uint("sticker_id");
        im_assert(stickerId > 0);

        return StickerInfoSptr(
            new StickerInfo(setId, stickerId)
        );
    }

    StickerInfoSptr StickerInfo::Make(const quint32 _setId, const quint32 _stickerId)
    {
        im_assert(_setId > 0);
        im_assert(_stickerId > 0);

        return StickerInfoSptr(
            new StickerInfo(_setId, _stickerId)
            );
    }

    StickerInfo::StickerInfo(const quint32 _setId, const quint32 _stickerId)
        : SetId_(_setId)
        , StickerId_(_stickerId)
    {
        im_assert(SetId_ > 0);
        im_assert(StickerId_ > 0);
    }

}