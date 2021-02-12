#include "stdafx.h"
#include "RecentsModel.h"
#include "UnknownsModel.h"
#include "ContactListModel.h"
#include "../../cache/avatars/AvatarStorage.h"
#include "../../main_window/MainWindow.h"

#include "../../core_dispatcher.h"
#include "../../utils/gui_coll_helper.h"
#include "../MainPage.h"
#include "../MainWindow.h"
#include "../containers/FriendlyContainer.h"
#include "../containers/StatusContainer.h"
#include "../../utils/InterConnector.h"
#include "../../utils/stat_utils.h"
#include "../../gui_settings.h"
#include "FavoritesUtils.h"


namespace
{
    auto isEqualDlgState = [](const auto& _aimId)
    {
        return [&_aimId](const auto& dlgState) { return _aimId == dlgState.AimId_; };
    };
}


namespace Logic
{
    std::unique_ptr<RecentsModel> g_recents_model;

    RecentsModel::RecentsModel(QObject *parent)
        : CustomAbstractListModel(parent)
        , PinnedCount_(0)
        , PinnedVisible_(true)
        , UnimportantCount_(0)
        , UnimportantVisible_(false)
        , needToAddFavorites_(true)
        , favoritesLastMessageSeq_(-1)
        , refreshTimer_(nullptr)
    {
        PinnedVisible_ = Ui::get_gui_settings()->get_value(settings_pinned_chats_visible, true);
        UnimportantVisible_ = Ui::get_gui_settings()->get_value(settings_unimportant_visible, true);

        connect(GetAvatarStorage(), &Logic::AvatarStorage::avatarChanged, this, &RecentsModel::contactAvatarChanged);
        connect(Logic::GetStatusContainer(), &Logic::StatusContainer::statusChanged, this, [this](const QString& _aimid)
        {
            contactAvatarChanged(_aimid);
        });
        connect(Ui::GetDispatcher(), &Ui::core_dispatcher::activeDialogHide, this, &RecentsModel::activeDialogHide);
        connect(Ui::GetDispatcher(), &Ui::core_dispatcher::dlgStates, this, &RecentsModel::dlgStates);
        connect(Ui::GetDispatcher(), &Ui::core_dispatcher::messageBuddies, this, &RecentsModel::messageBuddies);
        connect(Ui::GetDispatcher(), &Ui::core_dispatcher::messagesEmpty, this, &RecentsModel::messagesEmpty);

        connect(Logic::getContactListModel(), &Logic::ContactListModel::contactChanged, this, &RecentsModel::contactChanged);
        connect(Logic::getContactListModel(), &Logic::ContactListModel::selectedContactChanged, this, &RecentsModel::contactChanged);
        connect(Logic::getContactListModel(), &Logic::ContactListModel::selectedContactChanged, this, &RecentsModel::selectedContactChanged);
        connect(Logic::getContactListModel(), &Logic::ContactListModel::contact_removed, this, &RecentsModel::contactRemoved);

        connect(&Utils::InterConnector::instance(), &Utils::InterConnector::compactModeChanged, this, &RecentsModel::refresh);
        connect(&Utils::InterConnector::instance(), &Utils::InterConnector::hideHeads, this, &RecentsModel::refresh);
        connect(&Utils::InterConnector::instance(), &Utils::InterConnector::showHeads, this, &RecentsModel::refresh);

        connect(Ui::GetDispatcher(), &Ui::core_dispatcher::typingStatus, this, &RecentsModel::typingStatus);

        scheduleRefreshTimer();
    }

    int RecentsModel::rowCount(const QModelIndex &) const
    {
        if (Dialogs_.empty())
            return 0;

        auto count = (int)Dialogs_.size() + getVisibleServiceItems();
        if (!PinnedVisible_)
            count -= PinnedCount_;
        if (!UnimportantVisible_)
            count -= UnimportantCount_;

        return count;
    }

    QVariant RecentsModel::data(const QModelIndex &i, int r) const
    {
        if (!i.isValid() || (r != Qt::DisplayRole && !Testing::isAccessibleRole(r)))
            return QVariant();

        int cur = i.row();
        if (cur >= rowCount(i))
            return QVariant();

        if (cur == getPinnedHeaderIndex())
        {
            const static auto st = []() {
                Data::DlgState s;
                s.AimId_ = qsl("~pinned~");
                s.SetText(QT_TRANSLATE_NOOP("contact_list", "PINNED"));
                return s;
            }();

            return QVariant::fromValue(st);
        }
        else if (cur == getUnimportantHeaderIndex())
        {
            const static auto st = []() {
                Data::DlgState s;
                s.AimId_ = qsl("~unimportant~");
                s.SetText(QT_TRANSLATE_NOOP("contact_list", "UNIMPORTANT"));
                return s;
            }();

            return QVariant::fromValue(st);
        }
        else if (cur == getRecentsHeaderIndex())
        {
            const static auto st = []() {
                Data::DlgState s;
                s.AimId_ = qsl("~recents~");
                s.SetText(QT_TRANSLATE_NOOP("contact_list", "RECENTS"));
                return s;
            }();
            return QVariant::fromValue(st);
        }

        cur = correctIndex(cur);

        if (cur >= (int) Dialogs_.size() || cur < 0)
            return QVariant();

        const Data::DlgState& cont = Dialogs_[cur];

        if (Testing::isAccessibleRole(r))
            return QString(u"AS Recents " % cont.AimId_);

        return QVariant::fromValue(cont);
    }

