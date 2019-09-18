#include "stdafx.h"

#include "utils/gui_coll_helper.h"
#include "idinfo.h"

namespace Data
{
    IdInfo UnserializeIdInfo(core::coll_helper& helper)
    {
        IdInfo info;

        if (helper.is_value_exist("error"))
            return info;

        auto unserializeCommon = [](core::coll_helper _coll, IdInfo& _info)
        {
            _info.sn_ = _coll.get<QString>("sn");
            _info.name_ = _coll.get<QString>("name");
            _info.description_ = _coll.get<QString>("description");
        };

        if (helper.is_value_exist("user"))
        {
            auto user_helper = core::coll_helper(helper.get_value_as_collection("user"), false);
            unserializeCommon(user_helper, info);

            info.nick_ = user_helper.get<QString>("nick");
            info.type_ = IdInfo::IdType::User;
        }
        else if (helper.is_value_exist("chat"))
        {
            auto chat_helper = core::coll_helper(helper.get_value_as_collection("chat"), false);
            unserializeCommon(chat_helper, info);

            info.stamp_ = chat_helper.get<QString>("stamp");
            info.memberCount_ = chat_helper.get<int64_t>("member_count");
            info.type_ = IdInfo::IdType::Chat;
        }

        return info;
    }
}
