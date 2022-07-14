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
#include "main_window/containers/StatusContainer.h"
#include "main_window/ReleaseNotes.h"
#include "main_window/mini_apps/MiniAppsUtils.h"
#include "utils/gui_coll_helper.h"
#include "utils/InterConnector.h"
#include "utils/LoadPixmapFromDataTask.h"
#include "utils/LoadPixmapFromFileTask.h"
#include "utils/utils.h"
#include "utils/features.h"
#include "utils/gui_metrics.h"
#include "utils/exif.h"
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
#include "main_window/contact_list/ServiceContacts.h"
#include "main_window/history_control/reactions/DefaultReactions.h"
#include "types/message.h"

#if defined(IM_AUTO_TESTING)
    #include "webserver/webserver.h"
#endif

#ifdef _WIN32
    #include "../common.shared/win32/crash_handler.h"
#endif

using namespace Ui;

namespace
{
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
                        qsizetype idx = view.indexOf(emojiStart, prev);
                        while (idx != -1)
                        {
                            auto next = idx;
                            const auto emoji = Emoji::getEmoji(view, next);
                            im_assert(!emoji.isNull());
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

                            idx = view.indexOf(emojiStart, prev);
                        }

                        if (idx == -1 && prev < view.size())
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

    void postMessageStats(const Data::MessageBuddy& _msg)
    {
        if (_msg.Quotes_.size() > 0)
        {
            GetDispatcher()->post_stats_to_core(_msg.GetSourceText().isEmpty() ? core::stats::stats_event_names::quotes_send_alone : core::stats::stats_event_names::quotes_send_answer);
            GetDispatcher()->post_stats_to_core(core::stats::stats_event_names::quotes_messagescount, { { "Quotes_MessagesCount", std::to_string(_msg.Quotes_.size()) } });
        }

        auto quotesMentionsCount = 0;
        for (const auto& quote : _msg.Quotes_)
            quotesMentionsCount += quote.mentions_.size();

        auto messageMentionsCount = _msg.Mentions_.size() - quotesMentionsCount;
        if (messageMentionsCount > 0)
            GetDispatcher()->post_stats_to_core(core::stats::stats_event_names::mentions_sent, { { "Mentions_Count", std::to_string(messageMentionsCount) } });
    }

    std::map<core::chat_member_failure, std::vector<QString>> unserializeChatFailures(const core::coll_helper& _params)
    {
        if (!_params.is_value_exist("failures"))
            return {};

        std::map<core::chat_member_failure, std::vector<QString>> failures;

        auto array = _params.get_value_as_array("failures");
        const auto arr_size = array->size();
        for (auto i = 0; i < arr_size; ++i)
        {
            Ui::gui_coll_helper coll_fail(array->get_at(i)->get_as_collection(), false);

            auto c_array = coll_fail.get_value_as_array("contacts");
            if (const auto cont_size = c_array->size(); cont_size > 0)
            {
                const auto type = coll_fail.get_value_as_enum<core::chat_member_failure>("type");
                auto& contacts = failures[type];

                contacts.reserve(cont_size);
                for (auto j = 0; j < cont_size; ++j)
                    contacts.push_back(QString::fromUtf8(c_array->get_at(j)->get_as_string()));
            }
        }

        return failures;
    }

    constexpr std::chrono::milliseconds typingCheckInterval = std::chrono::seconds(1);
    constexpr auto typingLifetime = std::chrono::seconds(8);

