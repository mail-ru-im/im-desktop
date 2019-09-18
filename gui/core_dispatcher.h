#pragma once

#include "voip/VoipProxy.h"

#include "../corelib/core_face.h"
#include "../corelib/collection_helper.h"
#include "../corelib/enumerations.h"

#include "memory_stats/MemoryStatsRequest.h"
#include "memory_stats/MemoryStatsResponse.h"

#include "types/chat.h"
#include "types/chatheads.h"
#include "types/common_phone.h"
#include "types/contact.h"
#include "types/images.h"
#include "types/link_metadata.h"
#include "types/message.h"
#include "types/typing.h"
#include "types/mention_suggest.h"
#include "types/search_result.h"
#include "types/filesharing_meta.h"
#include "types/filesharing_download_result.h"
#include "types/idinfo.h"
#include "types/friendly.h"
#include "types/masks.h"
#include "my_info.h"

namespace voip_manager
{
    struct Contact;
    struct ContactEx;
}

namespace voip_proxy
{
    struct device_desc;
}

namespace core
{
    enum class file_sharing_function;
    enum class sticker_size;
    enum class group_chat_info_errors;
    enum class typing_status;
}

namespace launch
{
    int main(int argc, char *argv[]);
}

namespace Utils
{
    struct ProxySettings;
}

namespace stickers
{
    enum class sticker_size;
}

namespace Ui
{
    enum class ConnectionState
    {
        stateUnknown = 0,
        stateConnecting = 1,
        stateUpdating = 2,
        stateOnline = 3
    };

    typedef std::function<void(int64_t, core::coll_helper&)> message_function;

    #define REGISTER_IM_MESSAGE(_message_string, _callback) \
        messages_map_.emplace( \
        _message_string, \
        std::bind(&core_dispatcher::_callback, this, std::placeholders::_1, std::placeholders::_2));

    namespace Stickers
    {
        class Set;
        class Sticker;
    }

    class gui_signal : public QObject
    {
        Q_OBJECT
    public:

Q_SIGNALS:
        void received(const QString&, const qint64, core::icollection*);
    };

    class gui_connector : public gui_signal, public core::iconnector
    {
        std::atomic<int>    refCount_;

        // ibase interface
        virtual int addref() override;
        virtual int release() override;

        // iconnector interface
        virtual void link(iconnector*, const common::core_gui_settings&) override;
        virtual void unlink() override;
        virtual void receive(std::string_view, int64_t, core::icollection*) override;
    public:
        gui_connector() : refCount_(1) {}
    };

    enum class MessagesBuddiesOpt
    {
        Min,

        Requested,
        Updated,
        RequestedByIds,
        FromServer,
        DlgState,
        Intro,
        Pending,
        Init,
        MessageStatus,
        Search,
        Context,

        Max
    };

    enum class MessagesNetworkError
    {
        No,
        Yes
    };

    typedef std::function<void(core::icollection*)> message_processed_callback;

    class core_dispatcher : public QObject
    {
        Q_OBJECT

Q_SIGNALS:
        void needLogin(const bool _is_auth_error);
        void contactList(const std::shared_ptr<Data::ContactList>&, const QString&);
        void im_created();
        void loginComplete();
        void getImagesResult(const Data::ImageListPtr& images);
        void messageBuddies(const Data::MessageBuddies&, const QString&, Ui::MessagesBuddiesOpt, bool, qint64, qint64 last_msg_id);
        void messageIdsFromServer(const QVector<qint64>&, const QString&, qint64);
        void messageContextError(const QString&, qint64 _seq, qint64 _id, Ui::MessagesNetworkError) const;
        void messageLoadAfterSearchError(const QString&, qint64 _seq, qint64 _from, qint64 _countLater, qint64 _countEarly, Ui::MessagesNetworkError) const;
        void messageIdsUpdated(const QVector<qint64>&, const QString&, qint64);
        void getSmsResult(int64_t, int _errCode, int _codeLength, const QString& _ivrUrl, const QString& _checks);
        void loginResult(int64_t, int code);
        void loginResultAttachUin(int64_t, int _code);
        void loginResultAttachPhone(int64_t, int _code);
        void avatarLoaded(int64_t, const QString&, QPixmap&, int, bool _result);
        void avatarUpdated(const QString &);

