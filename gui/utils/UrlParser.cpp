#include "stdafx.h"

#include "../main_window/history_control/complex_message/FileSharingUtils.h"
#include "UrlParser.h"
#include "utils.h"
#include "../common.shared/config/config.h"
#include "../common.shared/uri_matcher/uri_matcher.h"
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

    static Utils::UrlParser::UrlCategory mapCategory(category_type c)
    {
        static constexpr std::pair<category_type, Utils::UrlParser::UrlCategory> mapping[] =
        {
            { category_type::undefined,   Utils::UrlParser::UrlCategory::Undefined   },
            { category_type::image,       Utils::UrlParser::UrlCategory::Image       },
            { category_type::video,       Utils::UrlParser::UrlCategory::Video       },
            { category_type::filesharing, Utils::UrlParser::UrlCategory::FileSharing },
            { category_type::site,        Utils::UrlParser::UrlCategory::Site        },
            { category_type::email,       Utils::UrlParser::UrlCategory::Email       },
            { category_type::ftp,         Utils::UrlParser::UrlCategory::Ftp         },
            { category_type::icqprotocol, Utils::UrlParser::UrlCategory::ImProtocol  },
            { category_type::profile,     Utils::UrlParser::UrlCategory::Profile     },
        };

        return mapping[(int)c].second;
    }

    static category_type mapCategory(Utils::UrlParser::UrlCategory c)
    {
        static constexpr std::pair<Utils::UrlParser::UrlCategory, category_type> mapping[] =
        {
            { Utils::UrlParser::UrlCategory::Undefined  , category_type::undefined    },
            { Utils::UrlParser::UrlCategory::Image      , category_type::image        },
            { Utils::UrlParser::UrlCategory::Video      , category_type::video        },
            { Utils::UrlParser::UrlCategory::FileSharing, category_type::filesharing  },
            { Utils::UrlParser::UrlCategory::Site       , category_type::site         },
            { Utils::UrlParser::UrlCategory::Email      , category_type::email        },
            { Utils::UrlParser::UrlCategory::Ftp        , category_type::ftp          },
            { Utils::UrlParser::UrlCategory::ImProtocol , category_type::icqprotocol  },
            { Utils::UrlParser::UrlCategory::Profile    , category_type::profile      },
        };

        return mapping[(int)c].second;
    }
}

namespace Utils
{
    class UrlParserPrivate
    {
    public:
        using matcher_type = basic_uri_matcher<wchar_t>;
        using category_matcher = uri_category_matcher<wchar_t>;
        using uri_view_type = basic_uri_view<wchar_t>;
        using scheme_traits = uri_scheme_traits<wchar_t>;

        category_matcher category_matcher_;
        QString urlStr_;
        QString formattedUrl_;
        QString suffix_;
        category_type category_;
        mmedia_type media_;
        scheme_type scheme_;
        bool forceNotUrl_;

        UrlParserPrivate();

        void registerPattern(std::wstring_view _pattern, category_type _category);
        void search(const wchar_t* _unicode, size_t _length);
        void parse(int32_t _scheme, const wchar_t* _unicode, size_t _length);

        void reset();
    };

    UrlParserPrivate::UrlParserPrivate() :
        category_(category_type::undefined),
        media_(mmedia_type::unknown),
        scheme_(scheme_type::undefined),
        forceNotUrl_(false)
    { }

    void UrlParserPrivate::registerPattern(std::wstring_view _pattern, category_type _category)
    {
        category_matcher_.emplace(_pattern, _category);
    }

    void UrlParserPrivate::search(const wchar_t* _unicode, size_t _length)
    {
        scheme_ = scheme_type::undefined;
        std::wstring_view result;
        matcher_type uri_matcher;
        uri_matcher.search(_unicode, _unicode + _length, result, scheme_);
        if (result.empty())
            return; // nothing was found

        urlStr_ = QString::fromWCharArray(result.data(), result.size());
        forceNotUrl_ = Utils::doesContainMentionLink(urlStr_);
        if (forceNotUrl_)
        {
            reset();
            return;
        }
        
        uri_view_type view(scheme_, result);
        category_ = category_matcher_.find_category(view);

        auto path_suffix = view.path_suffix();
        if (category_ == category_type::image || category_ == category_type::video && !path_suffix.empty())
            suffix_ = QString::fromWCharArray(path_suffix.data(), path_suffix.size());

        std::wstring formatted;
        if (category_ == category_type::email && scheme_ == scheme_type::undefined)
        {
            formatted = scheme_traits::signature(scheme_type::email);
            formatted.reserve(formatted.size() + view.length(authority_component));
            view.format(std::back_inserter(formatted), authority_component);
        }
        else
        {
            formatted = view.to_string<std::wstring>();
        }
        formattedUrl_ = QString::fromStdWString(formatted);
    }

