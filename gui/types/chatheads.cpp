#include "chatheads.h"

#include "../../corelib/collection_helper.h"
#include "../main_window/friendly/FriendlyContainer.h"

namespace Data
{
    ChatHeads UnserializeChatHeads(core::coll_helper* helper)
    {
        ChatHeads heads;
        heads.aimId_ = QString::fromUtf8(helper->get_value_as_string("sn"));
        heads.resetState_ = helper->get_value_as_bool("reset_state");
        const auto headsArray = helper->get_value_as_array("heads");
        for (core::iarray::size_type i = 0, size = headsArray->size(); i < size; ++i)
        {
            core::coll_helper value(headsArray->get_at(i)->get_as_collection(), false);
            const auto msgId = value.get_value_as_int64("msgId");
            const auto peopleArray = value.get_value_as_array("people");
            const auto peopleSize = peopleArray->size();
            QVector<ChatHead> people;
            people.reserve(peopleSize);
            for (core::iarray::size_type j = 0; j < peopleSize; ++j)
            {
                ChatHead h;
                h.aimid_ = QString::fromUtf8(peopleArray->get_at(j)->get_as_string());
                h.friendly_ = Logic::GetFriendlyContainer()->getFriendly(h.aimid_);

                people.push_back(std::move(h));
            }

            if (!people.isEmpty())
                heads.heads_[msgId] = std::move(people);
        }

        return heads;
    }

    bool isSameHeads(const HeadsVector& _first, const HeadsVector& _second)
    {
        return
            (_first.isEmpty() && _second.isEmpty()) ||
            (!_second.isEmpty() && std::all_of(_second.begin(), _second.end(), [&_first](const auto& _h) { return _first.contains(_h); }));
    }
}