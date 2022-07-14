#pragma once

#ifndef STRIP_VOIP
#include "voip/VoipProxy.h"
#endif

#include "../corelib/core_face.h"
#include "../corelib/collection_helper.h"
#include "../corelib/enumerations.h"

#include "memory_stats/MemoryStatsRequest.h"
#include "memory_stats/MemoryStatsResponse.h"

#include "types/chat.h"
#include "types/chatheads.h"
#include "types/common_phone.h"
#include "types/contact.h"
#include "types/link_metadata.h"
#include "types/message.h"
#include "types/typing.h"
#include "types/mention_suggest.h"
#include "types/search_result.h"
#include "types/filesharing_meta.h"
#include "types/filesharing_download_result.h"
#include "types/idinfo.h"
#include "types/friendly.h"
#include "types/smartreply_suggest.h"
#include "types/session_info.h"
#include "types/reactions.h"
#include "types/thread.h"
#include "types/thread_feed_data.h"
#include "types/authorization.h"
#include "my_info.h"

#include "statuses/Status.h"

#if defined(IM_AUTO_TESTING)
    class WebServer;
#endif

namespace core
{
    enum class sticker_size;
    enum class group_chat_info_errors;
}

namespace Utils
{
    struct FileSharingId;
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

    using message_function = std::function<void(int64_t, core::coll_helper&)>;

    #define REGISTER_IM_MESSAGE(_message_string, _callback) \
        messages_map_.emplace( \
        _message_string, \
        std::bind(&core_dispatcher::_callback, this, std::placeholders::_1, std::placeholders::_2));

    namespace Stickers
    {
        class Set;
        using stickerSptr = std::shared_ptr<class Sticker>;
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

    struct DeleteMessageInfo
    {
        DeleteMessageInfo(int64_t _id, const QString& _internalId, bool _forAll)
            : Id_(_id)
            , InternaId_(_internalId)
            , ForAll_(_forAll)
        {
        }

        int64_t Id_;
        QString InternaId_;
        bool ForAll_;
    };

    using message_processed_callback = std::function<void(core::icollection*)>;

    enum class ConferenceType
    {
        Webinar,
        Call
    };

    enum class ClosePage { No, Yes };

    struct SendMessageParams
    {
        std::optional<int64_t> draftDeleteTime_;
    };

    class core_dispatcher : public QObject
    {
        Q_OBJECT

Q_SIGNALS:
        void needLogin(const bool _is_auth_error);
        void needExternalUserAgreement(const QString& terms_of_use_url, const QString& privacy_policy_url);
        void contactList(const std::shared_ptr<Data::ContactList>&, const QString&);
        void im_created();
        void loginComplete();
        void messageBuddies(const Data::MessageBuddies&, const QString&, Ui::MessagesBuddiesOpt, bool, qint64, qint64 last_msg_id);
        void messageIdsFromServer(const QVector<qint64>&, const QString&, qint64);
        void messageContextError(const QString&, qint64 _seq, qint64 _id, Ui::MessagesNetworkError) const;
        void messageLoadAfterSearchError(const QString&, qint64 _seq, qint64 _from, qint64 _countLater, qint64 _countEarly, Ui::MessagesNetworkError) const;
        void messageIdsUpdated(const QVector<qint64>&, const QString&, qint64);
        void oauthRequired();
        void getSmsResult(int64_t, int _errCode, int _codeLength, const QString& _ivrUrl, const QString& _checks);
        void loginResult(int64_t, int code, bool fill);
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

        void searchedThreadsLocal(const Data::SearchResultsV& _msg, const qint64 _seq);
        void searchedThreadsLocalEmptyResult(const qint64 _seq);
        void searchedThreadsServer(const Data::SearchResultsV& _results, const QString& _cursorNext, const int _total, const qint64 _seq);

        void searchedContactsLocal(const Data::SearchResultsV& _results, const qint64 _seq);
        void searchedContactsServer(const Data::SearchResultsV& _results, const qint64 _seq);
        void syncronizedAddressBook();

        void activeDialogHide(const QString& _contact, Ui::ClosePage _closeHistoryPage);
        void guiSettings();
        void coreLogins(const bool _has_valid_login, const bool _locked, const QString& _validOrFirstLogin);
        void themeSettings();
        void chatInfo(const qint64, const std::shared_ptr<Data::ChatInfo>&, const int _requestMembersLimit);
        void chatInfoFailed(qint64, core::group_chat_info_errors, const QString&);
        void chatMemberInfo(const qint64 _seq, const std::shared_ptr<Data::ChatMembersPage>& _info);
        void chatMembersPage(const qint64 _seq, const std::shared_ptr<Data::ChatMembersPage>& _page, const int _pageSize, const bool _resetPages);
        void chatMembersPageCached(const std::shared_ptr<Data::ChatMembersPage>& _page, const QString& _role);
        void searchChatMembersPage(const qint64 _seq, const std::shared_ptr<Data::ChatMembersPage>& _page, const int _pageSize, const bool _resetPages);
        void chatContacts(const qint64 _seq, const std::shared_ptr<Data::ChatContacts>& _contacts);
        void myInfo();
        void login_new_user();
        void messagesDeleted(const QString&, const Data::MessageBuddies&);
        void messagesDeletedUpTo(const QString&, int64_t);
        void messagesModified(const QString&, const Data::MessageBuddies&);
        void messagesPatched(const Data::PatchedMessage&);
        void messagesClear(const QString&, qint64);
        void messagesEmpty(const QString&, qint64);
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
        void onStore();
        void onStoreSearch();