        void presense(const std::shared_ptr<Data::Buddy>&);
        void outgoingMsgCount(const QString& _aimid, const int _count);
        void searchResult(const QStringList&);
        void dlgStates(const QVector<Data::DlgState>&);

        void searchedMessagesLocal(const Data::SearchResultsV& _msg, const qint64 _seq);
        void searchedMessagesLocalEmptyResult(const qint64 _seq);
        void searchedMessagesServer(const Data::SearchResultsV& _results, const QString& _cursorNext, const int _total, const qint64 _seq);

        void searchedContactsLocal(const Data::SearchResultsV& _results, const qint64 _seq);
        void searchedContactsServer(const Data::SearchResultsV& _results, const qint64 _seq);
        void syncronizedAddressBook(const bool _result);


        void activeDialogHide(const QString&);
        void guiSettings();
        void coreLogins(const bool _has_valid_login, const bool _locked);
        void themeSettings();
        void chatInfo(const qint64, const std::shared_ptr<Data::ChatInfo>&, const int _requestMembersLimit);
        void chatInfoFailed(qint64, core::group_chat_info_errors, const QString&);
        void chatMemberInfo(const qint64 _seq, const std::shared_ptr<Data::ChatMembersPage>& _info);
        void chatMembersPage(const qint64 _seq, const std::shared_ptr<Data::ChatMembersPage>& _page, const int _pageSize, const bool _resetPages);
        void searchChatMembersPage(const qint64 _seq, const std::shared_ptr<Data::ChatMembersPage>& _page, const int _pageSize, const bool _resetPages);
        void chatContacts(const qint64 _seq, const std::shared_ptr<Data::ChatContacts>& _contacts);
        void myInfo();
        void login_new_user();
        void messagesDeleted(const QString&, const Data::MessageBuddies&);
        void messagesDeletedUpTo(const QString&, int64_t);
        void messagesModified(const QString&, const Data::MessageBuddies&);
        void messagesClear(const QString&, qint64);
        void mentionMe(const QString& _contact, Data::MessageBuddySptr _mention);
        void removePending(const QString& _aimId, const Logic::MessageKey& _key);

        void typingStatus(const Logic::TypingFires& _typing, bool _isTyping);

        void messagesReceived(const QString&, const QVector<QString>&);

        void contactRemoved(const QString&);
        void openChat(const QString& _aimid);

        void feedbackSent(bool);

        void friendlyUpdate(const QString& _aimid, const Data::Friendly& _friendly, QPrivateSignal);

        void memberAddFailed(const int _sipCode);

        // sticker signals
        void onStickers();
        void onSticker(
            const qint32 _error,
            const qint32 _setId,
            const qint32 _stickerId,
            const QString& _fsId);
        void onStore();
        void onStoreSearch();

        void onStickerMigration(const std::vector<Ui::Stickers::Sticker>&);

        void onSetBigIcon(const qint32 _error, const qint32 _setId);

        void onStickerpackInfo(const bool _result, const bool _exist, const std::shared_ptr<Ui::Stickers::Set>& _set);

        void onWallpaper(const QString& _id, const QPixmap& _image, QPrivateSignal);
        void onWallpaperPreview(const QString& _id, const QPixmap& _image, QPrivateSignal);

        // remote files signals
        void imageDownloaded(qint64 _seq, const QString& _rawUri, const QPixmap& _image, const QString& _localPath);
        void imageMetaDownloaded(qint64 _seq, const Data::LinkMetadata& _meta);
        void imageDownloadingProgress(qint64 _seq, int64_t _bytesTotal, int64_t _bytesTransferred, int32_t _pctTransferred);
        void imageDownloadError(int64_t _seq, const QString& _rawUri);