    void serializeMessageCommon(Ui::gui_coll_helper& _coll, const QString& _contact, const Data::MentionMap& _mentions, const Data::QuotesVec& _quotes)
    {
        im_assert(!_contact.isEmpty());
        _coll.set_value_as_qstring("contact", _contact);

        Data::serializeQuotes(_coll, _quotes, Data::ReplaceFilesPolicy::Replace);

        Data::MentionMap mentions(_mentions);
        for (const auto& q : _quotes)
            mentions.insert(q.mentions_.begin(), q.mentions_.end());
        Data::serializeMentions(_coll, mentions);
    }
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
#ifndef STRIP_VOIP
    , voipController_(*this)
#endif
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

#ifndef STRIP_VOIP
voip_proxy::VoipController& core_dispatcher::getVoipController()
{
    return voipController_;
}
#endif

qint64 core_dispatcher::getFileSharingPreviewSize(const QString& _url, const int32_t _originalSize)
{
    core::coll_helper helper(create_collection(), true);
    helper.set<QString>("url", _url);
    helper.set_value_as_int("orig_size", _originalSize);

    return post_message_to_core("files/preview_size", helper.get());
}

qint64 core_dispatcher::downloadFileSharingMetainfo(const Utils::FileSharingId& _fsId)
{
    core::coll_helper helper(create_collection(), true);
    helper.set<QString>("file_id", _fsId.fileId_);
    if (const auto source = _fsId.sourceId_)
        helper.set<QString>("source_id", *source);
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

qint64 core_dispatcher::uploadSharedFile(const QString& _contact, const QString& _localPath, const Data::QuotesVec& _quotes, const Data::FString& _description, const Data::MentionMap& _mentions, bool _keepExif)
{
    constexpr std::string_view c_coll_description_format = "description_format";

    gui_coll_helper collection(create_collection(), true);
    collection.set_value_as_qstring("file", _localPath);
    collection.set_value_as_qstring("description", _description.string());
    if (_description.hasFormatting())
        Data::serializeFormat(_description.formatting(), collection, c_coll_description_format);

    serializeMessageCommon(collection, _contact, _mentions, _quotes);

    if (!_keepExif)
    {
        collection.set<bool>("strip_exif", true);
        collection.set<int>("orientation_tag", (int)Utils::Exif::getExifOrientation(_localPath));
    }

    return post_message_to_core("files/upload", collection.get());
}

qint64 core_dispatcher::uploadSharedFile(const QString& _contact, const QByteArray& _array, QStringView _ext, const Data::QuotesVec& _quotes)
{
    return uploadSharedFile(_contact, _array, _ext, _quotes, QString(), {});
}

qint64 core_dispatcher::uploadSharedFile(const QString& _contact, const QByteArray& _array, QStringView _ext, const Data::QuotesVec& _quotes, const Data::FString& _description, const Data::MentionMap& _mentions)
{
    constexpr std::string_view c_coll_description_format = "description_format";

    Ui::gui_coll_helper collection(create_collection(), true);

    core::istream* stream = collection->create_stream();
    stream->write((uint8_t*)(_array.data()), _array.size());
    collection.set_value_as_stream("file_stream", stream);
    collection.set_value_as_qstring("ext", _ext);
    collection.set_value_as_qstring("description", _description.string());
    if (_description.hasFormatting())
        Data::serializeFormat(_description.formatting(), collection, c_coll_description_format);

    serializeMessageCommon(collection, _contact, _mentions, _quotes);

    return post_message_to_core("files/upload", collection.get());
}

qint64 core_dispatcher::uploadSharedFile(const QString& _contact, const QString& _localPath, const Data::QuotesVec& _quotes, std::optional<std::chrono::seconds> _duration)
{
    gui_coll_helper collection(create_collection(), true);
    collection.set_value_as_qstring("file", _localPath);
    collection.set_value_as_qstring("description", QString());
    if (_duration)
        collection.set<int64_t>("duration", (*_duration).count());

    serializeMessageCommon(collection, _contact, {}, _quotes);

    return post_message_to_core("files/upload", collection.get());
}

qint64 core_dispatcher::abortSharedFileUploading(const QString& _contact, const QString& _localPath, const QString& _uploadingProcessId)
{
    im_assert(!_contact.isEmpty());
    im_assert(!_localPath.isEmpty());
    im_assert(!_uploadingProcessId.isEmpty());

    core::coll_helper helper(create_collection(), true);
    helper.set<QString>("contact", _contact);
    helper.set<QString>("local_path", _localPath);
    helper.set<QString>("process_seq", _uploadingProcessId);

    return post_message_to_core("files/upload/abort", helper.get());
}

qint64 core_dispatcher::getSticker(const qint32 _setId, const qint32 _stickerId, const core::sticker_size _size)
{
    im_assert(_setId > 0);
    im_assert(_stickerId > 0);
    im_assert(_size > core::sticker_size::min);
    im_assert(_size < core::sticker_size::max);

    core::coll_helper collection(create_collection(), true);
    collection.set_value_as_int("set_id", _setId);
    collection.set_value_as_int("sticker_id", _stickerId);
    collection.set_value_as_enum("size", _size);

    return post_message_to_core("stickers/sticker/get", collection.get());
}

qint64 core_dispatcher::getSticker(const Utils::FileSharingId& _fsId, const core::sticker_size _size)
{
    im_assert(!_fsId.fileId_.isEmpty());
    im_assert(_size > core::sticker_size::min);
    im_assert(_size < core::sticker_size::max);

    Ui::gui_coll_helper collection(create_collection(), true);
    collection.set_value_as_qstring("file_id", _fsId.fileId_);
    if (_fsId.sourceId_)
        collection.set_value_as_qstring("source_id", *_fsId.sourceId_);
    collection.set_value_as_enum("size", _size);

    return post_message_to_core("stickers/sticker/get", collection.get());
}

qint64 core_dispatcher::getStickerCancel(const std::vector<Utils::FileSharingId>& _fsIds, core::sticker_size _size)
{
    if (_fsIds.empty())
        return -1;

    im_assert(_size > core::sticker_size::min);
    im_assert(_size < core::sticker_size::max);

    Ui::gui_coll_helper collection(create_collection(), true);
    collection.set_value_as_enum("size", _size);

    core::ifptr<core::iarray> arr(collection->create_array());
    arr->reserve(_fsIds.size());
    for (const auto& fsId : _fsIds)
    {
        Ui::gui_coll_helper coll(create_collection(), true);
        coll.set_value_as_qstring("file_id", fsId.fileId_);
        if (fsId.sourceId_)
            coll.set_value_as_qstring("source", *fsId.sourceId_);
        core::ifptr<core::ivalue> val(collection->create_value());
        val->set_as_collection(coll.get());
        arr->push_back(val.get());
    }

    collection.set_value_as_array("ids", arr.get());
    return post_message_to_core("stickers/sticker/get/cancel", collection.get());
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

qint64 core_dispatcher::getStickersPackInfo(const qint32 _setId, const QString& _storeId, const Utils::FileSharingId& _fileSharingId)
{
    Ui::gui_coll_helper collection(create_collection(), true);

    collection.set_value_as_int("set_id", _setId);
    collection.set_value_as_qstring("store_id", _storeId);
    collection.set_value_as_qstring("file_id", _fileSharingId.fileId_);
    if (_fileSharingId.sourceId_)
        collection.set_value_as_qstring("source", *_fileSharingId.sourceId_);

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
    im_assert(_uri.isValid());
    im_assert(!_uri.isLocalFile());
//    im_assert(!_uri.isRelative());
    im_assert(_previewWidth >= 0);
    im_assert(_previewHeight >= 0);

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
    im_assert(!_uri.isEmpty());
    im_assert(_previewWidth >= 0);
    im_assert(_previewHeight >= 0);
    im_assert(config::get().is_on(config::features::snippet_in_chat));

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
    im_assert(!_pttLink.isEmpty());
    im_assert(!_locale.isEmpty());

    Ui::gui_coll_helper collection(create_collection(), true);

    collection.set<QString>("url", _pttLink);
    collection.set<QString>("locale", _locale);

    return post_message_to_core("files/speech_to_text", collection.get());
}

qint64 core_dispatcher::deleteMessages(const QString& _contactAimId, const std::vector<DeleteMessageInfo>& _messages)
{
    im_assert(!_contactAimId.isEmpty());

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
    im_assert(!_contactAimId.isEmpty());
    im_assert(_fromId >= 0);

    core::coll_helper collection(create_collection(), true);

    collection.set<QString>("contact", _contactAimId);
    collection.set<int64_t>("from_id", _fromId);

    return post_message_to_core("archive/messages/delete_from", collection.get());
}

qint64 core_dispatcher::eraseHistory(const QString& _contactAimId)
{
    im_assert(!_contactAimId.isEmpty());

    core::coll_helper collection(create_collection(), true);

    collection.set<QString>("contact", _contactAimId);

    return post_message_to_core("archive/messages/delete_all", collection.get());
}

qint64 core_dispatcher::raiseDownloadPriority(const QString &_contactAimid, int64_t _procId)
{
    im_assert(_procId > 0);
    im_assert(!_contactAimid.isEmpty());

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

void core_dispatcher::sendStickerToContact(const QString& _contact, const Utils::FileSharingId& _stickerId)
{
    im_assert(!_contact.isEmpty());
    im_assert(!_stickerId.fileId_.isEmpty());

    if (_contact.isEmpty() || _stickerId.fileId_.isEmpty())
        return;

    Ui::gui_coll_helper collection(create_collection(), true);
    collection.set_value_as_qstring("contact", _contact);
    collection.set_value_as_bool("fs_sticker", true);
    collection.set_value_as_qstring("message", Stickers::getSendUrl(_stickerId));

    post_message_to_core("send_message", collection.get());
}

void core_dispatcher::sendMessageToContact(const QString& _contact, const Data::FString& _text, const Data::QuotesVec& _quotes, const Data::MentionMap& _mentions)
{
    im_assert(!_contact.isEmpty());
    im_assert(!_text.isEmpty());

    if (_contact.isEmpty() || (_text.isEmpty() && _quotes.empty()))
        return;

    MessageData data;
    data.mentions_ = _mentions;
    data.quotes_ = _quotes;
    data.text_ = _text;
    if (Features::isFormattingInInputEnabled())
        Utils::convertOldStyleMarkdownToFormats(data.text_);

    sendMessageToContact(_contact, std::move(data));
}

void core_dispatcher::sendMessage(const Data::MessageBuddy& _buddy, const SendMessageParams& _params)
{
    Ui::gui_coll_helper collection(create_collection(), true);
    _buddy.serialize(collection.get());
    postMessageStats(_buddy);

    if (_params.draftDeleteTime_)
        collection.set_value_as_int64("draft_delete_time", *_params.draftDeleteTime_);

    post_message_to_core("send_message", collection.get());
}

void core_dispatcher::updateMessage(const Data::MessageBuddy& _buddy)
{
    Ui::gui_coll_helper collection(create_collection(), true);
    _buddy.serialize(collection.get());
    postMessageStats(_buddy);
    post_message_to_core("update_message", collection.get());
}

void core_dispatcher::sendMessageToContact(const QString& _contact, core_dispatcher::MessageData _data)
{
    if (_contact.isEmpty() || (_data.text_.isEmpty() && _data.quotes_.empty() && !_data.task_))
        return;

    Ui::gui_coll_helper collection(create_collection(), true);

    if (Features::isFormattingInInputEnabled())
        Utils::convertOldStyleMarkdownToFormats(_data.text_);
    collection.set<QString>("message", _data.text_.string());
    Data::serializeFormat(_data.text_.formatting(), collection, "message_format");

    serializeMessageCommon(collection, _contact, _data.mentions_, _data.quotes_);

    if (_data.poll_)
        _data.poll_->serialize(collection.get());
    if (_data.task_)
        _data.task_->serialize(collection.get());

    post_message_to_core("send_message", collection.get());
}

void core_dispatcher::updateMessageToContact(const QString& _contact, int64_t _messageId, const Data::FString& _text)
{
    im_assert(!_contact.isEmpty());
    im_assert(!_text.isEmpty());

    Ui::gui_coll_helper collection(create_collection(), true);

    collection.set<QString>("contact", _contact);
    collection.set_value_as_int64("id", _messageId);

    auto postProcessedText = _text;
    if (Features::isFormattingInInputEnabled())
        Utils::convertOldStyleMarkdownToFormats(postProcessedText);
    collection.set<QString>("message", postProcessedText.string());
    Data::serializeFormat(postProcessedText.formatting(), collection, "message_format");

    post_message_to_core("update_message", collection.get());
}

void core_dispatcher::sendSmartreplyToContact(const QString& _contact, const Data::SmartreplySuggest& _smartreply, const Data::QuotesVec& _quotes)
{
    im_assert(!_contact.isEmpty());
    if (_contact.isEmpty())
        return;

    Ui::gui_coll_helper collection(create_collection(), true);

    if (_smartreply.isStickerType())
    {
        if (auto fileSharingId = std::get_if<Utils::FileSharingId>(&_smartreply.getData()))
        {
            collection.set_value_as_bool("fs_sticker", true);
            collection.set_value_as_qstring("message", Stickers::getSendUrl(*fileSharingId));
        }
    }
    else if (_smartreply.isTextType())
    {
        if (auto text = std::get_if<QString>(&_smartreply.getData()))
            collection.set_value_as_qstring("message", *text);
    }

    serializeMessageCommon(collection, _contact, {}, _quotes);

    {
        Ui::gui_coll_helper sr_helper(create_collection(), true);
        _smartreply.serialize(sr_helper);
        collection.set_value_as_collection("smartreply", sr_helper.get());
    }

    post_message_to_core("send_message", collection.get());
}

bool core_dispatcher::init()
{
#ifndef ICQ_CORELIB_STATIC_LINKING
    const QString library_path = QApplication::applicationDirPath() % ql1c('/') % ql1s(CORELIBRARY);
    QLibrary libcore(library_path);
    if (!libcore.load())
    {
        im_assert(false);
        return false;
    }

    typedef bool (*get_core_instance_function)(core::icore_interface**);
    get_core_instance_function get_core_instance = (get_core_instance_function) libcore.resolve("get_core_instance");

    core::icore_interface* coreFace = nullptr;
    if (!get_core_instance(&coreFace))
    {
        im_assert(false);
        return false;
    }

    coreFace_ = coreFace;
#else
    core::icore_interface* coreFace = nullptr;
    if (!get_core_instance(&coreFace) || !coreFace)
    {
        im_assert(false);
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
    REGISTER_IM_MESSAGE("need_external_user_agreement", onNeedExternalUserAgreement);
    REGISTER_IM_MESSAGE("im/created", onImCreated);
    REGISTER_IM_MESSAGE("login/require_oauth", onOAuthRequired);
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
    REGISTER_IM_MESSAGE("messages/received/patched/modified", onMessagesReceivedPatched);
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

    REGISTER_IM_MESSAGE("threads/search/local/result", onThreadsSearchLocalResults);
    REGISTER_IM_MESSAGE("threads/search/local/empty", onThreadsSearchLocalEmptyResult);
    REGISTER_IM_MESSAGE("threads/search/server/result", onThreadsSearchServerResults);

    REGISTER_IM_MESSAGE("dialogs/search/pattern_history", onDialogsSearchPatternHistory);

    REGISTER_IM_MESSAGE("contacts/search/local/result", onContactsSearchLocalResults);
    REGISTER_IM_MESSAGE("contacts/search/server/result", onContactsSearchServerResults);
    REGISTER_IM_MESSAGE("addressbook/sync/update", onSyncAddressBook);

#ifndef STRIP_VOIP
    REGISTER_IM_MESSAGE("voip_signal", onVoipSignal);
#endif
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
    REGISTER_IM_MESSAGE("chats/members/get/cached", onChatsMembersGetCached);
    REGISTER_IM_MESSAGE("chats/members/search/result", onChatsMembersSearchResult);
    REGISTER_IM_MESSAGE("chats/contacts/result", onChatsContactsResult);

    REGISTER_IM_MESSAGE("files/error", fileSharingErrorResult);
    REGISTER_IM_MESSAGE("files/download/progress", fileSharingDownloadProgress);
    REGISTER_IM_MESSAGE("files/preview_size/result", fileSharingGetPreviewSizeResult);
    REGISTER_IM_MESSAGE("files/metainfo/result", fileSharingMetainfoResult);
    REGISTER_IM_MESSAGE("files/metainfo/error", fileSharingMetainfoError);
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

    REGISTER_IM_MESSAGE("login_result_attach_phone", onLoginResultAttachPhone);
    REGISTER_IM_MESSAGE("update_profile/result", onUpdateProfileResult);
    REGISTER_IM_MESSAGE("chats/home/get/result", onChatsHomeGetResult);
    REGISTER_IM_MESSAGE("chats/home/get/failed", onChatsHomeGetFailed);
    REGISTER_IM_MESSAGE("proxy_change", onProxyChange);
    REGISTER_IM_MESSAGE("open_created_chat", onOpenCreatedChat);
    REGISTER_IM_MESSAGE("login_new_user", onLoginNewUser);
    REGISTER_IM_MESSAGE("set_avatar/result", onSetAvatarResult);
    REGISTER_IM_MESSAGE("chats/role/set/result", onChatsRoleSetResult);
    REGISTER_IM_MESSAGE("chats/block/result", onChatsBlockResult);
    REGISTER_IM_MESSAGE("livechat/invite/cancel/result", onCancelInviteResult);
    REGISTER_IM_MESSAGE("chats/pending/resolve/result", onChatsPendingResolveResult);
    REGISTER_IM_MESSAGE("phoneinfo/result", onPhoneinfoResult);
    REGISTER_IM_MESSAGE("contacts/ignore/remove", onContactRemovedFromIgnore);

    REGISTER_IM_MESSAGE("friendly/update", onFriendlyUpdate);

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
    REGISTER_IM_MESSAGE("session_finished", onSessionFinished);

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

    REGISTER_IM_MESSAGE("add_members/result", onAddMembersResult);
    REGISTER_IM_MESSAGE("remove_members/result", onRemoveMembersResult);

    REGISTER_IM_MESSAGE("group/invitebl/search/result", onInviterBlacklistSearchResult);
    REGISTER_IM_MESSAGE("group/invitebl/cl/result", onInviterBlacklistedCLContacts);
    REGISTER_IM_MESSAGE("group/invitebl/remove/result", onInviterBlacklistRemoveResult);
    REGISTER_IM_MESSAGE("group/invitebl/add/result", onInviterBlacklistAddResult);

    REGISTER_IM_MESSAGE("emoji/get/result", onGetEmojiResult);

    REGISTER_IM_MESSAGE("suggest_to_notify_user", onSuggestToNotifyUser);

    REGISTER_IM_MESSAGE("threads/update", onThreadsUpdate);
    REGISTER_IM_MESSAGE("threads/unread_count", onUnreadThreadsCount);
    REGISTER_IM_MESSAGE("threads/add/result", onAddThreadResult);
    REGISTER_IM_MESSAGE("threads/get/result", onGetThreadResult);
    REGISTER_IM_MESSAGE("threads/feed/get/result", onThreadsFeedGetResult);
    REGISTER_IM_MESSAGE("threads/subscribe/result", onSubscripionChanged);
    REGISTER_IM_MESSAGE("threads/unsubscribe/result", onSubscripionChanged);
    REGISTER_IM_MESSAGE("thread/subscribers/get/result", onThreadsSubscribersGetResult);
    REGISTER_IM_MESSAGE("thread/subscribers/search/result", onThreadsSubscribersSearchResult);
    REGISTER_IM_MESSAGE("thread/autosubscribe/result", onThreadAutosubscribeResult);

    REGISTER_IM_MESSAGE("auth_data", onAuthData);

    REGISTER_IM_MESSAGE("draft", onDraft);

    REGISTER_IM_MESSAGE("task/create/result", onTaskCreateResult);
    REGISTER_IM_MESSAGE("task/update", onTaskUpdate);
    REGISTER_IM_MESSAGE("task/edit_result", onTaskEditResult);
    REGISTER_IM_MESSAGE("task/load_all", onTasksLoaded);
    REGISTER_IM_MESSAGE("task/update_counter", onUnreadTasksCounter);

    REGISTER_IM_MESSAGE("mail/update_counter", onUnreadMailsCounter);

    REGISTER_IM_MESSAGE("antivirus/check_result", onAntivirusCheckResult);
    REGISTER_IM_MESSAGE("miniapp/unavailable", onMiniappUnavailable);
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
    im_assert(_seq > 0);

    auto node = callbacks_.extract(_seq);
    if (node.empty())
        return;

    const auto &callback = node.mapped().callback_;

    im_assert(callback);
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
    meta.trustRequired_ = _params.get<bool>("trustRequired") && Features::isRestrictedFilesEnabled();
    meta.antivirusCheck_.mode_ = _params.get_value_as_enum<core::antivirus::check::mode>("antivirus_check_mode");
    meta.antivirusCheck_.result_ = _params.get_value_as_enum<core::antivirus::check::result>("antivirus_check_result");

    auto fileId = _params.get<QString>("file_id");
    auto sourceId = _params.is_value_exist("source_id") ? std::make_optional(_params.get<QString>("source_id")) : std::nullopt;
    Utils::FileSharingId _fsId{ fileId, sourceId };

    Q_EMIT fileSharingFileMetainfoDownloaded(_seq, _fsId, meta);
}

void Ui::core_dispatcher::fileSharingMetainfoError(const int64_t _seq, core::coll_helper _params)
{
    auto fileId = _params.get<QString>("file_id");
    auto sourceId = _params.is_value_exist("source_id") ? std::make_optional(_params.get<QString>("source_id")) : std::nullopt;
    Utils::FileSharingId _fsId{ fileId, sourceId };
    const auto errorCode = _params.get<int32_t>("error", 0);

    Q_EMIT fileSharingFileMetainfoDownloadError(_seq, _fsId, errorCode);
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

    im_assert(bytesTotal > 0);
    im_assert(bytesTransferred >= 0);
    im_assert(pctTransferred >= 0);
    im_assert(pctTransferred <= 100);

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

    const auto local = _params.get<QString>("local", "");
    if (local.isEmpty())
    {
        Q_EMIT imageDownloadError(_seq, rawUri);
        return;
    }

    __INFO(
        "snippets",
        "completed image downloading\n"
            __LOGP(_seq, _seq)
            __LOGP(uri, rawUri)
            __LOGP(local_path, local));

    const auto is_preview = _params.get<bool>("preview", false);

    auto ext = QFileInfo(local).suffix();
    if (!ext.isEmpty() && !Utils::is_image_extension(ext))
    {
        Q_EMIT imageDownloaded(_seq, rawUri, QPixmap(), local, is_preview);
        return;
    }

    auto task = new Utils::LoadPixmapFromFileTask(local);

    const auto succeeded = QObject::connect(
        task, &Utils::LoadPixmapFromFileTask::loadedSignal,
        this,
        [this, _seq, rawUri, local, is_preview]
    (const QPixmap& pixmap)
    {
        if (pixmap.isNull())
        {
            Q_EMIT imageDownloadError(_seq, rawUri);
            return;
        }

        Q_EMIT imageDownloaded(_seq, rawUri, pixmap, local, is_preview);
    });
    im_assert(succeeded);

    QThreadPool::globalInstance()->start(task);
}

void core_dispatcher::imageDownloadResultMeta(const int64_t _seq, core::coll_helper _params)
{
    im_assert(_seq > 0);

    const auto previewWidth = _params.get<int32_t>("preview_width");
    const auto previewHeight = _params.get<int32_t>("preview_height");
    const auto originWidth = _params.get<int32_t>("origin_width");
    const auto originHeight = _params.get<int32_t>("origin_height");
    const auto downloadUri = _params.get<QString>("download_uri");
    const auto fileSize = _params.get<int64_t>("file_size");
    const auto contentType = _params.get<QString>("content_type");
    const auto fileFormat = _params.get<QString>("fileformat");

    im_assert(previewWidth >= 0);
    im_assert(previewHeight >= 0);

    const QSize previewSize(previewWidth, previewHeight);
    const QSize originSize(originWidth, originHeight);

    Data::LinkMetadata linkMeta(QString(), QString(), contentType, QString(), QString(), previewSize, downloadUri, fileSize, originSize, QString(), fileFormat);

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
    im_assert(_seq > 0);

    const auto success = _params.get<bool>("success");
    const auto title = _params.get<QString>("title", "");
    const auto annotation = _params.get<QString>("annotation", "");
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
    const auto fileFormat = _params.get<QString>("fileformat", "");

    im_assert(previewWidth >= 0);
    im_assert(previewHeight >= 0);
    const QSize previewSize(previewWidth, previewHeight);
    const QSize originSize(originWidth, originwHeight);

    const Data::LinkMetadata linkMetadata(title, annotation, contentType, previewUri, faviconUri, previewSize, downloadUri, fileSize, originSize, fileName, fileFormat);

    Q_EMIT linkMetainfoMetaDownloaded(_seq, success, linkMetadata);
}

void core_dispatcher::linkMetainfoDownloadResultImage(const int64_t _seq, core::coll_helper _params)
{
    im_assert(_seq > 0);

    const auto success = _params.get<bool>("success");

    if (!success)
    {
        Q_EMIT linkMetainfoImageDownloaded(_seq, false, QString());
        return;
    }

    auto task = new Utils::LoadPixmapFromFileTask(_params.get<QString>("local"));

    const auto succeeded = QObject::connect(
        task, &Utils::LoadPixmapFromFileTask::loadedSignal,
        this,
        [this, _seq, success]
    (const QPixmap& pixmap)
    {
        Q_EMIT linkMetainfoImageDownloaded(_seq, success, pixmap);
    });
    im_assert(succeeded);

    QThreadPool::globalInstance()->start(task);
}

void core_dispatcher::linkMetainfoDownloadResultFavicon(const int64_t _seq, core::coll_helper _params)
{
    im_assert(_seq > 0);

    const auto success = _params.get<bool>("success");

    if (!success)
    {
        Q_EMIT linkMetainfoFaviconDownloaded(_seq, false, QString());
        return;
    }

    const auto local = _params.get<QString>("local");

    auto task = new Utils::LoadPixmapFromFileTask(local);

    const auto succeeded = QObject::connect(
        task, &Utils::LoadPixmapFromFileTask::loadedSignal,
        this,
        [this, _seq, success, local]
    (const QPixmap& pixmap)
    {
        Q_EMIT linkMetainfoFaviconDownloaded(_seq, success, local);
    });
    im_assert(succeeded);

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
    if (GetAppConfig().IsTestingEnable())
    {
        if (server)
            return;
        server = std::make_unique<WebServer>();
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
        sendersAimIds = Utils::toContainerOfString<QVector<QString>>(_params.get_value_as_array("senders"));

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
    const auto ignoredAimIds = Utils::toContainerOfString<QVector<QString>>(_params.get_value_as_array("aimids"));
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
        im_assert(result.second);
    }

    if (_object)
    {
        QObject::connect(_object, &QObject::destroyed, this, [this](QObject* _obj)
        {
            if (!callbacks_.empty())
            {
                for (auto it = callbacks_.cbegin(); it != callbacks_.cend(); )
                {
                    if (it->second.object_ == _obj)
                        it = callbacks_.erase(it);
                    else
                        ++it;
                }
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

    im_assert(_eventName > core::stats::stats_event_names::min);
    im_assert(_eventName < core::stats::stats_event_names::max);

    core::coll_helper coll(create_collection(), true);
    coll.set_value_as_enum("event", _eventName);

    core::ifptr<core::iarray> propsArray(coll->create_array());
    propsArray->reserve(_props.size());
    for (const auto &[name, value] : _props)
    {
        im_assert(!name.empty() && !value.empty());

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
    if (!get_enabled_stats_post())
        return -1;

    im_assert(_eventName > core::stats::im_stat_event_names::min);
    im_assert(_eventName < core::stats::im_stat_event_names::max);

    core::coll_helper coll(create_collection(), true);
    coll.set_value_as_enum("event", _eventName);

    core::ifptr<core::iarray> propsArray(coll->create_array());
    propsArray->reserve(_props.size());

    for (const auto &[name, value] : _props)
    {
        im_assert(!name.empty() && !value.empty());

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

int64_t core_dispatcher::groupSubscribeImpl(std::string_view _message, const QString& _chatId, const QString& _stamp)
{
    gui_coll_helper collection(create_collection(), true);
    if (!_stamp.isEmpty())
        collection.set_value_as_qstring("stamp", _stamp);
    else
        collection.set_value_as_qstring("aimid", _chatId);
    return post_message_to_core(_message, collection.get());
}

void core_dispatcher::setUserState(const core::profile_state state)
{
    im_assert(state > core::profile_state::min);
    im_assert(state < core::profile_state::max);

    Ui::gui_coll_helper collection(create_collection(), true);

    std::stringstream stream;
    stream << state;

    collection.set_value_as_string("state", stream.str());
    collection.set<QString>("aimid", MyInfo()->aimId());
    post_message_to_core("set_state", collection.get());
}

void core_dispatcher::onNeedExternalUserAgreement(const int64_t _seq, core::coll_helper _params)
{
    Q_EMIT needExternalUserAgreement(QString::fromUtf8(_params.get_value_as_string("terms_of_use_url")), QString::fromUtf8(_params.get_value_as_string("privacy_policy_url")));
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
        get_gui_settings()->set_value(Ui::Stickers::recentsStickerSettingsPath().data(), QString());
        get_gui_settings()->set_value<std::vector<int32_t>>(settings_old_recents_stickers, {});
    }

    Q_EMIT needLogin(is_auth_error);
}

void core_dispatcher::onImCreated(const int64_t _seq, core::coll_helper _params)
{
    isImCreated_ = true;

    Q_EMIT im_created();
}

void core_dispatcher::onOAuthRequired(const int64_t _seq, core::coll_helper _params)
{
    Q_EMIT oauthRequired();
}

void core_dispatcher::onLoginComplete(const int64_t _seq, core::coll_helper _params)
{
    Utils::InterConnector::instance().setLoginComplete(true);
    Q_EMIT loginComplete();
    Q_EMIT Utils::InterConnector::instance().loginCompleted();
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
    Ui::gui_coll_helper collection(create_collection(), true);

    collection.set_value_as_qstring("ivr_url", _ivr_url);
    post_message_to_core("get_code_by_phone_call", collection.get());
}

qint64 core_dispatcher::getDialogGallery(const QString& _aimId, const QStringList& _types, int64_t _after_msg, int64_t _after_seq, const int _pageSize, const bool _download_holes)
{
    Ui::gui_coll_helper collection(create_collection(), true);

    collection.set_value_as_qstring("aimid", _aimId);
    collection.set_value_as_int("page_size", _pageSize);
    collection.set_value_as_int64("after_msg", _after_msg);
    collection.set_value_as_int64("after_seq", _after_seq);
    collection.set_value_as_array("type", Utils::toArrayOfStrings(_types, collection).get());
    collection.set_value_as_bool("download_holes", _download_holes);

    return post_message_to_core("get_dialog_gallery", collection.get());
}

qint64 core_dispatcher::requestDialogGalleryState(const QString& _aimId)
{
    if (_aimId.isEmpty())
        return -1;

    Ui::gui_coll_helper collection(create_collection(), true);
    collection.set_value_as_qstring("aimid", _aimId);
    return post_message_to_core("request_gallery_state", collection.get());
}

qint64 core_dispatcher::getDialogGalleryIndex(const QString& _aimId, const QStringList& _types, qint64 _msg, qint64 _seq)
{
    Ui::gui_coll_helper collection(create_collection(), true);
    collection.set_value_as_qstring("aimid", _aimId);
    collection.set_value_as_int64("msg", _msg);
    collection.set_value_as_int64("seq", _seq);
    collection.set_value_as_array("type", Utils::toArrayOfStrings(_types, collection).get());

    return post_message_to_core("get_gallery_index", collection.get());
}

qint64 core_dispatcher::getDialogGalleryByMsg(const QString& _aimId, const QStringList& _types, int64_t _id)
{
    Ui::gui_coll_helper collection(create_collection(), true);

    collection.set_value_as_qstring("aimid", _aimId);
    collection.set_value_as_int64("msg", _id);
    collection.set_value_as_array("type", Utils::toArrayOfStrings(_types, collection).get());

    return post_message_to_core("get_dialog_gallery_by_msg", collection.get());
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
        im_assert(std::is_sorted(result.AllIds_.begin(), result.AllIds_.end()));
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

void core_dispatcher::onMessagesReceivedPatched(const int64_t _seq, core::coll_helper _params)
{
    Data::PatchedMessage patches;
    patches.unserialize(_params);
    Q_EMIT messagesPatched(patches);
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
    im_assert(id > -1);

    const auto contact = _params.get<QString>("contact");
    im_assert(!contact.isEmpty());

    Q_EMIT messagesDeletedUpTo(contact, id);
}

void core_dispatcher::onDlgStates(const int64_t _seq, core::coll_helper _params)
{
    const auto myAimid = _params.get<QString>("my_aimid");

    QVector<Data::DlgState> dlgStatesList;

    auto arrayDlgStates = _params.get_value_as_array("dlg_states");
    if (!arrayDlgStates)
    {
        im_assert(false);
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
        Q_EMIT searchedMessagesServer(res, QString(), _params->is_value_exist("api_version_error") ? 0 : -1, _seq);
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

            if (helper.is_value_exist("parentTopic"))
            {
                resItem->parentTopic_ = std::make_shared<Data::MessageParentTopic>();
                resItem->parentTopic_->unserialize(helper);
            }

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

void core_dispatcher::onThreadsSearchLocalResults(const int64_t _seq, core::coll_helper _params)
{
    onThreadsSearchResult(_seq, _params, true);
}

void core_dispatcher::onThreadsSearchLocalEmptyResult(const int64_t _seq, core::coll_helper _params)
{
    Q_EMIT searchedThreadsLocalEmptyResult(_seq);
}

void core_dispatcher::onThreadsSearchServerResults(const int64_t _seq, core::coll_helper _params)
{
    onThreadsSearchResult(_seq, _params, false);
}

void core_dispatcher::onThreadsSearchResult(const int64_t _seq, core::coll_helper _params, const bool _fromLocalSearch)
{
    Data::SearchResultsV res;

    if (!_fromLocalSearch && _params->is_value_exist("error"))
    {
        Q_EMIT searchedThreadsServer(res, QString(), -1, _seq);
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

            if (helper.is_value_exist("parentTopic"))
            {
                resItem->parentTopic_ = std::make_shared<Data::MessageParentTopic>();
                resItem->parentTopic_->unserialize(helper);
            }

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
        Q_EMIT searchedThreadsLocal(res, _seq);
    }
    else
    {
        QString cursorNext;
        if (_params.is_value_exist("cursor_next"))
            cursorNext = _params.get<QString>("cursor_next");

        const auto totalEntries = _params.get_value_as_int("total_count");
        Q_EMIT searchedThreadsServer(res, cursorNext, totalEntries, _seq);
    }
}

void core_dispatcher::onDialogsSearchPatternHistory(const int64_t _seq, core::coll_helper _params)
{
    const auto contact = _params.get<QString>("contact");
    const auto patterns = Utils::toContainerOfString<QVector<QString>>(_params.get_value_as_array("patterns"));

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
    Q_EMIT syncronizedAddressBook();
}

#ifndef STRIP_VOIP
void core_dispatcher::onVoipSignal(const int64_t _seq, core::coll_helper _params)
{
    voipController_.handlePacket(_params);
}
#endif

void core_dispatcher::onActiveDialogsAreEmpty(const int64_t _seq, core::coll_helper _params)
{
    Q_EMIT Utils::InterConnector::instance().showRecentsPlaceholder();
}

void core_dispatcher::onActiveDialogsHide(const int64_t _seq, core::coll_helper _params)
{
    auto closePage = true;
    if (_params.is_value_exist("close_page"))
        closePage = _params.get_value_as_bool("close_page");

    Q_EMIT activeDialogHide(_params.get<QString>("contact"), closePage ? ClosePage::Yes : ClosePage::No);
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

    QVector<Utils::FileSharingId> stickerIds;
    auto stickers = _params.get_value_as_array("stickers");
    if (stickers)
    {
        const auto size = stickers->size();
        stickerIds.reserve(size);
        for (std::remove_const_t<decltype(size)> i = 0; i < size; ++i)
        {
            auto str = QString::fromUtf8(stickers->get_at(i)->get_as_string());
            if (!str.isEmpty())
                stickerIds.push_back({ std::move(str), std::nullopt });
        }
    }
    Q_EMIT stickersRequestedSuggestsResult(_params.get<QString>("contact"), stickerIds);
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
}

void core_dispatcher::onStickersGetSetBigIconResult(const int64_t _seq, core::coll_helper _params)
{
    if (const auto err = _params.get_value_as_int("error"); err == 0)
        Ui::Stickers::setSetBigIcon(_params);
}

void core_dispatcher::onStickersSetIcon(const int64_t _seq, core::coll_helper _params)
{
    if (const auto err = _params.get_value_as_int("error"); err == 0)
        Ui::Stickers::setSetIcon(_params);
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

void Ui::core_dispatcher::onThreadAutosubscribeResult(const int64_t _seq, core::coll_helper _params)
{
    Q_UNUSED(_seq);
    Q_EMIT threadAutosubscribe(_params.get_value_as_int("error"));
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

void core_dispatcher::onChatsMembersGetCached(const int64_t _seq, core::coll_helper _params)
{
    const auto role = QString::fromUtf8(_params.get_value_as_string("role"));
    if (role.isEmpty())
    {
        im_assert(false);
        return;
    }
    const auto info = std::make_shared<Data::ChatMembersPage>(Data::UnserializeChatMembersPage(&_params));
    Q_EMIT chatMembersPageCached(info, role);
}

void core_dispatcher::onChatsMembersSearchResult(const int64_t _seq, core::coll_helper _params)
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

void core_dispatcher::onProxyChange(const int64_t _seq, core::coll_helper _params)
{
    Utils::ProxySettings userProxy;

    userProxy.type_ = _params.get_value_as_enum<core::proxy_type>("settings_proxy_type", core::proxy_type::auto_proxy);
    userProxy.proxyServer_ = QString::fromUtf8(_params.get_value_as_string("settings_proxy_server", ""));
    userProxy.port_ = _params.get_value_as_int("settings_proxy_port", Utils::ProxySettings::invalidPort);
    userProxy.username_ = QString::fromUtf8(_params.get_value_as_string("settings_proxy_username", ""));
    userProxy.password_ = QString::fromUtf8(_params.get_value_as_string("settings_proxy_password", ""));
    userProxy.needAuth_ = _params.get_value_as_bool("settings_proxy_need_auth", false);
    userProxy.authType_ = _params.get_value_as_enum<core::proxy_auth>("settings_proxy_auth_type", core::proxy_auth::basic);

    if (build::has_webengine())
    {
        const QNetworkProxy::ProxyType proxyType = Utils::ProxySettings::proxyType(userProxy.type_);
        QNetworkProxy proxy;
        proxy.setType(proxyType);
        if (proxyType != QNetworkProxy::ProxyType::NoProxy)
        {
            proxy.setHostName(userProxy.proxyServer_);
            proxy.setPort(userProxy.port_);
            if (userProxy.needAuth_)
            {
                proxy.setUser(userProxy.username_);
                proxy.setPassword(userProxy.password_);
            }
        }
        QNetworkProxy::setApplicationProxy(proxy);
    }

    if (_params.get_value_as_bool("is_system", false))
        return;
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
    if (auto failures = unserializeChatFailures(_params); !failures.empty())
    {
        if (_params.is_value_exist("block"))
        {
            const auto operation = _params.get_value_as_bool("block") ? Utils::ChatMembersOperation::Block : Utils::ChatMembersOperation::Unblock;
            Utils::InterConnector::instance().showChatMembersFailuresPopup(operation, QString::fromUtf8(_params.get_value_as_string("chat")), std::move(failures));
        }
    }

    Q_EMIT blockMemberResult(_params.get_value_as_int("error"));
}

void core_dispatcher::onCancelInviteResult(const int64_t _seq, core::coll_helper _params)
{
    Q_EMIT inviteCancelResult(_params.get_value_as_int("error"));
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

    im_assert(newState != Ui::ConnectionState::stateUnknown);

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
        im_assert(!"couldn't unserialize req_handle");
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
        im_assert(!"couldn't unserialize mem usage response");
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
    Ui::gui_coll_helper collection(create_collection(), true);
    collection.set_value_as_qstring("password", _password);

    post_message_to_core("localpin/set", collection.get());
}

qint64 core_dispatcher::getUserInfo(const QString& _aimid)
{
    Ui::gui_coll_helper collection(create_collection(), true);
    collection.set_value_as_qstring("contact", _aimid);

    return post_message_to_core("get_user_info", collection.get());
}

qint64 core_dispatcher::getUserLastSeen(const std::vector<QString>& _contacts)
{
    Ui::gui_coll_helper collection(create_collection(), true);
    collection.set_value_as_array("contacts", Utils::toArrayOfStrings(_contacts, collection).get());
    return post_message_to_core("get_user_last_seen", collection.get());
}

qint64 core_dispatcher::subscribeStatus(const std::vector<QString>& _contacts)
{
    Ui::gui_coll_helper collection(create_collection(), true);
    collection.set_value_as_array("contacts", Utils::toArrayOfStrings(_contacts, collection).get());
    return post_message_to_core("subscribe/status", collection.get());
}

qint64 core_dispatcher::unsubscribeStatus(const std::vector<QString>& _contacts)
{
    Ui::gui_coll_helper collection(create_collection(), true);
    collection.set_value_as_array("contacts", Utils::toArrayOfStrings(_contacts, collection).get());
    return post_message_to_core("unsubscribe/status", collection.get());
}

qint64 core_dispatcher::subscribeThread(const std::vector<QString>& _threads)
{
    Ui::gui_coll_helper collection(create_collection(), true);
    collection.set_value_as_array("contacts", Utils::toArrayOfStrings(_threads, collection).get());
    return post_message_to_core("subscribe/thread", collection.get());
}

qint64 core_dispatcher::unsubscribeThread(const std::vector<QString>& _threads)
{
    Ui::gui_coll_helper collection(create_collection(), true);
    collection.set_value_as_array("contacts", Utils::toArrayOfStrings(_threads, collection).get());
    return post_message_to_core("unsubscribe/thread", collection.get());
}

qint64 core_dispatcher::subscribeCallRoomInfo(const QString& _roomId)
{
    if (!Features::callRoomInfoEnabled())
        return 0;

    Ui::gui_coll_helper collection(create_collection(), true);
    collection.set_value_as_qstring("room_id", _roomId);
    return post_message_to_core("subscribe/call_room_info", collection.get());
}

qint64 core_dispatcher::unsubscribeCallRoomInfo(const QString& _roomId)
{
    Ui::gui_coll_helper collection(create_collection(), true);
    collection.set_value_as_qstring("room_id", _roomId);
    return post_message_to_core("unsubscribe/call_room_info", collection.get());
}

qint64 Ui::core_dispatcher::subscribeTask(const QString& _taskId)
{
    Ui::gui_coll_helper collection(create_collection(), true);
    collection.set_value_as_qstring("task_id", _taskId);
    return post_message_to_core("subscribe/task", collection.get());
}

qint64 Ui::core_dispatcher::unsubscribeTask(const QString& _taskId)
{
    Ui::gui_coll_helper collection(create_collection(), true);
    collection.set_value_as_qstring("task_id", _taskId);
    return post_message_to_core("unsubscribe/task", collection.get());
}

qint64 Ui::core_dispatcher::subscribeFileSharingAntivirus(const Utils::FileSharingId& _fsId)
{
    Ui::gui_coll_helper collection(create_collection(), true);
    collection.set_value_as_qstring("file_id", _fsId.fileId_);
    if (_fsId.sourceId_)
        collection.set_value_as_qstring("source", *_fsId.sourceId_);
    return post_message_to_core("subscribe/filesharing_antivirus", collection.get());
}

qint64 Ui::core_dispatcher::unsubscribeFileSharingAntivirus(const Utils::FileSharingId& _fsId)
{
    Ui::gui_coll_helper collection(create_collection(), true);
    collection.set_value_as_qstring("file_id", _fsId.fileId_);
    if (_fsId.sourceId_)
        collection.set_value_as_qstring("source", *_fsId.sourceId_);
    return post_message_to_core("unsubscribe/filesharing_antivirus", collection.get());
}

qint64 core_dispatcher::subscribeTasksCounter()
{
    return post_message_to_core("subscribe/tasks_counter", nullptr);
}

qint64 core_dispatcher::unsubscribeTasksCounter()
{
    return post_message_to_core("unsubscribe/tasks_counter", nullptr);
}

qint64 Ui::core_dispatcher::subscribeMailsCounter()
{
    return post_message_to_core("subscribe/mails_counter", nullptr);
}

qint64 Ui::core_dispatcher::unsubscribeMailsCounter()
{
    return post_message_to_core("unsubscribe/mails_counter", nullptr);
}

qint64 core_dispatcher::addToInvitersBlacklist(const std::vector<QString>& _contacts)
{
    Ui::gui_coll_helper collection(create_collection(), true);
    collection.set_value_as_array("contacts", Utils::toArrayOfStrings(_contacts, collection).get());
    return post_message_to_core("group/invitebl/add", collection.get());
}

qint64 core_dispatcher::removeFromInvitersBlacklist(const std::vector<QString>& _contacts)
{
    Ui::gui_coll_helper collection(create_collection(), true);
    collection.set_value_as_array("contacts", Utils::toArrayOfStrings(_contacts, collection).get());
    collection.set_value_as_bool("all", false);
    return post_message_to_core("group/invitebl/remove", collection.get());
}

qint64 core_dispatcher::localPINEntered(const QString& _password)
{
    Ui::gui_coll_helper collection(create_collection(), true);
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
    Ui::gui_coll_helper collection(create_collection(), true);
    collection.set_value_as_qstring("id", _id);

    return post_message_to_core("idinfo/get", collection.get());
}

qint64 core_dispatcher::checkNickname(const QString& _nick, bool _set_nick)
{
    Ui::gui_coll_helper collection(create_collection(), true);
    collection.set_value_as_qstring("nick", _nick);
    collection.set_value_as_bool("set_nick", _set_nick);

    return post_message_to_core("nickname/check", collection.get());
}

qint64 core_dispatcher::checkGroupNickname(const QString& _nick)
{
    Ui::gui_coll_helper collection(create_collection(), true);
    collection.set_value_as_qstring("nick", _nick);

    return post_message_to_core("group_nickname/check", collection.get());
}

void core_dispatcher::updateRecentAvatarsSize()
{
    Ui::gui_coll_helper collection(create_collection(), true);
    collection.set_value_as_int("size", Utils::scale_bitmap(Ui::GetRecentsParams().getAvatarSize()));

    post_message_to_core("recent_avatars_size/update", collection.get());
}

void Ui::core_dispatcher::setInstallBetaUpdates(bool _enabled)
{
    Ui::gui_coll_helper collection(create_collection(), true);
    collection.set_value_as_bool("enabled", _enabled);

    post_message_to_core("set_install_beta_updates", collection.get());
}

bool Ui::core_dispatcher::isInstallBetaUpdatesEnabled() const
{
    return Features::betaUpdateAllowed() && installBetaUpdates_;
}

int64_t core_dispatcher::getPoll(const QString& _pollId)
{
    Ui::gui_coll_helper collection(create_collection(), true);
    collection.set_value_as_qstring("id", _pollId);

    return post_message_to_core("poll/get", collection.get());
}

int64_t core_dispatcher::voteInPoll(const QString& _pollId, const QString& _answerId)
{
    Ui::gui_coll_helper collection(create_collection(), true);
    collection.set_value_as_qstring("id", _pollId);
    collection.set_value_as_qstring("answer_id", _answerId);

    return post_message_to_core("poll/vote", collection.get());
}

int64_t core_dispatcher::revokeVote(const QString& _pollId)
{
    Ui::gui_coll_helper collection(create_collection(), true);
    collection.set_value_as_qstring("id", _pollId);

    return post_message_to_core("poll/revoke", collection.get());
}

int64_t core_dispatcher::stopPoll(const QString& _pollId)
{
    Ui::gui_coll_helper collection(create_collection(), true);
    collection.set_value_as_qstring("id", _pollId);

    return post_message_to_core("poll/stop", collection.get());
}

int64_t core_dispatcher::createTask(Data::Task _task)
{
    if (!_task)
        return -1;

    Ui::gui_coll_helper collection(create_collection(), true);
    _task->serialize(collection.get());
    return post_message_to_core("task/create", collection.get());
}

int64_t Ui::core_dispatcher::editTask(const Data::TaskChange& _task)
{
    Ui::gui_coll_helper collection(create_collection(), true);
    _task.serialize(collection.get());

    return post_message_to_core("task/edit", collection.get());
}

int64_t Ui::core_dispatcher::requestInitialTasks()
{
    return post_message_to_core("task/request_initial", nullptr);
}

int64_t Ui::core_dispatcher::updateTaskLastUsedTime(const QString& _taskId)
{
    Ui::gui_coll_helper collection(create_collection(), true);
    collection.set_value_as_qstring("task_id", _taskId);
    return post_message_to_core("task/update_last_used", collection.get());
}

int64_t core_dispatcher::getBotCallbackAnswer(const QString& _chatId, const QString& _callbackData, qint64 _msgId)
{
    Ui::gui_coll_helper collection(create_collection(), true);
    collection.set_value_as_qstring("chat_id", _chatId);
    collection.set_value_as_qstring("callback_data", _callbackData);
    collection.set_value_as_int64("msg_id", _msgId);

    return post_message_to_core("get_bot_callback_answer", collection.get());
}

qint64 core_dispatcher::pinContact(const QString& _aimId, bool _enable)
{
    Ui::gui_coll_helper collection(create_collection(), true);
    collection.set_value_as_qstring("contact", _aimId);

    return post_message_to_core(_enable ? "pin_chat" : "unfavorite", collection.get());
}

int64_t core_dispatcher::getChatInfo(const QString& _chatId, const QString& _stamp, int _membersLimit)
{
    Ui::gui_coll_helper collection(create_collection(), true);
    collection.set_value_as_qstring("aimid", _chatId);
    if (!_stamp.isEmpty())
        collection.set_value_as_qstring("stamp", _stamp);
    collection.set_value_as_int("limit", _membersLimit);
    return Ui::GetDispatcher()->post_message_to_core("chats/info/get", collection.get());
}

qint64 Ui::core_dispatcher::loadChatMembersInfo(const QString& _aimId, const std::vector<QString>& _members)
{
    if (_members.empty())
        return -1;

    Ui::gui_coll_helper collection(create_collection(), true);
    collection.set_value_as_qstring("aimid", _aimId);
    collection.set_value_as_array("members", Utils::toArrayOfStrings(_members, collection).get());

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

    Ui::gui_coll_helper collection(create_collection(), true);
    collection.set_value_as_qstring("aimid", _aimid);

    return post_message_to_core("stop_gallery_holes_downloading", collection.get());
}

int64_t core_dispatcher::createConference(ConferenceType _type, const std::vector<QString>& _participants, bool _callParticipants)
{
    Ui::gui_coll_helper collection(create_collection(), true);
    collection.set_value_as_qstring("name", MyInfo()->friendly());
    collection.set_value_as_bool("is_webinar", _type == ConferenceType::Webinar);
    collection.set_value_as_bool("call_participants", _callParticipants);
    collection.set_value_as_array("participants", Utils::toArrayOfStrings(_participants, collection).get());

    return post_message_to_core("conference/create", collection.get());
}

qint64 core_dispatcher::resolveChatPendings(const QString& _chatAimId, const std::vector<QString>& _pendings, bool _approve)
{
    if (_pendings.empty())
        return -1;

    Ui::gui_coll_helper collection(create_collection(), true);
    collection.set_value_as_qstring("aimid", _chatAimId);
    collection.set_value_as_array("contacts", Utils::toArrayOfStrings(_pendings, collection).get());
    collection.set_value_as_bool("approve", _approve);
    return post_message_to_core("chats/pending/resolve", collection.get());
}

void core_dispatcher::getReactions(const QString& _chatId, std::vector<int64_t> _msgIds, bool _firstLoad)
{
    Ui::gui_coll_helper collection(create_collection(), true);
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

int64_t core_dispatcher::addReaction(int64_t _msgId, const QString& _chatId, const QString& _reaction)
{
    Ui::gui_coll_helper collection(create_collection(), true);

    collection.set_value_as_int64("msg_id", _msgId);
    collection.set_value_as_qstring("chat_id", _chatId);
    collection.set_value_as_qstring("reaction", _reaction);

    core::ifptr<core::iarray> reactionsArray(collection->create_array());

    auto& defaultReactions = DefaultReactions::instance()->reactionsWithTooltip();
    reactionsArray->reserve(defaultReactions.size());
    for (const auto& reaction : defaultReactions)
        reactionsArray->push_back(collection.create_qstring_value(reaction.reaction_).get());
    collection.set_value_as_array("reactions", reactionsArray.get());

    return post_message_to_core("reaction/add", collection.get());
}

int64_t core_dispatcher::removeReaction(int64_t _msgId, const QString& _chatId)
{
    Ui::gui_coll_helper collection(create_collection(), true);

    collection.set_value_as_int64("msg_id", _msgId);
    collection.set_value_as_qstring("chat_id", _chatId);

    return post_message_to_core("reaction/remove", collection.get());
}

int64_t core_dispatcher::getReactionsPage(int64_t _msgId, QStringView _chatId, QStringView _reaction, QStringView _newerThan, QStringView _olderThan, int64_t _limit)
{
    Ui::gui_coll_helper collection(create_collection(), true);

    collection.set_value_as_int64("msg_id", _msgId);
    collection.set_value_as_qstring("chat_id", _chatId);
    collection.set_value_as_qstring("reaction", _reaction);
    collection.set_value_as_qstring("newer_than", _newerThan);
    collection.set_value_as_qstring("older_than", _olderThan);
    collection.set_value_as_int64("limit", _limit);

    return post_message_to_core("reaction/list", collection.get());
}

int64_t core_dispatcher::getEmojiImage(const QString& _code, ServerEmojiSize _size)
{
    Ui::gui_coll_helper collection(create_collection(), true);

    collection.set_value_as_qstring("code", _code);
    collection.set_value_as_int("size", static_cast<int>(_size));

    return post_message_to_core("emoji/get", collection.get());
}

int64_t core_dispatcher::setStatus(const QString& _emojiCode, int64_t _duration, const QString& _description)
{
    Ui::gui_coll_helper collection(create_collection(), true);

    collection.set_value_as_qstring("status", _emojiCode);
    collection.set_value_as_int64("duration", _duration);

    if (!_description.isEmpty())
        collection.set_value_as_qstring("description", _description);

    auto status = Statuses::Status(_emojiCode, _description);
    auto currentTime = QDateTime::currentDateTime();
    status.setStartTime(currentTime);
    if (_duration != 0)
        status.setEndTime(currentTime.addSecs(_duration));
    Logic::GetStatusContainer()->setStatus(MyInfo()->aimId(), status);

    return post_message_to_core("status/set", collection.get());
}

int64_t core_dispatcher::addThread(const Data::MessageParentTopic& _topic)
{
    gui_coll_helper collection(create_collection(), true);
    _topic.serialize(collection);
    return post_message_to_core("threads/add", collection.get());
}

int64_t core_dispatcher::getThreadsFeed(const QString& _cursor)
{
    gui_coll_helper collection(create_collection(), true);
    collection.set_value_as_qstring("cursor", _cursor);
    return post_message_to_core("threads/feed/get", collection.get());
}

int64_t core_dispatcher::updateThreadSubscription(const QString& _threadId, bool _subscribe)
{
    Ui::gui_coll_helper collection(create_collection(), true);
    im_assert(!_threadId.isEmpty());
    collection.set_value_as_qstring("thread_id", _threadId);
    if (_subscribe)
        return post_message_to_core("threads/subscribe", collection.get());
    else
        return post_message_to_core("threads/unsubscribe", collection.get());
}

int64_t core_dispatcher::setDraft(const Data::MessageBuddy& _msg, int64_t _timestamp, bool _synch)
{
    Ui::gui_coll_helper collection(create_collection(), true);
    _msg.serialize(collection.get());
    collection.set_value_as_bool("sync", _synch);
    collection.set_value_as_int64("timestamp", _timestamp);
    return post_message_to_core("draft/set", collection.get());
}

int64_t core_dispatcher::getDraft(const QString& _contact)
{
    Ui::gui_coll_helper collection(create_collection(), true);
    collection.set_value_as_qstring("contact", _contact);
    return post_message_to_core("draft/get", collection.get());
}

int64_t core_dispatcher::setDialogOpen(const QString& _contact, bool _open)
{
    gui_coll_helper collection(create_collection(), true);
    collection.set_value_as_qstring("contact", _contact);
    return post_message_to_core(_open ? "dialogs/add" : "dialogs/remove", collection.get());
}

int64_t core_dispatcher::getHistory(const QString& _contact, const GetHistoryParams& _params)
{
    Ui::gui_coll_helper collection(create_collection(), true);
    collection.set_value_as_qstring("contact", _contact);
    collection.set_value_as_int64("from", _params.from_); // this id is included
    collection.set_value_as_int64("count_early", _params.early_);
    collection.set_value_as_int64("count_later", _params.later_);
    collection.set_value_as_bool("need_prefetch", _params.needPrefetch_);
    collection.set_value_as_bool("is_first_request", _params.firstRequest_);
    collection.set_value_as_bool("after_search", _params.afterSearch_);

    return post_message_to_core("archive/messages/get", collection.get());
}

int64_t core_dispatcher::groupSubscribe(const QString& _chatId, const QString& _stamp)
{
    return groupSubscribeImpl("group/subscribe", _chatId, _stamp);
}

int64_t core_dispatcher::cancelGroupSubscription(const QString& _chatId, const QString& _stamp)
{
    return groupSubscribeImpl("group/cancel_subscription", _chatId, _stamp);
}

void core_dispatcher::setLastRead(const QString& _contact, int64_t _lastMsgId, core_dispatcher::ReadAll _readAll)
{
    Ui::gui_coll_helper collection(create_collection(), true);
    collection.set_value_as_qstring("contact", _contact);
    collection.set_value_as_int64("message", _lastMsgId);
    collection.set_value_as_bool("read_all", _readAll == ReadAll::Yes);
    post_message_to_core("dlg_state/set_last_read", collection.get());
}

void core_dispatcher::addChatsThread(const QString& _contact, int64_t _msgId, const QString& _threadId)
{
    gui_coll_helper collection(create_collection(), true);
    collection.set_value_as_qstring("chat_id", _contact);
    collection.set_value_as_int64("msg_id", _msgId);
    collection.set_value_as_qstring("thread_id", _threadId);
    post_message_to_core("chats/thread/add", collection.get(), this);
}

void core_dispatcher::removeChatsThread(const QString& _contact, int64_t _msgId, const QString& _threadId)
{
    gui_coll_helper collection(create_collection(), true);
    collection.set_value_as_qstring("chat_id", _contact);
    collection.set_value_as_int64("msg_id", _msgId);
    collection.set_value_as_qstring("thread_id", _threadId);
    post_message_to_core("chats/thread/remove", collection.get(), this);
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

void Ui::core_dispatcher::onSessionFinished(const int64_t, core::coll_helper)
{
    isImCreated_ = false;

    Q_EMIT sessionFinished(QPrivateSignal());
}

void Ui::core_dispatcher::onAntivirusCheckResult(const int64_t _seq, core::coll_helper _params)
{
    Utils::FileSharingId fileSharingId;
    fileSharingId.fileId_ = _params.get<QString>("file_id");
    if (_params.is_value_exist("source_id"))
        fileSharingId.sourceId_ = _params.get<QString>("source_id");

    const auto checkResult = _params.get_value_as_enum<core::antivirus::check::result>("check_result");
    Q_EMIT antivirusCheckResult(fileSharingId, checkResult);
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

    Q_EMIT subscribeResult(_seq, _params.get_value_as_int("error"), _params.get_value_as_int("resubscribe_in"));
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
    Data::Reactions reactions;

    const auto success = _params.get_value_as_int("error") == 0;
    if (success)
        reactions.unserialize(_params.get_value_as_collection("reactions"));

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

void core_dispatcher::onSuggestToNotifyUser(const int64_t _seq, core::coll_helper _params)
{
    Q_EMIT suggestNotifyUser(_params.get<QString>("contact"), _params.get<QString>("smsNotifyContext"));
}

void core_dispatcher::onThreadsUpdate(const int64_t _seq, core::coll_helper _params)
{
    Data::ThreadUpdates res;
    const auto array = _params.get_value_as_array("updates");
    const auto size = array->size();
    res.messages_.reserve(size);
    for (std::decay_t<decltype(size)> i = 0; i < size; ++i)
    {
        Ui::gui_coll_helper coll(array->get_at(i)->get_as_collection(), false);

        auto u = std::make_unique<Data::ThreadUpdate>();
        u->unserialize(coll);
        if (Data::ParentTopic::Type::message == u->parent_->type_)
        {
            auto parentTopic = static_cast<Data::MessageParentTopic*>(u->parent_.get());
            res.messages_[parentTopic->msgId_] = std::move(u);
        }
        else if (Data::ParentTopic::Type::task == u->parent_->type_)
        {
            // thread updates for task go through TaskContainer
            continue;
        }
    }

    Q_EMIT threadUpdates(res, QPrivateSignal());
}

void core_dispatcher::onUnreadThreadsCount(const int64_t _seq, core::coll_helper _params)
{
    const auto unreadThreads = _params.get_value_as_int("threads");
    const auto unreadMentions = _params.get_value_as_int("mentions");
    Q_EMIT unreadThreadCount(unreadThreads, unreadMentions, QPrivateSignal());
}

void core_dispatcher::onAddThreadResult(const int64_t _seq, core::coll_helper _params)
{
    Data::MessageParentTopic topic;
    topic.unserialize(_params);

    const auto error = _params.get_value_as_int("error");
    const auto threadId = _params.get<QString>("id");

    Q_EMIT messageThreadAdded(_seq, topic, threadId, error, QPrivateSignal());
}

void core_dispatcher::onGetThreadResult(const int64_t _seq, core::coll_helper _params)
{
    Data::ThreadInfo threadInfo;
    threadInfo.unserialize(_params);
    Q_EMIT threadInfoLoaded(_seq, threadInfo, QPrivateSignal());
}

void core_dispatcher::onSubscripionChanged(const int64_t _seq, core::coll_helper _params)
{
    Q_UNUSED(_seq);
    Q_EMIT subscriptionChanged(_params.get_value_as_bool("subscription"));
}

void core_dispatcher::onThreadsFeedGetResult(const int64_t _seq, core::coll_helper _params)
{
    auto items = _params.get_value_as_array("data");
    const auto resetPage = _params.get_value_as_bool("reset_page");
    const auto itemsSize = items->size();
    std::vector<Data::ThreadFeedItemData> itemsData;
    itemsData.reserve(itemsSize);
    for (int32_t i = 0; i < itemsSize; ++i)
    {
        Data::ThreadFeedItemData data;
        core::coll_helper value(items->get_at(i)->get_as_collection(), false);
        const auto type = Data::ParentTopic::unserialize_type(value);
        auto parentTopic = Data::ParentTopic::createParentTopic(type);
        if (Data::ParentTopic::Type::message != type)
            continue;
        auto messageParentTopic = std::static_pointer_cast<Data::MessageParentTopic>(parentTopic);
        messageParentTopic->unserialize(value);
        const auto threadId = value.get<QString>("thread_id");
        auto msg = Data::unserializeMessage(core::coll_helper(value.get_value_as_collection("parent_message"), false), threadId, MyInfo()->aimId(), -1, -1);
        msg->threadData_.parentTopic_ = messageParentTopic;
        msg->threadData_.threadFeedMessage_ = true;
        data.parentMessage_ = msg;
        data.threadId_ = threadId;
        data.parent_ = messageParentTopic;
        data.olderMsgId_ = value.get_value_as_int64("older_msg_id");
        data.repliesCount_ = value.get_value_as_int64("replies_count");
        data.unreadCount_ = value.get_value_as_int64("unread_count");

        Data::unserializeMessages(value.get_value_as_array("messages"), data.threadId_, MyInfo()->aimId(), -1, -1, Out data.messages_);
        for (auto& msg : data.messages_)
        {
            msg->SetOutgoing(msg->getSender() == MyInfo()->aimId()); // fix https://jira.mail.ru/browse/IMSERVER-12561 until it is fixed on server
            msg->threadData_.threadFeedMessage_ = true;
        }

        itemsData.push_back(std::move(data));
    }

    const auto cursor = _params.get<QString>("cursor");
    Q_EMIT threadsFeedGetResult(itemsData, cursor, resetPage);
}

void core_dispatcher::onThreadsSubscribersGetResult(const int64_t _seq, core::coll_helper _params)
{
    if (_params->is_value_exist("error"))
    {
        Q_EMIT chatMembersPage(_seq, nullptr, -1, false);
        return;
    }

    const auto info = std::make_shared<Data::ChatMembersPage>(Data::UnserializeChatMembersPage(&_params));
    Q_EMIT chatMembersPage(_seq, info, _params.get_value_as_int("page_size"), _params.get_value_as_bool("reset_pages", false));
}

void core_dispatcher::onThreadsSubscribersSearchResult(const int64_t _seq, core::coll_helper _params)
{
    if (_params->is_value_exist("error"))
    {
        Q_EMIT searchChatMembersPage(_seq, nullptr, -1, false);
        return;
    }

    const auto info = std::make_shared<Data::ChatMembersPage>(Data::UnserializeChatMembersPage(&_params));
    Q_EMIT searchChatMembersPage(_seq, info, _params.get_value_as_int("page_size"), _params.get_value_as_bool("reset_pages", false));
}

void core_dispatcher::onAuthData(const int64_t _seq, core::coll_helper _params)
{
    Q_UNUSED(_seq)
    Q_EMIT authParamsChanged(Data::AuthParams(_params));
}

void core_dispatcher::onDraft(const int64_t _seq, core::coll_helper _params)
{
    const auto contact = _params.get<QString>("contact");
    Data::Draft draftData;
    draftData.unserialize(_params, contact);

    Q_EMIT draft(contact, draftData);
}

void core_dispatcher::onTaskCreateResult(const int64_t _seq, core::coll_helper _params)
{
    int error = _params.get_value_as_int("error");
    Q_EMIT taskCreated(_seq, error);
}

void core_dispatcher::onTaskUpdate(const int64_t _seq, core::coll_helper _params)
{
    Data::TaskData task;
    task.unserialize(_params.get_value_as_collection("task"));
    Q_EMIT updateTask(task);
}

void core_dispatcher::onTaskEditResult(const int64_t _seq, core::coll_helper _params)
{
    if (_params.get_value_as_int("error"))
        Utils::showTextToastOverContactDialog(QT_TRANSLATE_NOOP("task_block", "Task edit: an error occured"));
}

void core_dispatcher::onTasksLoaded(const int64_t _seq, core::coll_helper _params)
{
    if (auto array = _params.get_value_as_array("tasks"))
    {
        const auto size = array->size();
        QVector<Data::TaskData> tasks;
        tasks.reserve(size);
        for (std::decay_t<decltype(size)> i = 0; i < size; ++i)
        {
            Data::TaskData task;
            core::coll_helper helper(array->get_at(i)->get_as_collection(), false);
            task.unserialize(helper.get_value_as_collection("task"));
            tasks.append(std::move(task));
        }
        Q_EMIT tasksLoaded(tasks);
    }
}

void Ui::core_dispatcher::onUnreadTasksCounter(const int64_t /*_seq*/, core::coll_helper _params)
{
    Q_EMIT Utils::InterConnector::instance().updateMiniAppCounter(Utils::MiniApps::getTasksId(), _params.get_value_as_int("counter"));
}

void Ui::core_dispatcher::onUnreadMailsCounter(const int64_t /*_seq*/, core::coll_helper _params)
{
    Q_EMIT Utils::InterConnector::instance().updateMiniAppCounter(Utils::MiniApps::getMailId(), _params.get_value_as_int("counter"));
}

void Ui::core_dispatcher::onMiniappUnavailable(const int64_t _seq, core::coll_helper _params)
{
    Q_UNUSED(_seq);
    const auto id = _params.get<QString>("id");
    Q_EMIT appUnavailable(id);
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

void core_dispatcher::onAddMembersResult(const int64_t _seq, core::coll_helper _params)
{
    if (auto failures = unserializeChatFailures(_params); !failures.empty())
        Utils::InterConnector::instance().showChatMembersFailuresPopup(Utils::ChatMembersOperation::Add, QString::fromUtf8(_params.get_value_as_string("chat")), std::move(failures));
}

void Ui::core_dispatcher::onRemoveMembersResult(const int64_t _seq, core::coll_helper _params)
{
    if (auto failures = unserializeChatFailures(_params); !failures.empty())
        Utils::InterConnector::instance().showChatMembersFailuresPopup(Utils::ChatMembersOperation::Remove, QString::fromUtf8(_params.get_value_as_string("chat")), std::move(failures));
}

void core_dispatcher::onInviterBlacklistSearchResult(const int64_t _seq, core::coll_helper _params)
{
    if (_params.is_value_exist("error"))
    {
        Q_EMIT inviterBlacklistSearchError(QPrivateSignal());
        return;
    }

    Data::SearchResultsV res;
    const auto array = _params.get_value_as_array("contacts");
    const auto size = array->size();
    res.reserve(size);
    for (std::remove_const_t<decltype(size)> i = 0; i < size; ++i)
    {
        auto resItem = std::make_shared<Data::SearchResultContactChatLocal>();
        resItem->aimId_ = QString::fromUtf8(array->get_at(i)->get_as_string());
        resItem->isChat_ = Utils::isChat(resItem->aimId_);
        res.push_back(std::move(resItem));
    }

    Q_EMIT inviterBlacklistSearchResults(res, QString::fromUtf8(_params.get_value_as_string("cursor")), _params.get_value_as_bool("reset_pages"), QPrivateSignal());
}

void core_dispatcher::onInviterBlacklistRemoveResult(const int64_t _seq, core::coll_helper _params)
{
    std::vector<QString> res;
    const auto array = _params.get_value_as_array("contacts");
    const auto size = array->size();
    res.reserve(size);
    for (int i = 0; i < size; ++i)
        res.push_back(QString::fromUtf8(array->get_at(i)->get_as_string()));

    Q_EMIT inviterBlacklistRemoveResult(res, _params.get_value_as_int("error") == 0, QPrivateSignal());
}

void core_dispatcher::onInviterBlacklistAddResult(const int64_t _seq, core::coll_helper _params)
{
    std::vector<QString> res;
    const auto array = _params.get_value_as_array("contacts");
    const auto size = array->size();
    res.reserve(size);
    for (int i = 0; i < size; ++i)
        res.push_back(QString::fromUtf8(array->get_at(i)->get_as_string()));

    Q_EMIT inviterBlacklistAddResult(res, _params.get_value_as_int("error") == 0, QPrivateSignal());
}

void core_dispatcher::onInviterBlacklistedCLContacts(const int64_t _seq, core::coll_helper _params)
{
    std::vector<QString> res;
    const auto array = _params.get_value_as_array("contacts");
    const auto size = array->size();
    res.reserve(size);
    for (int i = 0; i < size; ++i)
        res.push_back(QString::fromUtf8(array->get_at(i)->get_as_string()));

    Q_EMIT inviterBlacklistGetResults(res, QString::fromUtf8(_params.get_value_as_string("cursor")), _params.get_value_as_int("error"), QPrivateSignal());
}

void core_dispatcher::onGetEmojiResult(const int64_t _seq, core::coll_helper _params)
{
    if (_params.is_value_exist("local"))
    {
        const auto local = _params.get<QString>("local");

        auto task = new Utils::LoadImageFromFileTask(local);

        const auto succeeded = QObject::connect(
            task, &Utils::LoadImageFromFileTask::loadedSignal,
            this,
            [this, _seq]
        (const QImage& _image)
        {
            Q_EMIT getEmojiResult(_seq, _image);
        });
        im_assert(succeeded);

        QThreadPool::globalInstance()->start(task);
    }
    else
    {
        auto networkError = false;
        if (_params.is_value_exist("network_error"))
            networkError = _params.get_value_as_bool("network_error");

        Q_EMIT getEmojiFailed(_seq, networkError);
    }
}

namespace { std::unique_ptr<core_dispatcher> gDispatcher; }

core_dispatcher* Ui::GetDispatcher()
{
    if (!gDispatcher)
    {
        im_assert(false);
    }

    return gDispatcher.get();
}

void Ui::createDispatcher()
{
    if (gDispatcher)
    {
        im_assert(false);
    }

    gDispatcher = std::make_unique<core_dispatcher>();
}

void Ui::destroyDispatcher()
{
    if (!gDispatcher)
    {
        im_assert(false);
    }

    gDispatcher.reset();
}