        void smartreplyInstantSuggests(const std::vector<Data::SmartreplySuggest>& _suggests);

        void stickersRequestedSuggestsResult(const QString& _contact, const QVector<Utils::FileSharingId>& _suggests);
        void smartreplyRequestedResult(const std::vector<Data::SmartreplySuggest>&);

        void onStickerMigration(const std::vector<Ui::Stickers::stickerSptr>&);

        void onStickerpackInfo(const bool _result, const bool _exist, const std::shared_ptr<Ui::Stickers::Set>& _set);

        void onWallpaper(const QString& _id, const QPixmap& _image, QPrivateSignal);
        void onWallpaperPreview(const QString& _id, const QPixmap& _image, QPrivateSignal);

        // remote files signals
        void imageDownloaded(qint64 _seq, const QString& _rawUri, const QPixmap& _image, const QString& _localPath, bool _isPreview);
        void imageMetaDownloaded(qint64 _seq, const Data::LinkMetadata& _meta);
        void imageDownloadingProgress(qint64 _seq, int64_t _bytesTotal, int64_t _bytesTransferred, int32_t _pctTransferred);
        void imageDownloadError(int64_t _seq, const QString& _rawUri);
        void externalFilePath(int64_t _seq, const QString& _path);

        void fileSharingError(qint64 _seq, const QString& _rawUri, qint32 _errorCode);

        void fileDownloaded(const QString& _requestedUri, const QString& _rawUri, const QString& _localPath);
        void fileSharingFileDownloaded(qint64 _seq, const Data::FileSharingDownloadResult& _result);
        void fileSharingFileDownloading(qint64 _seq, const QString& _rawUri, qint64 _bytesTransferred, qint64 _bytesTotal);

        void fileSharingFileMetainfoDownloaded(qint64 _seq, const Utils::FileSharingId _fsId, const Data::FileSharingMeta& _meta);
        void fileSharingFileMetainfoDownloadError(qint64 _seq, const Utils::FileSharingId _fsId, qint32 _errorCode);
        void fileSharingPreviewMetainfoDownloaded(qint64 _seq, const QString& _miniPreviewUri, const QString& _fullPreviewUri);

        void antivirusCheckResult(const Utils::FileSharingId& _fileHash, core::antivirus::check::result _result);

        void fileSharingLocalCopyCheckCompleted(qint64 _seq, bool _success, const QString& _localPath);

        void fileSharingUploadingProgress(const QString& _uploadingId, qint64 _bytesTransferred);
        void fileSharingUploadingResult(const QString& _seq, bool _success, const QString& localPath, const QString& _uri, int _contentType, qint64 _size, qint64 _lastModified, bool _isFileTooBig);

        void linkMetainfoMetaDownloaded(int64_t _seq, bool _success, const Data::LinkMetadata& _meta);
        void linkMetainfoImageDownloaded(int64_t _seq, bool _success, const QPixmap& _image);
        void linkMetainfoFaviconDownloaded(int64_t _seq, bool _success, const QPixmap& _image);

        void speechToText(qint64 _seq, int _error, const QString& _text, int _comeback);

        void chatsHome(const QVector<Data::ChatInfo>&, const QString&, bool, bool);
        void chatsHomeError(int);

        void recvPermitDeny(bool);

        void updateProfile(int);
        void getUserProxy();

        void setChatRoleResult(int);
        void blockMemberResult(int);
        void inviteCancelResult(int);
        void pendingListResult(int);

        void phoneInfoResult(qint64, const Data::PhoneInfo&);

        void appConfig();
        void appConfigChanged();

        void mailStatus(const QString&, unsigned, bool);
        void newMail(const QString&, const QString&, const QString&, const QString&);
        void mrimKey(qint64, const QString&);

        void chatHeads(const Data::ChatHeads&);

        void mentionsSuggestResults(const int64_t _seq, const Logic::MentionsSuggests& _suggests);
        void memoryUsageReportReady(const Memory_Stats::Response& _response,
                                    Memory_Stats::RequestId _req_id);

        void updateReady();
        void upToDate(int64_t _seq, bool _isNetworkError);

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

        void userLastSeen(const int64_t _seq, const std::map<QString, Data::LastSeen>& _lastseens);

        void nickCheckSetResult(qint64 _seq, int _result);

