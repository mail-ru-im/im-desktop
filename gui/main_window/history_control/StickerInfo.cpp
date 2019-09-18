#include "stdafx.h"

#include "../../../corelib/collection_helper.h"

#include "StickerInfo.h"

namespace HistoryControl
{

    StickerInfoSptr StickerInfo::Make(const core::coll_helper& _coll)
    {
        const auto setId = _coll.get_value_as_uint("set_id");
        assert(setId > 0);

        const auto stickerId = _coll.get_value_as_uint("sticker_id");
        assert(stickerId > 0);

        return StickerInfoSptr(
            new StickerInfo(setId, stickerId)
        );
    }

    StickerInfoSptr StickerInfo::Make(const quint32 _setId, const quint32 _stickerId)
    {
        assert(_setId > 0);
        assert(_stickerId > 0);

        return StickerInfoSptr(
            new StickerInfo(_setId, _stickerId)
            );
    }

    StickerInfo::StickerInfo(const quint32 _setId, const quint32 _stickerId)
        : SetId_(_setId)
        , StickerId_(_stickerId)
    {
        assert(SetId_ > 0);
        assert(StickerId_ > 0);
    }

}