        void fileSharingError(qint64 _seq, const QString& _rawUri, qint32 _errorCode);

        void fileDownloaded(const QString& _requestedUri, const QString& _rawUri, const QString& _localPath);
        void fileSharingFileDownloaded(qint64 _seq, const Data::FileSharingDownloadResult& _result);
        void fileSharingFileDownloading(qint64 _seq, const QString& _rawUri, qint64 _bytesTransferred, qint64 _bytesTotal);

        void fileSharingFileMetainfoDownloaded(qint64 _seq, const Data::FileSharingMeta& _meta);
        void fileSharingPreviewMetainfoDownloaded(qint64 _seq, const QString& _miniPreviewUri, const QString& _fullPreviewUri);

        void fileSharingLocalCopyCheckCompleted(qint64 _seq, bool _success, const QString& _localPath);

        void fileSharingUploadingProgress(const QString& _uploadingId, qint64 _bytesTransferred);
        void fileSharingUploadingResult(const QString& _seq, bool _success, const QString& localPath, const QString& _uri, int _contentType, qint64 _size, qint64 _lastModified, bool _isFileTooBig);

        void linkMetainfoMetaDownloaded(int64_t _seq, bool _success, const Data::LinkMetadata& _meta);
        void linkMetainfoImageDownloaded(int64_t _seq, bool _success, const QPixmap& _image);
        void linkMetainfoFaviconDownloaded(int64_t _seq, bool _success, const QPixmap& _image);

        void signedUrl(const QString&);
        void speechToText(qint64 _seq, int _error, const QString& _text, int _comeback);

        void chatsHome(const QVector<Data::ChatInfo>&, const QString&, bool, bool);
        void chatsHomeError(int);

        void recvPermitDeny(bool);

        void updateProfile(int);
        void getUserProxy();

        void setChatRoleResult(int);
        void blockMemberResult(int);
        void pendingListResult(int);

        void phoneInfoResult(qint64, const Data::PhoneInfo&);

        // masks
        void maskListLoaded(const QVector<QString>& maskList);
        void maskPreviewLoaded(int64_t _seq, const QString& _localPath);
        void maskModelLoaded();
        void maskLoaded(int64_t _seq, const QString& _localPath);
        void maskLoadingProgress(int64_t _seq, unsigned _percent);
        void maskRetryUpdate();
        void existentMasksLoaded(const std::vector<Data::Mask>& _masks);

        void appConfig();

        void mailStatus(const QString&, unsigned, bool);
        void newMail(const QString&, const QString&, const QString&, const QString&);
        void mrimKey(qint64, const QString&);

        void historyUpdate(const QString&, qint64);
        void chatHeads(const Data::ChatHeads&);

        void mentionsSuggestResults(const int64_t _seq, const Logic::MentionsSuggests& _suggests);
        void memoryUsageReportReady(const Memory_Stats::Response& _response,
                                    Memory_Stats::RequestId _req_id);

        void updateReady();

        void connectionStateChanged(const Ui::ConnectionState& _state);

        void dialogGalleryResult(const int64_t _seq, const QVector<Data::DialogGalleryEntry>& _entries, bool _exhausted);

        void dialogGalleryByMsgResult(const int64_t _seq, const QVector<Data::DialogGalleryEntry>& _entries, int _index, int _total);

        void searchPatternsLoaded(const QString& _contact, const QVector<QString>& _patterns, QPrivateSignal);

        void dialogGalleryState(const QString& _aimid, const Data::DialogGalleryState& _state);

        void dialogGalleryUpdate(const QString& _aimid, const QVector<Data::DialogGalleryEntry>& _entries);

        void dialogGalleryInit(const QString& _aimid);

        void dialogGalleryHolesDownloaded(const QString& _aimid);

        void dialogGalleryHolesDownloading(const QString& _aimid);

