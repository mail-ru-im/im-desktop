#include "stat_utils.h"
#include "core_dispatcher.h"
#include "main_window/contact_list/ContactListModel.h"

namespace Utils
{

void onGalleryMediaAction(bool _actionHappened, const std::string &_type, const QString& _aimid)
{
    assert(!_type.empty());
    if (!_actionHappened || _type.empty())
        return;

    Ui::GetDispatcher()->post_stats_to_core(core::stats::stats_event_names::chatgalleryscr_media_action,
        { { "category", _type }, { "chat_type", chatTypeByAimId(_aimid) } });
}

std::string chatTypeByAimId(const QString& _aimid)
{
    if (Logic::getContactListModel()->isChannel(_aimid))
        return "channel";

    if (Logic::getContactListModel()->isChat(_aimid))
        return "group";

    return "chat";
}

std::string averageCount(int count)
{
    switch (count)
    {
    case 0:
        return "0";
    case 1:
        return "1";
    case 2:
        return "2";
    case 3:
        return "3";
    case 4:
    case 5:
        return "4-5";
    case 6:
    case 7:
    case 8:
    case 9:
        return "6-9";

    default:
        return "10+";
    }
}

}
