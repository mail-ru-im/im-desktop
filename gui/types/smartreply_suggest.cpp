#include "stdafx.h"

#include "../../../common.shared/smartreply/smartreply_types.h"
#include "../utils/gui_coll_helper.h"
#include "smartreply_suggest.h"

namespace Data
{
    SmartreplySuggest::SmartreplySuggest()
        : type_(core::smartreply::type::invalid)
        , msgId_(-1)
    {
    }

    void SmartreplySuggest::unserialize(core::coll_helper& _coll)
    {
        type_ = (core::smartreply::type)_coll.get_value_as_int("type");
        msgId_ = _coll.get_value_as_int64("msgId");
        contact_ = QString::fromUtf8(_coll.get_value_as_string("contact"));
        data_ = QString::fromUtf8(_coll.get_value_as_string("data"));
    }

    void SmartreplySuggest::serialize(Ui::gui_coll_helper& _coll) const
    {
        _coll.set_value_as_qstring("contact", contact_);
        _coll.set_value_as_int64("msgId", msgId_);
        _coll.set_value_as_int("type", int(type_));
        _coll.set_value_as_qstring("data", data_);
    }

    bool SmartreplySuggest::isStickerType() const noexcept
    {
        return type_ == core::smartreply::type::sticker || type_ == core::smartreply::type::sticker_by_text;
    }

    bool SmartreplySuggest::isTextType() const noexcept
    {
        return type_ == core::smartreply::type::text;
    }
}
