#include "stdafx.h"
#include "core_dispatcher.h"

#include "app_config.h"
#include "url_config.h"
#include "gui_settings.h"
#include "styles/ThemesContainer.h"
#include "main_window/contact_list/RecentsModel.h"
#include "main_window/contact_list/MentionModel.h"
#include "main_window/contact_list/contact_profile.h"
#include "main_window/contact_list/ChatMembersModel.h"
#include "main_window/contact_list/IgnoreMembersModel.h"
#include "main_window/contact_list/Common.h"
#include "main_window/containers/LastseenContainer.h"
#include "main_window/ReleaseNotes.h"
#include "utils/gui_coll_helper.h"
#include "utils/InterConnector.h"
#include "utils/LoadPixmapFromDataTask.h"
#include "utils/utils.h"
#include "utils/features.h"
#include "utils/gui_metrics.h"
#include "cache/stickers/stickers.h"
#include "utils/log/log.h"
#include "../common.shared/common_defs.h"
#include "../corelib/corelib.h"
#include "../corelib/core_face.h"
#include "memory_stats/MemoryStatsRequest.h"
#include "memory_stats/MemoryStatsResponse.h"
#include "../common.shared/loader_errors.h"
#include "../common.shared/version_info.h"
#include "../common.shared/config/config.h"
#include "memory_stats/gui_memory_monitor.h"
#include "main_window/LocalPIN.h"
#include "omicron/omicron_helper.h"
#include "main_window/contact_list/FavoritesUtils.h"

#if defined(IM_AUTO_TESTING)
    #include "webserver/webserver.h"
#endif

#ifdef _WIN32
    #include "../common.shared/win32/crash_handler.h"
#endif

using namespace Ui;

namespace
{
    template<typename T>
    T toContainerOfString(const core::iarray* array)
    {
        T container;
        if (array)
        {
            const auto size = array->size();
            container.reserve(size);
            for (std::remove_const_t<decltype(size)> i = 0; i < size; ++i)
                container.push_back(QString::fromUtf8(array->get_at(i)->get_as_string()));
        }
        return container;
    }

    template<typename T>
    core::ifptr<core::iarray> toArrayOfStrings(const T& _cont, Ui::gui_coll_helper& _coll)
    {
        core::ifptr<core::iarray> arr(_coll->create_array());
        arr->reserve(_cont.size());
        for (const auto& item : _cont)
            arr->push_back(_coll.create_qstring_value(item).get());

        return arr;
    }

    using highlightsV = std::vector<QString>;
    Ui::highlightsV unserializeHighlights(const core::coll_helper& _helper)
    {
        Ui::highlightsV res;

        if (_helper->is_value_exist("highlights"))
        {
            const auto hArray = _helper.get_value_as_array("highlights");
            const auto hSize = hArray->size();
            res.reserve(hSize);

            for (std::remove_const_t<decltype(hSize)> h = 0; h < hSize; ++h)
            {
                if (auto hl = QString::fromUtf8(hArray->get_at(h)->get_as_string()).trimmed(); !hl.isEmpty())
                {
                    static const QRegularExpression re(
                        qsl("\\s+"),
                        QRegularExpression::UseUnicodePropertiesOption);

                    const auto words = hl.splitRef(re, QString::SkipEmptyParts);

                    std::vector<QStringView> wordsAndEmojis;
                    wordsAndEmojis.reserve(words.size());
                    bool hasEmoji = false;

                    for (const auto& ref : words)
                    {
                        const auto view = QStringView(ref);
                        static const auto emojiStart = QChar(0xD83D);

                        qsizetype prev = 0;
                        qsizetype idx = ref.indexOf(emojiStart, prev); // use view since 5.14
                        while (idx != -1)
                        {
                            auto next = idx;
                            const auto emoji = Emoji::getEmoji(view, next);
                            assert(!emoji.isNull());
                            if (!emoji.isNull())
                            {
                                hasEmoji = true;
                                const auto emojiSize = next - idx;
                                if (idx == 0)
                                {
                                    wordsAndEmojis.push_back(view.left(emojiSize));
                                    prev = wordsAndEmojis.back().size();
                                }
                                else
                                {
                                    wordsAndEmojis.push_back(view.mid(prev, idx - prev));
                                    wordsAndEmojis.push_back(view.mid(idx, emojiSize));
                                    prev = idx + wordsAndEmojis.back().size();
                                }
                            }
                            else
                            {
                                prev = idx + 1;
                            }

                            idx = ref.indexOf(emojiStart, prev);
                        }

                        if (idx == -1 && prev < ref.size())
                            wordsAndEmojis.push_back(view.mid(prev));
                    }

                    for (auto ref : wordsAndEmojis)
                    {
                        if (std::any_of(res.begin(), res.end(), [ref](const auto& x) { return ref.compare(x, Qt::CaseInsensitive) == 0; }))
                            continue;

                        res.push_back(ref.toString());
                    }

                    if (hasEmoji) // for search results list
                    {
                        if (std::none_of(res.begin(), res.end(), [&hl](const auto& x) { return x.compare(hl, Qt::CaseInsensitive) == 0; }))
                            res.push_back(std::move(hl));
                    }
                }
            }
        }

        return res;
    }

    std::vector<Data::SmartreplySuggest> unserializeSmartreplySuggests(const core::coll_helper& _params)
    {
        std::vector<Data::SmartreplySuggest> suggests;
        if (_params.is_value_exist("suggests"))
        {
            if (const auto suggests_array = _params.get_value_as_array("suggests"))
            {
                const auto arr_size = suggests_array->size();
                suggests.reserve(arr_size);
                for (auto i = 0; i < arr_size; ++i)
                {
                    Ui::gui_coll_helper coll_suggest(suggests_array->get_at(i)->get_as_collection(), false);

                    Data::SmartreplySuggest suggest;
                    suggest.unserialize(coll_suggest);
                    suggests.push_back(std::move(suggest));
                }
            }
        }

        return suggests;
    }

    constexpr std::chrono::milliseconds typingCheckInterval = std::chrono::seconds(1);
    constexpr auto typingLifetime = std::chrono::seconds(8);
}


int Ui::gui_connector::addref()
{
    return ++refCount_;
}

int Ui::gui_connector::release()
{
    int32_t r = (--refCount_);
    if (0 == r)
    {
        delete this;
        return 0;
    }
    return r;
}

void Ui::gui_connector::link(iconnector*, const common::core_gui_settings&)
{

}

void Ui::gui_connector::unlink()
{

}

void Ui::gui_connector::receive(std::string_view _message, int64_t _seq, core::icollection* _messageData)
{
    if (_messageData)
        _messageData->addref();

    Q_EMIT received(QString::fromLatin1(_message.data(), _message.size()), _seq, _messageData);
}

core_dispatcher::core_dispatcher()
    : coreConnector_(nullptr)
    , coreFace_(nullptr)
    , voipController_(*this)
    , guiConnector_(nullptr)
    , lastTimeCallbacksCleanedUp_(QDateTime::currentMSecsSinceEpoch())
    , isImCreated_(false)
    , userStateGoneAway_(false)
    , installBetaUpdates_(false)
    , connectionState_(Ui::ConnectionState::stateConnecting)
    , typingCheckTimer_(new QTimer(this))
{
    init();
}


core_dispatcher::~core_dispatcher()
{
    uninit();
}

voip_proxy::VoipController& core_dispatcher::getVoipController()
{
    return voipController_;
}

qint64 core_dispatcher::getFileSharingPreviewSize(const QString& _url, const int32_t _originalSize)
{
    core::coll_helper helper(create_collection(), true);
    helper.set<QString>("url", _url);
    helper.set_value_as_int("orig_size", _originalSize);

    return post_message_to_core("files/preview_size", helper.get());
}

qint64 core_dispatcher::downloadFileSharingMetainfo(const QString& _url)
{
    core::coll_helper helper(create_collection(), true);
    helper.set<QString>("url", _url);
    return post_message_to_core("files/download/metainfo", helper.get());
}

qint64 core_dispatcher::downloadSharedFile(const QString& _contactAimid, const QString& _url, bool _forceRequestMetainfo, const QString& _fileName, bool _raise_priority)
{
    core::coll_helper helper(create_collection(), true);
    helper.set<QString>("contact", _contactAimid);
    helper.set<QString>("url", _url);
    helper.set<bool>("force_request_metainfo", _forceRequestMetainfo);
    helper.set<QString>("filename", _fileName);
    helper.set<QString>("download_dir", Ui::getDownloadPath());
    helper.set<bool>("raise", _raise_priority);
    return post_message_to_core("files/download", helper.get());
}

qint64 core_dispatcher::abortSharedFileDownloading(const QString& _url, std::optional<qint64> _seq)
{
    core::coll_helper helper(create_collection(), true);
    helper.set<QString>("url", _url);
    if (_seq)
        helper.set<int64_t>("seq", *_seq);
    return post_message_to_core("files/download/abort", helper.get());
}

qint64 core_dispatcher::cancelLoaderTask(const QString& _url, std::optional<qint64> _seq)
{
    if (_url.isEmpty())
        return -1;

    core::coll_helper collection(create_collection(), true);
    collection.set<QString>("url", _url);
    if (_seq)
        collection.set<int64_t>("seq", *_seq);
    return post_message_to_core("loader/download/cancel", collection.get());
}

qint64 core_dispatcher::uploadSharedFile(const QString& _contact, const QString& _localPath, const Data::QuotesVec& _quotes, const QString& _description, const Data::MentionMap& _mentions)
{
    core::coll_helper collection(create_collection(), true);
    collection.set<QString>("contact", _contact);
    collection.set<QString>("file", _localPath);
    collection.set<QString>("description", _description);
    Data::serializeQuotes(collection, _quotes);
    Data::MentionMap mentions(_mentions);
    for (const auto& q : _quotes)
        mentions.insert(q.mentions_.begin(), q.mentions_.end());
    Data::serializeMentions(collection, mentions);

    return post_message_to_core("files/upload", collection.get());
}

qint64 core_dispatcher::uploadSharedFile(const QString& _contact, const QByteArray& _array, QStringView _ext, const Data::QuotesVec& _quotes)
{
    return uploadSharedFile(_contact, _array, _ext, _quotes, QString(), {}, std::nullopt);
}

qint64 core_dispatcher::uploadSharedFile(const QString& _contact, const QByteArray& _array, QStringView _ext, const Data::QuotesVec& _quotes, const QString& _description, const Data::MentionMap& _mentions)
{
    return uploadSharedFile(_contact, _array, _ext, _quotes, _description, _mentions, std::nullopt);
}

qint64 core_dispatcher::uploadSharedFile(const QString& _contact, const QByteArray& _array, QStringView _ext, std::optional<std::chrono::seconds> _duration)
{
    return uploadSharedFile(_contact, _array, _ext, {}, QString(), {}, _duration);
}

qint64 core_dispatcher::uploadSharedFile(const QString& _contact, const QByteArray& _array, QStringView _ext, const Data::QuotesVec& _quotes, const QString& _description, const Data::MentionMap& _mentions, std::optional<std::chrono::seconds> _duration)
{
    Ui::gui_coll_helper collection(create_collection(), true);
    collection.set<QString>("contact", _contact);

    core::istream* stream = collection->create_stream();
    stream->write((uint8_t*)(_array.data()), _array.size());
    collection.set_value_as_stream("file_stream", stream);
    collection.set_value_as_qstring("ext", _ext);

    if (_duration)
        collection.set<int64_t>("duration", (*_duration).count());

    collection.set<QString>("description", _description);
    Data::serializeQuotes(collection, _quotes);
    Data::MentionMap mentions(_mentions);
    for (const auto& q : _quotes)
        mentions.insert(q.mentions_.begin(), q.mentions_.end());
    Data::serializeMentions(collection, mentions);

    return post_message_to_core("files/upload", collection.get());
}

qint64 core_dispatcher::uploadSharedFile(const QString& _contact, const QString& _localPath, const Data::QuotesVec& _quotes, std::optional<std::chrono::seconds> _duration)
{
    core::coll_helper collection(create_collection(), true);
    collection.set<QString>("contact", _contact);
    collection.set<QString>("file", _localPath);
    collection.set<QString>("description", QString());
    if (_duration)
        collection.set<int64_t>("duration", (*_duration).count());

    Data::serializeQuotes(collection, _quotes);
    Data::MentionMap mentions;
    for (const auto& q : _quotes)
        mentions.insert(q.mentions_.begin(), q.mentions_.end());
    Data::serializeMentions(collection, mentions);

    return post_message_to_core("files/upload", collection.get());
}

qint64 core_dispatcher::abortSharedFileUploading(const QString& _contact, const QString& _localPath, const QString& _uploadingProcessId)
{
    assert(!_contact.isEmpty());
    assert(!_localPath.isEmpty());
    assert(!_uploadingProcessId.isEmpty());

    core::coll_helper helper(create_collection(), true);
    helper.set<QString>("contact", _contact);
    helper.set<QString>("local_path", _localPath);
    helper.set<QString>("process_seq", _uploadingProcessId);

    return post_message_to_core("files/upload/abort", helper.get());
}

qint64 core_dispatcher::getSticker(const qint32 _setId, const qint32 _stickerId, const core::sticker_size _size)
{
    assert(_setId > 0);
    assert(_stickerId > 0);
    assert(_size > core::sticker_size::min);
    assert(_size < core::sticker_size::max);

    core::coll_helper collection(create_collection(), true);
    collection.set_value_as_int("set_id", _setId);
    collection.set_value_as_int("sticker_id", _stickerId);
    collection.set_value_as_enum("size", _size);

    return post_message_to_core("stickers/sticker/get", collection.get());
}

qint64 core_dispatcher::getSticker(const qint32 /*_setId*/, const QString& _fsId, const core::sticker_size _size)
{
    return getSticker(_fsId, _size);
}

qint64 core_dispatcher::getSticker(const QString& _fsId, const core::sticker_size _size)
{
    assert(!_fsId.isEmpty());
    assert(_size > core::sticker_size::min);
    assert(_size < core::sticker_size::max);

    core::coll_helper collection(create_collection(), true);
    collection.set<QString>("fs_id", _fsId);
    collection.set_value_as_enum("size", _size);

    return post_message_to_core("stickers/sticker/get", collection.get());
}

qint64 core_dispatcher::getSetIconBig(const qint32 _setId)
{
    core::coll_helper collection(create_collection(), true);
    collection.set_value_as_int("set_id", _setId);

    return post_message_to_core("stickers/big_set_icon/get", collection.get());
}

qint64 core_dispatcher::cleanSetIconBig(const qint32 _setId)
{
    core::coll_helper collection(create_collection(), true);
    collection.set_value_as_int("set_id", _setId);

    return post_message_to_core("stickers/big_set_icon/clean", collection.get());
}

qint64 core_dispatcher::getStickersPackInfo(const qint32 _setId, const QString& _storeId, const QString& _fileId)
{
    core::coll_helper collection(create_collection(), true);

    collection.set_value_as_int("set_id", _setId);
    collection.set<QString>("store_id", _storeId);
    collection.set<QString>("file_id", _fileId);

    return post_message_to_core("stickers/pack/info", collection.get());
}

qint64 core_dispatcher::addStickersPack(const qint32 _setId, const QString& _storeId)
{
    core::coll_helper collection(create_collection(), true);

    collection.set_value_as_int("set_id", _setId);
    collection.set<QString>("store_id", _storeId);

    return post_message_to_core("stickers/pack/add", collection.get());
}

qint64 core_dispatcher::removeStickersPack(const qint32 _setId, const QString& _storeId)
{
    core::coll_helper collection(create_collection(), true);

    collection.set_value_as_int("set_id", _setId);
    collection.set<QString>("store_id", _storeId);

    return post_message_to_core("stickers/pack/remove", collection.get());
}

qint64 core_dispatcher::getStickersStore()
{
    return post_message_to_core("stickers/store/get", nullptr);
}

qint64 core_dispatcher::searchStickers(const QString& _searchTerm)
{
    Ui::Stickers::clean_search_cache();

    core::coll_helper collection(create_collection(), true);
    collection.set<QString>("term", _searchTerm);
    return post_message_to_core("stickers/store/search", collection.get());
}

qint64 core_dispatcher::getFsStickersByIds(const std::vector<int32_t>& _ids)
{
    core::coll_helper collection(create_collection(), true);
    core::ifptr<core::iarray> idsArray(collection->create_array());
    idsArray->reserve(_ids.size());
    for (auto x : _ids)
    {
        core::ifptr<core::ivalue> val(collection->create_value());
        val->set_as_int(x);
        idsArray->push_back(val.get());
    }

    collection.set_value_as_array("ids", idsArray.get());
    return post_message_to_core("stickers/fs_by_ids/get", collection.get());
}

