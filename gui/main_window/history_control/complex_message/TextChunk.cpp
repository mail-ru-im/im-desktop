#include "stdafx.h"

#include "../../../../corelib/enumerations.h"

#include "../../../gui_settings.h"

#include "FileSharingUtils.h"

#include "TextChunk.h"

#include "../../../url_config.h"


const Ui::ComplexMessage::TextChunk Ui::ComplexMessage::TextChunk::Empty(Ui::ComplexMessage::TextChunk::Type::Undefined, QString(), QString(), -1);

namespace
{
    bool arePreviewsEnabled()
    {
        return Ui::get_gui_settings()->get_value<bool>(settings_show_video_and_images, true);
    }

    bool isSnippetUrl(QStringView _url)
    {
        static constexpr QStringView urls[] = { u"https://jira.mail.ru", u"jira.mail.ru", u"https://confluence.mail.ru", u"confluence.mail.ru", u"https://sys.mail.ru", u"sys.mail.ru" };
        return std::none_of(std::begin(urls), std::end(urls), [&_url](const auto& x) { return _url.startsWith(x); });
    }

    bool shouldForceSnippetUrl(QStringView _url)
    {
        static constexpr QStringView urls[] = { u"cloud.mail.ru", u"https://cloud.mail.ru" };
        return std::any_of(std::begin(urls), std::end(urls), [&_url](const auto& x) { return _url.startsWith(x); });
    }

    bool isAlwaysWithPreview(const common::tools::url& _url)
    {
        const auto isSticker =
            _url.type_ == common::tools::url::type::filesharing &&
            Ui::ComplexMessage::getFileSharingContentType(QString::fromStdString(_url.original_)).is_sticker();
        const auto isProfile = _url.type_ == common::tools::url::type::profile;
        const auto isMedia = _url.type_ == common::tools::url::type::image || _url.type_ == common::tools::url::type::video;
        return isSticker || isProfile || isMedia;
    }
}



Ui::ComplexMessage::ChunkIterator::ChunkIterator(const QString &_text)
    : ChunkIterator(_text, FixedUrls())
{
}

Ui::ComplexMessage::ChunkIterator::ChunkIterator(const QString& _text, FixedUrls&& _urls)
    : tokenizer_(_text.toStdU16String()
               , Ui::getUrlConfig().getUrlFilesParser().toStdString()
               , std::move(_urls))
{
}

Ui::ComplexMessage::ChunkIterator::ChunkIterator(const Data::FormattedString& _text, FixedUrls&& _urls)
    : tokenizer_(_text.string().toStdU16String()
               , _text.formatting()
               , Ui::getUrlConfig().getUrlFilesParser().toStdString()
               , std::move(_urls))
    , formattedText_(_text)
{
}

bool Ui::ComplexMessage::ChunkIterator::hasNext() const
{
    return tokenizer_.has_token();
}