        void dialogGalleryErased(const QString& _aimid);

        void dialogGalleryIndex(const QString& _aimdId, qint64 _msg, qint64 _seq, int _index, int _total);

        void localPINStatus(const bool _enabled, const bool _locked);

        void localPINChecked(const int64_t _seq, const bool _result);

        void attachPhoneInfo(const int64_t _seq, const Ui::AttachPhoneInfo& _result);

        void idInfo(const int64_t _seq, const Data::IdInfo& _info, QPrivateSignal);

        void userInfo(const int64_t _seq, const QString& _aimid, const Data::UserInfo& _info);

        void userLastSeen(const int64_t _seq, const std::map<QString, int32_t>& _lastseens);

        void nickCheckSetResult(qint64 _seq, int _result);

        void setPrivacyParamsResult(const int64_t _seq, const bool _success);

        void commonChatsGetResult(const int64_t _seq, const std::vector<Data::CommonChatInfo>& _chats);

    public Q_SLOTS:
        void received(const QString&, const qint64, core::icollection*);

    public:
        core_dispatcher();
        virtual ~core_dispatcher();

        core::icollection* create_collection() const;
        qint64 post_message_to_core(std::string_view _message, core::icollection* _collection, const QObject* _object = nullptr, message_processed_callback _callback = nullptr);

        qint64 post_stats_to_core(core::stats::stats_event_names _eventName, const core::stats::event_props_type& _props = {});
        qint64 post_im_stats_to_core(core::stats::im_stat_event_names _eventName, const core::stats::event_props_type& _props = {});

        void set_enabled_stats_post(bool _isStatsEnabled);
        bool get_enabled_stats_post() const;

        voip_proxy::VoipController& getVoipController();

        qint64 getFileSharingPreviewSize(const QString& _url, const int32_t _originalSize = -1);
        qint64 downloadFileSharingMetainfo(const QString& _url);
        qint64 downloadSharedFile(const QString& _contactAimid, const QString& _url, bool _forceRequestMetainfo, const QString& _fileName = QString(), bool _raise_priority = false);

        qint64 abortSharedFileDownloading(const QString& _url);

        qint64 uploadSharedFile(const QString& _contact, const QString& _localPath, const Data::QuotesVec& _quotes = {}, const QString& _description = QString(), const Data::MentionMap& _mentions = {});
        qint64 uploadSharedFile(const QString& _contact, const QByteArray& _array, const QString& _ext, const Data::QuotesVec& _quotes = {});
        qint64 uploadSharedFile(const QString& _contact, const QByteArray& _array, const QString& _ext, const Data::QuotesVec& _quotes, const QString& _description, const Data::MentionMap& _mentions);
        qint64 uploadSharedFile(const QString& _contact, const QByteArray& _array, const QString& _ext, std::optional<std::chrono::seconds> _duration);
        qint64 uploadSharedFile(const QString& _contact, const QByteArray& _array, const QString& _ext, const Data::QuotesVec& _quotes, const QString& _description, const Data::MentionMap& _mentions, std::optional<std::chrono::seconds> _duration);
        qint64 uploadSharedFile(const QString& _contact, const QString& _localPath, const Data::QuotesVec& _quotes, std::optional<std::chrono::seconds> _duration);
        qint64 abortSharedFileUploading(const QString& _contact, const QString& _localPath, const QString& _uploadingProcessId);

        qint64 getSticker(const qint32 _setId, const qint32 _stickerId, const core::sticker_size _size);
        qint64 getSticker(const qint32 _setId, const QString& _fsId, const core::sticker_size _size);
        qint64 getSticker(const QString& _fsId, const core::sticker_size _size);
        qint64 getSetIconBig(const qint32 _setId);
        qint64 cleanSetIconBig(const qint32 _setId);
        qint64 getStickersPackInfo(const qint32 _setId, const QString& _storeId, const QString& _fileId);
        qint64 addStickersPack(const qint32 _setId, const QString& _storeId);
        qint64 removeStickersPack(const qint32 _setId, const QString& _storeId);
        qint64 getStickersStore();
        qint64 searchStickers(const QString& _searchTerm);

