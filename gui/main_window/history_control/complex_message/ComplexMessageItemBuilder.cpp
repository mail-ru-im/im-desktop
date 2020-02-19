#include "stdafx.h"

#include "../common.shared/config/config.h"
#include "../../../../corelib/enumerations.h"

#include "../../../utils/utils.h"
#include "../../../utils/UrlParser.h"
#include "../../../utils/features.h"

#include "../FileSharingInfo.h"

#include "ComplexMessageItem.h"
#include "FileSharingBlock.h"
#include "FileSharingUtils.h"
#include "PttBlock.h"
#include "TextBlock.h"
#include "DebugTextBlock.h"
#include "QuoteBlock.h"
#include "StickerBlock.h"
#include "TextChunk.h"
#include "ProfileBlock.h"
#include "GeolocationBlock.h"
#include "PollBlock.h"
#include "SnippetBlock.h"

#include "ComplexMessageItemBuilder.h"
#include "../../../controls/TextUnit.h"
#include "../../../app_config.h"
#include "memory_stats/MessageItemMemMonitor.h"
#include "main_window/contact_list/RecentsModel.h"


UI_COMPLEX_MESSAGE_NS_BEGIN

core::file_sharing_content_type getFileSharingContentType(const TextChunk::Type _type)
{
    switch (_type)
    {
    case TextChunk::Type::FileSharingImage:
        return core::file_sharing_base_content_type::image;

    case TextChunk::Type::FileSharingImageSticker:
        return core::file_sharing_content_type(core::file_sharing_base_content_type::image, core::file_sharing_sub_content_type::sticker);

    case TextChunk::Type::FileSharingGif:
        return core::file_sharing_base_content_type::gif;

    case TextChunk::Type::FileSharingGifSticker:
        return core::file_sharing_content_type(core::file_sharing_base_content_type::gif, core::file_sharing_sub_content_type::sticker);

    case TextChunk::Type::FileSharingVideo:
        return core::file_sharing_base_content_type::video;

    case TextChunk::Type::FileSharingVideoSticker:
        return core::file_sharing_content_type(core::file_sharing_base_content_type::video, core::file_sharing_sub_content_type::sticker);

    default:
        break;
    }

    return core::file_sharing_content_type();
}

FixedUrls getFixedUrls()
{
    common::tools::url_parser::compare_item profileDomain;
    profileDomain.str = Features::getProfileDomain().toStdString() + '/';
    profileDomain.ok_state = common::tools::url_parser::states::profile_id;
    profileDomain.safe_pos = profileDomain.str.length() - 1;

    common::tools::url_parser::compare_item profileDomainAgent;
    profileDomainAgent.str = Features::getProfileDomainAgent().toStdString() + '/';
    profileDomainAgent.ok_state = common::tools::url_parser::states::profile_id;
    profileDomainAgent.safe_pos = profileDomainAgent.str.length() - 1;

    const auto& additional_fs_urls = Utils::UrlParser::additionalFsUrls();

    auto makeProfileItem = [](std::string&& domain)
    {
        common::tools::url_parser::compare_item profileDomain;
        profileDomain.str = std::move(domain) + '/';
        profileDomain.ok_state = common::tools::url_parser::states::profile_id;
        profileDomain.safe_pos = profileDomain.str.size() - 1;
        return profileDomain;
    };

    auto makeFsItem = [](const std::string& url)
    {
        constexpr std::string_view get = "/get";

        common::tools::url_parser::compare_item profileDomain;
        profileDomain.str = url + '/';
        profileDomain.ok_state = common::tools::url_parser::states::filesharing_id;
        profileDomain.safe_pos = url.size() - get.size() - 1;
        return profileDomain;
    };

    FixedUrls res;
    res.reserve(2 + additional_fs_urls.size());
    res.push_back(makeProfileItem(Features::getProfileDomain().toStdString()));
    res.push_back(makeProfileItem(Features::getProfileDomainAgent().toStdString()));

    for (const auto& x : additional_fs_urls)
        res.push_back(makeFsItem(x));

    return res;
}

