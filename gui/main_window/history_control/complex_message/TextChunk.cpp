#include "stdafx.h"

#include "../../../../corelib/enumerations.h"

#include "../../../gui_settings.h"

#include "FileSharingUtils.h"

#include "TextChunk.h"

#include "../../../url_config.h"


const Ui::ComplexMessage::TextChunk Ui::ComplexMessage::TextChunk::Empty(Ui::ComplexMessage::TextChunk::Type::Undefined, QString(), QString(), -1);

Ui::ComplexMessage::ChunkIterator::ChunkIterator(const QString &_text)
    : ChunkIterator(_text, FixedUrls())
{
}

Ui::ComplexMessage::ChunkIterator::ChunkIterator(const QString& _text, FixedUrls&& _urls)
    : tokenizer_(_text.toStdString(), Ui::getUrlConfig().getUrlFilesParser().toStdString(), std::move(_urls))
{
}

bool Ui::ComplexMessage::ChunkIterator::hasNext() const
{
    return tokenizer_.has_token();
}

static inline bool isPreviewsEnabled()
{
    return Ui::get_gui_settings()->get_value<bool>(settings_show_video_and_images, true);
}

const std::vector<QString>& getNoSnippetUrls()
{
    static const std::vector<QString> urls =
    {
        qsl("https://jira.mail.ru"),
        qsl("jira.mail.ru"),
        qsl("https://confluence.mail.ru"),
        qsl("confluence.mail.ru"),
        qsl("https://sys.mail.ru"),
        qsl("sys.mail.ru")
    };

    return urls;
}

static bool isSnippetUrl(const QString& _url)
{
    const auto& urls = getNoSnippetUrls();
    return std::none_of(std::begin(urls), std::end(urls), [&_url](const auto& x) { return _url.startsWith(x); });
}

static bool isForceSnippetUrl(const QString& _url)
{
    static const QString urls[] = { qsl("cloud.mail.ru"), qsl("https://cloud.mail.ru") };
    return std::any_of(std::begin(urls), std::end(urls), [&_url](const auto& x) { return _url.startsWith(x); });
}

static bool isAlwaysWithPreview(const common::tools::url& _url)
{
    bool isSticker =
            _url.type_ == common::tools::url::type::filesharing &&
            Ui::ComplexMessage::getFileSharingContentType(QString::fromStdString(_url.original_)).is_sticker();

    bool isProfile = _url.type_ == common::tools::url::type::profile;

    bool isMedia = _url.type_ == common::tools::url::type::image || _url.type_ == common::tools::url::type::video;

    return isSticker || isProfile || isMedia;
}

Ui::ComplexMessage::TextChunk Ui::ComplexMessage::ChunkIterator::current(bool _allowSnipet, bool _forcePreview) const
{
    const auto& token = tokenizer_.current();

    if (token.type_ == common::tools::message_token::type::text)
    {
        const auto& text = boost::get<std::string>(token.data_);
        return TextChunk(TextChunk::Type::Text, QString::fromStdString(text), QString(), -1);
    }

    assert(token.type_ == common::tools::message_token::type::url);

    const auto& url = boost::get<common::tools::url>(token.data_);

    auto text = QString::fromStdString(url.original_);

    const auto forbidPreview = url.type_ == common::tools::url::type::site && !isPreviewsEnabled();

    if (url.type_ != common::tools::url::type::filesharing &&
        url.type_ != common::tools::url::type::image &&
        url.type_ != common::tools::url::type::email &&
        url.type_ != common::tools::url::type::profile &&
        !_allowSnipet || !isSnippetUrl(text) ||
        (forbidPreview && !_forcePreview && !isAlwaysWithPreview(url)))
    {
        return TextChunk(TextChunk::Type::Text, std::move(text), QString(), -1);
    }

    switch (url.type_)
    {
    case common::tools::url::type::image:
    case common::tools::url::type::video:
    {
        // spyke for cloud.mail.ru, (temporary code)
        if (isForceSnippetUrl(text))
            return TextChunk(TextChunk::Type::GenericLink, std::move(text), QString(), -1);

        return TextChunk(TextChunk::Type::ImageLink, std::move(text), QString::fromLatin1(to_string(url.extension_)), -1);
    }
    case common::tools::url::type::filesharing:
    {
        const QString id = extractIdFromFileSharingUri(text);
        const auto content_type = extractContentTypeFromFileSharingId(id);

        auto Type = TextChunk::Type::FileSharingGeneral;
        switch (content_type.type_)
        {
        case core::file_sharing_base_content_type::image:
            Type = content_type.is_sticker() ? TextChunk::Type::FileSharingImageSticker : TextChunk::Type::FileSharingImage;
            break;
        case core::file_sharing_base_content_type::gif:
            Type = content_type.is_sticker() ? TextChunk::Type::FileSharingGifSticker : TextChunk::Type::FileSharingGif;
            break;
        case core::file_sharing_base_content_type::video:
            Type = content_type.is_sticker() ? TextChunk::Type::FileSharingVideoSticker : TextChunk::Type::FileSharingVideo;
            break;
        case core::file_sharing_base_content_type::ptt:
            Type = TextChunk::Type::FileSharingPtt;
            break;
        default:
            break;
        }

        const auto durationSec = extractDurationFromFileSharingId(id);

        return TextChunk(Type, std::move(text), QString(), durationSec);
    }
    case common::tools::url::type::site:
        {
            if (url.has_prtocol())
                return TextChunk(TextChunk::Type::GenericLink, std::move(text), QString(), -1);
            else
                return TextChunk(TextChunk::Type::Text, std::move(text), QString(), -1);
        }
    case common::tools::url::type::email:
        return TextChunk(TextChunk::Type::Text, std::move(text), QString(), -1);
    case common::tools::url::type::ftp:
        return TextChunk(TextChunk::Type::GenericLink, std::move(text), QString(), -1);
    case common::tools::url::type::icqprotocol:
        return TextChunk(TextChunk::Type::GenericLink, std::move(text), QString(), -1);
    case common::tools::url::type::profile:
        return TextChunk(TextChunk::Type::ProfileLink, std::move(text), QString(), -1);
    default:
        break;
    }

    assert(!"invalid url type");
    return TextChunk(TextChunk::Type::Text, std::move(text), QString(), -1);
}

void Ui::ComplexMessage::ChunkIterator::next()
{
    tokenizer_.next();
}

Ui::ComplexMessage::TextChunk::TextChunk()
    : Ui::ComplexMessage::TextChunk(Ui::ComplexMessage::TextChunk::Empty)
{
}

Ui::ComplexMessage::TextChunk::TextChunk(const Type type, QString _text, QString imageType, const int32_t durationSec)
    : Type_(type)
    , text_(std::move(_text))
    , ImageType_(std::move(imageType))
    , DurationSec_(durationSec)
{
    assert(Type_ > Type::Min);
    assert(Type_ < Type::Max);
    assert((Type_ != Type::ImageLink) || !ImageType_.isEmpty());
    assert(DurationSec_ >= -1);
}

int32_t Ui::ComplexMessage::TextChunk::length() const
{
    return text_.length();
}

Ui::ComplexMessage::TextChunk Ui::ComplexMessage::TextChunk::mergeWith(const TextChunk &chunk) const
{
    if (Type_ != Type::Text && Type_ != Type::GenericLink && Type_ != Type::ProfileLink)
        return TextChunk::Empty;

    if (chunk.Type_ != Type::Text && chunk.Type_ != Type::GenericLink && chunk.Type_ != Type::ProfileLink)
        return TextChunk::Empty;

    return TextChunk(Type::Text, text_ + chunk.text_, QString(), -1);
}
