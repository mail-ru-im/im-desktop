#include "stdafx.h"

#include "authorization.h"
#include "utils/gui_coll_helper.h"
#include "../main_window/mini_apps/MiniAppsUtils.h"

namespace Data
{
    AuthParams::AuthParams(const core::coll_helper& _coll)
        : aimsid_(QString::fromUtf8(_coll.get_value_as_string("aimsid", "")))
    {
        if (_coll.is_value_exist("miniapps"))
        {
            if (const auto arr = _coll.get_value_as_array("miniapps"))
            {
                const auto arr_size = arr->size();
                for (auto i = 0; i < arr_size; ++i)
                {
                    Ui::gui_coll_helper maColl(arr->get_at(i)->get_as_collection(), false);

                    auto id = maColl.get<QString>("id");

                    MiniAppAuthParams params;
                    params.aimsid_ = maColl.get<QString>("aimsid");

                    miniApps_[std::move(id)] = std::move(params);
                }
            }
        }
    }

    const QString& AuthParams::getMiniAppAimsid(const QString& _miniAppId, bool _canUseMainAimisid) const
    {
        if (_canUseMainAimisid && Utils::MiniApps::isAppUseMainAimsid(_miniAppId))
            return aimsid_;

        if (auto it = miniApps_.find(_miniAppId); it != miniApps_.end())
            return it->second.aimsid_;

        static const QString empty;
        return empty;
    }
}

Q_DECLARE_METATYPE(Data::AuthParams);