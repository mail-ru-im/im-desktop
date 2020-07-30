#include "stdafx.h"

#include "utils/gui_coll_helper.h"
#include "reactions.h"


namespace Data
{

void MessageReactionsData::serialize(core::icollection* _collection) const
{

}

void MessageReactionsData::unserialize(core::icollection* _collection)
{
    Ui::gui_coll_helper helper(_collection, false);

    if (helper.is_value_exist("exist"))
        exist_ = helper.get<bool>("exist");
}

void Reactions::Reaction::unserialize(core::icollection* _collection)
{
    Ui::gui_coll_helper helper(_collection, false);

    reaction_ = helper.get<QString>("reaction");
    count_ = helper.get_value_as_int64("count");
}

void Reactions::unserialize(core::icollection* _collection)
{
    Ui::gui_coll_helper helper(_collection, false);

    msgId_ = helper.get_value_as_int64("msg_id");
    myReaction_ = helper.get<QString>("my_reaction");

    auto reactionsArray = helper.get_value_as_array("reactions");
    reactions_.reserve(reactionsArray->size());

    for (auto i = 0; i < reactionsArray->size(); ++i)
    {
        Ui::gui_coll_helper coll_reaction(reactionsArray->get_at(i)->get_as_collection(), false);

        Reaction reaction;
        reaction.unserialize(coll_reaction.get());
        reactions_.push_back(std::move(reaction));
    }
}

bool Reactions::isEmpty() const
{
    return std::all_of(reactions_.begin(), reactions_.end(), [](const auto& _reaction) { return _reaction.count_ == 0; });
}

void ReactionsPage::Reaction::unserialize(core::icollection* _collection)
{
    Ui::gui_coll_helper helper(_collection, false);

    reaction_ = helper.get<QString>("reaction");
    contact_ = helper.get<QString>("user");
    time_ = helper.get_value_as_int64("time");
}

void ReactionsPage::unserialize(core::icollection* _collection)
{
    Ui::gui_coll_helper helper(_collection, false);

    exhausted_ = helper.get_value_as_bool("exhausted");
    newest_ = helper.get<QString>("newest");
    oldest_ = helper.get<QString>("oldest");

    auto reactionsArray = helper.get_value_as_array("reactions");
    reactions_.reserve(reactionsArray->size());

    for (auto i = 0; i < reactionsArray->size(); ++i)
    {
        Ui::gui_coll_helper coll_reaction(reactionsArray->get_at(i)->get_as_collection(), false);

        Reaction reaction;
        reaction.unserialize(coll_reaction.get());
        reactions_.push_back(std::move(reaction));
    }
}

}