        qint64 getFsStickersByIds(const std::vector<int32_t>& _ids);

        int64_t downloadImage(const QUrl& _uri,
            const QString& _destination,
            const bool _isPreview,
            const int32_t _maxPreviewWidth,
            const int32_t _maxPreviewHeight,
            const bool _externalResource,
            const bool _raisePriority = false,
            const bool _withData = true);

        void cancelLoaderTask(const QString& _url);

        int64_t downloadLinkMetainfo(
            const QString& _contactAimid,
            const QString& _uri,
            const int32_t _previewWidth,
            const int32_t _previewHeight);

        qint64 deleteMessage(const int64_t _messageId, const QString& _internalId, const QString& _contactAimId, const bool _forAll);
        qint64 deleteMessagesFrom(const QString& _contactAimId, const int64_t _fromId);
        qint64 eraseHistory(const QString& _contactAimId);

        bool isImCreated() const;

        qint64 raiseDownloadPriority(const QString &_contactAimid, int64_t _procId);

        qint64 contactSwitched(const QString &_contactAimid);

        void sendMessageToContact(const QString& _contact, const QString& _text, const Data::QuotesVec& _quotes, const Data::MentionMap& _mentions);
        void updateMessageToContact(const QString& _contact, int64_t _messageId, const QString& _text);

        int64_t pttToText(const QString& _pttLink, const QString& _locale);

        int64_t setUrlPlayed(const QString& _url, const bool _isPlayed);

        void setUserState(const core::profile_state state);
        void invokeStateAway();
        void invokePreviousState();

        void onArchiveMessages(Ui::MessagesBuddiesOpt _type, const int64_t _seq, core::coll_helper _params);

        void getCodeByPhoneCall(const QString& _ivr_url);

        qint64 getDialogGallery(const QString& _aimId, const QStringList& _types, int64_t _after_msg, int64_t _after_seq, const int _pageSize);
        qint64 getDialogGalleryByMsg(const QString& _aimId, const QStringList& _types, int64_t _id);
        qint64 requestDialogGalleryState(const QString& _aimId);
        qint64 getDialogGalleryIndex(const QString& _aimId, const QStringList& _types, qint64 _msg, qint64 _seq);

        void postUiActivity();

        void setLocalPIN(const QString& _password);

        qint64 getUserInfo(const QString& _aimid);

        qint64 getUserLastSeen(const std::vector<QString>& _ids);

        qint64 localPINEntered(const QString& _password);

        void disableLocalPIN();

        qint64 getIdInfo(const QString& _id);

        qint64 checkNickname(const QString& _nick, bool _set_nick);

        void updateRecentAvatarsSize();