    void RecentsModel::contactChanged(const QString& _aimid)
    {
        if (const auto it = std::as_const(Indexes_).find(_aimid); it != std::as_const(Indexes_).end())
        {
            const auto dialogIdx = it->second;
            if (dialogIdx < Dialogs_.size())
                Q_EMIT dlgStateChanged(Dialogs_[dialogIdx]);

            const auto modelIdx = index(getVisibleIndex(int(dialogIdx)));
            Q_EMIT dataChanged(modelIdx, modelIdx);
        }
    }

    void RecentsModel::selectedContactChanged(const QString& _new, const QString& _prev)
    {
        const auto updateIndex = [this](const QString& _aimId)
        {
            if (const auto it = std::as_const(Indexes_).find(_aimId); it != std::as_const(Indexes_).end())
            {
                const auto modelIdx = index(getVisibleIndex(int(it->second)));
                Q_EMIT dataChanged(modelIdx, modelIdx);
            }
        };

        updateIndex(_new);
        updateIndex(_prev);
    }

    void RecentsModel::contactAvatarChanged(const QString& _aimid)
    {
        for (size_t i = 0; i < Dialogs_.size(); ++i)
        {
            const auto& dialog = Dialogs_[i];
            const bool needUpdate = dialog.AimId_ == _aimid ||
                std::any_of(dialog.heads_.begin(), dialog.heads_.end(), [&_aimid](const auto& _head) { return _head.first == _aimid; });

            if (needUpdate)
            {
                const auto idx = index(getVisibleIndex(int(i)));
                Q_EMIT dataChanged(idx, idx);
            }
        }
    }

    void RecentsModel::activeDialogHide(const QString& _aimId)
    {
        auto iter = std::find_if(Dialogs_.begin(), Dialogs_.end(), isEqualDlgState(_aimId));
        if (iter == Dialogs_.end())
            return;

        if (iter->PinnedTime_ != -1)
            --PinnedCount_;

        if (iter->UnimportantTime_ != -1)
            --UnimportantCount_;

        contactChanged(iter->AimId_);

        friendlyTexts_.erase(iter->AimId_);
        Dialogs_.erase(iter);

        makeIndexes();

        if (Logic::getContactListModel()->selectedContact() == _aimId)
            Logic::getContactListModel()->setCurrent(QString(), -1, true);
        if (Dialogs_.empty() && !Logic::getUnknownsModel()->itemsCount())
            Q_EMIT Utils::InterConnector::instance().showRecentsPlaceholder();

        refresh();

        Q_EMIT updated();
    }

