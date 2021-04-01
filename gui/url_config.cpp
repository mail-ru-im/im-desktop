#include "stdafx.h"
#include "url_config.h"

#include "utils/gui_coll_helper.h"
#include "utils/features.h"
#include "../common.shared/config/config.h"

namespace Ui
{
    void UrlConfig::updateUrls(const core::coll_helper& _coll)
    {
        baseBinary_ = _coll.get<QString>("baseBinary");
        filesParse_ = _coll.get<QString>("filesParse");
        stickerShare_= _coll.get<QString>("stickerShare");
        profile_ = _coll.get<QString>("profile");

        mailAuth_ = _coll.get<QString>("mailAuth");
        mailRedirect_ = _coll.get<QString>("mailRedirect");
        mailWin_ = _coll.get<QString>("mailWin");
        mailRead_ = _coll.get<QString>("mailRead");

        if (Features::isUpdateFromBackendEnabled())
            appUpdate_ = _coll.get<QString>("appUpdate");

        vcsUrls_ = Utils::toContainerOfString<QVector<QString>>(_coll.get_value_as_array("vcs_urls"));
        if (vcsUrls_.empty())
        {
            const auto splittedUrls = Features::getVcsRoomList().split(ql1c(';'), QString::SkipEmptyParts);

            QVector<QString> urls;
            urls.reserve(splittedUrls.size());

            for (const auto& url : splittedUrls)
                urls.push_back(url);

            vcsUrls_ = std::move(urls);
        }
    }

    const QVector<QString>& UrlConfig::getVCSUrls() const noexcept
    {
        return vcsUrls_;
    }

    bool UrlConfig::isMailConfigPresent() const noexcept
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