        void groupNickCheckSetResult(qint64 _seq, int _result);

        void setPrivacyParamsResult(const int64_t _seq, const bool _success);

        void commonChatsGetResult(const int64_t _seq, const std::vector<Data::CommonChatInfo>& _chats);

        void pollLoaded(const Data::PollData& _poll);
        void pollUpdate(const Data::PollData& _poll);
        void voteResult(const int64_t _seq, const Data::PollData& _poll, bool _success);
        void revokeVoteResult(const int64_t _seq, const QString& _pollId, bool _success);
        void stopPollResult(const int64_t _seq, const QString& _pollId, bool _success);

        void modChatParamsResult(int _error);
        void modChatAboutResult(const int64_t _seq, int _error);

        void suggestGroupNickResult(const QString& _nick);

        void sessionStarted(const QString& _aimId);
        void externalUrlConfigUpdated(int _error);
        void subscribeResult(const int64_t _seq, int _error, int _resubscribeIn);

        void sessionFinished(QPrivateSignal);

        void botCallbackAnswer(const int64_t _seq, const QString& _reqId, const QString& text, const QString& _url, bool _showAlert);

        void activeSessionsList(const std::vector<Data::SessionInfo>& _sessions);
        void sessionResetResult(const QString& _hash, bool _success);

        void createConferenceResult(int64_t _seq, int _error, bool _isWebinar, const QString& _url, int64_t _expires, QPrivateSignal) const;

        void callLog(const Data::CallInfoVec& _calls);
        void newCall(const Data::CallInfo& _call);
        void callRemoveMessages(const QString& _aimid, const std::vector<qint64> _messages);
        void callDelUpTo(const QString& _aimid, qint64 _del_up_to);
        void callRemoveContact(const QString& _contact);

        void reactions(const QString& _contact, const std::vector<Data::Reactions> _reactions);
        void reactionsListResult(int64_t _seq, const Data::ReactionsPage& _page, bool _success);
        void reactionAddResult(int64_t _seq, const Data::Reactions& _reactions, bool _success);
        void reactionRemoveResult(int64_t _seq, bool _success);

        void updateStatus(const QString& _aimId, const Statuses::Status& _status);

        void chatJoinResultBlocked(const QString& _stamp);
        void callRoomInfo(const QString& _roomId, int64_t _membersCount, bool _failed);
        void inviterBlacklistSearchResults(const Data::SearchResultsV& _contacts, const QString& _cursor, bool _resetPage, QPrivateSignal) const;
        void inviterBlacklistSearchError(QPrivateSignal) const;
        void inviterBlacklistGetResults(const std::vector<QString>& _contacts, const QString& _cursor, int _error, QPrivateSignal) const;
        void inviterBlacklistRemoveResult(const std::vector<QString>& _contacts, bool _success, QPrivateSignal) const;
        void inviterBlacklistAddResult(const std::vector<QString>& _contacts, bool _success, QPrivateSignal) const;

        void getEmojiResult(int64_t _seq, const QImage& _emoji);
        void getEmojiFailed(int64_t _seq, bool _isNetworkError);

        void suggestNotifyUser(const QString& _contact, const QString& _sms_notify_context);

        void unreadThreadCount(int _unreadThreads, int _unreadMentionsInThreads, QPrivateSignal) const;
        void threadUpdates(const Data::ThreadUpdates& _updates, QPrivateSignal) const;
        void messageThreadAdded(int64_t _seq, const Data::MessageParentTopic& _parentTopic, const QString& _threadId, int _error, QPrivateSignal) const;
        void threadInfoLoaded(int64_t _seq, const Data::ThreadInfo& _threadInfo, QPrivateSignal) const;
        void threadsFeedGetResult(const std::vector<Data::ThreadFeedItemData>& _items, const QString& _cursor, bool _resetPage) const;
        void subscriptionChanged(bool _subscription) const;
        void threadAutosubscribe(int _error) const;

        void authParamsChanged(const Data::AuthParams&);

        void draft(const QString& _contact, const Data::Draft& _draft);

        void taskCreated(qint64 _seq, int _error);
        void updateTask(const Data::TaskData& _task);
        void tasksLoaded(const QVector<Data::TaskData>& _tasks);
        void appUnavailable(const QString&);

    public Q_SLOTS:
        void received(const QString&, const qint64, core::icollection*);

    public:
        core_dispatcher();
        virtual ~core_dispatcher();

        core::icollection* create_collection() const;
        qint64 post_message_to_core(std::string_view _message, core::icollection* _collection, const QObject* _object = nullptr, message_processed_callback _callback = nullptr);

        qint64 post_stats_to_core(core::stats::stats_event_names _eventName, const core::stats::event_props_type& _props = {});
        qint64 post_im_stats_to_core(core::stats::im_stat_event_names _eventName, const core::stats::event_props_type& _props = {});

        bool get_enabled_stats_post() const;

#ifndef STRIP_VOIP
        voip_proxy::VoipController& getVoipController();
#endif