QString convertPollQuoteText(const QString& _text) // replace media with placeholders
{
    QString result;
    ChunkIterator it(_text, getFixedUrls());
    while (it.hasNext())
    {
        auto chunk = it.current(true, false);

        switch (chunk.Type_)
        {
            case TextChunk::Type::FileSharingImage:
                result += QT_TRANSLATE_NOOP("poll_block", "Photo");
                break;
            case TextChunk::Type::FileSharingGif:
                result += QT_TRANSLATE_NOOP("poll_block", "GIF");
                break;
            case TextChunk::Type::FileSharingGifSticker:
            case TextChunk::Type::FileSharingImageSticker:
            case TextChunk::Type::FileSharingVideoSticker:
                result += QT_TRANSLATE_NOOP("poll_block", "Sticker");
                break;
            case TextChunk::Type::FileSharingVideo:
                result += QT_TRANSLATE_NOOP("poll_block", "Video");
                break;
            case TextChunk::Type::FileSharingPtt:
                result += QT_TRANSLATE_NOOP("poll_block", "Voice message");
                break;
            case TextChunk::Type::FileSharingUpload:
            case TextChunk::Type::FileSharingGeneral:
                result += QT_TRANSLATE_NOOP("poll_block", "File");
                break;
            case TextChunk::Type::Text:
            case TextChunk::Type::GenericLink:
            case TextChunk::Type::ProfileLink:
            case TextChunk::Type::ImageLink:
            case TextChunk::Type::Junk:
                if (chunk.text_.startsWith(ql1c('\n')))
                    result += QChar::LineFeed;
                else if(chunk.text_.startsWith(ql1c(' ')))
                    result += QChar::Space;

                result += chunk.text_.trimmed();
                break;

            default:
                assert(!"unknown chunk type");
                result += chunk.text_;
                break;
        }

        if (chunk.text_.endsWith(ql1c('\n')))
            result += QChar::LineFeed;

        it.next();
    }

    return result;
}

SnippetBlock::EstimatedType estimatedTypeFromExtension(const QString& _extension)
{
    if (Utils::is_video_extension(_extension))
        return SnippetBlock::EstimatedType::Video;
    else if (Utils::is_image_extension_not_gif(_extension))
        return SnippetBlock::EstimatedType::Image;
    else if (Utils::is_image_extension(_extension))
        return SnippetBlock::EstimatedType::GIF;

    return SnippetBlock::EstimatedType::Article;
}

struct parseResult
{
    std::list<TextChunk> chunks;
    TextChunk snippet;

    QString sourceText_;
    int snippetsCount = 0;
    int mediaCount = 0;
    bool linkInsideText = false;
    bool hasTrailingLink = false;
    Data::SharedContact sharedContact_;
    Data::Geo geo_;
    Data::Poll poll_;

    void mergeChunks()
    {
        for (auto iter = chunks.begin(); iter != chunks.end(); )
        {
            const auto next = std::next(iter);

            if (next == chunks.end())
                break;

            auto merged = iter->mergeWith(*next);
            if (merged.Type_ != TextChunk::Type::Undefined)
            {
                chunks.erase(iter);
                *next = std::move(merged);
            }

            iter = next;
        }
    }
};