    void UrlParserPrivate::parse(int32_t _scheme, const wchar_t* _unicode, size_t _length)
    {
        scheme_ = (scheme_type)_scheme;

        uri_view_type view(static_cast<scheme_type>(_scheme), _unicode, _length);
        category_ = category_matcher_.find_category(view);
        auto path_suffix = view.path_suffix();
        if (category_ == category_type::image || category_ == category_type::video && !path_suffix.empty())
            suffix_ = QString::fromWCharArray(path_suffix.data(), path_suffix.size());

        urlStr_ = QString::fromWCharArray(_unicode, _length);
        std::wstring formatted;
        if (category_ == category_type::email && scheme_ == scheme_type::undefined)
        {
            formatted = scheme_traits::signature(scheme_type::email);
            formatted.reserve(formatted.size() + view.length(authority_component));
            view.format(std::back_inserter(formatted), authority_component);
        }
        else
        {
            formatted = view.to_string<std::wstring>();
        }
        formattedUrl_ = QString::fromStdWString(formatted);
    }

    void UrlParserPrivate::reset()
    {
        scheme_ = scheme_type::undefined;
        category_ = category_type::undefined;
        forceNotUrl_ = false;
        urlStr_.clear();
        formattedUrl_.clear();
    }

    UrlParser::UrlParser()
        : d(std::make_unique<UrlParserPrivate>())
    {
        registerUrlPattern(Ui::getUrlConfig().getUrlFilesParser());
        const auto& extraUrls = Ui::getUrlConfig().getFsUrls();
        registerUrlPatterns(extraUrls.cbegin(), extraUrls.cend());
    }

    UrlParser::~UrlParser()
    {
    }

    void UrlParser::processImpl(const wchar_t* _unicode, size_t _length)
    {
        d->reset();
        d->search(_unicode, _length);
    }

    void UrlParser::setUrl(int32_t _scheme_hint, std::wstring_view _text)
    {
        d->reset();
        d->parse(_scheme_hint, _text.data(), _text.size());
    }

    void UrlParser::registerUrlPattern(const QString& _pattern, UrlCategory category)
    {
        d->registerPattern(_pattern.toStdWString(), mapCategory(category));
    }

    bool UrlParser::hasScheme() const
    {
        return d->scheme_ != scheme_type::undefined;
    }

    bool UrlParser::hasUrl() const
    {
        return !d->forceNotUrl_ && !d->urlStr_.isEmpty();
    }

    QString UrlParser::rawUrlString() const
    {
        im_assert(!d->urlStr_.isEmpty());
        return d->urlStr_;
    }

    QString UrlParser::formattedUrl() const
    {
        im_assert(!d->formattedUrl_.isEmpty());
        return d->formattedUrl_;
    }

    void UrlParser::reset()
    {
        d->reset();
    }

    FileSharingId UrlParser::getFilesharingId() const
    {
        if (!d->urlStr_.isEmpty() && d->category_ == category_type::filesharing)
        {
            if (const auto lastSlash = d->formattedUrl_.lastIndexOf(ql1c('/')); lastSlash != -1)
            {
                const auto lastQuestion = d->formattedUrl_.lastIndexOf(ql1c('?'));
                const auto lastEqual = d->formattedUrl_.lastIndexOf(ql1c('='));
                const auto idLength = lastQuestion == -1 ? -1 : (lastQuestion - lastSlash - 1);
                return { d->formattedUrl_.mid(lastSlash + 1, idLength), (lastEqual == -1) ? std::nullopt : std::make_optional(d->formattedUrl_.mid(lastEqual + 1)) };
            }
        }
        return {};
    }

    QString UrlParser::fileSuffix() const
    {
        return d->suffix_;
    }

    UrlParser::UrlCategory UrlParser::category() const
    {
        return mapCategory(d->category_);
    }

    bool UrlParser::isFileSharing() const
    {
        return d->category_ == category_type::filesharing;
    }

    bool UrlParser::isMedia() const
    {
        return isImage() || isVideo();
    }

    bool UrlParser::isImage() const
    {
        return d->category_ == category_type::image;
    }

    bool UrlParser::isVideo() const
    {
        return d->category_ == category_type::video;
    }

    bool UrlParser::isSite() const
    {
        return d->category_ == category_type::site;
    }

    bool UrlParser::isEmail() const
    {
        return d->category_ == category_type::email;
    }

    bool UrlParser::isFtp() const
    {
        return d->category_ == category_type::ftp;
    }

    bool UrlParser::isProfile() const
    {
        return d->category_ == category_type::profile;
    }

}