        // messages
        void onNeedLogin(const int64_t _seq, core::coll_helper _params);
        void onImCreated(const int64_t _seq, core::coll_helper _params);
        void onLoginComplete(const int64_t _seq, core::coll_helper _params);
        void onContactList(const int64_t _seq, core::coll_helper _params);
        void onLoginGetSmsCodeResult(const int64_t _seq, core::coll_helper _params);
        void onLoginResult(const int64_t _seq, core::coll_helper _params);
        void onAvatarsGetResult(const int64_t _seq, core::coll_helper _params);
        void onAvatarsPresenceUpdated(const int64_t _seq, core::coll_helper _params);
        void onContactPresence(const int64_t _seq, core::coll_helper _params);
        void onContactOutgoingMsgCount(const int64_t _seq, core::coll_helper _params);
        void onGuiSettings(const int64_t _seq, core::coll_helper _params);
        void onCoreLogins(const int64_t _seq, core::coll_helper _params);
        void onArchiveImagesGetResult(const int64_t _seq, core::coll_helper _params);
        void onArchiveMessagesGetResult(const int64_t _seq, core::coll_helper _params);
        void onArchiveBuddiesGetResult(const int64_t _seq, core::coll_helper _params);
        void onArchiveMentionsGetResult(const int64_t _seq, core::coll_helper _params);
        void onMessagesReceivedDlgState(const int64_t _seq, core::coll_helper _params);
        void onMessagesReceivedUpdated(const int64_t _seq, core::coll_helper _params);
        void onMessagesReceivedServer(const int64_t _seq, core::coll_helper _params);
        void onMessagesReceivedSearch(const int64_t _seq, core::coll_helper _params);
        void onMessagesClear(const int64_t _seq, core::coll_helper _params);
        void onArchiveMessagesPending(const int64_t _seq, core::coll_helper _params);
        void onMessagesReceivedInit(const int64_t _seq, core::coll_helper _params);
        void onMessagesReceivedContext(const int64_t _seq, core::coll_helper _params);
        void onMessagesContextError(const int64_t _seq, core::coll_helper _params);
        void onMessagesLoadAfterSearchError(const int64_t _seq, core::coll_helper _params);
        void onMessagesReceivedMessageStatus(const int64_t _seq, core::coll_helper _params);
        void onMessagesDelUpTo(const int64_t _seq, core::coll_helper _params);
        void onDlgStates(const int64_t _seq, core::coll_helper _params);

        void onDialogsSearchLocalResults(const int64_t _seq, core::coll_helper _params);
        void onDialogsSearchLocalEmptyResult(const int64_t _seq, core::coll_helper _params);
        void onDialogsSearchServerResults(const int64_t _seq, core::coll_helper _params);
        void onDialogsSearchResult(const int64_t _seq, core::coll_helper _params, const bool _fromLocalSearch);

        void onDialogsSearchPatternHistory(const int64_t _seq, core::coll_helper _params);

        void onContactsSearchLocalResults(const int64_t _seq, core::coll_helper _params);
        void onContactsSearchServerResults(const int64_t _seq, core::coll_helper _params);

        void onSyncAddressBook(const int64_t _seq, core::coll_helper _params);

        void onHistoryUpdate(const int64_t _seq, core::coll_helper _params);

        void onVoipSignal(const int64_t _seq, core::coll_helper _params);
        void onActiveDialogsAreEmpty(const int64_t _seq, core::coll_helper _params);
        void onActiveDialogsHide(const int64_t _seq, core::coll_helper _params);
        void onActiveDialogsSent(const int64_t _seq, core::coll_helper _params);
        void onStickersMetaGetResult(const int64_t _seq, core::coll_helper _params);
        void onStickerGetFsByIdResult(const int64_t _seq, core::coll_helper _params);

        void onStickersStickerGetResult(const int64_t _seq, core::coll_helper _params);
        void onStickersGetSetBigIconResult(const int64_t _seq, core::coll_helper _params);
        void onStickersPackInfo(const int64_t _seq, core::coll_helper _params);
        void onStickersStore(const int64_t _seq, core::coll_helper _params);
        void onStickersStoreSearch(const int64_t _seq, core::coll_helper _params);
        void onStickersSuggests(const int64_t _seq, core::coll_helper _params);

        void onThemeSettings(const int64_t _seq, core::coll_helper _params);
        void onThemesWallpaperGetResult(const int64_t _seq, core::coll_helper _params);
        void onThemesWallpaperPreviewGetResult(const int64_t _seq, core::coll_helper _params);

        void onChatsInfoGetResult(const int64_t _seq, core::coll_helper _params);
        void onChatsInfoGetFailed(const int64_t _seq, core::coll_helper _params);
        void onMemberAddFailed(const int64_t _seq, core::coll_helper _params);
        void onChatsMemberInfoResult(const int64_t _seq, core::coll_helper _params);
        void onChatsMembersGetResult(const int64_t _seq, core::coll_helper _params);
        void onChatsMembersSearchResult(const int64_t _seq, core::coll_helper _params);
        void onChatsContactsResult(const int64_t _seq, core::coll_helper _params);