    void RecentsModel::dlgStates(const QVector<Data::DlgState>& _states)
    {
        auto updatedItems = 0;

        const auto curSelected = Logic::getContactListModel()->selectedContact();
        const auto w = Utils::InterConnector::instance().getMainWindow();
        const auto mainPageOpened = w && w->isUIActive() && w->isMainPage();

        auto processDlgState = [this, &curSelected, &updatedItems, mainPageOpened](const auto& dlgState)
        {
            if (dlgState.noRecentsUpdate_)
            {
                auto iter = std::find_if(HiddenDialogs_.begin(), HiddenDialogs_.end(), isEqualDlgState(dlgState.AimId_));
                if (iter != HiddenDialogs_.end())
                {
                    auto &curDlgState = *iter;
                    if (curDlgState.YoursLastRead_ != dlgState.YoursLastRead_)
                        Q_EMIT readStateChanged(dlgState.AimId_);

                    curDlgState = dlgState;
                }
                else
                {
                    HiddenDialogs_.push_back(dlgState);
                }

                if (mainPageOpened && dlgState.AimId_ == curSelected)
                    sendLastRead(dlgState.AimId_);

                Q_EMIT updated();
                return;
            }

            auto iter = std::find_if(Dialogs_.begin(), Dialogs_.end(), isEqualDlgState(dlgState.AimId_));

            if (!dlgState.Chat_)
            {
                if (dlgState.isSuspicious_)
                {
                    if (iter != Dialogs_.end())
                    {
                        Dialogs_.erase(iter);
                        ++updatedItems;

                        if (dlgState.AimId_ == curSelected)
                            Q_EMIT Utils::InterConnector::instance().unknownsGoSeeThem();
                    }

                    return;
                }
            }


            Logic::getContactListModel()->setContactVisible(dlgState.AimId_, !dlgState.isStranger_);

            if (iter != Dialogs_.end())
            {
                ++updatedItems;

                auto &curDlgState = *iter;

                if (curDlgState.YoursLastRead_ != dlgState.YoursLastRead_)
                    Q_EMIT readStateChanged(dlgState.AimId_);

                if (curDlgState.isStranger_ && !dlgState.isStranger_)
                    Ui::GetDispatcher()->post_stats_to_core(core::stats::stats_event_names::chatscr_blockbar_event, { { "type", "disappeared" },{ "chat_type", Utils::chatTypeByAimId(dlgState.AimId_) } });

                const auto prevText = curDlgState.GetText();
                const auto prevMediaType = curDlgState.mediaType_;

                const auto prevLastId = curDlgState.LastMsgId_;
                const auto prevYoursLastRead = curDlgState.YoursLastRead_;
                const auto prevTime = curDlgState.Time_;

                bool fChanged = curDlgState.PinnedTime_ != dlgState.PinnedTime_;
                if (fChanged)
                {
                    if (dlgState.PinnedTime_ != -1 && curDlgState.PinnedTime_ == -1)
                        ++PinnedCount_;
                    else if (dlgState.PinnedTime_ == -1 && curDlgState.PinnedTime_ != -1)
                        --PinnedCount_;
                }

                bool uChanged = curDlgState.UnimportantTime_ != dlgState.UnimportantTime_;
                if (uChanged)
                {
                    if (dlgState.UnimportantTime_ != -1)
                        ++UnimportantCount_;
                    else
                        --UnimportantCount_;
                }
                const bool keepZeroUnread = curDlgState.YoursLastRead_ >= dlgState.YoursLastRead_ && curDlgState.UnreadCount_ == 0 && curDlgState.YoursLastRead_ == dlgState.LastMsgId_;
#ifdef DEBUG
                const auto prevState = curDlgState;
#endif // DEBUG
                curDlgState = dlgState;

                if (fChanged)
                    Q_EMIT favoriteChanged(dlgState.AimId_);

                if (uChanged)
                    Q_EMIT unimportantChanged(dlgState.AimId_);

                if (keepZeroUnread)
                {
                    curDlgState.UnreadCount_ = 0;
                    curDlgState.YoursLastRead_ = prevYoursLastRead;
                }

                auto keepPrevTime = false;
                if (curDlgState.HasLastMsgId())
                {
                    if (!curDlgState.HasText() && curDlgState.DelUpTo_ != curDlgState.LastMsgId_)
                    {
                        curDlgState.SetText(prevText);
                        curDlgState.mediaType_ = prevMediaType;
                    }

                    if (const auto it = friendlyTexts_.find(curDlgState.AimId_); it != friendlyTexts_.end())
                    {
                        if (it->second.msgId_ == curDlgState.LastMsgId_)
                        {
                            if (curDlgState.GetText() == prevText)
                                curDlgState.LastMessageFriendly_ = it->second.friendlyText_;
                        }
                        else if (it->second.msgId_ < curDlgState.LastMsgId_)
                        {
                            friendlyTexts_.erase(it);
                        }
                    }

                    keepPrevTime =
                        curDlgState.PinnedTime_ == dlgState.PinnedTime_ &&
                        curDlgState.UnimportantTime_ == dlgState.UnimportantTime_ &&
                        curDlgState.LastMsgId_ == prevLastId &&
                        curDlgState.UnreadCount_ == 0 &&
                        curDlgState.GetText() == prevText;
                }
                else
                {
                    keepPrevTime = true;
                }

                if (keepPrevTime)
                    curDlgState.Time_ = prevTime;
            }
            else if (!dlgState.GetText().isEmpty() || dlgState.PinnedTime_ != -1 || dlgState.UnimportantTime_ != -1)
            {
                ++updatedItems;

                if (dlgState.PinnedTime_ != -1)
                {
                    ++PinnedCount_;
                    Q_EMIT favoriteChanged(dlgState.AimId_);
                }

                if (dlgState.UnimportantTime_ != -1)
                {
                    ++UnimportantCount_;
                    Q_EMIT unimportantChanged(dlgState.AimId_);
                }

                auto newDlgState = dlgState;

                Dialogs_.push_back(newDlgState);

                if (Dialogs_.size() == 1)
                    Q_EMIT Utils::InterConnector::instance().hideRecentsPlaceholder();
            }

            if (mainPageOpened && dlgState.AimId_ == curSelected)
                sendLastRead(dlgState.AimId_);

            Q_EMIT dlgStateChanged(dlgState);
        };

        for (const auto& dlgState : _states)
            processDlgState(dlgState);

        if (!_states.empty() && !Favorites::pinnedOnStart())
        {
            Ui::GetDispatcher()->pinContact(Favorites::aimId(), true);
            Favorites::setPinnedOnStart(true);
            requestFavoritesLastMessage();
        }

        if (updatedItems > 0)
        {
            sortDialogs();
            Q_EMIT updated();
        }

        Q_EMIT dlgStatesHandled(_states);
    }

    void RecentsModel::messageBuddies(const Data::MessageBuddies& _buddies, const QString& _aimId, Ui::MessagesBuddiesOpt _type, bool _havePending, qint64 _seq, int64_t _lastMsgId)
    {
        if (_type != Ui::MessagesBuddiesOpt::DlgState)
            return;

        auto it = std::find_if(Dialogs_.begin(), Dialogs_.end(), isEqualDlgState(_aimId));
        if (it == Dialogs_.end() || !it->HasLastMsgId())
            return;

        for (const auto& buddy : _buddies)
        {
            const auto needUpdateItem =
                buddy->Id_ == it->LastMsgId_ &&
                !buddy->GetUpdatePatchVersion().is_empty() &&
                !it->LastMessageFriendly_.isEmpty();

            if (needUpdateItem)
            {
                it->LastMessageFriendly_.clear();
                friendlyTexts_.erase(_aimId);
                contactChanged(_aimId);
                break;
            }
        }
    }