parseResult parseText(const QString& _text, const bool _allowSnippet, const bool _forcePreview)
{
    parseResult result;

    static const auto sl = ql1c('`');
    static const auto ml = qsl("```");

    std::vector<QStringRef> toSkipLinks;
    const auto mcount = _text.count(ml);
    const auto scount = _text.count(sl);
    if (mcount > 1)
    {
        auto idxBeg = 0;
        while (idxBeg < _text.size())
        {
            const auto openingMD = idxBeg + _text.midRef(idxBeg).indexOf(ml) + ml.size();
            const auto closingMD = openingMD + _text.midRef(openingMD).indexOf(ml);
            if (openingMD != -1 && closingMD != -1)
            {
                toSkipLinks.push_back(_text.midRef(openingMD, closingMD - openingMD + idxBeg));
                idxBeg += closingMD + ml.size() + 1;
            }
            else
            {
                idxBeg = openingMD + 1;
            }
        }
    }
    if (scount > 1)
    {
        const auto lines = _text.splitRef(QChar::LineFeed);
        for (const auto& l : lines)
        {
            auto first = -1, i = 0;
            const auto words = l.split(QChar::Space);
            for (const auto& w : words)
            {
                const auto startsWith = w.startsWith(sl);
                const auto endsWith = w.endsWith(sl);
                if (startsWith || endsWith)
                {
                    if (first == -1)
                    {
                        if (startsWith && endsWith && w.size() > 1)
                            toSkipLinks.push_back(w);
                        else
                            first = i;
                    }
                    else
                    {
                        for (auto j = first; j <= i; ++j)
                            toSkipLinks.push_back(words.at(j));

                        first = -1;
                    }
                }
                ++i;
            }
        }
    }

    ChunkIterator it(_text, getFixedUrls());
    while (it.hasNext())
    {
        auto chunk = it.current(_allowSnippet, _forcePreview);
        if (!toSkipLinks.empty() && chunk.Type_ != TextChunk::Type::Text)
        {
            for (const auto& m : std::as_const(toSkipLinks))
            {
                if (m.contains(chunk.text_))
                {
                    chunk.Type_ = TextChunk::Type::Text;
                    break;
                }
            }
        }

        if (chunk.Type_ == TextChunk::Type::GenericLink)
        {
            ++result.snippetsCount;
            if (result.snippetsCount == 1)
                result.snippet = chunk;

            // append link if either there is more than one snippet, link is not last or message contains media
            it.next();
            if (it.hasNext() || result.snippetsCount > 1 || result.mediaCount > 0)
            {
                result.linkInsideText = true;
                result.chunks.emplace_back(std::move(chunk));
            }

            if (!it.hasNext() && result.snippetsCount == 1 && result.mediaCount == 0)
                result.hasTrailingLink = true;

            continue;
        }
        else
        {
            const auto isMedia =
                chunk.Type_ == TextChunk::Type::ImageLink ||
                chunk.Type_ == TextChunk::Type::FileSharingImage ||
                chunk.Type_ == TextChunk::Type::FileSharingImageSticker ||
                chunk.Type_ == TextChunk::Type::FileSharingGif ||
                chunk.Type_ == TextChunk::Type::FileSharingGifSticker ||
                chunk.Type_ == TextChunk::Type::FileSharingVideo ||
                chunk.Type_ == TextChunk::Type::FileSharingVideoSticker;

            if (isMedia)
                ++result.mediaCount;

            if (chunk.Type_ == TextChunk::Type::ImageLink && !_allowSnippet)
                chunk.Type_ = TextChunk::Type::GenericLink;

            result.chunks.emplace_back(std::move(chunk));
        }

        it.next();
    }

    result.mergeChunks();
    return result;
}

parseResult parseQuote(const Data::Quote& _quote, const Data::MentionMap& _mentions, const bool _allowSnippet, const bool _forcePreview)
{
    parseResult result;
    const auto& text = _quote.text_;

    if (_quote.isSticker())
    {
        TextChunk chunk(TextChunk::Type::Sticker, text, QString(), -1);
        chunk.Sticker_ = HistoryControl::StickerInfo::Make(_quote.setId_, _quote.stickerId_);
        result.chunks.emplace_back(std::move(chunk));
    }
    else if (text.startsWith(ql1c('>')))
    {
        result.chunks.emplace_back(TextChunk(TextChunk::Type::Text, text, QString(), -1));
    }
    else if (!_quote.description_.isEmpty())
    {
        result = parseText(_quote.url_, _allowSnippet, _forcePreview);
        result.chunks.push_back(TextChunk(TextChunk::Type::Text, _quote.description_, QString(), -1));
    }
    else
    {
        result = parseText(text, _allowSnippet, _forcePreview);
    }

    result.sharedContact_ = _quote.sharedContact_;
    result.geo_ = _quote.geo_;
    result.poll_ = _quote.poll_;
    result.sourceText_ = text;

    return result;
}

