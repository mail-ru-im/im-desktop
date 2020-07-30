#include "stdafx.h"
#include "url_config.h"

#include "utils/gui_coll_helper.h"
#include "utils/features.h"
#include "../common.shared/config/config.h"

namespace Ui
{
    template<typename T>
    static T toContainerOfString(const core::iarray* array)
    {
        T container;
        if (array)
        {
            const auto size = array->size();
            container.reserve(size);
            for (int i = 0; i < size; ++i)
                container.push_back(QString::fromUtf8(array->get_at(i)->get_as_string()));
        }
        return container;
    }

    void UrlConfig::updateUrls(const core::coll_helper& _coll)
    {
        filesParse_ = _coll.get<QString>("filesParse");
        stickerShare_= _coll.get<QString>("stickerShare");
        profile_ = _coll.get<QString>("profile");

        mailAuth_ = _coll.get<QString>("mailAuth");
        mailRedirect_ = _coll.get<QString>("mailRedirect");
        mailWin_ = _coll.get<QString>("mailWin");
        mailRead_ = _coll.get<QString>("mailRead");

        const auto array = _coll.get_value_as_array("vcs_urls");
        vcsUrls_ = toContainerOfString<QVector<QString>>(array);
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

    const QVector<QString>& UrlConfig::getVCSUrls() const
    {
        return vcsUrls_;
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