    void RecentsModel::messagesEmpty(const QString& _aimId, qint64 _seq)
    {
        Q_UNUSED(_aimId)

        if (_seq != favoritesLastMessageSeq_)
            return;

        Ui::GetDispatcher()->sendMessageToContact(Favorites::aimId(), Favorites::firstMessageText());
    }

    void RecentsModel::unknownToRecents(const Data::DlgState& dlgState)
    {
        auto iter = std::find(Dialogs_.begin(), Dialogs_.end(), dlgState);
        if (iter == Dialogs_.end() && (!dlgState.GetText().isEmpty() || dlgState.PinnedTime_ != -1 || dlgState.UnimportantTime_ != -1))
        {
            if (dlgState.PinnedTime_ != -1)
            {
                ++PinnedCount_;
                Q_EMIT favoriteChanged(dlgState.AimId_);
            }

            if (dlgState.UnimportantTime_ != -1)
            {
                ++UnimportantCount_;
                Q_EMIT unimportantChanged(dlgState.AimId_);
            }

            Dialogs_.push_back(dlgState);
            sortDialogs();
        }
    }

    void RecentsModel::sortDialogs()
    {
        std::stable_sort(Dialogs_.begin(), Dialogs_.end(), [](const Data::DlgState& first, const Data::DlgState& second)
        {
            if (first.PinnedTime_ == -1 && second.PinnedTime_ == -1)
            {
                if (first.UnimportantTime_ == -1 && second.UnimportantTime_ == -1)
                    return first.Time_ > second.Time_;

                if (first.UnimportantTime_ == -1)
                    return false;
                else if (second.UnimportantTime_ == -1)
                    return true;

                if (first.UnimportantTime_ == second.UnimportantTime_)
                    return first.AimId_ > second.AimId_;

                return first.UnimportantTime_ < second.UnimportantTime_;
            }

            if (first.PinnedTime_ == -1)
                return false;
            else if (second.PinnedTime_ == -1)
                return true;

            if (Favorites::isFavorites(first.AimId_))
                return true;

            if (Favorites::isFavorites(second.AimId_))
                return false;

            if (first.PinnedTime_ == second.PinnedTime_)
                return first.AimId_ > second.AimId_;

            return first.PinnedTime_ < second.PinnedTime_;
        });

        makeIndexes();

        Q_EMIT dataChanged(index(0), index(rowCount() - 1));
        Q_EMIT orderChanged();
    }

    void RecentsModel::contactRemoved(const QString& _aimId)
    {
        activeDialogHide(_aimId);
    }

    void RecentsModel::refresh()
    {
        Q_EMIT refreshAll();
        Q_EMIT dataChanged(index(0), index(rowCount() - 1));
    }

    void RecentsModel::refreshUnknownMessages()
    {
        Data::DlgState state;
        state.AimId_ = qsl("~unknowns~");
        Q_EMIT dlgStateChanged(state);

        const QModelIndex ndx(index(0));
        Q_EMIT dataChanged(ndx, ndx);
    }

    void RecentsModel::typingStatus(const Logic::TypingFires& _typing, bool _isTyping)
    {
        const auto it = std::find_if(Dialogs_.begin(), Dialogs_.end(), isEqualDlgState(_typing.aimId_));

        if (it == Dialogs_.end())
            return;

        it->updateTypings(_typing, _isTyping);

        Q_EMIT dlgStateChanged(*it);

        const auto dist = std::distance(Dialogs_.begin(), it);

        Q_EMIT dataChanged(index(0), index(getVisibleIndex(int(dist))));
    }

    Data::DlgState RecentsModel::getDlgState(const QString& aimId, bool fromDialog)
    {
        Data::DlgState state;
        auto getState = [&aimId](auto& dialogs) -> std::optional<Data::DlgState>
        {
            if (const auto iter = std::find_if(dialogs.cbegin(), dialogs.cend(), isEqualDlgState(aimId)); iter != dialogs.cend())
                return *iter;
            return std::nullopt;
        };
        if (auto d = getState(Dialogs_))
            state = std::move(*d);
        else if (auto h = getState(HiddenDialogs_))
            state = std::move(*h);

        if (fromDialog)
            sendLastRead(aimId);

        return state;
    }

    void RecentsModel::togglePinnedVisible()
    {
        PinnedVisible_ = !PinnedVisible_;
        Ui::get_gui_settings()->set_value(settings_pinned_chats_visible, PinnedVisible_);
        refresh();
        Q_EMIT updated();
    }

    void RecentsModel::toggleUnimportantVisible()
    {
        UnimportantVisible_ = !UnimportantVisible_;
        Ui::get_gui_settings()->set_value(settings_unimportant_visible, UnimportantVisible_);
        refresh();
        Q_EMIT updated();
    }

    void RecentsModel::unknownAppearance()
    {
        refresh();
    }

