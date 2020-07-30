#include "stdafx.h"

#include "UrlParser.h"
#include "utils.h"
#include "../common.shared/config/config.h"
#include "features.h"
#include "url_config.h"

namespace
{
    static int32_t utf8_char_size(char _s)
    {
        const unsigned char b = *reinterpret_cast<unsigned char*>(&_s);

        if ((b & 0xE0) == 0xC0) return 2;
        if ((b & 0xF0) == 0xE0) return 3;
        if ((b & 0xF8) == 0xF0) return 4;
        if ((b & 0xFC) == 0xF8) return 5;
        if ((b & 0xFE) == 0xFC) return 6;

        return 1;
    }
}

Utils::UrlParser::UrlParser()
    : parser_(Ui::getUrlConfig().getUrlFilesParser().toStdString(), additionalFsUrls())
    , charsProcessed_(0)
    , forceNotUrl_(false)
{
}

void Utils::UrlParser::processImpl(const QByteArray& _utf8)
{
    int size = 0;

    int utf8counter = 0;

    for (auto c : _utf8)
    {
        ++size;

        if (utf8counter == 0)
        {
            utf8counter = utf8_char_size(c);
        }

        parser_.process(c);

        --utf8counter;

        if (utf8counter == 0)
        {
            if (parser_.skipping_chars())
                continue;

            if (parser_.has_url())
                goto finish;
        }
    }

    parser_.finish();

finish:
    const auto text = QString::fromUtf8(_utf8.constData(), size);

    if (parser_.has_url())
    {
        if (Utils::isContainsMentionLink(text))
        {
            forceNotUrl_ = true;
            charsProcessed_ = text.size();
        }
        else
        {
            const auto& url = parser_.get_url();
            charsProcessed_ = QString::fromStdString(url.original_).size();
        }
    }
    else
    {
        charsProcessed_ = text.size();
    }
}

bool Utils::UrlParser::hasUrl() const
{
    return !forceNotUrl_ && parser_.has_url();
}

const common::tools::url& Utils::UrlParser::getUrl() const
{
    assert(parser_.has_url());
    return parser_.get_url();
}

QString::size_type Utils::UrlParser::charsProcessed() const
{
    return charsProcessed_;
}

void Utils::UrlParser::reset()
{
    charsProcessed_ = 0;
    parser_.reset();
}

QString Utils::UrlParser::getFilesharingId() const
{
    if (parser_.has_url())
    {
        if (const auto& url = parser_.get_url(); url.is_filesharing())
        {
            const auto urlStr = QString::fromStdString(url.url_);

            if (const auto i = urlStr.lastIndexOf(ql1c('/')); i != -1)
                return urlStr.mid(i + 1);
        }
    }

    return QString();
}

const std::vector<std::string>& Utils::UrlParser::additionalFsUrls()
{
    const static auto urls = []() {
        const auto urls_csv = config::get().string(config::values::additional_fs_parser_urls_csv);
        const auto str = QString::fromUtf8(urls_csv.data(), urls_csv.size());
        const auto splitted = str.splitRef(ql1c(','), QString::SkipEmptyParts);
        std::vector<std::string> urls;
        urls.reserve(splitted.size());
        for (const auto& x : splitted)
            urls.push_back(x.toString().toStdString());
        return urls;
    }();
    return urls;
}