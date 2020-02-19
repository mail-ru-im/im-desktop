#pragma once

#ifdef __APPLE__
#include "../../common.shared/url_parser/url_parser.h"
#else
#include "../common.shared/url_parser/url_parser.h"
#endif

namespace Utils
{
    class UrlParser
    {
    public:
        UrlParser();

        template<typename T>
        void process(T&& _text)
        {
            processImpl(std::forward<T>(_text).toUtf8());
        }

        bool hasUrl() const;
        const common::tools::url& getUrl() const;
        QString::size_type charsProcessed() const;

        void reset();

        QString getFilesharingId() const;

        static const std::vector<std::string>& additionalFsUrls();

    private:
        void processImpl(const QByteArray& _utf8);
        common::tools::url_parser parser_;
        QString::size_type charsProcessed_;
        bool forceNotUrl_;
    };
}