    void RecentsModel::sendLastRead(const QString& _aimId, bool force, ReadMode _mode)
    {
        if (const auto searchedAimId = _aimId.isEmpty() ? Logic::getContactListModel()->selectedContact() : _aimId; force && !searchedAimId.isEmpty())
        {
            auto sendLastReadImpl = [&searchedAimId, _mode](auto& dialogs) -> std::optional<int>
            {
                const auto iter = std::find_if(dialogs.begin(), dialogs.end(), isEqualDlgState(searchedAimId));
                if (iter != dialogs.end() && (iter->UnreadCount_ != 0 || iter->YoursLastRead_ < iter->LastMsgId_))
                {
                    iter->UnreadCount_ = 0;

                    Ui::gui_coll_helper collection(Ui::GetDispatcher()->create_collection(), true);
                    collection.set_value_as_qstring("contact", searchedAimId);
                    collection.set_value_as_int64("message", iter->LastMsgId_);
                    if (_mode == ReadMode::ReadAll)
                        collection.set_value_as_bool("read_all", true);
                    Ui::GetDispatcher()->post_message_to_core("dlg_state/set_last_read", collection.get());

                    return int(std::distance(dialogs.begin(), iter));
                }
                return std::nullopt;
            };
            if (auto idx = sendLastReadImpl(Dialogs_))
            {
                const auto i = index(getVisibleIndex(*idx));
                Q_EMIT dataChanged(i, i);
                Q_EMIT updated();
            }
            else
            {
                sendLastReadImpl(HiddenDialogs_);
            }
        }
    }

    std::pair<int, int> RecentsModel::markAllRead()
    {
        int readCnt = 0;
        int readChatsCnt = 0;

        const auto confirmed = Utils::GetConfirmationWithTwoButtons(
            QT_TRANSLATE_NOOP("popup_window", "Cancel"),
            QT_TRANSLATE_NOOP("popup_window", "Yes"),
            QT_TRANSLATE_NOOP("popup_window", "Do you really want mark all as read?"),
            QT_TRANSLATE_NOOP("popup_window", "Mark all as read"),
            nullptr);

        if (confirmed)
        {
            for (auto iter = Dialogs_.begin(); iter != Dialogs_.end(); ++iter)
            {
                if (iter->UnreadCount_ != 0 || iter->YoursLastRead_ < iter->LastMsgId_)
                {
                    readCnt += iter->UnreadCount_;
                    ++readChatsCnt;

                    Ui::gui_coll_helper collection(Ui::GetDispatcher()->create_collection(), true);
                    collection.set_value_as_qstring("contact", iter->AimId_);
                    collection.set_value_as_int64("message", iter->LastMsgId_);
                    collection.set_value_as_bool("read_all", true);
                    Ui::GetDispatcher()->post_message_to_core("dlg_state/set_last_read", collection.get());

                    const auto ind = std::distance(Dialogs_.begin(), iter);
                    const auto idx = index(getVisibleIndex(int(ind)));
                    Q_EMIT dataChanged(idx, idx);
                    Q_EMIT updated();
                }
                else if (iter->Attention_)
                {
                    setAttention(iter->AimId_, false);
                }
            }
        }

        return { readChatsCnt, readCnt };
    }

    void RecentsModel::setAttention(const QString& _aimId, const bool _value)
    {
        const auto searchedAimId = _aimId.isEmpty() ? Logic::getContactListModel()->selectedContact() : _aimId;
        if (const auto iter = std::find_if(Dialogs_.begin(), Dialogs_.end(), isEqualDlgState(searchedAimId)); iter != Dialogs_.end())
        {
            iter->Attention_ = _value;

            const auto ind = std::distance(Dialogs_.begin(), iter);
            const auto idx = index(getVisibleIndex(int(ind)));

            Q_EMIT dlgStateChanged(*iter);

            Q_EMIT dataChanged(idx, idx);
            Q_EMIT updated();
        }

        Ui::gui_coll_helper collection(Ui::GetDispatcher()->create_collection(), true);
        collection.set_value_as_qstring("contact", _aimId);
        collection.set_value_as_bool("value", _value);
        Ui::GetDispatcher()->post_message_to_core("dialogs/set_attention_attribute", collection.get());
    }

    bool RecentsModel::getAttention(const QString& _aimId) const
    {
        if (const auto iter = std::find_if(Dialogs_.cbegin(), Dialogs_.cend(), isEqualDlgState(_aimId)); iter != Dialogs_.cend())
            return iter->Attention_;

        return false;
    }

    void RecentsModel::hideChat(const QString& aimId)
    {
        Utils::InterConnector::instance().getMainWindow()->closeGallery();
        Ui::gui_coll_helper collection(Ui::GetDispatcher()->create_collection(), true);
        collection.set_value_as_qstring("contact", aimId);
        Ui::GetDispatcher()->post_message_to_core("dialogs/hide", collection.get());
    }

    void RecentsModel::muteChat(const QString& aimId, bool mute)
    {
        Ui::gui_coll_helper collMute(Ui::GetDispatcher()->create_collection(), true);
        collMute.set_value_as_qstring("contact", aimId);
        collMute.set_value_as_bool("mute", mute);
        Ui::GetDispatcher()->post_message_to_core("dialogs/mute", collMute.get());
    }

    bool RecentsModel::isFavorite(const QString& _aimId) const
    {
        if (const auto iter = std::find_if(Dialogs_.begin(), Dialogs_.end(), isEqualDlgState(_aimId)); iter != Dialogs_.end())
            return iter->PinnedTime_ != -1;

        return false;
    }