        qint64 getFileSharingPreviewSize(const QString& _url, const int32_t _originalSize = -1);
        qint64 downloadFileSharingMetainfo(const Utils::FileSharingId& _fsId);
        qint64 downloadSharedFile(const QString& _contactAimid, const QString& _url, bool _forceRequestMetainfo, const QString& _fileName = QString(), bool _raise_priority = false);

        qint64 abortSharedFileDownloading(const QString& _url, std::optional<qint64> _seq);
        qint64 abortSharedFileDownloading(const QString& _url, qint64 _seq)
        {
            return abortSharedFileDownloading(_url, _seq > 0 ? std::make_optional(_seq) : std::nullopt);
        }

        qint64 cancelLoaderTask(const QString& _url, std::optional<qint64> _seq);
        qint64 cancelLoaderTask(const QString& _url, qint64 _seq)
        {
            if (_seq > 0)
                return cancelLoaderTask(_url,  std::make_optional(_seq));

            return -1;
        }

        qint64 uploadSharedFile(const QString& _contact, const QString& _localPath, const Data::QuotesVec& _quotes = {}, const Data::FString& _description = QString(), const Data::MentionMap& _mentions = {}, bool _keepExif = false);
        qint64 uploadSharedFile(const QString& _contact, const QByteArray& _array, QStringView _ext, const Data::QuotesVec& _quotes = {});
        qint64 uploadSharedFile(const QString& _contact, const QByteArray& _array, QStringView _ext, const Data::QuotesVec& _quotes, const Data::FString& _description, const Data::MentionMap& _mentions);
        qint64 uploadSharedFile(const QString& _contact, const QString& _localPath, const Data::QuotesVec& _quotes, std::optional<std::chrono::seconds> _duration);
        qint64 abortSharedFileUploading(const QString& _contact, const QString& _localPath, const QString& _uploadingProcessId);

        qint64 getSticker(const qint32 _setId, const qint32 _stickerId, const core::sticker_size _size);
        qint64 getSticker(const Utils::FileSharingId& _fsId, const core::sticker_size _size);
        qint64 getStickerCancel(const std::vector<Utils::FileSharingId>& _fsIds, core::sticker_size _size);
        qint64 getSetIconBig(const qint32 _setId);
        qint64 cleanSetIconBig(const qint32 _setId);
        qint64 getStickersPackInfo(const qint32 _setId, const QString& _storeId, const Utils::FileSharingId& _fileId);
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


        int64_t getExternalFilePath(const QString& _url);

        enum class LoadPreview
        {
            No,
            Yes
        };

        int64_t downloadLinkMetainfo(const QString& _uri,
            LoadPreview _loadPreview = LoadPreview::Yes,
            const int32_t _previewWidth = 0,
            const int32_t _previewHeight = 0,
            const QString& _logStr = QString());

        qint64 deleteMessages(const QString& _contactAimId, const std::vector<DeleteMessageInfo>& _messages);
        qint64 deleteMessagesFrom(const QString& _contactAimId, const int64_t _fromId);
        qint64 eraseHistory(const QString& _contactAimId);

        bool isImCreated() const;

        qint64 raiseDownloadPriority(const QString &_contactAimid, int64_t _procId);

        qint64 contactSwitched(const QString &_contactAimid);

        void sendStickerToContact(const QString& _contact, const Utils::FileSharingId& _stickerId);
        void sendMessageToContact(const QString& _contact, const Data::FString& _text, const Data::QuotesVec& _quotes = {}, const Data::MentionMap& _mentions = {});
        void sendMessage(const Data::MessageBuddy& _buddy, const SendMessageParams& _params = SendMessageParams());
        void updateMessage(const Data::MessageBuddy& _buddy);

        struct MessageData
        {
            Data::FString text_;
            Data::QuotesVec quotes_;
            Data::MentionMap mentions_;
            Data::Poll poll_;
            Data::Task task_;
        };

        void sendMessageToContact(const QString& _contact, MessageData _data);

        void updateMessageToContact(const QString& _contact, int64_t _messageId, const Data::FString& _text);
        void sendSmartreplyToContact(const QString& _contact, const Data::SmartreplySuggest& _smartreply, const Data::QuotesVec& _quotes = {});

        int64_t pttToText(const QString& _pttLink, const QString& _locale);

        void setUserState(const core::profile_state state);
        void invokeStateAway();
        void invokePreviousState();

        void onArchiveMessages(Ui::MessagesBuddiesOpt _type, const int64_t _seq, core::coll_helper _params);

        void getCodeByPhoneCall(const QString& _ivr_url);

        qint64 getDialogGallery(const QString& _aimId, const QStringList& _types, int64_t _after_msg, int64_t _after_seq, const int _pageSize, const bool _download_holes);
        qint64 getDialogGalleryByMsg(const QString& _aimId, const QStringList& _types, int64_t _id);
        qint64 requestDialogGalleryState(const QString& _aimId);
        qint64 getDialogGalleryIndex(const QString& _aimId, const QStringList& _types, qint64 _msg, qint64 _seq);