enum class QuoteType
{
    None,
    Quote,
    Forward
};

std::vector<GenericBlock*> createBlocks(const parseResult& _parseRes, ComplexMessageItem* _parent, const int64_t _id, const int64_t _prev, QuoteType _quoteType = QuoteType::None)
{
    std::vector<GenericBlock*> blocks;
    blocks.reserve(_parseRes.chunks.size());

    const auto isQuote = _quoteType == QuoteType::Quote;
    const auto isForward = _quoteType == QuoteType::Forward;

    if (_parseRes.poll_ && !isForward)
    {
        blocks.push_back(new PollBlock(_parent, *_parseRes.poll_, convertPollQuoteText(_parseRes.sourceText_)));
        return blocks;
    }

    for (const auto& chunk : _parseRes.chunks)
    {
        switch (chunk.Type_)
        {
            case TextChunk::Type::Text:
            case TextChunk::Type::GenericLink:
            {
                if (!QStringRef(&chunk.text_).trimmed().isEmpty())
                    blocks.push_back(new TextBlock(_parent, chunk.text_, isForward ? TextRendering::EmojiSizeType::REGULAR : TextRendering::EmojiSizeType::ALLOW_BIG));
                break;
            }

            case TextChunk::Type::ImageLink:
            {
                auto block = _parent->addSnippetBlock(chunk.text_, _parseRes.linkInsideText, estimatedTypeFromExtension(chunk.ImageType_), blocks.size(), isQuote || isForward);
                if (block)
                    blocks.push_back(block);
                break;
            }

            case TextChunk::Type::FileSharingImage:
            case TextChunk::Type::FileSharingImageSticker:
            case TextChunk::Type::FileSharingGif:
            case TextChunk::Type::FileSharingGifSticker:
            case TextChunk::Type::FileSharingVideo:
            case TextChunk::Type::FileSharingVideoSticker:
            case TextChunk::Type::FileSharingGeneral:
                blocks.push_back(chunk.FsInfo_ ? new FileSharingBlock(_parent, chunk.FsInfo_, getFileSharingContentType(chunk.Type_))
                    : new FileSharingBlock(_parent, chunk.text_, getFileSharingContentType(chunk.Type_)));
                break;

            case TextChunk::Type::FileSharingPtt:
                blocks.push_back(chunk.FsInfo_ ? new PttBlock(_parent, chunk.FsInfo_, chunk.FsInfo_->duration() ? int32_t(*(chunk.FsInfo_->duration())) : chunk.DurationSec_, _id, _prev)
                    : new PttBlock(_parent, chunk.text_, chunk.DurationSec_, _id, _prev));
                break;

            case TextChunk::Type::Sticker:
                blocks.push_back(new StickerBlock(_parent, chunk.Sticker_));
                break;

            case TextChunk::Type::ProfileLink:
                blocks.push_back(new ProfileBlock(_parent, chunk.text_));
                break;

            case TextChunk::Type::Junk:
                break;

            default:
                assert(!"unexpected chunk type");
                break;
        }
    }

    // create snippet
    if (_parseRes.snippetsCount == 1 && _parseRes.mediaCount == 0)
    {
        if (_parseRes.geo_)
        {
            blocks.push_back(new GeolocationBlock(_parent, _parseRes.snippet.text_));
        }
        else
        {
            auto block = _parent->addSnippetBlock(_parseRes.snippet.text_, _parseRes.linkInsideText, SnippetBlock::EstimatedType::Article, blocks.size(), isQuote || isForward);
            if (block)
                blocks.push_back(block);
        }
    }

    if (_parseRes.sharedContact_)
        blocks.push_back(new PhoneProfileBlock(_parent, _parseRes.sharedContact_->name_, _parseRes.sharedContact_->phone_, _parseRes.sharedContact_->sn_));

    if (_parseRes.poll_)
        blocks.push_back(new PollBlock(_parent, *_parseRes.poll_, convertPollQuoteText(_parseRes.sourceText_)));

    return blocks;
}