    bool RecentsModel::isUnimportant(const QString& _aimId) const
    {
        if (const auto iter = std::find_if(Dialogs_.begin(), Dialogs_.end(), isEqualDlgState(_aimId)); iter != Dialogs_.end())
            return iter->UnimportantTime_ != -1;

        return false;
    }

    bool RecentsModel::isSuspicious(const QString& _aimId) const
    {
        if (const auto iter = std::find_if(Dialogs_.begin(), Dialogs_.end(), isEqualDlgState(_aimId)); iter != Dialogs_.end())
            return iter->isSuspicious_;

        return getUnknownsModel()->contains(_aimId);
    }

    bool RecentsModel::isStranger(const QString& _aimid)
    {
        auto st = getDlgState(_aimid);
        if (st.AimId_.isEmpty())
            st = getUnknownsModel()->getDlgState(_aimid);

        return st.isStranger_;
    }

    bool RecentsModel::isServiceItem(const QModelIndex& _index) const
    {
        auto row = _index.row();
        return row == getPinnedHeaderIndex() || row == getRecentsHeaderIndex() || row == getUnimportantHeaderIndex();
    }

    bool RecentsModel::isClickableItem(const QModelIndex& _index) const
    {
        auto row = _index.row();
        return row != getRecentsHeaderIndex();
    }

    bool RecentsModel::isPinnedGroupButton(const QModelIndex& i) const
    {
        int r = i.row();
        return r == getPinnedHeaderIndex();
    }

    bool RecentsModel::isPinnedVisible() const
    {
        return PinnedVisible_;
    }

    quint16 RecentsModel::getPinnedCount() const
    {
        return PinnedCount_;
    }

    bool RecentsModel::isUnimportantGroupButton(const QModelIndex& i) const
    {
        int r = i.row();
        return r == getUnimportantHeaderIndex();
    }

    bool RecentsModel::isUnimportantVisible() const
    {
        return UnimportantVisible_;
    }

    quint16 RecentsModel::getUnimportantCount() const
    {
        return UnimportantCount_;
    }

    bool RecentsModel::isRecentsHeader(const QModelIndex& i) const
    {
        const int r = i.row();
        return r == getRecentsHeaderIndex();
    }

    bool RecentsModel::contains(const QString& _aimId) const
    {
        return std::any_of(Dialogs_.begin(), Dialogs_.end(), isEqualDlgState(_aimId));
    }

    QModelIndex RecentsModel::contactIndex(const QString& _aimId) const
    {
        const auto it = std::find_if(Dialogs_.begin(), Dialogs_.end(), isEqualDlgState(_aimId));
        if (it == Dialogs_.end())
            return QModelIndex();

        const auto dist = std::distance(Dialogs_.begin(), it);
        return index(getVisibleIndex(int(dist)));
    }

    int64_t RecentsModel::getLastMsgId(const QString& _aimId) const
    {
        if (const auto it = std::find_if(Dialogs_.begin(), Dialogs_.end(), isEqualDlgState(_aimId)); it != Dialogs_.end())
            return it->LastMsgId_;

        return -1;
    }

    void RecentsModel::setItemFriendlyText(const QString& _aimId, const Recents::FriendlyItemText& _frText)
    {
        auto it = std::find_if(Dialogs_.begin(), Dialogs_.end(), isEqualDlgState(_aimId));
        if (it == Dialogs_.end())
            return;

        if (_frText.msgId_ >= it->LastMsgId_)
        {
            if (_frText.msgId_ == it->LastMsgId_)
            {
                it->LastMessageFriendly_ = _frText.friendlyText_;
                contactChanged(_aimId);
            }

            friendlyTexts_[_aimId] = _frText;
        }
    }

    QString RecentsModel::firstContact() const
    {
        if (!Dialogs_.empty())
            return Dialogs_.front().AimId_;

        return QString();
    }

    int RecentsModel::totalUnreads() const
    {
        int result = 0;
        for (const auto& dlg : Dialogs_)
        {
            if (!Logic::getContactListModel()->isMuted(dlg.AimId_))
                result += dlg.UnreadCount_;
        }
        return result;
    }

    int RecentsModel::unimportantUnreads() const
    {
        int result = 0;
        for (const auto& dlg : Dialogs_)
        {
            if (dlg.UnimportantTime_ != -1 && !Logic::getContactListModel()->isMuted(dlg.AimId_))
                result += dlg.UnreadCount_;
        }
        return result;
    }

    int RecentsModel::pinnedUnreads() const
    {
        int result = 0;
        for (const auto& dlg : Dialogs_)
        {
            if (dlg.PinnedTime_ != -1 && !Logic::getContactListModel()->isMuted(dlg.AimId_))
                result += dlg.UnreadCount_;
        }
        return result;
    }

    bool RecentsModel::hasAttentionDialogs() const
    {
        for (const auto& iter : Dialogs_)
        {
            if (iter.Attention_)
                return true;
        }

        return false;
    }

    bool RecentsModel::hasAttentionPinned() const
    {
        for (const auto& iter : Dialogs_)
        {
            if (iter.Attention_ && iter.PinnedTime_ != -1)
                return true;
        }

        return false;
    }