        void fileSharingErrorResult(const int64_t _seq, core::coll_helper _params);
        void fileSharingDownloadProgress(const int64_t _seq, core::coll_helper _params);
        void fileSharingGetPreviewSizeResult(const int64_t _seq, core::coll_helper _params);
        void fileSharingMetainfoResult(const int64_t _seq, core::coll_helper _params);
        void fileSharingCheckExistsResult(const int64_t _seq, core::coll_helper _params);
        void fileSharingDownloadResult(const int64_t _seq, core::coll_helper _params);

        void imageDownloadProgress(const int64_t _seq, core::coll_helper _params);
        void imageDownloadResult(const int64_t _seq, core::coll_helper _params);
        void imageDownloadResultMeta(const int64_t _seq, core::coll_helper _params);
        void fileUploadingProgress(const int64_t _seq, core::coll_helper _params);
        void fileUploadingResult(const int64_t _seq, core::coll_helper _params);
        void linkMetainfoDownloadResultMeta(const int64_t _seq, core::coll_helper _params);
        void linkMetainfoDownloadResultImage(const int64_t _seq, core::coll_helper _params);
        void linkMetainfoDownloadResultFavicon(const int64_t _seq, core::coll_helper _params);
        void onFilesSpeechToTextResult(const int64_t _seq, core::coll_helper _params);
        void onContactsRemoveResult(const int64_t _seq, core::coll_helper _params);
        void onAppConfig(const int64_t _seq, core::coll_helper _params);
        void onMyInfo(const int64_t _seq, core::coll_helper _params);
        void onSignedUrl(const int64_t _seq, core::coll_helper _params);
        void onFeedbackSent(const int64_t _seq, core::coll_helper _params);
        void onMessagesReceivedSenders(const int64_t _seq, core::coll_helper _params);
        void onTyping(const int64_t _seq, core::coll_helper _params);
        void onTypingStop(const int64_t _seq, core::coll_helper _params);
        void onContactsGetIgnoreResult(const int64_t _seq, core::coll_helper _params);
        void onLoginResultAttachUin(const int64_t _seq, core::coll_helper _params);
        void onLoginResultAttachPhone(const int64_t _seq, core::coll_helper _params);
        void onUpdateProfileResult(const int64_t _seq, core::coll_helper _params);
        void onChatsHomeGetResult(const int64_t _seq, core::coll_helper _params);
        void onChatsHomeGetFailed(const int64_t _seq, core::coll_helper _params);
        void onUserProxyResult(const int64_t _seq, core::coll_helper _params);
        void onOpenCreatedChat(const int64_t _seq, core::coll_helper _params);
        void onLoginNewUser(const int64_t _seq, core::coll_helper _params);
        void onSetAvatarResult(const int64_t _seq, core::coll_helper _params);
        void onChatsRoleSetResult(const int64_t _seq, core::coll_helper _params);
        void onChatsBlockResult(const int64_t _seq, core::coll_helper _params);
        void onChatsPendingResolveResult(const int64_t _seq, core::coll_helper _params);
        void onPhoneinfoResult(const int64_t _seq, core::coll_helper _params);
        void onContactRemovedFromIgnore(const int64_t _seq, core::coll_helper _params);
        void onFriendlyUpdate(const int64_t _seq, core::coll_helper _params);
        void onMasksGetIdListResult(const int64_t _seq, core::coll_helper _params);
        void onMasksPreviewResult(const int64_t _seq, core::coll_helper _params);
        void onMasksModelResult(const int64_t _seq, core::coll_helper _params);
        void onMasksGetResult(const int64_t _seq, core::coll_helper _params);
        void onMasksProgress(const int64_t _seq, core::coll_helper _params);
        void onMasksRetryUpdate(const int64_t _seq, core::coll_helper _params);
        void onExistentMasks(const int64_t _seq, core::coll_helper _params);
        void onMailStatus(const int64_t _seq, core::coll_helper _params);
        void onMailNew(const int64_t _seq, core::coll_helper _params);
        void getMrimKeyResult(const int64_t _seq, core::coll_helper _params);
        void onMentionsMeReceived(const int64_t _seq, core::coll_helper _params);
        void onMentionsSuggestResults(const int64_t _seq, core::coll_helper _params);
        void onChatHeads(const int64_t _seq, core::coll_helper _params);
        void onAttachPhoneInfo(const int64_t _seq, core::coll_helper _params);
        void onUpdateReady(const int64_t _seq, core::coll_helper _params);
        void onEventCachedObjectsLoaded(const int64_t _seq, core::coll_helper _params);
        void onEventConnectionState(const int64_t _seq, core::coll_helper _params);
        void onGetDialogGalleryResult(const int64_t _seq, core::coll_helper _params);
        void onGetDialogGalleryByMsgResult(const int64_t _seq, core::coll_helper _params);
        void onGalleryState(const int64_t _seq, core::coll_helper _params);
        void onGalleryUpdate(const int64_t _seq, core::coll_helper _params);
        void onGalleryInit(const int64_t _seq, core::coll_helper _params);
        void onGalleryHolesDownloaded(const int64_t _seq, core::coll_helper _params);
        void onGalleryHolesDownloading(const int64_t _seq, core::coll_helper _params);
        void onGalleryErased(const int64_t _seq, core::coll_helper _params);
        void onGalleryIndex(const int64_t _seq, core::coll_helper _params);
        void onLocalPINChecked(const int64_t _seq, core::coll_helper _params);
        void onIdInfo(const int64_t _seq, core::coll_helper _params);