int64_t core_dispatcher::downloadImage(
    const QUrl& _uri,
    const QString& _destination,
    const bool _isPreview,
    const int32_t _previewWidth,
    const int32_t _previewHeight,
    const bool _externalResource,
    const bool _raisePriority,
    const bool _withData)
{
    assert(_uri.isValid());
    assert(!_uri.isLocalFile());
//    assert(!_uri.isRelative());
    assert(_previewWidth >= 0);
    assert(_previewHeight >= 0);

    Ui::gui_coll_helper collection(create_collection(), true);
    collection.set_value_as_qstring("uri", _uri.toString());
    collection.set_value_as_qstring("destination", _destination);
    collection.set<bool>("is_preview", _isPreview);
    collection.set<bool>("ext_resource", _externalResource);
    collection.set<bool>("raise", _raisePriority);
    collection.set<bool>("with_data", _withData);

    if (_isPreview)
    {
        collection.set<int32_t>("preview_height", _previewHeight);
        collection.set<int32_t>("preview_width", _previewWidth);
    }

    const auto seq = post_message_to_core("image/download", collection.get());

    __INFO(
        "snippets",
        "GUI(1): requested image\n"
            __LOGP(seq, seq)
            __LOGP(uri, _uri)
            __LOGP(dst, _destination)
            __LOGP(preview, _isPreview)
            __LOGP(preview_width, _previewWidth)
            __LOGP(preview_height, _previewHeight));

    return seq;
}

int64_t core_dispatcher::getExternalFilePath(const QString& _url)
{
    core::coll_helper collection(create_collection(), true);

    collection.set<QString>("url", _url);

    return post_message_to_core("external_file_path/get", collection.get());
}

int64_t core_dispatcher::downloadLinkMetainfo(const QString& _uri,
    LoadPreview _loadPreview,
    const int32_t _previewWidth,
    const int32_t _previewHeight,
    const QString& _logStr)
{
    assert(!_uri.isEmpty());
    assert(_previewWidth >= 0);
    assert(_previewHeight >= 0);
    assert(config::get().is_on(config::features::snippet_in_chat));

    core::coll_helper collection(create_collection(), true);
    collection.set<QString>("uri", _uri);
    collection.set<bool>("load_preview", _loadPreview == LoadPreview::Yes);
    collection.set<int32_t>("preview_height", _previewHeight);
    collection.set<int32_t>("preview_width", _previewWidth);
    collection.set<QString>("log_str", _logStr);

    return post_message_to_core("link_metainfo/download", collection.get());
}

int64_t core_dispatcher::pttToText(const QString &_pttLink, const QString &_locale)
{
    assert(!_pttLink.isEmpty());
    assert(!_locale.isEmpty());

    Ui::gui_coll_helper collection(Ui::GetDispatcher()->create_collection(), true);

    collection.set<QString>("url", _pttLink);
    collection.set<QString>("locale", _locale);

    return post_message_to_core("files/speech_to_text", collection.get());
}

qint64 core_dispatcher::deleteMessages(const QString& _contactAimId, const std::vector<DeleteMessageInfo>& _messages)
{
    assert(!_contactAimId.isEmpty());

    Ui::gui_coll_helper params(create_collection(), true);

    core::ifptr<core::iarray> array(params->create_array());
    array->reserve(_messages.size());
    for (const auto &msg : _messages)
    {
        Ui::gui_coll_helper coll(create_collection(), true);
        coll.set_value_as_int64("id", msg.Id_);
        coll.set_value_as_qstring("internal_id", msg.InternaId_);
        coll.set_value_as_bool("for_all", msg.ForAll_);
        core::ifptr<core::ivalue> val(params->create_value());
        val->set_as_collection(coll.get());
        array->push_back(val.get());
    }

    params.set_value_as_array("messages", array.get());

    params.set<QString>("contact", _contactAimId);

    return post_message_to_core("archive/messages/delete", params.get());
}

qint64 core_dispatcher::deleteMessagesFrom(const QString& _contactAimId, const int64_t _fromId)
{
    assert(!_contactAimId.isEmpty());
    assert(_fromId >= 0);

    core::coll_helper collection(create_collection(), true);

    collection.set<QString>("contact", _contactAimId);
    collection.set<int64_t>("from_id", _fromId);

    return post_message_to_core("archive/messages/delete_from", collection.get());
}

qint64 core_dispatcher::eraseHistory(const QString& _contactAimId)
{
    assert(!_contactAimId.isEmpty());

    core::coll_helper collection(create_collection(), true);

    collection.set<QString>("contact", _contactAimId);

    return post_message_to_core("archive/messages/delete_all", collection.get());
}

qint64 core_dispatcher::raiseDownloadPriority(const QString &_contactAimid, int64_t _procId)
{
    assert(_procId > 0);
    assert(!_contactAimid.isEmpty());

    core::coll_helper collection(create_collection(), true);

    collection.set<int64_t>("proc_id", _procId);

    __TRACE(
        "prefetch",
        "requesting to raise download priority\n"
        "    contact_aimid=<" << _contactAimid << ">\n"
            "request_id=<" << _procId << ">");

    return post_message_to_core("download/raise_priority", collection.get());
}

qint64 core_dispatcher::contactSwitched(const QString &_contactAimid)
{
    core::coll_helper collection(create_collection(), true);
    collection.set<QString>("contact", _contactAimid);
    return post_message_to_core("contact/switched", collection.get());
}

void core_dispatcher::sendStickerToContact(const QString& _contact, const QString& _stickerId)
{
    assert(!_contact.isEmpty());
    assert(!_stickerId.isEmpty());

    if (_contact.isEmpty() || _stickerId.isEmpty())
        return;

    Ui::gui_coll_helper collection(GetDispatcher()->create_collection(), true);
    collection.set_value_as_qstring("contact", _contact);
    collection.set_value_as_bool("fs_sticker", true);
    collection.set_value_as_qstring("message", Stickers::getSendBaseUrl() % _stickerId);

    Ui::GetDispatcher()->post_message_to_core("send_message", collection.get());
}

void core_dispatcher::sendMessageToContact(const QString& _contact, const QString& _text, const Data::QuotesVec& _quotes, const Data::MentionMap& _mentions)
{
    assert(!_contact.isEmpty());
    assert(!_text.isEmpty());

    if (_contact.isEmpty() || (_text.isEmpty() && _quotes.empty()))
        return;

    MessageData data;
    data.mentions_ = _mentions;
    data.quotes_ = _quotes;
    data.text_ = _text;

    sendMessageToContact(_contact, std::move(data));
}

void core_dispatcher::sendMessageToContact(const QString& _contact, core_dispatcher::MessageData _data)
{
    if (_contact.isEmpty() || (_data.text_.isEmpty() && _data.quotes_.empty()))
        return;

    Ui::gui_coll_helper collection(Ui::GetDispatcher()->create_collection(), true);

    collection.set<QString>("contact", _contact);
    collection.set<QString>("message", _data.text_);

    for (auto& quote : _data.quotes_)
        quote.text_ = Utils::replaceFilesPlaceholders(quote.text_, quote.files_);

    Data::serializeQuotes(collection, _data.quotes_);
    Data::MentionMap mentions(_data.mentions_);
    for (const auto& q : _data.quotes_)
        mentions.insert(q.mentions_.begin(), q.mentions_.end());
    Data::serializeMentions(collection, mentions);

    if (_data.poll_)
        _data.poll_->serialize(collection.get());

    Ui::GetDispatcher()->post_message_to_core("send_message", collection.get());
}

void core_dispatcher::updateMessageToContact(const QString& _contact, int64_t _messageId, const QString& _text)
{
    assert(!_contact.isEmpty());
    assert(!_text.isEmpty());

    Ui::gui_coll_helper collection(Ui::GetDispatcher()->create_collection(), true);

    collection.set<QString>("contact", _contact);
    collection.set_value_as_int64("id", _messageId);
    collection.set<QString>("message", _text);

    Ui::GetDispatcher()->post_message_to_core("update_message", collection.get());
}

void core_dispatcher::sendSmartreplyToContact(const QString& _contact, const Data::SmartreplySuggest& _smartreply, const Data::QuotesVec& _quotes)
{
    assert(!_contact.isEmpty());
    if (_contact.isEmpty())
        return;

    Ui::gui_coll_helper collection(create_collection(), true);
    collection.set_value_as_qstring("contact", _contact);

    if (_smartreply.isStickerType())
    {
        collection.set_value_as_bool("fs_sticker", true);
        collection.set_value_as_qstring("message", Stickers::getSendBaseUrl() % _smartreply.getData());
    }
    else if (_smartreply.isTextType())
    {
        collection.set<QString>("message", _smartreply.getData());
    }

    Data::serializeQuotes(collection, _quotes);
    Data::MentionMap mentions;
    for (const auto& q : _quotes)
        mentions.insert(q.mentions_.begin(), q.mentions_.end());
    Data::serializeMentions(collection, mentions);

    {
        Ui::gui_coll_helper sr_helper(create_collection(), true);
        _smartreply.serialize(sr_helper);
        collection.set_value_as_collection("smartreply", sr_helper.get());
    }

    Ui::GetDispatcher()->post_message_to_core("send_message", collection.get());
}

bool core_dispatcher::init()
{
#ifndef ICQ_CORELIB_STATIC_LINKING
    const QString library_path = QApplication::applicationDirPath() % ql1c('/') % ql1s(CORELIBRARY);
    QLibrary libcore(library_path);
    if (!libcore.load())
    {
        assert(false);
        return false;
    }

    typedef bool (*get_core_instance_function)(core::icore_interface**);
    get_core_instance_function get_core_instance = (get_core_instance_function) libcore.resolve("get_core_instance");

    core::icore_interface* coreFace = nullptr;
    if (!get_core_instance(&coreFace))
    {
        assert(false);
        return false;
    }

    coreFace_ = coreFace;
#else
    core::icore_interface* coreFace = nullptr;
    if (!get_core_instance(&coreFace) || !coreFace)
    {
        assert(false);
        return false;
    }
    coreFace_ = coreFace;
#endif //ICQ_CORELIB_STATIC_LINKING
    coreConnector_ = coreFace_->get_core_connector();
    if (!coreConnector_)
        return false;

    gui_connector* connector = new gui_connector();

    QObject::connect(connector, &gui_connector::received, this, &core_dispatcher::received, Qt::QueuedConnection);

    guiConnector_ = connector;

    common::core_gui_settings settings(
        Utils::scale_bitmap(Ui::GetRecentsParams().getAvatarSize()),
        QSysInfo::productVersion().toStdString(),
        Utils::GetTranslator()->getLocaleStr().toStdString());

    coreConnector_->link(guiConnector_, settings);

    initMessageMap();

    QObject::connect(&GuiMemoryMonitor::instance(), &GuiMemoryMonitor::reportsReadyFor,
                     this, &core_dispatcher::onGuiComponentsMemoryReportsReady);

    typingCheckTimer_->setInterval(typingCheckInterval.count());
    connect(typingCheckTimer_, &QTimer::timeout, this, &core_dispatcher::onTypingCheckTimer);

    return true;
}