    bool RecentsModel::hasAttentionUnimportant() const
    {
        for (const auto& iter : Dialogs_)
        {
            if (iter.Attention_ && iter.UnimportantTime_ != -1)
                return true;
        }

        return false;
    }

    bool RecentsModel::hasMentionsInPinned() const
    {
        for (const auto& iter : Dialogs_)
        {
            if (iter.unreadMentionsCount_ > 0 && iter.PinnedTime_ != -1)
                return true;
        }

        return false;
    }

    bool RecentsModel::hasMentionsInUnimportant() const
    {
        for (const auto& iter : Dialogs_)
        {
            if (iter.unreadMentionsCount_ > 0 && iter.UnimportantTime_ != -1)
                return true;
        }

        return false;
    }

    int RecentsModel::getMutedPinnedWithMentions() const
    {
        auto result = 0;
        for (const auto& iter : Dialogs_)
        {
            if (iter.unreadMentionsCount_ > 0 && iter.PinnedTime_ != -1 && Logic::getContactListModel()->isMuted(iter.AimId_))
                ++result;
        }

        return result;
    }

    int RecentsModel::getMutedUnimportantWithMentions() const
    {
        auto result = 0;
        for (const auto& iter : Dialogs_)
        {
            if (iter.unreadMentionsCount_ > 0 && iter.UnimportantTime_ != -1 && Logic::getContactListModel()->isMuted(iter.AimId_))
                ++result;
        }

        return result;
    }

    QString RecentsModel::nextUnreadAimId(const QString& _current) const
    {
        auto findFirstUnread = [](auto first, auto last)
        {
            return std::find_if(first, last, [](const auto& _state)
            {
                return _state.Attention_ || (_state.UnreadCount_ > 0 && !Logic::getContactListModel()->isMuted(_state.AimId_));
            });
        };

        const auto currentIt = std::find_if(Dialogs_.begin(), Dialogs_.end(), [&_current](const auto& _state)
        {
            return !_current.isEmpty() && _state.AimId_ == _current;
        });

        if (currentIt == Dialogs_.end())
        {
            if (const auto it = findFirstUnread(Dialogs_.begin(), Dialogs_.end()); it != Dialogs_.end())
                return it->AimId_;
        }
        else
        {
            if (const auto afterIt = findFirstUnread(std::next(currentIt), Dialogs_.end()); afterIt != Dialogs_.end())
                return afterIt->AimId_;
            else if (const auto beforeIt = findFirstUnread(Dialogs_.begin(), currentIt); beforeIt != currentIt)
                return beforeIt->AimId_;
        }

        return QString();
    }

    QString RecentsModel::nextAimId(const QString& aimId) const
    {
        if (!Dialogs_.empty())
        {
            for (size_t i = 0; i < Dialogs_.size() - 1; ++i)
            {
                if (Dialogs_[i].AimId_ == aimId)
                {
                    while ((i < Dialogs_.size() - 2) && isSpecialAndHidden(Dialogs_[i + 1]))
                        ++i;
                    return isSpecialAndHidden(Dialogs_[i + 1]) ? QString() : Dialogs_[i + 1].AimId_;
                }
            }
        }

        return QString();
    }

    QString RecentsModel::prevAimId(const QString& aimId) const
    {
        if (!Dialogs_.empty())
        {
            for (size_t i = 1; i < Dialogs_.size(); ++i)
            {
                if (Dialogs_[i].AimId_ == aimId)
                {
                    while (i > 1 && isSpecialAndHidden(Dialogs_[i - 1]))
                        --i;
                    return isSpecialAndHidden(Dialogs_[i - 1]) ? QString() : Dialogs_[i - 1].AimId_;
                }
            }
        }

        return QString();
    }

    int RecentsModel::getUnreadCount(const QString & _aimId) const
    {
        if (const auto it = std::find_if(Dialogs_.begin(), Dialogs_.end(), isEqualDlgState(_aimId)); it != Dialogs_.end())
            return it->UnreadCount_;

        return 0;
    }

    int RecentsModel::getUnreadMentionsCount(const QString& _aimId) const
    {
        if (const auto it = std::find_if(Dialogs_.begin(), Dialogs_.end(), isEqualDlgState(_aimId)); it != Dialogs_.end())
            return it->unreadMentionsCount_;

        return 0;
    }

    int32_t RecentsModel::getTime(const QString & _aimId) const
    {
        if (const auto it = std::find_if(Dialogs_.begin(), Dialogs_.end(), isEqualDlgState(_aimId)); it != Dialogs_.end())
            return it->Time_;

        return -1;
    }

    int RecentsModel::correctIndex(int i) const
    {
        if (i == getPinnedHeaderIndex() || i == getRecentsHeaderIndex() || i == getUnimportantHeaderIndex())
            return i;

        auto result = i;
        if (PinnedCount_ != 0 || UnimportantCount_ != 0)
        {
            if (i < getRecentsHeaderIndex())
            {
                if (UnimportantCount_ != 0 && PinnedCount_ != 0 && i > getUnimportantHeaderIndex())
                    result = i - 2;
                else
                    result = i - 1;
            }
            else
            {
                result -= getVisibleServiceItems();
            }

            if (!PinnedVisible_)
                result += PinnedCount_;

            if (!UnimportantVisible_ && i > getUnimportantHeaderIndex())
                result += UnimportantCount_;
        }

        return result;
    }

