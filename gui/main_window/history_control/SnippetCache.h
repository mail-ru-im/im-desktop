#pragma once

#include "types/link_metadata.h"

namespace Ui
{
    class SnippetCache_p;

    class SnippetCache : public QObject
    {
        Q_OBJECT

    public:
        ~SnippetCache();

        static SnippetCache* instance();

        bool contains(const QString& _url) const;
        std::optional<Data::LinkMetadata> get(const QString& _url) const;

        void load(const QString& _url);

    private Q_SLOTS:
        void onLinkMetainfoMetaDownloaded(int64_t _seq, bool _success, Data::LinkMetadata _meta);

    private:
        SnippetCache();

        Q_DISABLE_COPY(SnippetCache);

        std::unique_ptr<SnippetCache_p> d;
    };
}