void core_dispatcher::initMessageMap()
{
    REGISTER_IM_MESSAGE("need_login", onNeedLogin);
    REGISTER_IM_MESSAGE("im/created", onImCreated);
    REGISTER_IM_MESSAGE("login/complete", onLoginComplete);
    REGISTER_IM_MESSAGE("contactlist", onContactList);
    REGISTER_IM_MESSAGE("contactlist/diff", onContactList);
    REGISTER_IM_MESSAGE("login_get_sms_code_result", onLoginGetSmsCodeResult);
    REGISTER_IM_MESSAGE("login_result", onLoginResult);
    REGISTER_IM_MESSAGE("avatars/get/result", onAvatarsGetResult);
    REGISTER_IM_MESSAGE("avatars/presence/updated", onAvatarsPresenceUpdated);
    REGISTER_IM_MESSAGE("contact/presence", onContactPresence);
    REGISTER_IM_MESSAGE("contact/outgoing_count", onContactOutgoingMsgCount);
    REGISTER_IM_MESSAGE("gui_settings", onGuiSettings);
    REGISTER_IM_MESSAGE("core/logins", onCoreLogins);
    REGISTER_IM_MESSAGE("archive/messages/get/result", onArchiveMessagesGetResult);
    REGISTER_IM_MESSAGE("archive/messages/pending", onArchiveMessagesPending);
    REGISTER_IM_MESSAGE("archive/buddies/get/result", onArchiveBuddiesGetResult);
    REGISTER_IM_MESSAGE("archive/mentions/get/result", onArchiveMentionsGetResult);
    REGISTER_IM_MESSAGE("messages/received/dlg_state", onMessagesReceivedDlgState);
    REGISTER_IM_MESSAGE("messages/received/server", onMessagesReceivedServer);
    REGISTER_IM_MESSAGE("messages/received/search_mode", onMessagesReceivedSearch);
    REGISTER_IM_MESSAGE("messages/received/updated", onMessagesReceivedUpdated);
    REGISTER_IM_MESSAGE("messages/received/init", onMessagesReceivedInit);
    REGISTER_IM_MESSAGE("messages/received/context", onMessagesReceivedContext);
    REGISTER_IM_MESSAGE("messages/load_after_search/error", onMessagesLoadAfterSearchError);
    REGISTER_IM_MESSAGE("messages/context/error", onMessagesContextError);
    REGISTER_IM_MESSAGE("messages/received/message_status", onMessagesReceivedMessageStatus);
    REGISTER_IM_MESSAGE("messages/del_up_to", onMessagesDelUpTo);
    REGISTER_IM_MESSAGE("messages/clear", onMessagesClear);
    REGISTER_IM_MESSAGE("messages/empty", onMessagesEmpty);
    REGISTER_IM_MESSAGE("dlg_states", onDlgStates);

    REGISTER_IM_MESSAGE("dialogs/search/local/result", onDialogsSearchLocalResults);
    REGISTER_IM_MESSAGE("dialogs/search/local/empty", onDialogsSearchLocalEmptyResult);
    REGISTER_IM_MESSAGE("dialogs/search/server/result", onDialogsSearchServerResults);

    REGISTER_IM_MESSAGE("dialogs/search/pattern_history", onDialogsSearchPatternHistory);

    REGISTER_IM_MESSAGE("contacts/search/local/result", onContactsSearchLocalResults);
    REGISTER_IM_MESSAGE("contacts/search/server/result", onContactsSearchServerResults);
    REGISTER_IM_MESSAGE("addressbook/sync/update", onSyncAddressBook);

    REGISTER_IM_MESSAGE("history_update", onHistoryUpdate);

    REGISTER_IM_MESSAGE("voip_signal", onVoipSignal);
    REGISTER_IM_MESSAGE("active_dialogs_are_empty", onActiveDialogsAreEmpty);
    REGISTER_IM_MESSAGE("active_dialogs_sent", onActiveDialogsSent);
    REGISTER_IM_MESSAGE("active_dialogs_hide", onActiveDialogsHide);
    REGISTER_IM_MESSAGE("stickers/meta/get/result", onStickersMetaGetResult);

    REGISTER_IM_MESSAGE("stickers/sticker/get/result", onStickersStickerGetResult);
    REGISTER_IM_MESSAGE("stickers/set_icon_big", onStickersGetSetBigIconResult);
    REGISTER_IM_MESSAGE("stickers/set_icon", onStickersSetIcon);
    REGISTER_IM_MESSAGE("stickers/pack/info/result", onStickersPackInfo);
    REGISTER_IM_MESSAGE("stickers/store/get/result", onStickersStore);
    REGISTER_IM_MESSAGE("stickers/store/search/result", onStickersStoreSearch);
    REGISTER_IM_MESSAGE("stickers/suggests", onStickersSuggests);
    REGISTER_IM_MESSAGE("stickers/fs_by_ids/get/result", onStickerGetFsByIdResult);
    REGISTER_IM_MESSAGE("stickers/suggests/result", onStickersRequestedSuggests);

    REGISTER_IM_MESSAGE("smartreply/suggests/instant", onSmartreplyInstantSuggests);
    REGISTER_IM_MESSAGE("smartreply/get/result", onSmartreplyRequestedSuggests);

    REGISTER_IM_MESSAGE("themes/settings", onThemeSettings);
    REGISTER_IM_MESSAGE("themes/wallpaper/get/result", onThemesWallpaperGetResult);
    REGISTER_IM_MESSAGE("themes/wallpaper/preview/get/result", onThemesWallpaperPreviewGetResult);

    REGISTER_IM_MESSAGE("chats/info/get/result", onChatsInfoGetResult);
    REGISTER_IM_MESSAGE("chats/info/get/failed", onChatsInfoGetFailed);
    REGISTER_IM_MESSAGE("chats/member/add/failed", onMemberAddFailed);
    REGISTER_IM_MESSAGE("chats/member/info/result", onChatsMemberInfoResult);
    REGISTER_IM_MESSAGE("chats/members/get/result", onChatsMembersGetResult);
    REGISTER_IM_MESSAGE("chats/members/search/result", onChatsMembersSearchResult);
    REGISTER_IM_MESSAGE("chats/contacts/result", onChatsContactsResult);

    REGISTER_IM_MESSAGE("files/error", fileSharingErrorResult);
    REGISTER_IM_MESSAGE("files/download/progress", fileSharingDownloadProgress);
    REGISTER_IM_MESSAGE("files/preview_size/result", fileSharingGetPreviewSizeResult);
    REGISTER_IM_MESSAGE("files/metainfo/result", fileSharingMetainfoResult);
    REGISTER_IM_MESSAGE("files/check_exists/result", fileSharingCheckExistsResult);
    REGISTER_IM_MESSAGE("files/download/result", fileSharingDownloadResult);
    REGISTER_IM_MESSAGE("image/download/result", imageDownloadResult);
    REGISTER_IM_MESSAGE("image/download/progress", imageDownloadProgress);
    REGISTER_IM_MESSAGE("image/download/result/meta", imageDownloadResultMeta);
    REGISTER_IM_MESSAGE("external_file_path/result", externalFilePathResult);
    REGISTER_IM_MESSAGE("link_metainfo/download/result/meta", linkMetainfoDownloadResultMeta);
    REGISTER_IM_MESSAGE("link_metainfo/download/result/image", linkMetainfoDownloadResultImage);
    REGISTER_IM_MESSAGE("link_metainfo/download/result/favicon", linkMetainfoDownloadResultFavicon);
    REGISTER_IM_MESSAGE("files/upload/progress", fileUploadingProgress);
    REGISTER_IM_MESSAGE("files/upload/result", fileUploadingResult);

    REGISTER_IM_MESSAGE("files/speech_to_text/result", onFilesSpeechToTextResult);
    REGISTER_IM_MESSAGE("contacts/remove/result", onContactsRemoveResult);
    REGISTER_IM_MESSAGE("app_config", onAppConfig);
    REGISTER_IM_MESSAGE("app_config_changed", onAppConfigChanged);

    REGISTER_IM_MESSAGE("my_info", onMyInfo);
    REGISTER_IM_MESSAGE("feedback/sent", onFeedbackSent);
    REGISTER_IM_MESSAGE("messages/received/senders", onMessagesReceivedSenders);
    REGISTER_IM_MESSAGE("typing", onTyping);
    REGISTER_IM_MESSAGE("typing/stop", onTypingStop);
    REGISTER_IM_MESSAGE("contacts/get_ignore/result", onContactsGetIgnoreResult);

    REGISTER_IM_MESSAGE("login_result_attach_uin", onLoginResultAttachUin);
    REGISTER_IM_MESSAGE("login_result_attach_phone", onLoginResultAttachPhone);
    REGISTER_IM_MESSAGE("update_profile/result", onUpdateProfileResult);
    REGISTER_IM_MESSAGE("chats/home/get/result", onChatsHomeGetResult);
    REGISTER_IM_MESSAGE("chats/home/get/failed", onChatsHomeGetFailed);
    REGISTER_IM_MESSAGE("user_proxy/result", onUserProxyResult);
    REGISTER_IM_MESSAGE("open_created_chat", onOpenCreatedChat);
    REGISTER_IM_MESSAGE("login_new_user", onLoginNewUser);
    REGISTER_IM_MESSAGE("set_avatar/result", onSetAvatarResult);
    REGISTER_IM_MESSAGE("chats/role/set/result", onChatsRoleSetResult);
    REGISTER_IM_MESSAGE("chats/block/result", onChatsBlockResult);
    REGISTER_IM_MESSAGE("chats/pending/resolve/result", onChatsPendingResolveResult);
    REGISTER_IM_MESSAGE("phoneinfo/result", onPhoneinfoResult);
    REGISTER_IM_MESSAGE("contacts/ignore/remove", onContactRemovedFromIgnore);

    REGISTER_IM_MESSAGE("friendly/update", onFriendlyUpdate);

    REGISTER_IM_MESSAGE("masks/get_id_list/result", onMasksGetIdListResult);
    REGISTER_IM_MESSAGE("masks/preview/result", onMasksPreviewResult);
    REGISTER_IM_MESSAGE("masks/model/result", onMasksModelResult);
    REGISTER_IM_MESSAGE("masks/get/result", onMasksGetResult);
    REGISTER_IM_MESSAGE("masks/progress", onMasksProgress);
    REGISTER_IM_MESSAGE("masks/update/retry", onMasksRetryUpdate);
    REGISTER_IM_MESSAGE("masks/existent", onExistentMasks);

    REGISTER_IM_MESSAGE("mailboxes/status", onMailStatus);
    REGISTER_IM_MESSAGE("mailboxes/new", onMailNew);

    REGISTER_IM_MESSAGE("mrim/get_key/result", getMrimKeyResult);
    REGISTER_IM_MESSAGE("mentions/me/received", onMentionsMeReceived);
    REGISTER_IM_MESSAGE("mentions/suggests/result", onMentionsSuggestResults);

    REGISTER_IM_MESSAGE("chat/heads", onChatHeads);
    REGISTER_IM_MESSAGE("update_ready", onUpdateReady);
    REGISTER_IM_MESSAGE("up_to_date", onUpToDate);
    REGISTER_IM_MESSAGE("cachedobjects/loaded", onEventCachedObjectsLoaded);
    REGISTER_IM_MESSAGE("connection/state", onEventConnectionState);

    REGISTER_IM_MESSAGE("dialog/gallery/get/result", onGetDialogGalleryResult);
    REGISTER_IM_MESSAGE("dialog/gallery/get_by_msg/result", onGetDialogGalleryByMsgResult);
    REGISTER_IM_MESSAGE("dialog/gallery/state", onGalleryState);
    REGISTER_IM_MESSAGE("dialog/gallery/update", onGalleryUpdate);
    REGISTER_IM_MESSAGE("dialog/gallery/init", onGalleryInit);
    REGISTER_IM_MESSAGE("dialog/gallery/holes_downloaded", onGalleryHolesDownloaded);
    REGISTER_IM_MESSAGE("dialog/gallery/holes_downloading", onGalleryHolesDownloading);
    REGISTER_IM_MESSAGE("dialog/gallery/erased", onGalleryErased);
    REGISTER_IM_MESSAGE("dialog/gallery/index", onGalleryIndex);

    REGISTER_IM_MESSAGE("localpin/checked", onLocalPINChecked);

    REGISTER_IM_MESSAGE("idinfo/get/result", onIdInfo);

    // core asks gui for its memory reports
    REGISTER_IM_MESSAGE("request_memory_consumption_gui_components", onRequestMemoryConsumption);
    // core has finished generating reports for a previous request
    REGISTER_IM_MESSAGE("memory_usage_report_ready", onMemUsageReportReady);

    REGISTER_IM_MESSAGE("get_user_info/result", onGetUserInfoResult);

    REGISTER_IM_MESSAGE("get_user_last_seen/result", onGetUserLastSeenResult);

    REGISTER_IM_MESSAGE("nickname/check/set/result", onNickCheckSetResult);

    REGISTER_IM_MESSAGE("group_nickname/check/set/result", onGroupNickCheckSetResult);

    REGISTER_IM_MESSAGE("omicron/update/data", onOmicronUpdateData);

    REGISTER_IM_MESSAGE("privacy_settings/set/result", onSetPrivacySettingsResult);

    REGISTER_IM_MESSAGE("common_chats/get/result", onGetCommonChatsResult);
    REGISTER_IM_MESSAGE("files_sharing/upload/aborted", onFilesharingUploadAborted);

    REGISTER_IM_MESSAGE("poll/get/result", onPollGetResult);
    REGISTER_IM_MESSAGE("poll/vote/result", onPollVoteResult);
    REGISTER_IM_MESSAGE("poll/revoke/result", onPollRevokeResult);
    REGISTER_IM_MESSAGE("poll/stop/result", onPollStopResult);
    REGISTER_IM_MESSAGE("poll/update", onPollUpdate);

    REGISTER_IM_MESSAGE("chats/mod/params/result", onModChatParamsResult);

    REGISTER_IM_MESSAGE("chats/mod/about/result", onModChatAboutResult);

    REGISTER_IM_MESSAGE("suggest_group_nick/result", onSuggestGroupNickResult);

    REGISTER_IM_MESSAGE("session_started", onSessionStarted);

    REGISTER_IM_MESSAGE("external_url_config/error", onExternalUrlConfigError);
    REGISTER_IM_MESSAGE("external_url_config/hosts", onExternalUrlConfigHosts);

    REGISTER_IM_MESSAGE("group/subscribe/result", onGroupSubscribeResult);

    REGISTER_IM_MESSAGE("get_bot_callback_answer/result", onBotCallbackAnswer);
    REGISTER_IM_MESSAGE("async_responce", onBotCallbackAnswer);

    REGISTER_IM_MESSAGE("sessions/get/result", onGetSessionsResult);
    REGISTER_IM_MESSAGE("sessions/reset/result", onResetSessionResult);

    REGISTER_IM_MESSAGE("conference/create/result", onCreateConferenceResult);

    REGISTER_IM_MESSAGE("call_log/log", onCallLog);
    REGISTER_IM_MESSAGE("call_log/add_call", onNewCall);
    REGISTER_IM_MESSAGE("call_log/removed_messages", onCallRemoveMessages);
    REGISTER_IM_MESSAGE("call_log/del_up_to", onCallDelUpTo);
    REGISTER_IM_MESSAGE("call_log/remove_contact", onCallRemoveContact);

    REGISTER_IM_MESSAGE("reactions", onReactions);
    REGISTER_IM_MESSAGE("reactions/list/result", onReactionsListResult);
    REGISTER_IM_MESSAGE("reaction/add/result", onReactionAddResult);
    REGISTER_IM_MESSAGE("reaction/remove/result", onReactionRemoveResult);

    REGISTER_IM_MESSAGE("status", onStatus);

    REGISTER_IM_MESSAGE("call_room_info", onCallRoomInfo);
    REGISTER_IM_MESSAGE("livechat/join/result", onChatJoinResult);
}

void core_dispatcher::uninit()
{
    if (guiConnector_)
    {
        if (coreConnector_)
            coreConnector_->unlink();

        guiConnector_->release();

        guiConnector_ = nullptr;
    }

    if (coreConnector_)
    {
        coreConnector_->release();
        coreConnector_ = nullptr;
    }

    if (coreFace_)
    {
        coreFace_->release();
        coreFace_ = nullptr;
    }

}

void core_dispatcher::executeCallback(const int64_t _seq, core::icollection* _params)
{
    assert(_seq > 0);

    auto node = callbacks_.extract(_seq);
    if (node.empty())
        return;

    const auto &callback = node.mapped().callback_;

    assert(callback);
    callback(_params);
}

void core_dispatcher::cleanupCallbacks()
{
    const auto now = QDateTime::currentMSecsSinceEpoch();

    const auto secsPassedFromLastCleanup = now - lastTimeCallbacksCleanedUp_;
    constexpr qint64 cleanTimeoutInMs = 30 * 1000;
    if (secsPassedFromLastCleanup < cleanTimeoutInMs)
    {
        return;
    }

    for (auto pairIter = callbacks_.begin(); pairIter != callbacks_.end();)
    {
        const auto callbackInfo = pairIter->second;

        const auto callbackRegisteredTimestamp = callbackInfo.date_;

        const auto callbackAgeInMs = now - callbackRegisteredTimestamp;

        constexpr qint64 callbackAgeInMsMax = 60 * 1000;
        if (callbackAgeInMs > callbackAgeInMsMax)
        {
            pairIter = callbacks_.erase(pairIter);
        }
        else
        {
            ++pairIter;
        }
    }

    lastTimeCallbacksCleanedUp_ = now;
}

void core_dispatcher::fileSharingErrorResult(const int64_t _seq, core::coll_helper _params)
{
    const auto rawUri = _params.get<QString>("file_url");
    const auto errorCode = _params.get<int32_t>("error", 0);

    Q_EMIT fileSharingError(_seq, rawUri, errorCode);
}

void core_dispatcher::fileSharingDownloadProgress(const int64_t _seq, core::coll_helper _params)
{
    const auto rawUri = _params.get<QString>("file_url");
    const auto size = _params.get<int64_t>("file_size");
    const auto bytesTransfer = _params.get<int64_t>("bytes_transfer", 0);

    Q_EMIT fileSharingFileDownloading(_seq, rawUri, bytesTransfer, size);
}

void core_dispatcher::fileSharingGetPreviewSizeResult(const int64_t _seq, core::coll_helper _params)
{
    const auto miniPreviewUri = _params.get<QString>("mini_preview_uri");
    const auto fullPreviewUri = _params.get<QString>("full_preview_uri");

    Q_EMIT fileSharingPreviewMetainfoDownloaded(_seq, miniPreviewUri, fullPreviewUri);
}

void core_dispatcher::fileSharingMetainfoResult(const int64_t _seq, core::coll_helper _params)
{
    Data::FileSharingMeta meta;

    meta.size_ = _params.get<int64_t>("file_size");
    meta.gotAudio_ = _params.get<bool>("got_audio");
    meta.duration_ = _params.get<int32_t>("duration");
    meta.savedByUser_ = _params.get<bool>("saved_by_user");
    meta.localPath_ = _params.get<QString>("local_path");
    meta.recognize_ = _params.get<bool>("recognize");
    meta.downloadUri_ = _params.get<QString>("file_dlink");
    meta.lastModified_ = _params.get<int64_t>("last_modified");
    meta.filenameShort_ = _params.get<QString>("file_name_short");

    Q_EMIT fileSharingFileMetainfoDownloaded(_seq, meta);
}

void core_dispatcher::fileSharingCheckExistsResult(const int64_t _seq, core::coll_helper _params)
{
    const auto filename = _params.get<QString>("file_name");
    const auto exists = _params.get<bool>("exists");

    Q_EMIT fileSharingLocalCopyCheckCompleted(_seq, exists, filename);
}

void core_dispatcher::fileSharingDownloadResult(const int64_t _seq, core::coll_helper _params)
{
    Data::FileSharingDownloadResult result;

    result.rawUri_ = _params.get<QString>("file_url");
    result.filename_ = _params.get<QString>("file_name");
    result.requestedUrl_ = _params.get<QString>("requested_url");
    result.savedByUser_ = _params.get<bool>("saved_by_user");

    Q_EMIT fileSharingFileDownloaded(_seq, result);
}

void core_dispatcher::imageDownloadProgress(const int64_t _seq, core::coll_helper _params)
{
    const auto bytesTotal = _params.get<int64_t>("bytes_total");
    const auto bytesTransferred = _params.get<int64_t>("bytes_transferred");
    const auto pctTransferred = _params.get<int32_t>("pct_transferred");

    assert(bytesTotal > 0);
    assert(bytesTransferred >= 0);
    assert(pctTransferred >= 0);
    assert(pctTransferred <= 100);

    Q_EMIT imageDownloadingProgress(_seq, bytesTotal, bytesTransferred, pctTransferred);
}

void core_dispatcher::imageDownloadResult(const int64_t _seq, core::coll_helper _params)
{
    const auto isProgress = !_params.is_value_exist("error");
    if (isProgress)
    {
        return;
    }

    const auto rawUri = _params.get<QString>("url");

    if (!_params.is_value_exist("data"))
    {
        Q_EMIT imageDownloadError(_seq, rawUri);
        return;
    }

    const auto data = _params.get_value_as_stream("data");
    const auto local = _params.get<QString>("local");
    const auto withData = _params.get<bool>("with_data");

    __INFO(
        "snippets",
        "completed image downloading\n"
            __LOGP(_seq, _seq)
            __LOGP(uri, rawUri)
            __LOGP(local_path, local)
            __LOGP(success, !data->empty()));

    if (withData)
    {
        if (data->empty())
        {
            Q_EMIT imageDownloadError(_seq, rawUri);
            return;
        }

        assert(!local.isEmpty());
        assert(!rawUri.isEmpty());

        auto task = new Utils::LoadPixmapFromDataTask(data);

        const auto succeeded = QObject::connect(
            task, &Utils::LoadPixmapFromDataTask::loadedSignal,
            this,
            [this, _seq, rawUri, local]
            (const QPixmap& pixmap)
            {
                if (pixmap.isNull())
                {
                    Q_EMIT imageDownloadError(_seq, rawUri);
                    return;
                }

                Q_EMIT imageDownloaded(_seq, rawUri, pixmap, local);
            });
        assert(succeeded);

        QThreadPool::globalInstance()->start(task);
    }
    else
    {
        Q_EMIT imageDownloaded(_seq, rawUri, QPixmap(), local);
    }
}