        void postUiActivity();

        void setLocalPIN(const QString& _password);

        qint64 getUserInfo(const QString& _aimid);

        qint64 getUserLastSeen(const std::vector<QString>& _contacts);

        qint64 subscribeStatus(const std::vector<QString>& _contacts);
        qint64 unsubscribeStatus(const std::vector<QString>& _contacts);

        qint64 subscribeThread(const std::vector<QString>& _threads);
        qint64 unsubscribeThread(const std::vector<QString>& _threads);

        qint64 addToInvitersBlacklist(const std::vector<QString>& _contacts);
        qint64 removeFromInvitersBlacklist(const std::vector<QString>& _contacts);

        qint64 subscribeCallRoomInfo(const QString& _roomId);
        qint64 unsubscribeCallRoomInfo(const QString& _roomId);

        qint64 subscribeTask(const QString& _taskId);
        qint64 unsubscribeTask(const QString& _taskId);

        qint64 subscribeFileSharingAntivirus(const Utils::FileSharingId& _fsId);
        qint64 unsubscribeFileSharingAntivirus(const Utils::FileSharingId& _fsId);

        qint64 subscribeTasksCounter();
        qint64 unsubscribeTasksCounter();

        qint64 subscribeMailsCounter();
        qint64 unsubscribeMailsCounter();

        qint64 localPINEntered(const QString& _password);

        void disableLocalPIN();

        qint64 getIdInfo(const QString& _id);

        qint64 checkNickname(const QString& _nick, bool _set_nick);

        qint64 checkGroupNickname(const QString& _nick);

        void updateRecentAvatarsSize();

        void setInstallBetaUpdates(bool _enabled);

        bool isInstallBetaUpdatesEnabled() const;

        int64_t getPoll(const QString& _pollId);
        int64_t voteInPoll(const QString& _pollId, const QString& _answerId);
        int64_t revokeVote(const QString& _pollId);
        int64_t stopPoll(const QString& _pollId);

        int64_t createTask(Data::Task _task);
        int64_t editTask(const Data::TaskChange& _task);
        int64_t requestInitialTasks();
        int64_t updateTaskLastUsedTime(const QString& _taskId);

        int64_t getBotCallbackAnswer(const QString& _chatId, const QString& _callbackData, qint64 _msgId);
        qint64 pinContact(const QString& _aimId, bool _enable);

        int64_t getChatInfo(const QString& _chatId, const QString& _stamp = QString(), int _membersLimit = 5);
        qint64 loadChatMembersInfo(const QString& _aimId, const std::vector<QString>& _members);

        qint64 requestSessionsList();

        qint64 cancelGalleryHolesDownloading(const QString& _aimid);
        int64_t createConference(ConferenceType _type, const std::vector<QString>& _participants = {}, bool _callParticipants = false);

        qint64 resolveChatPendings(const QString& _chatAimId, const std::vector<QString>& _pendings, bool _approve);

        void getReactions(const QString& _chatId, std::vector<int64_t> _msgIds, bool _firstLoad = false);
        int64_t addReaction(int64_t _msgId, const QString& _chatId, const QString& _reaction);
        int64_t removeReaction(int64_t _msgId, const QString& _chatId);
        int64_t getReactionsPage(int64_t _msgId, QStringView _chatId, QStringView _reaction, QStringView _newerThan, QStringView _olderThan, int64_t _limit);

        enum class ServerEmojiSize
        {
            _20 = 20,
            _32 = 32,
            _40 = 40,
            _48 = 48,
            _64 = 64,
            _96 = 96,
            _160 = 160
        };

        int64_t getEmojiImage(const QString& _code, ServerEmojiSize _size);

        int64_t setStatus(const QString& _emojiCode, int64_t _duration, const QString& _description = QString());

        int64_t addThread(const Data::MessageParentTopic& _topic);
        int64_t getThreadsFeed(const QString& _cursor);
        int64_t updateThreadSubscription(const QString& _threadId, bool _subscribe);

        int64_t setDraft(const Data::MessageBuddy& _msg, int64_t _timestamp, bool _synch = false);
        int64_t getDraft(const QString& _contact);

        int64_t setDialogOpen(const QString& _contact, bool _open);

        struct GetHistoryParams
        {
            int early_ = 0;
            int later_ = 0;
            int64_t from_ = 0;
            bool needPrefetch_ = false;
            bool firstRequest_ = false;
            bool afterSearch_ = false;
        };

        int64_t getHistory(const QString& _contact, const GetHistoryParams& _params);

        int64_t groupSubscribe(const QString& _chatId, const QString& _stamp = QString());
        int64_t cancelGroupSubscription(const QString& _chatId, const QString& _stamp = QString());

        enum class ReadAll
        {
            No,
            Yes
        };

