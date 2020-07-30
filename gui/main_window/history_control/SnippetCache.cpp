#include "stdafx.h"

#include "core_dispatcher.h"
#include "SnippetCache.h"

#include "../../../common.shared/config/config.h"

namespace Ui
{

class SnippetCache_p
{
public:
    QCache<QString, Data::LinkMetadata> cache;
    std::map<int64_t, QString> requests;
};

SnippetCache::SnippetCache()
    : d(std::make_unique<SnippetCache_p>())
{
    d->cache.setMaxCost(10);
    connect(GetDispatcher(), &core_dispatcher::linkMetainfoMetaDownloaded, this, &SnippetCache::onLinkMetainfoMetaDownloaded);
}

SnippetCache::~SnippetCache() = default;

SnippetCache* SnippetCache::instance()
{
    static SnippetCache cache;
    return &cache;
}

bool SnippetCache::contains(const QString& _url) const
{
    return d->cache[_url];
}

std::optional<Data::LinkMetadata> SnippetCache::get(const QString& _url) const
{
    if (auto itemPtr = d->cache[_url])
        return *itemPtr;
    else
        return {};
}

void SnippetCache::load(const QString& _url)
{
    if (!config::get().is_on(config::features::snippet_in_chat))
        return;

    if (d->cache.contains(_url))
        return;

    const auto seq = GetDispatcher()->downloadLinkMetainfo(_url, core_dispatcher::LoadPreview::No);
    d->requests[seq] = _url;
}

void SnippetCache::onLinkMetainfoMetaDownloaded(int64_t _seq, bool _success, Data::LinkMetadata _meta)
{
    if (auto it = d->requests.find(_seq); it != d->requests.end())
    {
        d->cache.insert(it->second, _success ? new Data::LinkMetadata(_meta) : nullptr);
        d->requests.erase(_seq);
    }
}

}
