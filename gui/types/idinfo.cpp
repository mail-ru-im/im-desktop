#include "stdafx.h"

#include "utils/gui_coll_helper.h"
#include "utils/features.h"
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
            _info.firstName_ = _coll.get<QString>("first_name");
            _info.middleName_ = _coll.get<QString>("middle_name");
            _info.lastName_ = _coll.get<QString>("last_name");
        };

        if (helper.is_value_exist("user"))
        {
            auto user_helper = core::coll_helper(helper.get_value_as_collection("user"), false);
            unserializeCommon(user_helper, info);

            info.nick_ = user_helper.get<QString>("nick");
            info.isBot_ = user_helper.get_value_as_bool("bot");
            info.type_ = IdInfo::IdType::User;
        }
        else if (helper.is_value_exist("chat"))
        {
            auto chat_helper = core::coll_helper(helper.get_value_as_collection("chat"), false);
            unserializeCommon(chat_helper, info);

            info.stamp_ = chat_helper.get<QString>("stamp");
            info.memberCount_ = chat_helper.get_value_as_int64("member_count");
            info.type_ = IdInfo::IdType::Chat;
        }

        return info;
    }

    QString IdInfo::getName() const
    {
        if (isChatInfo() || Features::changeContactNamesAllowed() || firstName_.isEmpty())
            return name_;

        QStringList names;
        if (Features::leadingLastName())
            names << lastName_ << firstName_ << middleName_;
        else
            names << firstName_ << middleName_ << lastName_;
        names.removeAll({});

        return names.join(QChar::Space);
    }
} // namespace Data