void core_dispatcher::imageDownloadResultMeta(const int64_t _seq, core::coll_helper _params)
{
    assert(_seq > 0);

    const auto previewWidth = _params.get<int32_t>("preview_width");
    const auto previewHeight = _params.get<int32_t>("preview_height");
    const auto originWidth = _params.get<int32_t>("origin_width");
    const auto originHeight = _params.get<int32_t>("origin_height");
    const auto downloadUri = _params.get<QString>("download_uri");
    const auto fileSize = _params.get<int64_t>("file_size");
    const auto contentType = _params.get<QString>("content_type");

    assert(previewWidth >= 0);
    assert(previewHeight >= 0);

    const QSize previewSize(previewWidth, previewHeight);
    const QSize originSize(originWidth, originHeight);

    Data::LinkMetadata linkMeta(QString(), QString(), QString(), contentType, QString(), QString(), previewSize, downloadUri, fileSize, originSize, QString());

    Q_EMIT imageMetaDownloaded(_seq, linkMeta);
}

void core_dispatcher::externalFilePathResult(const int64_t _seq, core::coll_helper _params)
{
    Q_EMIT externalFilePath(_seq, _params.get<QString>("path"));
}

void core_dispatcher::fileUploadingProgress(const int64_t _seq, core::coll_helper _params)
{
    const auto bytesUploaded = _params.get<int64_t>("bytes_transfer");
    const auto uploadingId = _params.get<QString>("uploading_id");

    Q_EMIT fileSharingUploadingProgress(uploadingId, bytesUploaded);
}

void core_dispatcher::fileUploadingResult(const int64_t _seq, core::coll_helper _params)
{
    const auto uploadingId = _params.get<QString>("uploading_id");
    const auto localPath = _params.get<QString>("local_path");
    const auto size = _params.get<int64_t>("file_size");
    const auto lastModified = _params.get<int64_t>("last_modified");
    const auto link = _params.get<QString>("file_url");
    const auto contentType = _params.get<int32_t>("content_type");
    const auto error = _params.get<int32_t>("error");

    const auto success = (error == 0);
    const auto isFileTooBig = (error == (int32_t)loader_errors::too_large_file);
    Q_EMIT fileSharingUploadingResult(uploadingId, success, localPath, link, contentType, size, lastModified, isFileTooBig);
}

void core_dispatcher::linkMetainfoDownloadResultMeta(const int64_t _seq, core::coll_helper _params)
{
    assert(_seq > 0);

    const auto success = _params.get<bool>("success");
    const auto title = _params.get<QString>("title", "");
    const auto annotation = _params.get<QString>("annotation", "");
    const auto siteName = _params.get<QString>("site_name", "");
    const auto contentType = _params.get<QString>("content_type", "");
    const auto previewWidth = _params.get<int32_t>("preview_width", 0);
    const auto previewHeight = _params.get<int32_t>("preview_height", 0);
    const auto originWidth = _params.get<int32_t>("origin_width", 0);
    const auto originwHeight = _params.get<int32_t>("origin_height", 0);
    const auto downloadUri = _params.get<QString>("download_uri", "");
    const auto fileSize = _params.get<int64_t>("file_size", -1);
    const auto previewUri = _params.get<QString>("preview_uri", "");
    const auto faviconUri = _params.get<QString>("favicon_uri", "");
    const auto fileName = _params.get<QString>("filename", "");

    assert(previewWidth >= 0);
    assert(previewHeight >= 0);
    const QSize previewSize(previewWidth, previewHeight);
    const QSize originSize(originWidth, originwHeight);

    const Data::LinkMetadata linkMetadata(title, annotation, siteName, contentType, previewUri, faviconUri, previewSize, downloadUri, fileSize, originSize, fileName);

    Q_EMIT linkMetainfoMetaDownloaded(_seq, success, linkMetadata);
}

void core_dispatcher::linkMetainfoDownloadResultImage(const int64_t _seq, core::coll_helper _params)
{
    assert(_seq > 0);

    const auto success = _params.get<bool>("success");

    if (!success)
    {
        Q_EMIT linkMetainfoImageDownloaded(_seq, false, QPixmap());
        return;
    }

    const auto data = _params.get_value_as_stream("data");

    auto task = new Utils::LoadPixmapFromDataTask(data);

    const auto succeeded = QObject::connect(
        task, &Utils::LoadPixmapFromDataTask::loadedSignal,
        this,
        [this, _seq, success]
        (const QPixmap& pixmap)
        {
            Q_EMIT linkMetainfoImageDownloaded(_seq, success, pixmap);
        });
    assert(succeeded);

    QThreadPool::globalInstance()->start(task);
}

void core_dispatcher::linkMetainfoDownloadResultFavicon(const int64_t _seq, core::coll_helper _params)
{
    assert(_seq > 0);

    const auto success = _params.get<bool>("success");

    if (!success)
    {
        Q_EMIT linkMetainfoFaviconDownloaded(_seq, false, QPixmap());
        return;
    }

    const auto data = _params.get_value_as_stream("data");

    auto task = new Utils::LoadPixmapFromDataTask(data);

    const auto succeeded = QObject::connect(
        task, &Utils::LoadPixmapFromDataTask::loadedSignal,
        this,
        [this, _seq, success]
        (const QPixmap& pixmap)
        {
            Q_EMIT linkMetainfoFaviconDownloaded(_seq, success, pixmap);
        });
    assert(succeeded);

    QThreadPool::globalInstance()->start(task);
}

void core_dispatcher::onFilesSpeechToTextResult(const int64_t _seq, core::coll_helper _params)
{
    int error = _params.get_value_as_int("error");
    int comeback = _params.get_value_as_int("comeback");
    const QString text = QString::fromUtf8(_params.get_value_as_string("text"));

    Q_EMIT speechToText(_seq, error, text, comeback);
}

void core_dispatcher::onContactsRemoveResult(const int64_t _seq, core::coll_helper _params)
{
    const QString contact = QString::fromUtf8(_params.get_value_as_string("contact"));

    Q_EMIT contactRemoved(contact);
}

void core_dispatcher::onMasksGetIdListResult(const int64_t _seq, core::coll_helper _params)
{
    const auto array = _params.get_value_as_array("mask_id_list");
    const auto maskList = toContainerOfString<QVector<QString>>(array);

    Q_EMIT maskListLoaded(maskList);
}

void core_dispatcher::onMasksPreviewResult(const int64_t _seq, core::coll_helper _params)
{
    Q_EMIT maskPreviewLoaded(_seq, QString::fromUtf8(_params.get_value_as_string("local_path")));
}

void core_dispatcher::onMasksModelResult(const int64_t _seq, core::coll_helper _params)
{
    Q_EMIT maskModelLoaded();
}

void core_dispatcher::onMasksGetResult(const int64_t _seq, core::coll_helper _params)
{
    Q_EMIT maskLoaded(_seq, QString::fromUtf8(_params.get_value_as_string("local_path")));
}

void core_dispatcher::onMasksProgress(const int64_t _seq, core::coll_helper _params)
{
    Q_EMIT maskLoadingProgress(_seq, _params.get_value_as_uint("percent"));
}

void core_dispatcher::onMasksRetryUpdate(const int64_t _seq, core::coll_helper _params)
{
    Q_EMIT maskRetryUpdate();
}

void core_dispatcher::onExistentMasks(const int64_t _seq, core::coll_helper _params)
{
    Q_EMIT existentMasksLoaded(Data::UnserializeMasks(&_params));
}

void core_dispatcher::onMailStatus(const int64_t _seq, core::coll_helper _params)
{
    auto array = _params.get_value_as_array("mailboxes");
    if (!array->empty()) //only one email
    {
        bool init = _params->is_value_exist("init") ? _params.get_value_as_bool("init") : false;
        core::coll_helper helper(array->get_at(0)->get_as_collection(), false);
        Q_EMIT mailStatus(QString::fromUtf8(helper.get_value_as_string("email")), helper.get_value_as_uint("unreads"), init);
    }
}

void core_dispatcher::onMailNew(const int64_t _seq, core::coll_helper _params)
{
    Q_EMIT newMail(
        QString::fromUtf8(_params.get_value_as_string("email")),
        QString::fromUtf8(_params.get_value_as_string("from")),
        QString::fromUtf8(_params.get_value_as_string("subj")),
        QString::fromUtf8(_params.get_value_as_string("uidl")));
}

void core_dispatcher::getMrimKeyResult(const int64_t _seq, core::coll_helper _params)
{
    Q_EMIT mrimKey(_seq, QString::fromUtf8(_params.get_value_as_string("key")));
}

void core_dispatcher::onAppConfig(const int64_t _seq, core::coll_helper _params)
{
    Ui::SetAppConfig(std::make_unique<AppConfig>(_params));

    Q_EMIT appConfig();

#if defined(IM_AUTO_TESTING)
    //enable_testing in app.ini
    AppConfig appConfig = GetAppConfig();
    if (appConfig.IsTestingEnable())
    {
        qDebug() << "### Ui::GetAppConfig().isTestingEnable()";
        //WEBSERVER
        WebServer* server;
        server = new WebServer();
        server->startWebserver();
    }
#endif
}

void core_dispatcher::onAppConfigChanged(const int64_t _seq, core::coll_helper _params)
{
    Q_UNUSED(_seq)
    Q_UNUSED(_params)

    Q_EMIT appConfigChanged();
}

void core_dispatcher::onMyInfo(const int64_t _seq, core::coll_helper _params)
{
    Ui::MyInfo()->unserialize(&_params);
    Ui::MyInfo()->CheckForUpdate();

    onStatus(0, _params);

    Q_EMIT myInfo();
}

void core_dispatcher::onFeedbackSent(const int64_t _seq, core::coll_helper _params)
{
    Q_EMIT feedbackSent(_params.get_value_as_bool("succeeded"));
}

void core_dispatcher::onMessagesReceivedSenders(const int64_t _seq, core::coll_helper _params)
{
    const QString aimId = QString::fromUtf8(_params.get_value_as_string("aimid"));
    QVector<QString> sendersAimIds;
    if (_params.is_value_exist("senders"))
    {
        const auto array = _params.get_value_as_array("senders");
        sendersAimIds = toContainerOfString<QVector<QString>>(array);
    }

    Q_EMIT messagesReceived(aimId, sendersAimIds);
}

void core_dispatcher::onTyping(const int64_t _seq, core::coll_helper _params)
{
    onEventTyping(_params, true);
}

void core_dispatcher::onTypingStop(const int64_t _seq, core::coll_helper _params)
{
    onEventTyping(_params, false);
}

void core_dispatcher::onContactsGetIgnoreResult(const int64_t _seq, core::coll_helper _params)
{
    const auto array = _params.get_value_as_array("aimids");
    const auto ignoredAimIds = toContainerOfString<QVector<QString>>(array);
    Logic::updateIgnoredModel(ignoredAimIds);

    Q_EMIT recvPermitDeny(ignoredAimIds.isEmpty());
}

core::icollection* core_dispatcher::create_collection() const
{
    core::ifptr<core::icore_factory> factory(coreFace_->get_factory());
    return factory->create_collection();
}

static auto get_uid() noexcept
{
    static int64_t seq = 0;
    return ++seq;
};

qint64 core_dispatcher::post_message_to_core(std::string_view _message, core::icollection* _collection, const QObject* _object, message_processed_callback _callback)
{
    const auto seq = get_uid();

    coreConnector_->receive(_message, seq, _collection);

    if (_callback)
    {
        const auto result = callbacks_.emplace(seq, callback_info(std::move(_callback), QDateTime::currentMSecsSinceEpoch(), _object));
        assert(result.second);
    }

    if (_object)
    {
        QObject::connect(_object, &QObject::destroyed, this, [this](QObject* _obj)
        {
            for (auto it = callbacks_.cbegin(); it != callbacks_.cend(); )
            {
                if (it->second.object_ == _obj)
                    it = callbacks_.erase(it);
                else
                    ++it;
            }
        });
    }


    return seq;
}

bool core_dispatcher::get_enabled_stats_post() const
{
    return config::get().is_on(config::features::statistics);
}

qint64 core_dispatcher::post_stats_to_core(core::stats::stats_event_names _eventName, const core::stats::event_props_type& _props)
{
    if (!get_enabled_stats_post())
        return -1;

    assert(_eventName > core::stats::stats_event_names::min);
    assert(_eventName < core::stats::stats_event_names::max);

    core::coll_helper coll(create_collection(), true);
    coll.set_value_as_enum("event", _eventName);

    core::ifptr<core::iarray> propsArray(coll->create_array());
    propsArray->reserve(_props.size());
    for (const auto &[name, value] : _props)
    {
        assert(!name.empty() && !value.empty());

        core::coll_helper collProp(coll->create_collection(), true);

        collProp.set_value_as_string("name", name);
        collProp.set_value_as_string("value", value);
        core::ifptr<core::ivalue> val(coll->create_value());
        val->set_as_collection(collProp.get());
        propsArray->push_back(val.get());
    }

    coll.set_value_as_array("props", propsArray.get());
    return post_message_to_core("stats", coll.get());
}

qint64 core_dispatcher::post_im_stats_to_core(core::stats::im_stat_event_names _eventName, const core::stats::event_props_type& _props)
{
    assert(_eventName > core::stats::im_stat_event_names::min);
    assert(_eventName < core::stats::im_stat_event_names::max);

    core::coll_helper coll(create_collection(), true);
    coll.set_value_as_enum("event", _eventName);

    core::ifptr<core::iarray> propsArray(coll->create_array());
    propsArray->reserve(_props.size());

    for (const auto &[name, value] : _props)
    {
        assert(!name.empty() && !value.empty());

        core::coll_helper collProp(coll->create_collection(), true);

        collProp.set_value_as_string("name", name);
        collProp.set_value_as_string("value", value);

        core::ifptr<core::ivalue> val(coll->create_value());
        val->set_as_collection(collProp.get());

        propsArray->push_back(val.get());
    }

    coll.set_value_as_array("props", propsArray.get());

    return post_message_to_core("im_stats", coll.get());
}

void core_dispatcher::received(const QString& _receivedMessage, const qint64 _seq, core::icollection* _params)
{
    if (_seq > 0)
        executeCallback(_seq, _params);

    cleanupCallbacks();

    core::coll_helper collParams(_params, true);

    if (const auto iter_handler = messages_map_.find(_receivedMessage.toStdString()); iter_handler != messages_map_.end())
        iter_handler->second(_seq, collParams);
}

bool core_dispatcher::isImCreated() const
{
    return isImCreated_;
}

std::list<Logic::TypingFires>& getTypingFires()
{
    static std::list<Logic::TypingFires> typingFires;

    return typingFires;
}

void core_dispatcher::onEventTyping(core::coll_helper _params, bool _isTyping)
{
    Logic::TypingFires currentTypingStatus(
        QString::fromUtf8(_params.get_value_as_string("aimId")),
        QString::fromUtf8(_params.get_value_as_string("chatterAimId")),
        QString::fromUtf8(_params.get_value_as_string("chatterName")));

    auto iterFires = std::find(getTypingFires().begin(), getTypingFires().end(), currentTypingStatus);

    if (_isTyping)
    {
        if (iterFires == getTypingFires().end())
            getTypingFires().push_back(currentTypingStatus);
        else
            iterFires->refreshTime();

        if (!typingCheckTimer_->isActive())
            typingCheckTimer_->start();
    }
    else
    {
        if (iterFires != getTypingFires().end())
            getTypingFires().erase(iterFires);
    }

    Q_EMIT typingStatus(currentTypingStatus, _isTyping);
}

void core_dispatcher::onTypingCheckTimer()
{
    auto& typings = getTypingFires();
    const auto now = std::chrono::milliseconds(QDateTime::currentMSecsSinceEpoch());
    for (auto it = typings.begin(); it != typings.end(); )
    {
        if (now - it->time_ >= typingLifetime)
        {
            Q_EMIT typingStatus(*it, false);
            it = typings.erase(it);
        }
        else
        {
            ++it;
        }
    }

    if (typings.empty())
        typingCheckTimer_->stop();
}

void core_dispatcher::setUserState(const core::profile_state state)
{
    assert(state > core::profile_state::min);
    assert(state < core::profile_state::max);

    Ui::gui_coll_helper collection(Ui::GetDispatcher()->create_collection(), true);

    std::stringstream stream;
    stream << state;

    collection.set_value_as_string("state", stream.str());
    collection.set<QString>("aimid", MyInfo()->aimId());
    Ui::GetDispatcher()->post_message_to_core("set_state", collection.get());
}