        void setLastRead(const QString& _contact, int64_t _lastMsgId, ReadAll _readAll = ReadAll::No);

        void addChatsThread(const QString& _contact, int64_t _msgId, const QString& _threadId);
        void removeChatsThread(const QString& _contact, int64_t _msgId, const QString& _threadId);

        // messages
        void onNeedLogin(const int64_t _seq, core::coll_helper _params);
        void onNeedExternalUserAgreement(const int64_t _seq, core::coll_helper _params);
        void onImCreated(const int64_t _seq, core::coll_helper _params);
        void onOAuthRequired(const int64_t _seq, core::coll_helper _params);
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
        void onMessagesReceivedPatched(const int64_t _seq, core::coll_helper _params);
        void onMessagesReceivedServer(const int64_t _seq, core::coll_helper _params);
        void onMessagesReceivedSearch(const int64_t _seq, core::coll_helper _params);
        void onMessagesClear(const int64_t _seq, core::coll_helper _params);
        void onMessagesEmpty(const int64_t _seq, core::coll_helper _params);
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

        void onThreadsSearchLocalResults(const int64_t _seq, core::coll_helper _params);
        void onThreadsSearchLocalEmptyResult(const int64_t _seq, core::coll_helper _params);
        void onThreadsSearchServerResults(const int64_t _seq, core::coll_helper _params);
        void onThreadsSearchResult(const int64_t _seq, core::coll_helper _params, const bool _fromLocalSearch);

        void onDialogsSearchPatternHistory(const int64_t _seq, core::coll_helper _params);

        void onContactsSearchLocalResults(const int64_t _seq, core::coll_helper _params);
        void onContactsSearchServerResults(const int64_t _seq, core::coll_helper _params);

        void onSyncAddressBook(const int64_t _seq, core::coll_helper _params);

#ifndef STRIP_VOIP
        void onVoipSignal(const int64_t _seq, core::coll_helper _params);
#endif
        void onActiveDialogsAreEmpty(const int64_t _seq, core::coll_helper _params);
        void onActiveDialogsHide(const int64_t _seq, core::coll_helper _params);
        void onActiveDialogsSent(const int64_t _seq, core::coll_helper _params);
        void onStickersMetaGetResult(const int64_t _seq, core::coll_helper _params);
        void onStickerGetFsByIdResult(const int64_t _seq, core::coll_helper _params);

        void onStickersStickerGetResult(const int64_t _seq, core::coll_helper _params);
        void onStickersGetSetBigIconResult(const int64_t _seq, core::coll_helper _params);
        void onStickersSetIcon(const int64_t _seq, core::coll_helper _params);
        void onStickersPackInfo(const int64_t _seq, core::coll_helper _params);
        void onStickersStore(const int64_t _seq, core::coll_helper _params);
        void onStickersStoreSearch(const int64_t _seq, core::coll_helper _params);
        void onStickersSuggests(const int64_t _seq, core::coll_helper _params);
        void onStickersRequestedSuggests(const int64_t _seq, core::coll_helper _params);

        void onSmartreplyInstantSuggests(const int64_t _seq, core::coll_helper _params);
        void onSmartreplyRequestedSuggests(const int64_t _seq, core::coll_helper _params);

        void onThemeSettings(const int64_t _seq, core::coll_helper _params);
        void onThemesWallpaperGetResult(const int64_t _seq, core::coll_helper _params);
        void onThemesWallpaperPreviewGetResult(const int64_t _seq, core::coll_helper _params);

        void onChatsInfoGetResult(const int64_t _seq, core::coll_helper _params);
        void onChatsInfoGetFailed(const int64_t _seq, core::coll_helper _params);
        void onMemberAddFailed(const int64_t _seq, core::coll_helper _params);
        void onChatsMemberInfoResult(const int64_t _seq, core::coll_helper _params);
        void onChatsMembersGetResult(const int64_t _seq, core::coll_helper _params);
        void onChatsMembersGetCached(const int64_t _seq, core::coll_helper _params);
        void onChatsMembersSearchResult(const int64_t _seq, core::coll_helper _params);
        void onChatsContactsResult(const int64_t _seq, core::coll_helper _params);

        void fileSharingErrorResult(const int64_t _seq, core::coll_helper _params);
        void fileSharingDownloadProgress(const int64_t _seq, core::coll_helper _params);
        void fileSharingGetPreviewSizeResult(const int64_t _seq, core::coll_helper _params);
        void fileSharingMetainfoResult(const int64_t _seq, core::coll_helper _params);
        void fileSharingMetainfoError(const int64_t _seq, core::coll_helper _params);
        void fileSharingCheckExistsResult(const int64_t _seq, core::coll_helper _params);
        void fileSharingDownloadResult(const int64_t _seq, core::coll_helper _params);