        const Ui::ConnectionState& getConnectionState();

        void onRequestMemoryConsumption(const int64_t _seq, core::coll_helper _params);
        void onMemUsageReportReady(const int64_t _seq, core::coll_helper _params);
        void onGuiComponentsMemoryReportsReady(const Memory_Stats::RequestHandle& _req_handle,
                                               const Memory_Stats::ReportsList& _reports_list);

        void onGetUserInfoResult(const int64_t _seq, core::coll_helper _params);
        void onGetUserLastSeenResult(const int64_t _seq, core::coll_helper _params);

        void onSetPrivacySettingsResult(const int64_t _seq, core::coll_helper _params);

        void onGetCommonChatsResult(const int64_t _seq, core::coll_helper _params);

        void onNickCheckSetResult(const int64_t _seq, core::coll_helper _params);

        void onOmicronUpdateData(const int64_t _seq, core::coll_helper _params);

        void onFilesharingUploadAborted(const int64_t _seq, core::coll_helper _params);

    private:

        struct callback_info
        {
            message_processed_callback callback_;

            qint64 date_;

            const QObject* object_;

            callback_info(message_processed_callback&& _callback, qint64 _date, const QObject* _object)
            : callback_(std::move(_callback)), date_(_date), object_(_object)
            {
            }
        };

    private:

        bool init();
        void initMessageMap();
        void uninit();

        void cleanupCallbacks();
        void executeCallback(const int64_t _seq, core::icollection* _params);

        void onEventTyping(core::coll_helper _params, bool _isTyping);
        void onTypingCheckTimer();

    private:

        std::unordered_map<std::string, message_function> messages_map_;

        core::iconnector* coreConnector_;
        core::icore_interface* coreFace_;
        voip_proxy::VoipController voipController_;
        core::iconnector* guiConnector_;

        std::unordered_map<int64_t, callback_info> callbacks_;

        qint64 lastTimeCallbacksCleanedUp_;

        bool isStatsEnabled_;
        bool isImCreated_;

        bool userStateGoneAway_;

        ConnectionState connectionState_;

        QTimer* typingCheckTimer_;
    };

    core_dispatcher* GetDispatcher();
    void createDispatcher();
    void destroyDispatcher();
}