void core_dispatcher::onNeedLogin(const int64_t _seq, core::coll_helper _params)
{
    userStateGoneAway_ = false;

    bool is_auth_error = false;

    if (_params.get())
    {
        is_auth_error = _params.get_value_as_bool("is_auth_error", false);
    }

    LocalPIN::instance()->setEnabled(false);
    LocalPIN::instance()->setLocked(false);

    if (!get_gui_settings()->get_value(settings_keep_logged_in, settings_keep_logged_in_default()))
    {
        get_gui_settings()->set_value(login_page_last_entered_phone, QString());
        get_gui_settings()->set_value(login_page_last_entered_uin, QString());
        get_gui_settings()->set_value(settings_recents_fs_stickers, QString());
        get_gui_settings()->set_value<std::vector<int32_t>>(settings_old_recents_stickers, {});
    }

    Q_EMIT needLogin(is_auth_error);
}

void core_dispatcher::onImCreated(const int64_t _seq, core::coll_helper _params)
{
    isImCreated_ = true;

    Q_EMIT im_created();
}

void core_dispatcher::onLoginComplete(const int64_t _seq, core::coll_helper _params)
{
    Q_EMIT loginComplete();
}

void core_dispatcher::onContactList(const int64_t _seq, core::coll_helper _params)
{
    auto cl = std::make_shared<Data::ContactList>();

    QString type;

    Data::UnserializeContactList(&_params, *cl, type);

    core::iarray* groups = _params.get_value_as_array("groups");
    const auto groupsSize = groups->size();
    for (core::iarray::size_type igroups = 0; igroups < groupsSize; ++igroups)
    {
        core::coll_helper group_coll(groups->get_at(igroups)->get_as_collection(), false);
        core::iarray* contacts = group_coll.get_value_as_array("contacts");
        const auto contactsSize = contacts->size();
        for (core::iarray::size_type icontacts = 0; icontacts < contactsSize; ++icontacts)
        {
            core::coll_helper value(contacts->get_at(icontacts)->get_as_collection(), false);
            onStatus(0, value);
        }
    }

    Q_EMIT contactList(cl, type);
}

void core_dispatcher::onLoginGetSmsCodeResult(const int64_t _seq, core::coll_helper _params)
{
    bool result = _params.get_value_as_bool("result");
    int code_length = _params.get_value_as_int("code_length", 0);
    int error = result ? 0 : _params.get_value_as_int("error");
    QString ivrUrl;
    if (_params.is_value_exist("ivr_url"))
        ivrUrl = QString::fromUtf8(_params.get_value_as_string("ivr_url"));

    QString checks;
    if (_params.is_value_exist("checks"))
        checks = QString::fromUtf8(_params.get_value_as_string("checks"));

    Q_EMIT getSmsResult(_seq, error, code_length, ivrUrl, checks);
}

void core_dispatcher::onLoginResult(const int64_t _seq, core::coll_helper _params)
{
    bool result = _params.get_value_as_bool("result");
    int error = result ? 0 : _params.get_value_as_int("error");
    bool needFill = _params.get_value_as_bool("need_fill_profile");

    Q_EMIT loginResult(_seq, error, needFill);
}

void core_dispatcher::onAvatarsGetResult(const int64_t _seq, core::coll_helper _params)
{
    QPixmap avatar;

    bool result = Data::UnserializeAvatar(&_params, avatar);

    const QString contact = QString::fromUtf8(_params.get_value_as_string("contact"));

    const int size = _params.get_value_as_int("size");

    Q_EMIT avatarLoaded(_seq, contact, avatar, size, result);
}

void core_dispatcher::onAvatarsPresenceUpdated(const int64_t _seq, core::coll_helper _params)
{
    Q_EMIT avatarUpdated(QString::fromUtf8(_params.get_value_as_string("aimid")));
}

void core_dispatcher::onContactPresence(const int64_t _seq, core::coll_helper _params)
{
    Q_EMIT presense(Data::UnserializePresence(&_params));
}

void Ui::core_dispatcher::onContactOutgoingMsgCount(const int64_t _seq, core::coll_helper _params)
{
    const auto aimid = QString::fromUtf8(_params.get_value_as_string("aimid"));
    const auto count = _params.get_value_as_int("outgoing_count");
    Q_EMIT outgoingMsgCount(aimid, count);
}

void core_dispatcher::onGuiSettings(const int64_t _seq, core::coll_helper _params)
{
#ifdef _WIN32
    core::dump::set_os_version(_params.get_value_as_string("os_version"));
#endif

    get_gui_settings()->clear();
    get_gui_settings()->unserialize(_params);

    Q_EMIT guiSettings();
}

void core_dispatcher::onCoreLogins(const int64_t _seq, core::coll_helper _params)
{
    const bool has_valid_login = _params.get_value_as_bool("has_valid_login");
    const bool locked = _params.get_value_as_bool("locked");

    installBetaUpdates_ = _params.get_value_as_bool("install_beta_updates");

    LocalPIN::instance()->setEnabled(locked);

    Q_EMIT coreLogins(has_valid_login, locked, _params.get<QString>("login"));
}

void core_dispatcher::onArchiveMessages(Ui::MessagesBuddiesOpt _type, const int64_t _seq, core::coll_helper _params)
{
    const auto myAimid = _params.get<QString>("my_aimid");

    auto result = Data::UnserializeMessageBuddies(&_params, myAimid);

    if (!result.introMessages.isEmpty() && !result.messages.isEmpty() && std::as_const(result.messages).front()->Prev_ == std::as_const(result.introMessages).back()->Id_)
    {
        std::move(result.messages.begin(), result.messages.end(), std::back_inserter(result.introMessages));
        result.messages = std::move(result.introMessages);
    }

    Q_EMIT messageBuddies(result.messages, result.aimId, _type, result.havePending, _seq, result.lastMsgId);

    if (_params.is_value_exist("deleted"))
    {
        Data::MessageBuddies deleted;

        auto deletedArray = _params.get_value_as_array("deleted");
        const auto theirs_last_delivered = _params.get_value_as_int64("theirs_last_delivered", -1);
        const auto theirs_last_read = _params.get_value_as_int64("theirs_last_read", -1);

        Data::unserializeMessages(deletedArray, result.aimId, myAimid, theirs_last_delivered, theirs_last_read, Out deleted);

        Q_EMIT messagesDeleted(result.aimId, deleted);
    }

    if (!result.modifications.isEmpty())
        Q_EMIT messagesModified(result.aimId, result.modifications);
}

void core_dispatcher::getCodeByPhoneCall(const QString& _ivr_url)
{
    Ui::gui_coll_helper collection(Ui::GetDispatcher()->create_collection(), true);

    collection.set_value_as_qstring("ivr_url", _ivr_url);
    Ui::GetDispatcher()->post_message_to_core("get_code_by_phone_call", collection.get());
}

qint64 core_dispatcher::getDialogGallery(const QString& _aimId, const QStringList& _types, int64_t _after_msg, int64_t _after_seq, const int _pageSize, const bool _download_holes)
{
    Ui::gui_coll_helper collection(Ui::GetDispatcher()->create_collection(), true);

    collection.set_value_as_qstring("aimid", _aimId);
    collection.set_value_as_int("page_size", _pageSize);
    collection.set_value_as_int64("after_msg", _after_msg);
    collection.set_value_as_int64("after_seq", _after_seq);
    collection.set_value_as_array("type", toArrayOfStrings(_types, collection).get());
    collection.set_value_as_bool("download_holes", _download_holes);

    return Ui::GetDispatcher()->post_message_to_core("get_dialog_gallery", collection.get());
}

qint64 core_dispatcher::requestDialogGalleryState(const QString& _aimId)
{
    Ui::gui_coll_helper collection(Ui::GetDispatcher()->create_collection(), true);
    collection.set_value_as_qstring("aimid", _aimId);
    return Ui::GetDispatcher()->post_message_to_core("request_gallery_state", collection.get());
}

qint64 core_dispatcher::getDialogGalleryIndex(const QString& _aimId, const QStringList& _types, qint64 _msg, qint64 _seq)
{
    Ui::gui_coll_helper collection(Ui::GetDispatcher()->create_collection(), true);
    collection.set_value_as_qstring("aimid", _aimId);
    collection.set_value_as_int64("msg", _msg);
    collection.set_value_as_int64("seq", _seq);
    collection.set_value_as_array("type", toArrayOfStrings(_types, collection).get());

    return Ui::GetDispatcher()->post_message_to_core("get_gallery_index", collection.get());
}

qint64 core_dispatcher::getDialogGalleryByMsg(const QString& _aimId, const QStringList& _types, int64_t _id)
{
    Ui::gui_coll_helper collection(Ui::GetDispatcher()->create_collection(), true);

    collection.set_value_as_qstring("aimid", _aimId);
    collection.set_value_as_int64("msg", _id);
    collection.set_value_as_array("type", toArrayOfStrings(_types, collection).get());

    return Ui::GetDispatcher()->post_message_to_core("get_dialog_gallery_by_msg", collection.get());
}

void core_dispatcher::onArchiveMessagesGetResult(const int64_t _seq, core::coll_helper _params)
{
    onArchiveMessages(Ui::MessagesBuddiesOpt::Requested, _seq, _params);
}

void core_dispatcher::onArchiveBuddiesGetResult(const int64_t _seq, core::coll_helper _params)
{
    onArchiveMessages(_params.get_value_as_bool("updated") ? Ui::MessagesBuddiesOpt::Updated : Ui::MessagesBuddiesOpt::RequestedByIds, _seq, _params);
}

void core_dispatcher::onArchiveMentionsGetResult(const int64_t _seq, core::coll_helper _params)
{
    const auto contact = _params.get<QString>("contact");
    const auto myAimid = _params.get<QString>("my_aimid");

    const auto result = Data::UnserializeMessageBuddies(&_params, myAimid);

    if (!result.messages.isEmpty())
    {
        for (const auto& x : result.messages)
            Q_EMIT mentionMe(contact, x);
    }
}

void core_dispatcher::onMessagesReceivedDlgState(const int64_t _seq, core::coll_helper _params)
{
    onArchiveMessages(Ui::MessagesBuddiesOpt::DlgState, _seq, _params);
}

void core_dispatcher::onMessagesReceivedServer(const int64_t _seq, core::coll_helper _params)
{
    const auto result = Data::UnserializeServerMessagesIds(_params);

    if (!result.AllIds_.isEmpty())
    {
        assert(std::is_sorted(result.AllIds_.begin(), result.AllIds_.end()));
        Q_EMIT messageIdsFromServer(result.AllIds_, result.AimId_, _seq);
    }

    if (!result.Deleted_.isEmpty())
        Q_EMIT messagesDeleted(result.AimId_, result.Deleted_);

    if (!result.Modifications_.isEmpty())
        Q_EMIT messagesModified(result.AimId_, result.Modifications_);
}

void core_dispatcher::onMessagesReceivedSearch(const int64_t _seq, core::coll_helper _params)
{
    onArchiveMessages(Ui::MessagesBuddiesOpt::Search, _seq, _params);
}

void core_dispatcher::onMessagesClear(const int64_t _seq, core::coll_helper _params)
{
    Q_EMIT messagesClear(_params.get<QString>("contact"), _seq);
}

void core_dispatcher::onMessagesEmpty(const int64_t _seq, core::coll_helper _params)
{
    Q_EMIT messagesEmpty(_params.get<QString>("contact"), _seq);
}

void core_dispatcher::onMessagesReceivedUpdated(const int64_t _seq, core::coll_helper _params)
{
    const auto result = Data::UnserializeServerMessagesIds(_params);
    if (!result.AllIds_.isEmpty())
        Q_EMIT messageIdsUpdated(result.AllIds_, result.AimId_, _seq);

    if (!result.Deleted_.isEmpty())
        Q_EMIT messagesDeleted(result.AimId_, result.Deleted_);

    if (!result.Modifications_.isEmpty())
        Q_EMIT messagesModified(result.AimId_, result.Modifications_);
}

void core_dispatcher::onArchiveMessagesPending(const int64_t _seq, core::coll_helper _params)
{
    onArchiveMessages(Ui::MessagesBuddiesOpt::Pending, _seq, _params);
}

void core_dispatcher::onMessagesReceivedInit(const int64_t _seq, core::coll_helper _params)
{
    onArchiveMessages(Ui::MessagesBuddiesOpt::Init, _seq, _params);
}

void core_dispatcher::onMessagesReceivedContext(const int64_t _seq, core::coll_helper _params)
{
    onArchiveMessages(Ui::MessagesBuddiesOpt::Context, _seq, _params);
}

void core_dispatcher::onMessagesContextError(const int64_t _seq, core::coll_helper _params)
{
    const auto net_error = _params.get_value_as_bool("is_network_error") ? MessagesNetworkError::Yes : MessagesNetworkError::No;
    Q_EMIT messageContextError(_params.get<QString>("contact"), _seq, _params.get_value_as_int64("id"), net_error);
}

void core_dispatcher::onMessagesLoadAfterSearchError(const int64_t _seq, core::coll_helper _params)
{
    const auto net_error = _params.get_value_as_bool("is_network_error") ? MessagesNetworkError::Yes : MessagesNetworkError::No;
    Q_EMIT messageLoadAfterSearchError(_params.get<QString>("contact"),
        _seq,
        _params.get_value_as_int64("from"),
        _params.get_value_as_int64("count_later"),
        _params.get_value_as_int64("count_early"),
        net_error);
}

void core_dispatcher::onMessagesReceivedMessageStatus(const int64_t _seq, core::coll_helper _params)
{
    onArchiveMessages(Ui::MessagesBuddiesOpt::MessageStatus, _seq, _params);
}

void core_dispatcher::onMessagesDelUpTo(const int64_t _seq, core::coll_helper _params)
{
    const auto id = _params.get<int64_t>("id");
    assert(id > -1);

    const auto contact = _params.get<QString>("contact");
    assert(!contact.isEmpty());

    Q_EMIT messagesDeletedUpTo(contact, id);
}

void core_dispatcher::onDlgStates(const int64_t _seq, core::coll_helper _params)
{
    const auto myAimid = _params.get<QString>("my_aimid");

    QVector<Data::DlgState> dlgStatesList;

    auto arrayDlgStates = _params.get_value_as_array("dlg_states");
    if (!arrayDlgStates)
    {
        assert(false);
        return;
    }

    const auto size = arrayDlgStates->size();
    dlgStatesList.reserve(size);
    for (int32_t i = 0; i < size; ++i)
    {
        const auto val = arrayDlgStates->get_at(i);

        core::coll_helper helper(val->get_as_collection(), false);

        dlgStatesList.push_back(Data::UnserializeDlgState(&helper, myAimid));
    }

    Q_EMIT dlgStates(dlgStatesList);
}

void core_dispatcher::onMentionsMeReceived(const int64_t _seq, core::coll_helper _params)
{
    const auto contact = _params.get<QString>("contact");
    const auto myaimid = _params.get<QString>("my_aimid");

    core::coll_helper coll_message(_params.get_value_as_collection("message"), false);

    auto message = Data::unserializeMessage(coll_message, contact, myaimid, -1, -1);

    Q_EMIT mentionMe(contact, message);
}

void core_dispatcher::onMentionsSuggestResults(const int64_t _seq, core::coll_helper _params)
{
    Logic::MentionsSuggests vec;

    if (_params.is_value_exist("suggests"))
    {
        const auto array = _params.get_value_as_array("suggests");
        const auto size = array->size();
        vec.reserve(size);
        for (int i = 0; i < size; ++i)
        {
            const auto val = array->get_at(i);
            core::coll_helper helper(val->get_as_collection(), false);

            Logic::MentionSuggest s;
            s.unserialize(helper);

            vec.push_back(std::move(s));
        }
    }

    Q_EMIT mentionsSuggestResults(_seq, vec);
}

void core_dispatcher::onChatHeads(const int64_t _seq, core::coll_helper _params)
{
    auto heads = Data::UnserializeChatHeads(&_params);
    Q_EMIT chatHeads(heads);
}

void core_dispatcher::onDialogsSearchLocalResults(const int64_t _seq, core::coll_helper _params)
{
    onDialogsSearchResult(_seq, _params, true);
}

void core_dispatcher::onDialogsSearchLocalEmptyResult(const int64_t _seq, core::coll_helper _params)
{
    Q_EMIT searchedMessagesLocalEmptyResult(_seq);
}

void core_dispatcher::onDialogsSearchServerResults(const int64_t _seq, core::coll_helper _params)
{
    onDialogsSearchResult(_seq, _params, false);
}