namespace ComplexMessageItemBuilder
{
    std::unique_ptr<ComplexMessageItem> makeComplexItem(QWidget* _parent, const Data::MessageBuddy& _msg, ForcePreview _forcePreview)
    {
        auto complexItem = std::make_unique<ComplexMessage::ComplexMessageItem>(_parent, _msg);

        const auto isNotAuth = Logic::getRecentsModel()->isSuspicious(_msg.AimId_);
        const auto isOutgoing = _msg.IsOutgoing();
        const auto& text = _msg.GetText();
        const auto& description = _msg.GetDescription();
        const auto& mentions = _msg.Mentions_;
        const auto& quotes = _msg.Quotes_;
        const auto& id = _msg.Id_;
        const auto& prev = _msg.Prev_;
        const auto& filesharing = _msg.GetFileSharing();
        const auto sticker = _msg.GetSticker();

        const auto mediaType = complexItem->getMediaType();
        const bool allowSnippet = config::get().is_on(config::features::snippet_in_chat)
            && (isOutgoing || !isNotAuth || mediaType == Ui::MediaType::mediaTypePhoto);

        const auto quotesCount = quotes.size();
        std::vector<QuoteBlock*> quoteBlocks;
        quoteBlocks.reserve(quotesCount);

        int i = 0;
        for (auto quote: quotes)
        {
            quote.isFirstQuote_ = (i == 0);
            quote.isLastQuote_ = (i == quotesCount - 1);
            quote.mentions_ = mentions;
            quote.isSelectable_ = (quotesCount == 1);

            const auto& parsedQuote = parseQuote(quote, mentions, allowSnippet, _forcePreview == ForcePreview::Yes);
            const auto parsedBlocks = createBlocks(parsedQuote, complexItem.get(), id, prev, quote.isForward_ ? QuoteType::Forward : QuoteType::Quote);

            auto quoteBlock = new QuoteBlock(complexItem.get(), quote);

            for (auto& block : parsedBlocks)
                quoteBlock->addBlock(block);

            quoteBlocks.push_back(quoteBlock);
            i++;
        }

        std::vector<GenericBlock*> messageBlocks;
        bool hasTrailingLink = false;
        bool hasLinkInText = false;

        if (sticker)
        {
            TextChunk chunk(TextChunk::Type::Sticker, text, QString(), -1);
            chunk.Sticker_ = sticker;

            parseResult res;
            res.chunks.push_back(std::move(chunk));
            messageBlocks = createBlocks(res, complexItem.get(), id, prev);
        }
        else if (_msg.sharedContact_)
        {
            messageBlocks.push_back(new PhoneProfileBlock(complexItem.get(), _msg.sharedContact_->name_, _msg.sharedContact_->phone_, _msg.sharedContact_->sn_));
        }
        else if (_msg.geo_)
        {
            messageBlocks.push_back(new GeolocationBlock(complexItem.get(), text));
        }
        else if (filesharing && !filesharing->HasUri())
        {
            auto type = TextChunk::Type::FileSharingGeneral;
            if (filesharing->getContentType().type_ == core::file_sharing_base_content_type::undefined && !filesharing->GetLocalPath().isEmpty())
            {
                const static auto gifExt = ql1s("gif");
                const auto ext = QFileInfo(filesharing->GetLocalPath()).suffix();
                if (ext.compare(gifExt, Qt::CaseInsensitive) == 0)
                    type = TextChunk::Type::FileSharingGif;
                else if (Utils::is_image_extension(ext))
                    type = TextChunk::Type::FileSharingImage;
                else if (Utils::is_video_extension(ext))
                    type = TextChunk::Type::FileSharingVideo;
            }
            else
            {
                switch (filesharing->getContentType().type_)
                {
                case core::file_sharing_base_content_type::image:
                    type = filesharing->getContentType().is_sticker() ? TextChunk::Type::FileSharingImageSticker : TextChunk::Type::FileSharingImage;
                    break;
                case core::file_sharing_base_content_type::gif:
                    type = filesharing->getContentType().is_sticker() ? TextChunk::Type::FileSharingGifSticker : TextChunk::Type::FileSharingGif;
                    break;
                case core::file_sharing_base_content_type::video:
                    type = filesharing->getContentType().is_sticker() ? TextChunk::Type::FileSharingVideoSticker : TextChunk::Type::FileSharingVideo;
                    break;
                case core::file_sharing_base_content_type::ptt:
                    type = TextChunk::Type::FileSharingPtt;
                    break;
                default:
                    break;
                }
            }

            TextChunk chunk(type, text, QString(), -1);
            chunk.FsInfo_ = filesharing;

            parseResult res;
            res.chunks.push_back(std::move(chunk));

            if (!description.isEmpty())
                res.chunks.push_back(TextChunk(TextChunk::Type::Text, description, QString(), -1));

            messageBlocks = createBlocks(res, complexItem.get(), id, prev);
        }
        else
        {
            if (!description.isEmpty())
            {
                auto parsedMsg = parseText(_msg.GetUrl(), allowSnippet, _forcePreview == ForcePreview::Yes);
                parsedMsg.chunks.push_back(TextChunk(TextChunk::Type::Text, description, QString(), -1));
                messageBlocks = createBlocks(parsedMsg, complexItem.get(), id, prev);
                hasTrailingLink = parsedMsg.hasTrailingLink;
                hasLinkInText = parsedMsg.linkInsideText;
            }
            else
            {
                const auto parsedMsg = parseText(text, allowSnippet, _forcePreview == ForcePreview::Yes);

                messageBlocks = createBlocks(parsedMsg, complexItem.get(), id, prev);
                hasTrailingLink = parsedMsg.hasTrailingLink;
                hasLinkInText = parsedMsg.linkInsideText;
            }
        }

        if (_msg.poll_)
            messageBlocks.push_back(new PollBlock(complexItem.get(), *_msg.poll_));

        if (!messageBlocks.empty())
        {
            for (auto q : quoteBlocks)
                q->setReplyBlock(messageBlocks.front());
        }

        bool appendId = GetAppConfig().IsShowMsgIdsEnabled();
        const auto blockCount = quoteBlocks.size() + messageBlocks.size() + (appendId ? 1 : 0);

        IItemBlocksVec items;
        items.reserve(blockCount);

        if (appendId)
            items.insert(items.begin(), new DebugTextBlock(complexItem.get(), id, DebugTextBlock::Subtype::MessageId));

        items.insert(items.end(), quoteBlocks.begin(), quoteBlocks.end());
        items.insert(items.end(), messageBlocks.begin(), messageBlocks.end());

        if (!quoteBlocks.empty())
        {
            QString sourceText;
            for (auto item : items)
                sourceText += item->getSourceText();

            complexItem->setSourceText(std::move(sourceText));
        }

        int index = 0;
        for (auto val : quoteBlocks)
            val->setMessagesCountAndIndex(blockCount, index++);

        complexItem->setItems(std::move(items));
        complexItem->setHasTrailingLink(hasTrailingLink);
        complexItem->setHasLinkInText(hasLinkInText);

        complexItem->setUrlAndDescription(_msg.GetUrl(), description);

        if (GetAppConfig().WatchGuiMemoryEnabled())
            MessageItemMemMonitor::instance().watchComplexMsgItem(complexItem.get());

        return complexItem;
    }

}

UI_COMPLEX_MESSAGE_NS_END