        void imageDownloadProgress(const int64_t _seq, core::coll_helper _params);
        void imageDownloadResult(const int64_t _seq, core::coll_helper _params);
        void imageDownloadResultMeta(const int64_t _seq, core::coll_helper _params);
        void externalFilePathResult(const int64_t _seq, core::coll_helper _params);
        void fileUploadingProgress(const int64_t _seq, core::coll_helper _params);
        void fileUploadingResult(const int64_t _seq, core::coll_helper _params);
        void linkMetainfoDownloadResultMeta(const int64_t _seq, core::coll_helper _params);
        void linkMetainfoDownloadResultImage(const int64_t _seq, core::coll_helper _params);
        void linkMetainfoDownloadResultFavicon(const int64_t _seq, core::coll_helper _params);
        void onFilesSpeechToTextResult(const int64_t _seq, core::coll_helper _params);
        void onContactsRemoveResult(const int64_t _seq, core::coll_helper _params);
        void onAppConfig(const int64_t _seq, core::coll_helper _params);
        void onAppConfigChanged(const int64_t _seq, core::coll_helper _params);
        void onMyInfo(const int64_t _seq, core::coll_helper _params);
        void onFeedbackSent(const int64_t _seq, core::coll_helper _params);
        void onMessagesReceivedSenders(const int64_t _seq, core::coll_helper _params);
        void onTyping(const int64_t _seq, core::coll_helper _params);
        void onTypingStop(const int64_t _seq, core::coll_helper _params);
        void onContactsGetIgnoreResult(const int64_t _seq, core::coll_helper _params);
        void onLoginResultAttachPhone(const int64_t _seq, core::coll_helper _params);
        void onUpdateProfileResult(const int64_t _seq, core::coll_helper _params);
        void onChatsHomeGetResult(const int64_t _seq, core::coll_helper _params);
        void onChatsHomeGetFailed(const int64_t _seq, core::coll_helper _params);
        void onProxyChange(const int64_t _seq, core::coll_helper _params);
        void onOpenCreatedChat(const int64_t _seq, core::coll_helper _params);
        void onLoginNewUser(const int64_t _seq, core::coll_helper _params);
        void onSetAvatarResult(const int64_t _seq, core::coll_helper _params);
        void onChatsRoleSetResult(const int64_t _seq, core::coll_helper _params);
        void onChatsBlockResult(const int64_t _seq, core::coll_helper _params);
        void onCancelInviteResult(const int64_t _seq, core::coll_helper _params);
        void onChatsPendingResolveResult(const int64_t _seq, core::coll_helper _params);
        void onPhoneinfoResult(const int64_t _seq, core::coll_helper _params);
        void onContactRemovedFromIgnore(const int64_t _seq, core::coll_helper _params);
        void onFriendlyUpdate(const int64_t _seq, core::coll_helper _params);
        void onMailStatus(const int64_t _seq, core::coll_helper _params);
        void onMailNew(const int64_t _seq, core::coll_helper _params);
        void getMrimKeyResult(const int64_t _seq, core::coll_helper _params);
        void onMentionsMeReceived(const int64_t _seq, core::coll_helper _params);
        void onMentionsSuggestResults(const int64_t _seq, core::coll_helper _params);
        void onChatHeads(const int64_t _seq, core::coll_helper _params);
        void onUpdateReady(const int64_t _seq, core::coll_helper _params);
        void onUpToDate(const int64_t _seq, core::coll_helper _params);
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

        ConnectionState getConnectionState() const;

        void onRequestMemoryConsumption(const int64_t _seq, core::coll_helper _params);
        void onMemUsageReportReady(const int64_t _seq, core::coll_helper _params);
        void onGuiComponentsMemoryReportsReady(const Memory_Stats::RequestHandle& _req_handle,
                                               const Memory_Stats::ReportsList& _reports_list);

        void onGetUserInfoResult(const int64_t _seq, core::coll_helper _params);
        void onGetUserLastSeenResult(const int64_t _seq, core::coll_helper _params);

        void onSetPrivacySettingsResult(const int64_t _seq, core::coll_helper _params);
        void onGetCommonChatsResult(const int64_t _seq, core::coll_helper _params);
        void onNickCheckSetResult(const int64_t _seq, core::coll_helper _params);
        void onGroupNickCheckSetResult(const int64_t _seq, core::coll_helper _params);
        void onOmicronUpdateData(const int64_t _seq, core::coll_helper _params);
        void onFilesharingUploadAborted(const int64_t _seq, core::coll_helper _params);
        void onPollGetResult(const int64_t _seq, core::coll_helper _params);
        void onPollVoteResult(const int64_t _seq, core::coll_helper _params);
        void onPollRevokeResult(const int64_t _seq, core::coll_helper _params);
        void onPollStopResult(const int64_t _seq, core::coll_helper _params);
        void onPollUpdate(const int64_t _seq, core::coll_helper _params);
        void onSessionStarted(const int64_t _seq, core::coll_helper _params);
        void onSessionFinished(const int64_t _seq, core::coll_helper _params);