Ui::ComplexMessage::TextChunk Ui::ComplexMessage::ChunkIterator::current(bool _allowSnipet, bool _forcePreview) const
{
    const auto& token = tokenizer_.current();

    if (token.type_ == common::tools::message_token::type::text)
    {
        const auto& text = boost::get<common::tools::tokenizer_string>(token.data_);
        auto sourceText = QString::fromStdU16String(text);
        return TextChunk(TextChunk::Type::Text, std::move(sourceText));
    }
    else if (token.type_ == common::tools::message_token::type::formatted_text)
    {
        const auto& text = boost::get<common::tools::tokenizer_string>(token.data_);
        auto sourceText = QString::fromStdU16String(text);
        const auto textSize = sourceText.size();
        im_assert(token.formatted_source_offset_ >= 0);
        im_assert(token.formatted_source_offset_ < formattedText_.size());
        auto view = formattedText_.mid(token.formatted_source_offset_, textSize);
        return TextChunk(view);
    }

    im_assert(token.type_ == common::tools::message_token::type::url);

    const auto& url = boost::get<common::tools::url>(token.data_);

    auto linkText = QString::fromStdString(url.original_);
    im_assert(!linkText.isEmpty());
    auto linkTextView = decltype(formattedText_)();

    const auto forbidPreview = url.type_ == common::tools::url::type::site && !arePreviewsEnabled();
    if (!formattedText_.isEmpty() && token.formatted_source_offset_ != -1)
    {
        const auto textSize = linkText.size();
        im_assert(token.formatted_source_offset_ >= 0);
        im_assert(token.formatted_source_offset_ + textSize <= formattedText_.size());
        linkTextView = formattedText_.mid(token.formatted_source_offset_, textSize);
        im_assert(!linkTextView.isEmpty());
    }

    if (url.type_ != common::tools::url::type::filesharing &&
        url.type_ != common::tools::url::type::image &&
        url.type_ != common::tools::url::type::email &&
        url.type_ != common::tools::url::type::profile &&
        !_allowSnipet || !isSnippetUrl(linkText) ||
        (forbidPreview && !_forcePreview && !isAlwaysWithPreview(url)))
    {
        if (linkTextView.isEmpty())
            return TextChunk(TextChunk::Type::Text, std::move(linkText));
        else
            return TextChunk(linkTextView, TextChunk::Type::GenericLink);
    }

    switch (url.type_)
    {
    case common::tools::url::type::image:
    case common::tools::url::type::video:
    {
        // spike for cloud.mail.ru, (temporary code)
        if (shouldForceSnippetUrl(linkText))
            return TextChunk(TextChunk::Type::GenericLink, std::move(linkText));

        return TextChunk(TextChunk::Type::ImageLink, std::move(linkText), QString::fromLatin1(to_string(url.extension_)), -1);
    }
    case common::tools::url::type::filesharing:
    {
        const QString id = extractIdFromFileSharingUri(linkText);
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
        case core::file_sharing_base_content_type::lottie:
            Type = TextChunk::Type::FileSharingLottieSticker;
            break;
        default:
            break;
        }

        const auto durationSec = extractDurationFromFileSharingId(id);

        return TextChunk(Type, std::move(linkText), QString(), durationSec);
    }
    case common::tools::url::type::site:
    {
        if (url.has_prtocol())
        {
            if (linkTextView.isEmpty())
                return TextChunk(TextChunk::Type::GenericLink, std::move(linkText));
            else
                return TextChunk(linkTextView, TextChunk::Type::GenericLink);
            }
            else
                return TextChunk(TextChunk::Type::Text, std::move(linkText));
        }
    case common::tools::url::type::email:
        return TextChunk(TextChunk::Type::Text, std::move(linkText));
    case common::tools::url::type::ftp:
        return TextChunk(TextChunk::Type::GenericLink, std::move(linkText));
    case common::tools::url::type::icqprotocol:
        return TextChunk(TextChunk::Type::GenericLink, std::move(linkText));
    case common::tools::url::type::profile:
        return TextChunk(TextChunk::Type::ProfileLink, std::move(linkText));
    default:
        break;
    }

    im_assert(!"invalid url type");
    return TextChunk(TextChunk::Type::Text, std::move(linkText));
}

void Ui::ComplexMessage::ChunkIterator::next()
{
    tokenizer_.next();
}

Ui::ComplexMessage::TextChunk::TextChunk()
    : Ui::ComplexMessage::TextChunk(Ui::ComplexMessage::TextChunk::Empty)
{
}

Ui::ComplexMessage::TextChunk::TextChunk(Type _type, QString _text, QString _imageType, int32_t _durationSec)
    : Type_(_type)
    , text_(std::move(_text))
    , ImageType_(std::move(_imageType))
    , DurationSec_(_durationSec)
{
    im_assert(Type_ > Type::Min);
    im_assert(Type_ < Type::Max);
    im_assert((Type_ != Type::ImageLink) || !ImageType_.isEmpty());
    im_assert(DurationSec_ >= -1);
}

Ui::ComplexMessage::TextChunk::TextChunk(const Data::FormattedStringView& _view, Type _type)
    : TextChunk(_type, _view.string().toString(), {}, -1)
{
    formattedText_ = _view;
}

int32_t Ui::ComplexMessage::TextChunk::length() const
{
    return getPlainText().length();
}

Ui::ComplexMessage::TextChunk Ui::ComplexMessage::TextChunk::mergeWith(const TextChunk& _other) const
{
    if ((Type_ == Type::FormattedText && _other.Type_ == Type::FormattedText)
     || (Type_ == Type::FormattedText && _other.Type_ == Type::GenericLink)
     || (Type_ == Type::GenericLink && _other.Type_ == Type::FormattedText))
    {
        auto text = getText();
        if (text.tryToAppend(_other.getText()))
            return text;
        return TextChunk::Empty;
    }
    else if (Type_ != Type::Text && Type_ != Type::GenericLink)
    {
        return TextChunk::Empty;
    }
    else if (_other.Type_ != Type::Text && _other.Type_ != Type::GenericLink)
    {
        return TextChunk::Empty;
    }
    else
    {
        auto textResult = getPlainText().toString();
        textResult += _other.getPlainText().toString();
        return TextChunk(Type::Text, std::move(textResult));
    }
}
