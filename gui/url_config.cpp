#include "stdafx.h"
#include "url_config.h"

#include "utils/gui_coll_helper.h"
#include "utils/features.h"
#include "../common.shared/config/config.h"
#include "../core/core.h"

namespace Ui
{
    void UrlConfig::updateUrls(const core::coll_helper& _coll)
    {
        base_ = _coll.get<QString>("base");
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

        di_ = _coll.get<QString>("di", "");
        diDark_ = _coll.get<QString>("di_dark", "");

        tasks_ = _coll.get<QString>("tasks", "");

        calendar_ = _coll.get<QString>("calendar", "");

        mail_ = _coll.get<QString>("mail", "");

        miniappSharing_ = _coll.get<QString>("miniappSharing", "");

        tarm_mail_ = _coll.get<QString>("tarm_mail", "");

        tarm_calls_ = _coll.get<QString>("tarm_calls", "");

        tarm_cloud_ = _coll.get<QString>("tarm_cloud", "");

        configHost_ = _coll.get<QString>("config-host", "");

        avatars_ = _coll.get<QString>("avatars", "");

        authUrl_ = _coll.get<QString>("auth_url", "");

        redirectUri_ = _coll.get<QString>("redirect_uri", "");

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

    const QString& UrlConfig::getAuthUrl()
    {
        return authUrl_;
    }

    const QString& UrlConfig::getRedirectUri()
    {
        return redirectUri_;
    }

    const QVector<QString>& UrlConfig::getVCSUrls() const noexcept
    {
        return vcsUrls_;
    }

    const QVector<QString>& UrlConfig::getFsUrls() const noexcept
    {
        const static auto urls = []() {
            const auto urls_csv = config::get().string(config::values::additional_fs_parser_urls_csv);
            const auto str = QString::fromUtf8(urls_csv.data(), urls_csv.size());
            const auto splitted = str.splitRef(ql1c(','), Qt::SkipEmptyParts);
            QVector<QString> urls;
            urls.reserve(splitted.size());
            for (const auto& x : splitted)
                urls.push_back(x.toString());
            return urls;
        }();
        return urls;
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