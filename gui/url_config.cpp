#include "stdafx.h"
#include "url_config.h"

#include "utils/gui_coll_helper.h"
#include "../common.shared/config/config.h"

namespace Ui
{
    void UrlConfig::updateUrls(const core::coll_helper& _coll)
    {
        filesParse_ = _coll.get<QString>("filesParse");
        stickerShare_= _coll.get<QString>("stickerShare");
        profile_ = _coll.get<QString>("profile");

        mailAuth_ = _coll.get<QString>("mailAuth");
        mailRedirect_ = _coll.get<QString>("mailRedirect");
        mailWin_ = _coll.get<QString>("mailWin");
        mailRead_ = _coll.get<QString>("mailRead");
    }

    bool UrlConfig::isMailConfigPresent() const
    {
        if (!config::get().is_on(config::features::external_url_config))
            return true;

        return !mailAuth_.isEmpty();
    }

    UrlConfig& getUrlConfig()
    {
        static UrlConfig c;
        return c;
    }
}