        void onAntivirusCheckResult(const int64_t _seq, core::coll_helper _params);

        void onModChatParamsResult(const int64_t _seq, core::coll_helper _params);
        void onModChatAboutResult(const int64_t _seq, core::coll_helper _params);

        void onSuggestGroupNickResult(const int64_t _seq, core::coll_helper _params);

        void onExternalUrlConfigError(const int64_t _seq, core::coll_helper _params);
        void onExternalUrlConfigHosts(const int64_t _seq, core::coll_helper _params);

        void onGroupSubscribeResult(const int64_t _seq, core::coll_helper _params);

        void onBotCallbackAnswer(const int64_t _seq, core::coll_helper _params);

        void onGetSessionsResult(const int64_t _seq, core::coll_helper _params);
        void onResetSessionResult(const int64_t _seq, core::coll_helper _params);

        void onCreateConferenceResult(const int64_t _seq, core::coll_helper _params);

        void onCallLog(const int64_t _seq, core::coll_helper _params);
        void onNewCall(const int64_t _seq, core::coll_helper _params);
        void onCallRemoveMessages(const int64_t _seq, core::coll_helper _params);
        void onCallDelUpTo(const int64_t _seq, core::coll_helper _params);
        void onCallRemoveContact(const int64_t _seq, core::coll_helper _params);

        void onReactions(const int64_t _seq, core::coll_helper _params);
        void onReactionsListResult(const int64_t _seq, core::coll_helper _params);
        void onReactionAddResult(const int64_t _seq, core::coll_helper _params);
        void onReactionRemoveResult(const int64_t _seq, core::coll_helper _params);

        void onStatus(const int64_t _seq, core::coll_helper _params);

        void onCallRoomInfo(const int64_t _seq, core::coll_helper _params);
        void onChatJoinResult(const int64_t _seq, core::coll_helper _params);

        void onAddMembersResult(const int64_t _seq, core::coll_helper _params);
        void onRemoveMembersResult(const int64_t _seq, core::coll_helper _params);


        void onInviterBlacklistSearchResult(const int64_t _seq, core::coll_helper _params);
        void onInviterBlacklistedCLContacts(const int64_t _seq, core::coll_helper _params);
        void onInviterBlacklistRemoveResult(const int64_t _seq, core::coll_helper _params);
        void onInviterBlacklistAddResult(const int64_t _seq, core::coll_helper _params);

        void onGetEmojiResult(const int64_t _seq, core::coll_helper _params);

        void onSuggestToNotifyUser(const int64_t _seq, core::coll_helper _params);
        void onAuthData(const int64_t _seq, core::coll_helper _params);

        void onThreadsUpdate(const int64_t _seq, core::coll_helper _params);
        void onUnreadThreadsCount(const int64_t _seq, core::coll_helper _params);
        void onAddThreadResult(const int64_t _seq, core::coll_helper _params);
        void onSubscripionChanged(const int64_t _seq, core::coll_helper _params);
        void onGetThreadResult(const int64_t _seq, core::coll_helper _params);
        void onThreadsFeedGetResult(const int64_t _seq, core::coll_helper _params);
        void onThreadsSubscribersGetResult(const int64_t _seq, core::coll_helper _params);
        void onThreadsSubscribersSearchResult(const int64_t _seq, core::coll_helper _params);
        void onThreadAutosubscribeResult(const int64_t _seq, core::coll_helper _params);

        void onDraft(const int64_t _seq, core::coll_helper _params);

        void onTaskCreateResult(const int64_t _seq, core::coll_helper _params);
        void onTaskUpdate(const int64_t _seq, core::coll_helper _params);
        void onTaskEditResult(const int64_t _seq, core::coll_helper _params);
        void onTasksLoaded(const int64_t _seq, core::coll_helper _params);
        void onUnreadTasksCounter(const int64_t _seq, core::coll_helper _params);

        void onUnreadMailsCounter(const int64_t _seq, core::coll_helper _params);

        void onMiniappUnavailable(const int64_t _seq, core::coll_helper _params);

#ifdef IM_AUTO_TESTING
        std::unique_ptr<WebServer> server;
#endif

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

        int64_t groupSubscribeImpl(std::string_view _message, const QString& _chatId, const QString& _stamp);

    private:

        std::unordered_map<std::string, message_function> messages_map_;

        core::iconnector* coreConnector_;
        core::icore_interface* coreFace_;
#ifndef STRIP_VOIP
        voip_proxy::VoipController voipController_;
#endif
        core::iconnector* guiConnector_;

        std::unordered_map<int64_t, callback_info> callbacks_;

        qint64 lastTimeCallbacksCleanedUp_;

        bool isImCreated_;

        bool userStateGoneAway_;
        bool installBetaUpdates_;

        ConnectionState connectionState_;

        QTimer* typingCheckTimer_;
    };

    core_dispatcher* GetDispatcher();
    void createDispatcher();
    void destroyDispatcher();
}
