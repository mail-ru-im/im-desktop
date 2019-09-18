#include "stdafx.h"
#include "mention_suggest.h"

#include "../utils/gui_coll_helper.h"

namespace Logic
{
    MentionSuggest::MentionSuggest(const QString& _aimid, const QString& _friendly, const QString& _highlight, const QString& _nick)
        : aimId_(_aimid)
        , friendlyName_(_friendly)
        , nick_(_nick)
        , highlight_(_highlight)
    {
        checkNick();
    }

    void MentionSuggest::unserialize(core::coll_helper& _coll)
    {
        aimId_ = _coll.get<QString>("aimid");
        friendlyName_ = _coll.get<QString>("friendly");
        nick_ = _coll.get<QString>("nick", "");
        checkNick();
    }

    bool MentionSuggest::isServiceItem() const
    {
        return aimId_.startsWith(ql1c('~'));
    }

    void MentionSuggest::checkNick()
    {
        if (aimId_.contains(ql1c('@')))
            nick_ = aimId_;
    }
}
