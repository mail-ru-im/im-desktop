#include "stdafx.h"

#include "../../../../corelib/enumerations.h"

#include "../../../utils/utils.h"
#include "../../../utils/features.h"

#include "../FileSharingInfo.h"

#include "ComplexMessageItem.h"
#include "FileSharingBlock.h"
#include "FileSharingUtils.h"
#include "ImagePreviewBlock.h"
#include "LinkPreviewBlock.h"
#include "PttBlock.h"
#include "TextBlock.h"
#include "DebugTextBlock.h"
#include "QuoteBlock.h"
#include "StickerBlock.h"
#include "TextChunk.h"
#include "ProfileBlock.h"

#include "ComplexMessageItemBuilder.h"
#include "../../../controls/TextUnit.h"
#include "../../../app_config.h"
#include "memory_stats/MessageItemMemMonitor.h"


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

    return { std::move(profileDomain), std::move(profileDomainAgent) };
}

struct parseResult
{
    std::list<TextChunk> chunks;
    TextChunk snippet;

    int snippetsCount = 0;
    int mediaCount = 0;
    bool linkInsideText = false;
    bool hasTrailingLink = false;
    Data::SharedContact sharedContact_;

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
        const auto f = _text.indexOf(ml) + ml.length();
        const auto l = _text.lastIndexOf(ml);
        toSkipLinks.push_back(_text.midRef(f, l - f));
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
                result.mediaCount++;

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

    return result;
}