void core_dispatcher::onDialogsSearchResult(const int64_t _seq, core::coll_helper _params, const bool _fromLocalSearch)
{
    Data::SearchResultsV res;

    if (!_fromLocalSearch && _params->is_value_exist("error"))
    {
        Q_EMIT searchedMessagesServer(res, QString(), -1, _seq);
        return;
    }

    if (_params->is_value_exist("messages"))
    {
        const auto msgArray = _params.get_value_as_array("messages");
        const auto size = msgArray->size();
        res.reserve(size);

        const auto myAimId = MyInfo()->aimId();

        for (int i = 0; i < size; ++i)
        {
            const auto val = msgArray->get_at(i);
            core::coll_helper helper(val->get_as_collection(), false);
            if (!helper.is_value_exist("message"))
                continue;

            auto resItem = std::make_shared<Data::SearchResultMessage>();
            resItem->isLocalResult_ = _fromLocalSearch;
            resItem->dialogAimId_ = helper.get<QString>("contact");

            core::coll_helper msgHelper(helper.get_value_as_collection("message"), false);
            resItem->message_ = Data::unserializeMessage(msgHelper, resItem->dialogAimId_, myAimId, -1, -1);
            resItem->highlights_ = unserializeHighlights(helper);

            if (helper.is_value_exist("contact_entry_count"))
                resItem->dialogEntryCount_ = helper.get_value_as_int("contact_entry_count");
            if (helper.is_value_exist("cursor"))
                resItem->dialogCursor_ = helper.get<QString>("cursor");

            res.push_back(std::move(resItem));
        }
    }

    if (_fromLocalSearch)
    {
        Q_EMIT searchedMessagesLocal(res, _seq);
    }
    else
    {
        QString cursorNext;
        if (_params.is_value_exist("cursor_next"))
            cursorNext = _params.get<QString>("cursor_next");

        const auto totalEntries = _params.get_value_as_int("total_count");
        Q_EMIT searchedMessagesServer(res, cursorNext, totalEntries, _seq);
    }
}

void core_dispatcher::onDialogsSearchPatternHistory(const int64_t _seq, core::coll_helper _params)
{
    const auto contact = _params.get<QString>("contact");

    const auto array = _params.get_value_as_array("patterns");
    const auto patterns = toContainerOfString<QVector<QString>>(array);

    Q_EMIT searchPatternsLoaded(contact, patterns, QPrivateSignal());
}

void core_dispatcher::onContactsSearchLocalResults(const int64_t _seq, core::coll_helper _params)
{
    Data::SearchResultsV res;
    const auto msgArray = _params.get_value_as_array("results");
    const auto size = msgArray->size();
    res.reserve(size);
    for (int i = 0; i < size; ++i)
    {
        const auto val = msgArray->get_at(i);
        core::coll_helper helper(val->get_as_collection(), false);

        auto resItem = std::make_shared<Data::SearchResultContactChatLocal>();
        resItem->aimId_ = helper.get<QString>("contact");
        resItem->isChat_ = helper.get<bool>("is_chat");
        resItem->searchPriority_ = helper.get<int>("priority");
        resItem->highlights_ = unserializeHighlights(helper);

        res.push_back(std::move(resItem));
    }

    Q_EMIT searchedContactsLocal(res, _seq);
}

void core_dispatcher::onContactsSearchServerResults(const int64_t _seq, core::coll_helper _params)
{
    Data::SearchResultsV res;

    if (_params.is_value_exist("data"))
    {
        core::ifptr<core::iarray> cont_array(_params.get_value_as_array("data"), false);
        res.reserve(cont_array->size());

        for (int32_t i = 0; i < cont_array->size(); ++i)
        {
            Ui::gui_coll_helper coll_profile(cont_array->get_at(i)->get_as_collection(), false);

            Logic::contact_profile profile;
            if (profile.unserialize2(coll_profile))
            {
                auto resContact = std::make_shared<Data::SearchResultContact>();
                resContact->profile_ = std::make_shared<Logic::contact_profile>(std::move(profile));
                resContact->isLocalResult_ = false;
                resContact->highlights_ = unserializeHighlights(coll_profile);

                res.push_back(std::move(resContact));
            }

            if (const auto isBot = coll_profile.get_value_as_bool("bot"))
                Logic::GetLastseenContainer()->setContactBot(profile.get_aimid());
        }
    }

    if (_params.is_value_exist("chats"))
    {
        core::ifptr<core::iarray> chat_array(_params.get_value_as_array("chats"), false);
        res.reserve(res.size() + chat_array->size());

        for (int32_t i = 0; i < chat_array->size(); ++i)
        {
            Ui::gui_coll_helper coll_chat(chat_array->get_at(i)->get_as_collection(), false);

            auto resChat = std::make_shared<Data::SearchResultChat>();
            resChat->chatInfo_ = Data::UnserializeChatInfo(&coll_chat);
            resChat->isLocalResult_ = false;

            res.push_back(std::move(resChat));
        }
    }

    Q_EMIT searchedContactsServer(res, _seq);
}

void Ui::core_dispatcher::onSyncAddressBook(const int64_t _seq, core::coll_helper _params)
{
    const auto error = _params.get_value_as_int("error");
    Q_EMIT syncronizedAddressBook(error != 20000);
}

void core_dispatcher::onVoipSignal(const int64_t _seq, core::coll_helper _params)
{
    voipController_.handlePacket(_params);
}

void core_dispatcher::onActiveDialogsAreEmpty(const int64_t _seq, core::coll_helper _params)
{
    Q_EMIT Utils::InterConnector::instance().showRecentsPlaceholder();
}

void core_dispatcher::onActiveDialogsHide(const int64_t _seq, core::coll_helper _params)
{
    QString aimId = Data::UnserializeActiveDialogHide(&_params);

    Q_EMIT activeDialogHide(aimId);
}

void core_dispatcher::onActiveDialogsSent(const int64_t _seq, core::coll_helper _params)
{
    if (_params.get_value_as_int("dialogs_count", 0) == 0)
        Logic::updatePlaceholders({Logic::Placeholder::Recents});
}

void core_dispatcher::onStickersMetaGetResult(const int64_t _seq, core::coll_helper _params)
{
    Ui::Stickers::unserialize(_params);

    Q_EMIT onStickers();
}

void core_dispatcher::onStickerGetFsByIdResult(const int64_t _seq, core::coll_helper _params)
{
    const auto stickers = Ui::Stickers::getFsByIds(_params);
    Q_EMIT onStickerMigration(stickers);
}

void core_dispatcher::onStickersStore(const int64_t _seq, core::coll_helper _params)
{
    Ui::Stickers::unserialize_store(_params);

    Q_EMIT onStore();
}

void core_dispatcher::onStickersStoreSearch(const int64_t _seq, core::coll_helper _params)
{
    if (Ui::Stickers::unserialize_store_search(_seq, _params))
        Q_EMIT onStoreSearch();
}

void core_dispatcher::onStickersSuggests(const int64_t _seq, core::coll_helper _params)
{
    Ui::Stickers::unserialize_suggests(_params);
}

void core_dispatcher::onStickersRequestedSuggests(const int64_t _seq, core::coll_helper _params)
{
    if (_params->is_value_exist("error"))
        return;

    const auto array = _params.get_value_as_array("stickers");
    const auto stickers = toContainerOfString<QVector<QString>>(array);
    Q_EMIT stickersRequestedSuggestsResult(stickers);
}

void core_dispatcher::onSmartreplyInstantSuggests(const int64_t _seq, core::coll_helper _params)
{
    Q_EMIT smartreplyInstantSuggests(unserializeSmartreplySuggests(_params));
}

void core_dispatcher::onSmartreplyRequestedSuggests(const int64_t _seq, core::coll_helper _params)
{
    if (_params->is_value_exist("error"))
        return;

    Q_EMIT smartreplyRequestedResult(unserializeSmartreplySuggests(_params));
}

void core_dispatcher::onStickersStickerGetResult(const int64_t _seq, core::coll_helper _params)
{
    Ui::Stickers::setStickerData(_params);

    const auto fsId = QString::fromUtf8(_params.get_value_as_string("fs_id", ""));
    const auto sticker = Ui::Stickers::getSticker(fsId);

    Q_EMIT onSticker(
        _params.get_value_as_int("error"),
        _params.get_value_as_int("set_id", sticker ? sticker->getSetId() : -1),
        _params.get_value_as_int("sticker_id", 0),
        fsId);
}

void core_dispatcher::onStickersGetSetBigIconResult(const int64_t _seq, core::coll_helper _params)
{
    Ui::Stickers::setSetBigIcon(_params);

    Q_EMIT setBigIconChanged(_params.get_value_as_int("set_id"));
}

void core_dispatcher::onStickersSetIcon(const int64_t _seq, core::coll_helper _params)
{
    Ui::Stickers::setSetIcon(_params);

    Q_EMIT setIconChanged(_params.get_value_as_int("set_id"));
}

void core_dispatcher::onStickersPackInfo(const int64_t _seq, core::coll_helper _params)
{
    bool result = _params.get_value_as_bool("result");

    std::string store_id = _params.get_value_as_string("store_id");

    std::shared_ptr<Ui::Stickers::Set> info;

    if (result)
    {
        core::coll_helper coll_set(_params.get_value_as_collection("info"), false);

        info = Stickers::parseSet(coll_set);

        Stickers::addSet(info);
    }

    const bool exist = (_params.get_value_as_int("http_code") != 404);

    Q_EMIT onStickerpackInfo(result, exist, info);
}

void core_dispatcher::onThemeSettings(const int64_t _seq, core::coll_helper _params)
{
    Styling::getThemesContainer().unserialize(_params);

    Q_EMIT themeSettings();
}

void core_dispatcher::onThemesWallpaperGetResult(const int64_t _seq, core::coll_helper _params)
{
    const auto id = QString::fromUtf8(_params.get_value_as_string("id"));
    const auto data = _params.get_value_as_stream("data");

    if (data->empty())
    {
        Q_EMIT onWallpaper(id, QPixmap(), QPrivateSignal());
    }
    else
    {
        auto task = new Utils::LoadPixmapFromDataTask(data, QSize());
        connect(task, &Utils::LoadPixmapFromDataTask::loadedSignal, this, [this, id](const QPixmap& pixmap)
        {
            if (!pixmap.isNull())
                Q_EMIT onWallpaper(id, pixmap, QPrivateSignal());
        });
        QThreadPool::globalInstance()->start(task);
    }
}

void core_dispatcher::onThemesWallpaperPreviewGetResult(const int64_t _seq, core::coll_helper _params)
{
    const auto id = QString::fromUtf8(_params.get_value_as_string("id"));
    const auto data = _params.get_value_as_stream("data");

    if (data->empty())
    {
        Q_EMIT onWallpaperPreview(id, QPixmap(), QPrivateSignal());
    }
    else
    {
        auto task = new Utils::LoadPixmapFromDataTask(data);
        connect(task, &Utils::LoadPixmapFromDataTask::loadedSignal, this, [this, id](const QPixmap& pixmap)
        {
            if (!pixmap.isNull())
                Q_EMIT onWallpaperPreview(id, pixmap, QPrivateSignal());
        });
        QThreadPool::globalInstance()->start(task);
    }
}

void core_dispatcher::onChatsInfoGetResult(const int64_t _seq, core::coll_helper _params)
{
    const auto info = std::make_shared<Data::ChatInfo>(Data::UnserializeChatInfo(&_params));

    if (!info->AimId_.isEmpty())
        Q_EMIT chatInfo(_seq, info, _params.get_value_as_int("members_limit"));
}

void core_dispatcher::onChatsInfoGetFailed(const int64_t _seq, core::coll_helper _params)
{
    auto errorCode = _params.get_value_as_int("error");
    auto aimId = QString::fromUtf8(_params.get_value_as_string("aimid"));

    Q_EMIT chatInfoFailed(_seq, static_cast<core::group_chat_info_errors>(errorCode), aimId);
}

void Ui::core_dispatcher::onMemberAddFailed(const int64_t _seq, core::coll_helper _params)
{
    auto errorCode = _params.get_value_as_int("sip_code");
    Q_EMIT memberAddFailed(errorCode);
}

void Ui::core_dispatcher::onChatsMemberInfoResult(const int64_t _seq, core::coll_helper _params)
{
    if (_params->is_value_exist("error"))
    {
        Q_EMIT chatMemberInfo(_seq, nullptr);
        return;
    }

    const auto info = std::make_shared<Data::ChatMembersPage>(Data::UnserializeChatMembersPage(&_params));
    Q_EMIT chatMemberInfo(_seq, info);
}

void core_dispatcher::onChatsMembersGetResult(const int64_t _seq, core::coll_helper _params)
{
    if (_params->is_value_exist("error"))
    {
        Q_EMIT chatMembersPage(_seq, nullptr, -1, false);
        return;
    }

    const auto info = std::make_shared<Data::ChatMembersPage>(Data::UnserializeChatMembersPage(&_params));
    Q_EMIT chatMembersPage(_seq, info, _params.get_value_as_int("page_size"), _params.get_value_as_bool("reset_pages", false));
}

void Ui::core_dispatcher::onChatsMembersSearchResult(const int64_t _seq, core::coll_helper _params)
{
    if (_params->is_value_exist("error"))
    {
        Q_EMIT searchChatMembersPage(_seq, nullptr, -1, false);
        return;
    }

    const auto info = std::make_shared<Data::ChatMembersPage>(Data::UnserializeChatMembersPage(&_params));
    Q_EMIT searchChatMembersPage(_seq, info, _params.get_value_as_int("page_size"), _params.get_value_as_bool("reset_pages", false));
}

void core_dispatcher::onChatsContactsResult(const int64_t _seq, core::coll_helper _params)
{
    const auto contacts = std::make_shared<Data::ChatContacts>(Data::UnserializeChatContacts(&_params));
    Q_EMIT chatContacts(_seq, contacts);
}

void core_dispatcher::onLoginResultAttachUin(const int64_t _seq, core::coll_helper _params)
{
    bool result = _params.get_value_as_bool("result");
    int error = result ? 0 : _params.get_value_as_int("error");

    Q_EMIT loginResultAttachUin(_seq, error);
}

void core_dispatcher::onLoginResultAttachPhone(const int64_t _seq, core::coll_helper _params)
{
    bool result = _params.get_value_as_bool("result");
    int error = result ? 0 : _params.get_value_as_int("error");

    Q_EMIT loginResultAttachPhone(_seq, error);
}

void core_dispatcher::onUpdateProfileResult(const int64_t _seq, core::coll_helper _params)
{
    Q_EMIT updateProfile(_params.get_value_as_int("error"));
}

void core_dispatcher::onChatsHomeGetResult(const int64_t _seq, core::coll_helper _params)
{
    const auto result = Data::UnserializeChatHome(&_params);

    Q_EMIT chatsHome(result.chats, result.newTag, result.restart, result.finished);
}

void core_dispatcher::onChatsHomeGetFailed(const int64_t _seq, core::coll_helper _params)
{
    int error = _params.get_value_as_int("error");

    Q_EMIT chatsHomeError(error);
}

void core_dispatcher::onUserProxyResult(const int64_t _seq, core::coll_helper _params)
{
    Utils::ProxySettings userProxy;

    userProxy.type_ = _params.get_value_as_enum<core::proxy_type>("settings_proxy_type", core::proxy_type::auto_proxy);
    userProxy.proxyServer_ = QString::fromUtf8(_params.get_value_as_string("settings_proxy_server", ""));
    userProxy.port_ = _params.get_value_as_int("settings_proxy_port", Utils::ProxySettings::invalidPort);
    userProxy.username_ = QString::fromUtf8(_params.get_value_as_string("settings_proxy_username", ""));
    userProxy.password_ = QString::fromUtf8(_params.get_value_as_string("settings_proxy_password", ""));
    userProxy.needAuth_ = _params.get_value_as_bool("settings_proxy_need_auth", false);
    userProxy.authType_ = _params.get_value_as_enum<core::proxy_auth>("settings_proxy_auth_type", core::proxy_auth::basic);

    *Utils::get_proxy_settings() = userProxy;

    Q_EMIT getUserProxy();
}

void core_dispatcher::onOpenCreatedChat(const int64_t _seq, core::coll_helper _params)
{
    Q_EMIT openChat(QString::fromUtf8(_params.get_value_as_string("aimId")));
}

void core_dispatcher::onLoginNewUser(const int64_t _seq, core::coll_helper _params)
{
    Q_EMIT login_new_user();
}

void core_dispatcher::onSetAvatarResult(const int64_t _seq, core::coll_helper _params)
{
    Q_EMIT Utils::InterConnector::instance().setAvatar(_params.get_value_as_int64("seq"), _params.get_value_as_int("error"));
    Q_EMIT Utils::InterConnector::instance().setAvatarId(QString::fromUtf8(_params.get_value_as_string("id")));
}

void core_dispatcher::onChatsRoleSetResult(const int64_t _seq, core::coll_helper _params)
{
    Q_EMIT setChatRoleResult(_params.get_value_as_int("error"));
}

void core_dispatcher::onChatsBlockResult(const int64_t _seq, core::coll_helper _params)
{
    Q_EMIT blockMemberResult(_params.get_value_as_int("error"));
}

void core_dispatcher::onChatsPendingResolveResult(const int64_t _seq, core::coll_helper _params)
{
    Q_EMIT pendingListResult(_params.get_value_as_int("error"));
}