    int RecentsModel::getVisibleIndex(int i) const
    {
        if (i < 0 || size_t(i) >= Dialogs_.size())
            return -1;

        if (Dialogs_[i].PinnedTime_ != -1)
        {
            if (!PinnedVisible_)
                return -1;
            else
                return i + 1;
        }

        if (Dialogs_[i].UnimportantTime_ != -1)
        {
            if (!UnimportantVisible_)
                return -1;

            return (PinnedCount_ ? i + 2 : i + 1) - (PinnedVisible_ ? 0 : PinnedCount_);
        }

        if (UnimportantCount_ == 0 && PinnedCount_ == 0)
            return i;

        const auto result = i + getVisibleServiceItems() - (PinnedVisible_ ? 0 : PinnedCount_) - (UnimportantVisible_ ? 0 : UnimportantCount_);
        return result;
    }

    int RecentsModel::visiblePinnedContacts() const
    {
        return PinnedVisible_ ? PinnedCount_ : 0;
    }

    int RecentsModel::visibleContactsInUnimportant() const
    {
        return UnimportantVisible_ ? UnimportantCount_ : 0;
    }

    int RecentsModel::getPinnedHeaderIndex() const
    {
        return PinnedCount_ ? 0 : -1;
    }

    int RecentsModel::getUnimportantHeaderIndex() const
    {
        if (!UnimportantCount_)
            return -1;

        if (!PinnedCount_)
            return 0;

        return visiblePinnedContacts() + 1;
    }

    int RecentsModel::getRecentsHeaderIndex() const
    {
        if (!PinnedCount_ && !UnimportantCount_)
            return -1;

        auto i = 0;
        if (PinnedCount_)
            ++i;
        if (UnimportantCount_)
            ++i;

        return visiblePinnedContacts() + visibleContactsInUnimportant() + i;
    }

    int RecentsModel::getVisibleServiceItems() const
    {
        auto result = 0;

        if (PinnedCount_)
            ++result;
        if (UnimportantCount_)
            ++result;
        if (result > 0)
            ++result;

        return result;
    }

    std::vector<QString> RecentsModel::getSortedRecentsContacts() const
    {
        std::vector<QString> result;
        result.reserve(Dialogs_.size());
        for (const auto& item : Dialogs_)
            result.push_back(item.AimId_);

        return result;
    }

    void RecentsModel::moveToTop(const QString & _aimId)
    {
        auto it = std::find_if(Dialogs_.begin(), Dialogs_.end(), isEqualDlgState(_aimId));
        if (it == Dialogs_.end())
            return;

        if (it->PinnedTime_ != -1 || it->UnimportantTime_ != -1)
            return;

        std::rotate(Dialogs_.begin() + getPinnedCount() + getUnimportantCount(), it, it + 1);

        const auto dist = std::distance(Dialogs_.begin(), it);
        Q_EMIT dataChanged(index(0), index(getVisibleIndex(int(dist))));
        Q_EMIT updated();
    }

    int RecentsModel::dialogsCount() const
    {
        return int(Dialogs_.size());
    }

    void RecentsModel::scheduleRefreshTimer()
    {
        if (!refreshTimer_)
        {
            refreshTimer_ = new QTimer(this);

            QObject::connect(refreshTimer_, &QTimer::timeout, this, [this]
            {
                refresh();
                scheduleRefreshTimer();
            });
        }
        else
        {
            refreshTimer_->stop();
        }

        refreshTimer_->setSingleShot(true);

        const auto now = std::chrono::milliseconds(QTime::currentTime().msecsSinceStartOfDay());
        const auto oneDay = std::chrono::hours(24);
        const auto dur = (oneDay - now) + std::chrono::minutes(1);

        refreshTimer_->start(dur);
    }

    void RecentsModel::makeIndexes()
    {
        Indexes_.clear();
        size_t i = 0;
        for (const auto& iter : Dialogs_)
            Indexes_[iter.AimId_] = i++;
    }

    void RecentsModel::requestFavoritesLastMessage()
    {
        Ui::gui_coll_helper collection(Ui::GetDispatcher()->create_collection(), true);
        collection.set_value_as_qstring("contact", Favorites::aimId());
        collection.set_value_as_int64("from", -1);
        collection.set_value_as_int64("count_early", -1);
        collection.set_value_as_int64("count_later", -1);
        collection.set_value_as_bool("need_prefetch", true);
        collection.set_value_as_bool("is_first_request", true);
        collection.set_value_as_bool("after_search", false);

        favoritesLastMessageSeq_ = Ui::GetDispatcher()->post_message_to_core("archive/messages/get", collection.get());
    }

    bool RecentsModel::isSpecialAndHidden(const Data::DlgState& _s) const
    {
        if (_s.PinnedTime_ != -1)
            return !PinnedVisible_;

        if (_s.UnimportantTime_ != -1)
            return !UnimportantVisible_;

        return false;
    }

    RecentsModel* getRecentsModel()
    {
        if (!g_recents_model)
            g_recents_model = std::make_unique<RecentsModel>(nullptr);

        return g_recents_model.get();
    }

    void ResetRecentsModel()
    {
        g_recents_model.reset();
    }
}