std::vector<GenericBlock*> createBlocks(const parseResult& _parseRes, ComplexMessageItem* _parent, const int64_t _id, const int64_t _prev, bool _isForward = false)
{
    std::vector<GenericBlock*> blocks;
    blocks.reserve(_parseRes.chunks.size());

    for (const auto& chunk : _parseRes.chunks)
    {
        switch (chunk.Type_)
        {
        case TextChunk::Type::Text:
        case TextChunk::Type::GenericLink:
            if (!QStringRef(&chunk.text_).trimmed().isEmpty())
                blocks.push_back(new TextBlock(_parent, chunk.text_, _isForward ? TextRendering::EmojiSizeType::REGULAR : TextRendering::EmojiSizeType::ALLOW_BIG));
            break;

        case TextChunk::Type::ImageLink:
            blocks.push_back(new ImagePreviewBlock(_parent, chunk.text_, chunk.ImageType_));
            break;

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
        blocks.push_back(new LinkPreviewBlock(_parent, _parseRes.snippet.text_, _parseRes.linkInsideText));

    if (_parseRes.sharedContact_)
        blocks.push_back(new PhoneProfileBlock(_parent, _parseRes.sharedContact_->name_, _parseRes.sharedContact_->phone_, _parseRes.sharedContact_->sn_));

    return blocks;
}

namespace ComplexMessageItemBuilder
{
    std::unique_ptr<ComplexMessageItem> makeComplexItem(
        QWidget *_parent,
        const int64_t _id,
        const QString& _internalId,
        const QDate _date,
        const int64_t _prev,
        const QString &_text,
        const QString &_chatAimid,
        const QString &_senderAimid,
        const QString &_senderFriendly,
        const Data::QuotesVec& _quotes,
        const Data::MentionMap& _mentions,
        const HistoryControl::StickerInfoSptr& _sticker,
        const HistoryControl::FileSharingInfoSptr& _filesharing,
        const bool _isOutgoing,
        const bool _isNotAuth,
        const bool _forcePreview,
        const QString& _description,
        const QString& _url,
        const Data::SharedContact& _sharedContact)
    {
        assert(_id >= -1);
        assert(!_senderAimid.isEmpty());

        auto complexItem = std::make_unique<ComplexMessage::ComplexMessageItem>(
            _parent,
            _id,
            _prev,
            _internalId,
            _date,
            _chatAimid,
            _senderAimid,
            _senderFriendly,
            _sticker ? QT_TRANSLATE_NOOP("contact_list", "Sticker") : _text,
            _mentions,
            _isOutgoing);

        const bool allowSnippet = (_isOutgoing || !_isNotAuth) && !build::is_dit();

        const auto quotesCount = _quotes.size();
        std::vector<QuoteBlock*> quoteBlocks;
        quoteBlocks.reserve(quotesCount);

        int i = 0;
        for (auto quote: _quotes)
        {
            quote.isFirstQuote_ = (i == 0);
            quote.isLastQuote_ = (i == quotesCount - 1);
            quote.mentions_ = _mentions;
            quote.isSelectable_ = (quotesCount == 1);

            const auto& parsedQuote = parseQuote(quote, _mentions, allowSnippet, _forcePreview);
            const auto parsedBlocks = createBlocks(parsedQuote, complexItem.get(), _id, _prev, quote.isForward_);

            auto quoteBlock = new QuoteBlock(complexItem.get(), quote);

            for (auto& block : parsedBlocks)
                quoteBlock->addBlock(block);

            quoteBlocks.push_back(quoteBlock);
            i++;
        }

        std::vector<GenericBlock*> messageBlocks;
        bool hasTrailingLink = false;

        if (_sticker)
        {
            TextChunk chunk(TextChunk::Type::Sticker, _text, QString(), -1);
            chunk.Sticker_ = _sticker;

            parseResult res;
            res.chunks.push_back(std::move(chunk));
            messageBlocks = createBlocks(res, complexItem.get(), _id, _prev);
        }
        else if (_sharedContact)
        {
            messageBlocks.push_back(new PhoneProfileBlock(complexItem.get(), _sharedContact->name_, _sharedContact->phone_, _sharedContact->sn_));
        }
        else if (_filesharing && !_filesharing->HasUri())
        {
            auto type = TextChunk::Type::FileSharingGeneral;
            if (_filesharing->getContentType().type_ == core::file_sharing_base_content_type::undefined && !_filesharing->GetLocalPath().isEmpty())
            {
                const static auto gifExt = ql1s("gif");
                const auto ext = QFileInfo(_filesharing->GetLocalPath()).suffix();
                if (ext.compare(gifExt, Qt::CaseInsensitive) == 0)
                    type = TextChunk::Type::FileSharingGif;
                else if (Utils::is_image_extension(ext))
                    type = TextChunk::Type::FileSharingImage;
                else if (Utils::is_video_extension(ext))
                    type = TextChunk::Type::FileSharingVideo;
            }
            else
            {
                switch (_filesharing->getContentType().type_)
                {
                case core::file_sharing_base_content_type::image:
                    type = _filesharing->getContentType().is_sticker() ? TextChunk::Type::FileSharingImageSticker : TextChunk::Type::FileSharingImage;
                    break;
                case core::file_sharing_base_content_type::gif:
                    type = _filesharing->getContentType().is_sticker() ? TextChunk::Type::FileSharingGifSticker : TextChunk::Type::FileSharingGif;
                    break;
                case core::file_sharing_base_content_type::video:
                    type = _filesharing->getContentType().is_sticker() ? TextChunk::Type::FileSharingVideoSticker : TextChunk::Type::FileSharingVideo;
                    break;
                case core::file_sharing_base_content_type::ptt:
                    type = TextChunk::Type::FileSharingPtt;
                    break;
                default:
                    break;
                }
            }

            TextChunk chunk(type, _text, QString(), -1);
            chunk.FsInfo_ = _filesharing;

            parseResult res;
            res.chunks.push_back(std::move(chunk));

            if (!_description.isEmpty())
                res.chunks.push_back(TextChunk(TextChunk::Type::Text, _description, QString(), -1));

            messageBlocks = createBlocks(res, complexItem.get(), _id, _prev);
        }
        else
        {
            if (!_description.isEmpty())
            {
                auto parsedMsg = parseText(_url, allowSnippet, _forcePreview);
                parsedMsg.chunks.push_back(TextChunk(TextChunk::Type::Text, _description, QString(), -1));
                messageBlocks = createBlocks(parsedMsg, complexItem.get(), _id, _prev);
                hasTrailingLink = parsedMsg.hasTrailingLink;
            }
            else
            {
                const auto parsedMsg = parseText(_text, allowSnippet, _forcePreview);

                messageBlocks = createBlocks(parsedMsg, complexItem.get(), _id, _prev);
                hasTrailingLink = parsedMsg.hasTrailingLink;
            }
        }

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
            items.insert(items.begin(), new DebugTextBlock(complexItem.get(), _id, DebugTextBlock::Subtype::MessageId));

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

        complexItem->setUrlAndDescription(_url, _description);

        if (GetAppConfig().WatchGuiMemoryEnabled())
            MessageItemMemMonitor::instance().watchComplexMsgItem(complexItem.get());

        return complexItem;
    }
}

UI_COMPLEX_MESSAGE_NS_END