void core_dispatcher::onPhoneinfoResult(const int64_t _seq, core::coll_helper _params)
{
    Data::PhoneInfo data;
    data.deserialize(&_params);

    Q_EMIT phoneInfoResult(_seq, data);
}

void core_dispatcher::onContactRemovedFromIgnore(const int64_t _seq, core::coll_helper _params)
{
    auto cl = std::make_shared<Data::ContactList>();

    QString type;

    Data::UnserializeContactList(&_params, *cl, type);

    Q_EMIT contactList(cl, type);
}

void core_dispatcher::onFriendlyUpdate(const int64_t _seq, core::coll_helper _params)
{
    if (const auto friendlyList = _params.get_value_as_array("friendly_list"))
    {
        const bool isDefault = false;
        for (core::iarray::size_type i = 0, size = friendlyList->size(); i < size; ++i)
        {
            if (const auto val = friendlyList->get_at(i))
            {
                if (const auto item = val->get_as_collection())
                {
                    const auto aimid = QString::fromUtf8(item->get_value("sn")->get_as_string());
                    const auto friendly = QString::fromUtf8(item->get_value("friendly")->get_as_string()).trimmed();
                    const auto nick = QString::fromUtf8(item->get_value("nick")->get_as_string());
                    const auto official = item->get_value("official")->get_as_bool();
                    Q_EMIT friendlyUpdate(aimid, { friendly, nick, official, isDefault }, QPrivateSignal());
                }
            }
        }
    }
}

void Ui::core_dispatcher::onHistoryUpdate(const int64_t _seq, core::coll_helper _params)
{
    qint64 last_read_msg = _params.get_value_as_int64("last_read_msg");

    Q_EMIT historyUpdate(QString::fromUtf8(_params.get_value_as_string("contact")), last_read_msg);
}

void core_dispatcher::onUpdateReady(const int64_t _seq, core::coll_helper _params)
{
    Q_EMIT updateReady();
}

void core_dispatcher::onUpToDate(const int64_t _seq, core::coll_helper _params)
{
    Q_EMIT upToDate(_seq, _params.get_value_as_bool("is_network_error"));
}

void core_dispatcher::onEventCachedObjectsLoaded(const int64_t _seq, core::coll_helper _params)
{
    statistic::getGuiMetrics().eventCacheReady();
}

void core_dispatcher::onEventConnectionState(const int64_t _seq, core::coll_helper _params)
{
    const std::string state_s = _params.get_value_as_string("state");

    Ui::ConnectionState newState = Ui::ConnectionState::stateUnknown;

    if (state_s == "online")
    {
        newState = Ui::ConnectionState::stateOnline;

        statistic::getGuiMetrics().eventConnectionDataReady();
    }
    else if (state_s == "connecting")
    {
        newState = Ui::ConnectionState::stateConnecting;
    }
    else if (state_s == "updating")
    {
        newState = Ui::ConnectionState::stateUpdating;
    }

    assert(newState != Ui::ConnectionState::stateUnknown);

    if (connectionState_ != newState)
    {
        connectionState_ = newState;

        Q_EMIT connectionStateChanged(connectionState_);
    }
}

void core_dispatcher::onGetDialogGalleryResult(const int64_t _seq, core::coll_helper _params)
{
    QString aimid;
    if (_params.is_value_exist("aimid"))
        aimid = _params.get<QString>("aimid");

    QVector<Data::DialogGalleryEntry> entries;
    if (_params.is_value_exist("entries"))
    {
        auto entriesArray = _params.get_value_as_array("entries");
        for (auto i = 0; entriesArray && i < entriesArray->size(); ++i)
        {
            auto entry_coll = core::coll_helper(entriesArray->get_at(i)->get_as_collection(), false);
            entries.push_back(Data::UnserializeGalleryEntry(&entry_coll));
        }
    }

    auto exhausted = false;
    if (_params.is_value_exist("exhausted"))
        exhausted = _params.get_value_as_bool("exhausted");

    Q_EMIT dialogGalleryResult(_seq, entries, exhausted);
}

void core_dispatcher::onGetDialogGalleryByMsgResult(const int64_t _seq, core::coll_helper _params)
{
    QString aimid;
    if (_params.is_value_exist("aimid"))
        aimid = _params.get<QString>("aimid");

    QVector<Data::DialogGalleryEntry> entries;
    if (_params.is_value_exist("entries"))
    {
        auto entriesArray = _params.get_value_as_array("entries");
        for (auto i = 0; entriesArray && i < entriesArray->size(); ++i)
        {
            auto entry_coll = core::coll_helper(entriesArray->get_at(i)->get_as_collection(), false);
            entries.push_back(Data::UnserializeGalleryEntry(&entry_coll));
        }
    }

    auto index = 0;
    if (_params.is_value_exist("index"))
        index = _params.get_value_as_int("index");

    auto total = 0;
    if (_params.is_value_exist("total"))
        total = _params.get_value_as_int("total");

    Q_EMIT dialogGalleryByMsgResult(_seq, entries, index, total);
}

void core_dispatcher::onGalleryState(const int64_t _seq, core::coll_helper _params)
{
    auto aimid = _params.get<QString>("aimid");
    auto state = Data::UnserializeDialogGalleryState(&_params);
    Q_EMIT dialogGalleryState(aimid, state);
}

void core_dispatcher::onGalleryUpdate(const int64_t _seq, core::coll_helper _params)
{
    auto aimid = _params.get<QString>("aimid");
    QVector<Data::DialogGalleryEntry> entries;
    if (_params.is_value_exist("entries"))
    {
        auto entriesArray = _params.get_value_as_array("entries");
        for (auto i = 0; entriesArray && i < entriesArray->size(); ++i)
        {
            auto entry_coll = core::coll_helper(entriesArray->get_at(i)->get_as_collection(), false);
            entries.push_back(Data::UnserializeGalleryEntry(&entry_coll));
        }
    }

    Q_EMIT dialogGalleryUpdate(aimid, entries);
}

void core_dispatcher::onGalleryInit(const int64_t _seq, core::coll_helper _params)
{
    auto aimid = _params.get<QString>("aimid");
    Q_EMIT dialogGalleryInit(aimid);
}

void core_dispatcher::onGalleryHolesDownloaded(const int64_t _seq, core::coll_helper _params)
{
    auto aimid = _params.get<QString>("aimid");
    Q_EMIT dialogGalleryHolesDownloaded(aimid);
}

void core_dispatcher::onGalleryHolesDownloading(const int64_t _seq, core::coll_helper _params)
{
    auto aimid = _params.get<QString>("aimid");
    Q_EMIT dialogGalleryHolesDownloading(aimid);
}

void core_dispatcher::onGalleryErased(const int64_t _seq, core::coll_helper _params)
{
    auto aimid = _params.get<QString>("aimid");
    Q_EMIT dialogGalleryErased(aimid);
}

void core_dispatcher::onGalleryIndex(const int64_t _seq, core::coll_helper _params)
{
    auto aimid = _params.get<QString>("aimid");
    auto msg = _params.get<int64_t>("msg");
    auto seq = _params.get<int64_t>("seq");
    auto index = _params.get<int>("index");
    auto total = _params.get<int>("total");
    Q_EMIT dialogGalleryIndex(aimid, msg, seq, index, total);
}

void core_dispatcher::onLocalPINChecked(const int64_t _seq, core::coll_helper _params)
{
    auto result = _params.get<bool>("result");

    Q_EMIT localPINChecked(_seq, result);
}

void core_dispatcher::onIdInfo(const int64_t _seq, core::coll_helper _params)
{
    auto info = Data::UnserializeIdInfo(_params);
    Q_EMIT idInfo(_seq, info, QPrivateSignal());
}

ConnectionState core_dispatcher::getConnectionState() const
{
    return connectionState_;
}

void core_dispatcher::onRequestMemoryConsumption(const int64_t _seq, core::coll_helper _params)
{
    Q_UNUSED(_seq);
    Memory_Stats::RequestHandle req_handle(-1);
    if (!req_handle.unserialize(_params))
    {
        assert(!"couldn't unserialize req_handle");
        return;
    }

    GuiMemoryMonitor::instance().asyncGetReportsFor(req_handle);
}

void core_dispatcher::onMemUsageReportReady(const int64_t _seq, core::coll_helper _params)
{
    Q_UNUSED(_seq);
    Memory_Stats::Response response;
    if (!response.unserialize(_params))
    {
        assert(!"couldn't unserialize mem usage response");
        return;
    }

    Q_EMIT memoryUsageReportReady(response, _params.get_value_as_int64("request_id"));
}

void core_dispatcher::onGuiComponentsMemoryReportsReady(const Memory_Stats::RequestHandle &_req_handle,
                                                        const Memory_Stats::ReportsList &_reports_list)
{
    Memory_Stats::PartialResponse part_response;
    part_response.reports_ = std::move(_reports_list);

    core::coll_helper helper(create_collection(), true);
    part_response.serialize(helper);
    helper.set_value_as_int64("request_id", _req_handle.id_);
    post_message_to_core("report_memory_usage", helper.get());
}

void core_dispatcher::onGetUserInfoResult(const int64_t _seq, core::coll_helper _params)
{
    auto info = Data::UnserializeUserInfo(&_params);
    Q_EMIT userInfo(_seq, QString::fromUtf8(_params.get_value_as_string("aimid")), info);
}

void core_dispatcher::onGetUserLastSeenResult(const int64_t _seq, core::coll_helper _params)
{
    if (_params.is_value_exist("ids"))
    {
        std::map<QString, Data::LastSeen> lastseens;
        auto array = _params.get_value_as_array("ids");
        if (!array->empty())
        {
            for (int i = 0; i < array->size(); ++i)
            {
                core::coll_helper helper(array->get_at(i)->get_as_collection(), false);
                lastseens.insert({ QString::fromUtf8(helper.get_value_as_string("aimid")), Data::LastSeen(helper) });
            }

            Q_EMIT userLastSeen(_seq, lastseens);
        }
    }
}

void core_dispatcher::onSetPrivacySettingsResult(const int64_t _seq, core::coll_helper _params)
{
    const auto error = _params.get<int>("error");
    Q_EMIT setPrivacyParamsResult(_seq, error == 0);
}

void core_dispatcher::onGetCommonChatsResult(const int64_t _seq, core::coll_helper _params)
{
    if (_params.is_value_exist("chats"))
    {
        core::iarray* chatsArray = _params.get_value_as_array("chats");
        const auto size = chatsArray->size();
        std::vector<Data::CommonChatInfo> chats;
        chats.reserve(size);
        for (core::iarray::size_type i = 0; i < chatsArray->size(); ++i)
        {
            core::coll_helper value(chatsArray->get_at(i)->get_as_collection(), false);
            Data::CommonChatInfo info;
            info.aimid_ = QString::fromUtf8(value.get_value_as_string("aimid"));
            info.friendly_ = QString::fromUtf8(value.get_value_as_string("friendly"));
            info.membersCount_ = value.get_value_as_int("membersCount");

            chats.push_back(info);
        }

        Q_EMIT commonChatsGetResult(_seq, chats);
    }
}

void core_dispatcher::postUiActivity()
{
    core::coll_helper helper(create_collection(), true);

    const auto current_time = std::chrono::system_clock::now();

    helper.set_value_as_int64("time", int64_t(std::chrono::system_clock::to_time_t(current_time)));

    post_message_to_core("_ui_activity", helper.get());
}

void core_dispatcher::setLocalPIN(const QString& _password)
{
    Ui::gui_coll_helper collection(Ui::GetDispatcher()->create_collection(), true);
    collection.set_value_as_qstring("password", _password);

    post_message_to_core("localpin/set", collection.get());
}

qint64 core_dispatcher::getUserInfo(const QString& _aimid)
{
    Ui::gui_coll_helper collection(Ui::GetDispatcher()->create_collection(), true);
    collection.set_value_as_qstring("contact", _aimid);

    return post_message_to_core("get_user_info", collection.get());
}

qint64 core_dispatcher::getUserLastSeen(const std::vector<QString>& _ids)
{
    Ui::gui_coll_helper collection(Ui::GetDispatcher()->create_collection(), true);
    collection.set_value_as_array("contacts", toArrayOfStrings(_ids, collection).get());
    return post_message_to_core("get_user_last_seen", collection.get());
}

qint64 core_dispatcher::subscribeStatus(const std::vector<QString>& _ids)
{
    Ui::gui_coll_helper collection(Ui::GetDispatcher()->create_collection(), true);
    collection.set_value_as_array("contacts", toArrayOfStrings(_ids, collection).get());
    return post_message_to_core("subscribe/status", collection.get());
}

qint64 core_dispatcher::unsubscribeStatus(const std::vector<QString>& _ids)
{
    Ui::gui_coll_helper collection(Ui::GetDispatcher()->create_collection(), true);
    collection.set_value_as_array("contacts", toArrayOfStrings(_ids, collection).get());
    return post_message_to_core("unsubscribe/status", collection.get());
}

qint64 core_dispatcher::subscribeCallRoomInfo(const QString& _roomId)
{
    if (!Features::callRoomInfoEnabled())
        return 0;

    Ui::gui_coll_helper collection(Ui::GetDispatcher()->create_collection(), true);
    collection.set_value_as_qstring("room_id", _roomId);
    return post_message_to_core("subscribe/call_room_info", collection.get());
}

qint64 core_dispatcher::unsubscribeCallRoomInfo(const QString& _roomId)
{
    Ui::gui_coll_helper collection(Ui::GetDispatcher()->create_collection(), true);
    collection.set_value_as_qstring("room_id", _roomId);
    return post_message_to_core("unsubscribe/call_room_info", collection.get());
}

qint64 core_dispatcher::localPINEntered(const QString& _password)
{
    Ui::gui_coll_helper collection(Ui::GetDispatcher()->create_collection(), true);
    collection.set_value_as_qstring("password", _password);

    return post_message_to_core("localpin/entered", collection.get());
}

void core_dispatcher::disableLocalPIN()
{
    core::coll_helper helper(create_collection(), true);

    post_message_to_core("localpin/disable", helper.get());
}

qint64 core_dispatcher::getIdInfo(const QString& _id)
{
    Ui::gui_coll_helper collection(Ui::GetDispatcher()->create_collection(), true);
    collection.set_value_as_qstring("id", _id);

    return post_message_to_core("idinfo/get", collection.get());
}

qint64 core_dispatcher::checkNickname(const QString& _nick, bool _set_nick)
{
    Ui::gui_coll_helper collection(Ui::GetDispatcher()->create_collection(), true);
    collection.set_value_as_qstring("nick", _nick);
    collection.set_value_as_bool("set_nick", _set_nick);

    return post_message_to_core("nickname/check", collection.get());
}

qint64 core_dispatcher::checkGroupNickname(const QString& _nick)
{
    Ui::gui_coll_helper collection(Ui::GetDispatcher()->create_collection(), true);
    collection.set_value_as_qstring("nick", _nick);

    return post_message_to_core("group_nickname/check", collection.get());
}

void core_dispatcher::updateRecentAvatarsSize()
{
    Ui::gui_coll_helper collection(Ui::GetDispatcher()->create_collection(), true);
    collection.set_value_as_int("size", Utils::scale_bitmap(Ui::GetRecentsParams().getAvatarSize()));

    post_message_to_core("recent_avatars_size/update", collection.get());
}

void Ui::core_dispatcher::setInstallBetaUpdates(bool _enabled)
{
    Ui::gui_coll_helper collection(Ui::GetDispatcher()->create_collection(), true);
    collection.set_value_as_bool("enabled", _enabled);

    post_message_to_core("set_install_beta_updates", collection.get());
}

bool Ui::core_dispatcher::isInstallBetaUpdatesEnabled() const
{
    return Features::betaUpdateAllowed() && installBetaUpdates_;
}

int64_t core_dispatcher::getPoll(const QString& _pollId)
{
    Ui::gui_coll_helper collection(Ui::GetDispatcher()->create_collection(), true);
    collection.set_value_as_qstring("id", _pollId);

    return post_message_to_core("poll/get", collection.get());
}

int64_t core_dispatcher::voteInPoll(const QString& _pollId, const QString& _answerId)
{
    Ui::gui_coll_helper collection(Ui::GetDispatcher()->create_collection(), true);
    collection.set_value_as_qstring("id", _pollId);
    collection.set_value_as_qstring("answer_id", _answerId);

    return post_message_to_core("poll/vote", collection.get());
}

int64_t core_dispatcher::revokeVote(const QString& _pollId)
{
    Ui::gui_coll_helper collection(Ui::GetDispatcher()->create_collection(), true);
    collection.set_value_as_qstring("id", _pollId);

    return post_message_to_core("poll/revoke", collection.get());
}

int64_t core_dispatcher::stopPoll(const QString& _pollId)
{
    Ui::gui_coll_helper collection(Ui::GetDispatcher()->create_collection(), true);
    collection.set_value_as_qstring("id", _pollId);

    return post_message_to_core("poll/stop", collection.get());
}

int64_t core_dispatcher::getBotCallbackAnswer(const QString& _chatId, const QString& _callbackData, qint64 _msgId)
{
    Ui::gui_coll_helper collection(Ui::GetDispatcher()->create_collection(), true);
    collection.set_value_as_qstring("chat_id", _chatId);
    collection.set_value_as_qstring("callback_data", _callbackData);
    collection.set_value_as_int64("msg_id", _msgId);

    return post_message_to_core("get_bot_callback_answer", collection.get());
}

qint64 core_dispatcher::pinContact(const QString& _aimId, bool _enable)
{
    Ui::gui_coll_helper collection(Ui::GetDispatcher()->create_collection(), true);
    collection.set_value_as_qstring("contact", _aimId);

    return post_message_to_core(_enable ? "pin_chat" : "unfavorite", collection.get());
}

qint64 Ui::core_dispatcher::loadChatMembersInfo(const QString& _aimId, const std::vector<QString>& _members)
{
    if (_members.empty())
        return -1;

    Ui::gui_coll_helper collection(Ui::GetDispatcher()->create_collection(), true);
    collection.set_value_as_qstring("aimid", _aimId);
    collection.set_value_as_array("members", toArrayOfStrings(_members, collection).get());

    return post_message_to_core("chats/member/info", collection.get());
}

qint64 core_dispatcher::requestSessionsList()
{
    return post_message_to_core("sessions/get", nullptr);
}

qint64 core_dispatcher::cancelGalleryHolesDownloading(const QString& _aimid)
{
    if (_aimid.isEmpty())
        return -1;

    Ui::gui_coll_helper collection(Ui::GetDispatcher()->create_collection(), true);
    collection.set_value_as_qstring("aimid", _aimid);

    return post_message_to_core("stop_gallery_holes_downloading", collection.get());
}

int64_t core_dispatcher::createConference(ConferenceType _type, const std::vector<QString>& _participants, bool _callParticipants)
{
    Ui::gui_coll_helper collection(Ui::GetDispatcher()->create_collection(), true);
    collection.set_value_as_qstring("name", MyInfo()->friendly());
    collection.set_value_as_bool("is_webinar", _type == ConferenceType::Webinar);
    collection.set_value_as_bool("call_participants", _callParticipants);

    core::ifptr<core::iarray> participantsArray(collection->create_array());
    participantsArray->reserve(_participants.size());
    for (auto participant : _participants)
        participantsArray->push_back(collection.create_qstring_value(participant).get());
    collection.set_value_as_array("participants", participantsArray.get());

    return post_message_to_core("conference/create", collection.get());
}

qint64 core_dispatcher::resolveChatPendings(const QString& _chatAimId, const std::vector<QString>& _pendings, bool _approve)
{
    if (_pendings.empty())
        return -1;

    Ui::gui_coll_helper collection(Ui::GetDispatcher()->create_collection(), true);
    collection.set_value_as_qstring("aimid", _chatAimId);
    core::ifptr<core::iarray> contacts_array(collection->create_array());
    contacts_array->reserve(_pendings.size());
    for (const auto& aimid : _pendings)
        contacts_array->push_back(collection.create_qstring_value(aimid).get());
    collection.set_value_as_array("contacts", contacts_array.get());
    collection.set_value_as_bool("approve", _approve);
    return post_message_to_core("chats/pending/resolve", collection.get());
}

void core_dispatcher::getReactions(const QString& _chatId, std::vector<int64_t> _msgIds, bool _firstLoad)
{
    Ui::gui_coll_helper collection(Ui::GetDispatcher()->create_collection(), true);
    collection.set_value_as_qstring("chat_id", _chatId);
    collection.set_value_as_bool("first_load", _firstLoad);

    core::ifptr<core::iarray> idsArray(collection->create_array());
    idsArray->reserve(_msgIds.size());
    for (auto id : _msgIds)
    {
        core::ifptr<core::ivalue> val(collection->create_value());
        val->set_as_int64(id);
        idsArray->push_back(val.get());
    }
    collection.set_value_as_array("ids", idsArray.get());

    post_message_to_core("reaction/get", collection.get());

}

int64_t core_dispatcher::addReaction(int64_t _msgId, const QString& _chatId, const QString& _reaction, const std::vector<QString>& _reactionsList)
{
    Ui::gui_coll_helper collection(Ui::GetDispatcher()->create_collection(), true);

    collection.set_value_as_int64("msg_id", _msgId);
    collection.set_value_as_qstring("chat_id", _chatId);
    collection.set_value_as_qstring("reaction", _reaction);

    core::ifptr<core::iarray> reactionsArray(collection->create_array());
    reactionsArray->reserve(_reactionsList.size());
    for (const auto& reaction : _reactionsList)
        reactionsArray->push_back(collection.create_qstring_value(reaction).get());
    collection.set_value_as_array("reactions", reactionsArray.get());

    return post_message_to_core("reaction/add", collection.get());
}

int64_t core_dispatcher::removeReaction(int64_t _msgId, const QString& _chatId)
{
    Ui::gui_coll_helper collection(Ui::GetDispatcher()->create_collection(), true);

    collection.set_value_as_int64("msg_id", _msgId);
    collection.set_value_as_qstring("chat_id", _chatId);

    return post_message_to_core("reaction/remove", collection.get());
}

int64_t core_dispatcher::getReactionsPage(int64_t _msgId, const QString& _chatId, const QString& _reaction, const QString& _newerThan, const QString& _olderThan, int64_t _limit)
{
    Ui::gui_coll_helper collection(Ui::GetDispatcher()->create_collection(), true);

    collection.set_value_as_int64("msg_id", _msgId);
    collection.set_value_as_qstring("chat_id", _chatId);
    collection.set_value_as_qstring("reaction", _reaction);
    collection.set_value_as_qstring("newer_than", _newerThan);
    collection.set_value_as_qstring("older_than", _olderThan);
    collection.set_value_as_int64("limit", _limit);

    return post_message_to_core("reaction/list", collection.get());
}

void core_dispatcher::onNickCheckSetResult(const int64_t _seq, core::coll_helper _params)
{
    Q_EMIT nickCheckSetResult(_seq, _params.get_value_as_int("error"));
}

void core_dispatcher::onGroupNickCheckSetResult(const int64_t _seq, core::coll_helper _params)
{
    Q_EMIT groupNickCheckSetResult(_seq, _params.get_value_as_int("error"));
}

void Ui::core_dispatcher::onOmicronUpdateData(const int64_t _seq, core::coll_helper _params)
{
    Omicron::unserialize(_params);
    Q_EMIT Utils::InterConnector::instance().omicronUpdated();
}

void Ui::core_dispatcher::onFilesharingUploadAborted(const int64_t _seq, core::coll_helper _params)
{
    auto message = std::make_shared<Data::MessageBuddy>();

    message->InternalId_ = QString::fromUtf8(_params.get_value_as_string("id"));

    Q_EMIT messagesDeleted(QString::fromUtf8(_params.get_value_as_string("contact")), { message });
}

void core_dispatcher::onPollGetResult(const int64_t _seq, core::coll_helper _params)
{
    Q_UNUSED(_seq)

    Data::PollData poll;
    poll.unserialize(_params.get_value_as_collection("poll"));

    Q_EMIT pollLoaded(poll);
}

void core_dispatcher::onPollVoteResult(const int64_t _seq, core::coll_helper _params)
{
    Data::PollData poll;
    poll.unserialize(_params.get_value_as_collection("poll"));
    bool success = (_params.get_value_as_int("error") == 0);

    Q_EMIT voteResult(_seq, poll, success);
}

void core_dispatcher::onPollRevokeResult(const int64_t _seq, core::coll_helper _params)
{
    bool success = (_params.get_value_as_int("error") == 0);
    Q_EMIT revokeVoteResult(_seq, _params.get<QString>("id"), success);
}

void core_dispatcher::onPollStopResult(const int64_t _seq, core::coll_helper _params)
{
    bool success = (_params.get_value_as_int("error") == 0);
    Q_EMIT stopPollResult(_seq, _params.get<QString>("id"), success);
}

void core_dispatcher::onPollUpdate(const int64_t _seq, core::coll_helper _params)
{
    Q_UNUSED(_seq)

    Data::PollData poll;
    poll.unserialize(_params.get_value_as_collection("poll"));

    Q_EMIT pollLoaded(poll);
}

void Ui::core_dispatcher::onModChatParamsResult(const int64_t _seq, core::coll_helper _params)
{
    Q_EMIT modChatParamsResult(_params.get_value_as_int("error"));
}

void Ui::core_dispatcher::onModChatAboutResult(const int64_t _seq, core::coll_helper _params)
{
    Q_EMIT modChatAboutResult(_seq, _params.get_value_as_int("error"));
}

void Ui::core_dispatcher::onSuggestGroupNickResult(const int64_t _seq, core::coll_helper _params)
{
    Q_EMIT suggestGroupNickResult(QString::fromUtf8(_params.get_value_as_string("nick")));
}

void core_dispatcher::onSessionStarted(const int64_t _seq, core::coll_helper _params)
{
    Q_UNUSED(_seq)

    const auto aimId = _params.get<QString>("aimId");
    Ui::MyInfo()->setAimId(aimId);

    Q_EMIT sessionStarted(aimId);
}

void core_dispatcher::onExternalUrlConfigError(const int64_t, core::coll_helper _params)
{
    Q_EMIT externalUrlConfigUpdated(_params.get_value_as_int("error"));
}

void core_dispatcher::onExternalUrlConfigHosts(const int64_t _seq, core::coll_helper _params)
{
    Ui::getUrlConfig().updateUrls(_params);
    Q_EMIT externalUrlConfigUpdated(0);
}

void core_dispatcher::onGroupSubscribeResult(const int64_t _seq, core::coll_helper _params)
{
    Q_UNUSED(_seq);

    Q_EMIT subscribeResult(_params.get_value_as_int("error"), _params.get_value_as_int("resubscribe_in"));
}

void core_dispatcher::onGetSessionsResult(const int64_t _seq, core::coll_helper _params)
{
    std::vector<Data::SessionInfo> sessions;
    if (!_params.is_value_exist("error"))
    {
        const auto arr = _params.get_value_as_array("sessions");
        const auto arrSize = arr->size();
        sessions.reserve(arrSize);

        for (std::remove_const_t<decltype(arrSize)> i = 0; i < arrSize; ++i)
        {
            Ui::gui_coll_helper coll_session(arr->get_at(i)->get_as_collection(), false);

            Data::SessionInfo si;
            si.unserialize(coll_session);
            sessions.push_back(std::move(si));
        }
    }
    Q_EMIT activeSessionsList(sessions);
}

void core_dispatcher::onResetSessionResult(const int64_t _seq, core::coll_helper _params)
{
    Q_EMIT sessionResetResult(QString::fromUtf8(_params.get_value_as_string("hash")), _params.get_value_as_int("error") == 0);
}


void Ui::core_dispatcher::onCreateConferenceResult(const int64_t _seq, core::coll_helper _params)
{
    const auto error = _params.get_value_as_int("error");
    const auto isWebinar = _params.get_value_as_bool("is_webinar");
    const auto url = QString::fromUtf8(_params.get_value_as_string("url"));
    const auto expires = _params.get_value_as_int64("expires");

    Q_EMIT createConferenceResult(_seq, error, isWebinar, url, expires, QPrivateSignal());
}

void core_dispatcher::onCallLog(const int64_t _seq, core::coll_helper _params)
{
    Q_EMIT callLog(Data::UnserializeCallLog(&_params));
}

void core_dispatcher::onNewCall(const int64_t _seq, core::coll_helper _params)
{
    Q_EMIT newCall(Data::UnserializeCallInfo(&_params));
}

void core_dispatcher::onCallRemoveMessages(const int64_t _seq, core::coll_helper _params)
{
    std::vector<qint64> ids;
    auto messages = _params.get_value_as_array("messages");
    if (messages)
    {
        const auto size = messages->size();
        ids.reserve(size);
        for (int32_t i = 0; i < size; ++i)
        {
            ids.push_back(messages->get_at(i)->get_as_int64());
        }
    }

    Q_EMIT callRemoveMessages(QString::fromUtf8(_params.get_value_as_string("aimid")), ids);
}

void core_dispatcher::onCallDelUpTo(const int64_t _seq, core::coll_helper _params)
{
    Q_EMIT callDelUpTo(QString::fromUtf8(_params.get_value_as_string("aimid")), _params.get_value_as_int64("del_up_to"));
}

void core_dispatcher::onCallRemoveContact(const int64_t _seq, core::coll_helper _params)
{
    Q_EMIT callRemoveContact(QString::fromUtf8(_params.get_value_as_string("aimid")));
}

void core_dispatcher::onReactions(const int64_t _seq, core::coll_helper _params)
{
    const auto contact = _params.get<QString>("contact");
    const auto reactionsArray = _params.get_value_as_array("reactions");

    std::vector<Data::Reactions> reactionsData;
    reactionsData.reserve(reactionsArray->size());

    for (auto i = 0; i < reactionsArray->size(); ++i)
    {
        Ui::gui_coll_helper coll_reactions(reactionsArray->get_at(i)->get_as_collection(), false);

        Data::Reactions reactionsItem;
        reactionsItem.unserialize(coll_reactions.get());
        reactionsData.push_back(std::move(reactionsItem));
    }

    Q_EMIT reactions(contact, reactionsData);
}

void core_dispatcher::onReactionsListResult(const int64_t _seq, core::coll_helper _params)
{
    Data::ReactionsPage reactions;

    auto error = 0;
    if(_params.is_value_exist("error"))
        error = _params.get_value_as_int("error");
    else
        reactions.unserialize(_params.get());

    Q_EMIT reactionsListResult(_seq, reactions, error == 0);
}

void core_dispatcher::onReactionAddResult(const int64_t _seq, core::coll_helper _params)
{
    const auto reactionsColl = _params.get_value_as_collection("reactions");

    auto error = 0;
    if(_params.is_value_exist("error"))
        error = _params.get_value_as_int("error");

    const auto success = error == 0;

    Data::Reactions reactions;
    if (success)
        reactions.unserialize(reactionsColl);

    Q_EMIT reactionAddResult(_seq, reactions, success);
}

void core_dispatcher::onReactionRemoveResult(const int64_t _seq, core::coll_helper _params)
{
    auto error = _params.get_value_as_int("error");
    Q_EMIT reactionRemoveResult(_seq, error == 0);
}

void core_dispatcher::onStatus(const int64_t _seq, core::coll_helper _params)
{
    if (_params.is_value_exist("status"))
    {
        Statuses::Status status;

        core::coll_helper st(_params.get_value_as_collection("status"), false);
        status.unserialize(st);

        Q_EMIT updateStatus(QString::fromUtf8(_params.get_value_as_string("aimId")), status);
    }
}

void core_dispatcher::onCallRoomInfo(const int64_t _seq, core::coll_helper _params)
{
    Q_UNUSED(_seq)

    Q_EMIT callRoomInfo(_params.get<QString>("room_id"), _params.get_value_as_int64("members_count"), _params.get_value_as_bool("failed"));
}

void core_dispatcher::onBotCallbackAnswer(const int64_t _seq, core::coll_helper _params)
{
    QString reqId = _params.get<QString>("req_id");
    bool showAlert = false;
    QString text, url;
    if (_params.is_value_exist("payload"))
    {
        auto payloadColl = _params.get_value_as_collection("payload");
        Ui::gui_coll_helper helper(payloadColl, false);
        if (helper.is_value_exist("text"))
            text = QString::fromUtf8(helper.get_value_as_string("text"));
        if (helper.is_value_exist("url"))
            url = QString::fromUtf8(helper.get_value_as_string("url"));
        if (helper.is_value_exist("show_alert"))
            showAlert = helper.get_value_as_bool("show_alert");
    }

    Q_EMIT botCallbackAnswer(_seq, reqId, text, url, showAlert);
}

void core_dispatcher::onChatJoinResult(const int64_t _seq, core::coll_helper _params)
{
    if (_params.get_value_as_bool("blocked"))
        Q_EMIT chatJoinResultBlocked(_params.get<QString>("stamp"));
}

namespace { std::unique_ptr<core_dispatcher> gDispatcher; }

core_dispatcher* Ui::GetDispatcher()
{
    if (!gDispatcher)
    {
        assert(false);
    }

    return gDispatcher.get();
}

void Ui::createDispatcher()
{
    if (gDispatcher)
    {
        assert(false);
    }

    gDispatcher = std::make_unique<core_dispatcher>();
}

void Ui::destroyDispatcher()
{
    if (!gDispatcher)
    {
        assert(false);
    }

    gDispatcher.reset();
}
