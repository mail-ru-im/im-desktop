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
#include "../../utils/InterConnector.h"
#include "../../utils/stat_utils.h"
#include "../../gui_settings.h"


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
        , FavoritesCount_(0)
        , FavoritesVisible_(true)
        , refreshTimer_(nullptr)
    {
        FavoritesVisible_ = Ui::get_gui_settings()->get_value(settings_favorites_visible, true);

        connect(GetAvatarStorage(), &Logic::AvatarStorage::avatarChanged, this, &RecentsModel::contactAvatarChanged);
        connect(Ui::GetDispatcher(), &Ui::core_dispatcher::activeDialogHide, this, &RecentsModel::activeDialogHide);
        connect(Ui::GetDispatcher(), &Ui::core_dispatcher::dlgStates, this, &RecentsModel::dlgStates);
        connect(Ui::GetDispatcher(), &Ui::core_dispatcher::messageBuddies, this, &RecentsModel::messageBuddies);

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

        auto count = (int)Dialogs_.size() + getVisibleServiceItemInFavorites();
        if (!FavoritesVisible_)
            count -= FavoritesCount_;

        return count;
    }

    QVariant RecentsModel::data(const QModelIndex &i, int r) const
    {
        if (!i.isValid() || (r != Qt::DisplayRole && !Testing::isAccessibleRole(r)))
            return QVariant();

        int cur = i.row();
        if (cur >= rowCount(i))
            return QVariant();

        if (cur == getFavoritesHeaderIndex())
        {
            const static auto st = []() {
                Data::DlgState s;
                s.AimId_ = qsl("~favorites~");
                s.SetText(QT_TRANSLATE_NOOP("contact_list", "FAVORITES"));
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
            return cont.AimId_;

        return QVariant::fromValue(cont);
    }

    void RecentsModel::contactChanged(const QString& _aimid)
    {
        if (const auto it = std::as_const(Indexes_).find(_aimid); it != std::as_const(Indexes_).end())
        {
            const auto dialogIdx = it.value();
            if (dialogIdx < int(Dialogs_.size()))
                emit dlgStateChanged(Dialogs_[dialogIdx]);

            const auto modelIdx = index(getVisibleIndex(dialogIdx));
            emit dataChanged(modelIdx, modelIdx);
        }
    }

    void RecentsModel::selectedContactChanged(const QString& _new, const QString& _prev)
    {
        const auto updateIndex = [this](const QString& _aimId)
        {
            if (const auto it = std::as_const(Indexes_).find(_aimId); it != std::as_const(Indexes_).end())
            {
                const auto modelIdx = index(getVisibleIndex(it.value()));
                emit dataChanged(modelIdx, modelIdx);
            }
        };

        updateIndex(_new);
        updateIndex(_prev);
    }

    void RecentsModel::contactAvatarChanged(const QString& _aimid)
    {
        for (int i = 0; i < int(Dialogs_.size()); ++i)
        {
            const auto& dialog = Dialogs_[i];
            const bool needUpdate = dialog.AimId_ == _aimid ||
                std::any_of(dialog.heads_.begin(), dialog.heads_.end(), [&_aimid](const auto& _head) { return _head.first == _aimid; });

            if (needUpdate)
            {
                const auto idx = index(getVisibleIndex(i));
                emit dataChanged(idx, idx);
            }
        }
    }

    void RecentsModel::activeDialogHide(const QString& _aimId)
    {
        auto iter = std::find_if(Dialogs_.begin(), Dialogs_.end(), isEqualDlgState(_aimId));
        if (iter == Dialogs_.end())
            return;

        if (iter->FavoriteTime_ != -1)
            --FavoritesCount_;

        contactChanged(iter->AimId_);

        friendlyTexts_.erase(iter->AimId_);
        Dialogs_.erase(iter);

        makeIndexes();

        if (Logic::getContactListModel()->selectedContact() == _aimId)
            Logic::getContactListModel()->setCurrent(QString(), -1, true);
        if (Dialogs_.empty() && !Logic::getUnknownsModel()->itemsCount())
            emit Utils::InterConnector::instance().showRecentsPlaceholder();

        refresh();

        emit updated();
    }

    void RecentsModel::dlgStates(const QVector<Data::DlgState>& _states)
    {
        auto updatedItems = 0;

        const auto curSelected = Logic::getContactListModel()->selectedContact();
        const auto w = Utils::InterConnector::instance().getMainWindow();
        const auto mainPageOpened = w && w->isUIActive() && w->isMainPage();

        for (const auto& _dlgState : _states)
        {
            const auto contactItem = Logic::getContactListModel()->getContactItem(_dlgState.AimId_);

            if (!contactItem)
                Logic::getContactListModel()->add(_dlgState.AimId_, _dlgState.Friendly_);

            auto iter = std::find_if(Dialogs_.begin(), Dialogs_.end(), isEqualDlgState(_dlgState.AimId_));

            if (!_dlgState.Chat_)
            {
                if (_dlgState.isSuspicious_)
                {
                    if (iter != Dialogs_.end())
                    {
                        Dialogs_.erase(iter);
                        ++updatedItems;

                        if (_dlgState.AimId_ == curSelected)
                            emit Utils::InterConnector::instance().unknownsGoSeeThem();
                    }

                    continue;
                }
            }

            Logic::getContactListModel()->setContactVisible(_dlgState.AimId_, !_dlgState.isStranger_);

            if (iter != Dialogs_.end())
            {
                ++updatedItems;

                auto &curDlgState = *iter;

                if (curDlgState.YoursLastRead_ != _dlgState.YoursLastRead_)
                    emit readStateChanged(_dlgState.AimId_);

                if (curDlgState.isStranger_ && !_dlgState.isStranger_)
                    Ui::GetDispatcher()->post_stats_to_core(core::stats::stats_event_names::chatscr_blockbar_event, { { "type", "disappeared" },{ "chat_type", Utils::chatTypeByAimId(_dlgState.AimId_) } });

                const auto prevText = curDlgState.GetText();
                const auto prevMediaType = curDlgState.mediaType_;

                const auto prevLastId = curDlgState.LastMsgId_;
                const auto prevTime = curDlgState.Time_;

                bool fChanged = curDlgState.FavoriteTime_ != _dlgState.FavoriteTime_;
                if (fChanged)
                {
                    if (_dlgState.FavoriteTime_ != -1)
                        ++FavoritesCount_;
                    else
                        --FavoritesCount_;
                }
#ifdef DEBUG
                const auto prevState = curDlgState;
#endif // DEBUG
                curDlgState = _dlgState;

                if (fChanged)
                    emit favoriteChanged(_dlgState.AimId_);

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

                    const auto keepPrevTime =
                        curDlgState.FavoriteTime_ == _dlgState.FavoriteTime_ &&
                        curDlgState.LastMsgId_ == prevLastId &&
                        curDlgState.UnreadCount_ == 0 &&
                        curDlgState.GetText() == prevText;

                        if (keepPrevTime)
                            curDlgState.Time_ = prevTime;
                }
            }
            else if (!_dlgState.GetText().isEmpty() || _dlgState.FavoriteTime_ != -1)
            {
                ++updatedItems;

                if (_dlgState.FavoriteTime_ != -1)
                {
                    ++FavoritesCount_;
                    emit favoriteChanged(_dlgState.AimId_);
                }

                Dialogs_.push_back(_dlgState);

                if (Dialogs_.size() == 1)
                    emit Utils::InterConnector::instance().hideRecentsPlaceholder();
            }

            if (mainPageOpened && _dlgState.AimId_ == curSelected)
                sendLastRead(_dlgState.AimId_);

            emit dlgStateChanged(_dlgState);
        }

        if (updatedItems > 0)
        {
            sortDialogs();
            emit updated();
        }

        emit dlgStatesHandled(_states);
    }

    void RecentsModel::messageBuddies(const Data::MessageBuddies& _buddies, const QString& _aimId, Ui::MessagesBuddiesOpt _type, bool, qint64, int64_t)
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

    void RecentsModel::unknownToRecents(const Data::DlgState& dlgState)
    {
        auto iter = std::find(Dialogs_.begin(), Dialogs_.end(), dlgState);
        if (iter == Dialogs_.end() && (!dlgState.GetText().isEmpty() || dlgState.FavoriteTime_ != -1))
        {
            if (dlgState.FavoriteTime_ != -1)
            {
                ++FavoritesCount_;
                emit favoriteChanged(dlgState.AimId_);
            }
            Dialogs_.push_back(dlgState);
            sortDialogs();
        }
    }

    void RecentsModel::sortDialogs()
    {
        std::sort(Dialogs_.begin(), Dialogs_.end(), [](const Data::DlgState& first, const Data::DlgState& second)
        {
            if (first.FavoriteTime_ == -1 && second.FavoriteTime_ == -1)
                return first.Time_ > second.Time_;

            if (first.FavoriteTime_ == -1)
                return false;
            else if (second.FavoriteTime_ == -1)
                return true;

            if (first.FavoriteTime_ == second.FavoriteTime_)
                return first.AimId_ > second.AimId_;

            return first.FavoriteTime_ < second.FavoriteTime_;
        });

        makeIndexes();

        emit dataChanged(index(0), index(rowCount() - 1));
        emit orderChanged();
    }

    void RecentsModel::contactRemoved(const QString& _aimId)
    {
        activeDialogHide(_aimId);
    }

    void RecentsModel::refresh()
    {
        emit refreshAll();
        emit dataChanged(index(0), index(rowCount() - 1));
    }

    void RecentsModel::refreshUnknownMessages()
    {
        Data::DlgState state;
        state.AimId_ = qsl("~unknowns~");
        emit dlgStateChanged(state);

        const QModelIndex ndx(index(0));
        emit dataChanged(ndx, ndx);
    }

    void RecentsModel::typingStatus(const Logic::TypingFires& _typing, bool _isTyping)
    {
        const auto it = std::find_if(Dialogs_.begin(), Dialogs_.end(), isEqualDlgState(_typing.aimId_));

        if (it == Dialogs_.end())
            return;

        it->updateTypings(_typing, _isTyping);

        emit dlgStateChanged(*it);

        const auto dist = std::distance(Dialogs_.begin(), it);

        emit dataChanged(index(0), index(getVisibleIndex(dist)));
    }

    Data::DlgState RecentsModel::getDlgState(const QString& aimId, bool fromDialog)
    {
        Data::DlgState state;
        if (const auto iter = std::find_if(Dialogs_.cbegin(), Dialogs_.cend(), isEqualDlgState(aimId)); iter != Dialogs_.cend())
            state = *iter;

        if (fromDialog)
            sendLastRead(aimId);

        return state;
    }

    void RecentsModel::toggleFavoritesVisible()
    {
        FavoritesVisible_ = !FavoritesVisible_;
        Ui::get_gui_settings()->set_value(settings_favorites_visible, FavoritesVisible_);
        refresh();
        emit updated();
    }

    void RecentsModel::unknownAppearance()
    {
        refresh();
    }

    void RecentsModel::sendLastRead(const QString& _aimId, bool force, ReadMode _mode)
    {
        if (const auto searchedAimId = _aimId.isEmpty() ? Logic::getContactListModel()->selectedContact() : _aimId; !searchedAimId.isEmpty())
        {
            if (force || !Ui::get_gui_settings()->get_value<bool>(settings_partial_read, settings_partial_read_deafult()))
            {
                const auto iter = std::find_if(Dialogs_.begin(), Dialogs_.end(), isEqualDlgState(searchedAimId));
                if (iter != Dialogs_.end() && (iter->UnreadCount_ != 0 || iter->YoursLastRead_ < iter->LastMsgId_))
                {
                    iter->UnreadCount_ = 0;

                    Ui::gui_coll_helper collection(Ui::GetDispatcher()->create_collection(), true);
                    collection.set_value_as_qstring("contact", searchedAimId);
                    collection.set_value_as_int64("message", iter->LastMsgId_);
                    if (_mode == ReadMode::ReadAll)
                        collection.set_value_as_bool("read_all", true);
                    Ui::GetDispatcher()->post_message_to_core("dlg_state/set_last_read", collection.get());

                    const int ind = int(std::distance(Dialogs_.begin(), iter));
                    const auto idx = index(getVisibleIndex(ind));
                    emit dataChanged(idx, idx);
                    emit updated();
                }
            }
        }
    }

    int RecentsModel::markAllRead()
    {
        int readCnt = 0;

        const auto confirmed = Utils::GetConfirmationWithTwoButtons(
            QT_TRANSLATE_NOOP("popup_window", "CANCEL"),
            QT_TRANSLATE_NOOP("popup_window", "YES"),
            QT_TRANSLATE_NOOP("popup_window", "Do you really want mark all as read?"),
            QT_TRANSLATE_NOOP("popup_window", "Mark all as read"),
            nullptr);

        if (confirmed)
        {
            for (auto iter = Dialogs_.begin(); iter != Dialogs_.end(); ++iter)
            {
                if (iter->UnreadCount_ != 0 || iter->YoursLastRead_ < iter->LastMsgId_)
                {
                    ++readCnt;

                    Ui::gui_coll_helper collection(Ui::GetDispatcher()->create_collection(), true);
                    collection.set_value_as_qstring("contact", iter->AimId_);
                    collection.set_value_as_int64("message", iter->LastMsgId_);
                    collection.set_value_as_bool("read_all", true);
                    Ui::GetDispatcher()->post_message_to_core("dlg_state/set_last_read", collection.get());

                    const int ind = (int)std::distance(Dialogs_.begin(), iter);
                    const auto idx = index(getVisibleIndex(ind));
                    emit dataChanged(idx, idx);
                    emit updated();
                }
                else if (iter->Attention_)
                {
                    setAttention(iter->AimId_, false);
                }
            }
        }

        return readCnt;
    }

    void RecentsModel::setAttention(const QString& _aimId, const bool _value)
    {
        const auto searchedAimId = _aimId.isEmpty() ? Logic::getContactListModel()->selectedContact() : _aimId;

        const auto iter = std::find_if(Dialogs_.begin(), Dialogs_.end(), isEqualDlgState(searchedAimId));

        if (iter != Dialogs_.end())
        {
            iter->Attention_ = _value;

            const int ind = (int)std::distance(Dialogs_.begin(), iter);
            const auto idx = index(getVisibleIndex(ind));

            emit dlgStateChanged(*iter);

            emit dataChanged(idx, idx);
            emit updated();
        }

        Ui::gui_coll_helper collection(Ui::GetDispatcher()->create_collection(), true);
        collection.set_value_as_qstring("contact", _aimId);
        collection.set_value_as_bool("value", _value);
        Ui::GetDispatcher()->post_message_to_core("dialogs/set_attention_attribute", collection.get());
    }

    bool RecentsModel::getAttention(const QString& _aimId) const
    {
        if (const auto iter = std::find_if(Dialogs_.begin(), Dialogs_.end(), isEqualDlgState(_aimId)); iter != Dialogs_.end())
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
        Ui::gui_coll_helper collection(Ui::GetDispatcher()->create_collection(), true);
        collection.set_value_as_qstring("contact", aimId);
        collection.set_value_as_bool("mute", mute);
        Ui::GetDispatcher()->post_message_to_core("dialogs/mute", collection.get());
    }

    bool RecentsModel::isFavorite(const QString& _aimId) const
    {
        auto iter = std::find_if(Dialogs_.begin(), Dialogs_.end(), isEqualDlgState(_aimId));
        if (iter != Dialogs_.end())
            return iter->FavoriteTime_ != -1;

        return false;
    }

    bool RecentsModel::isSuspicious(const QString& _aimId) const
    {
        auto iter = std::find_if(Dialogs_.begin(), Dialogs_.end(), isEqualDlgState(_aimId));
        if (iter != Dialogs_.end())
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
        return row == getFavoritesHeaderIndex() || row == getRecentsHeaderIndex();
    }

    bool RecentsModel::isClickableItem(const QModelIndex& _index) const
    {
        auto row = _index.row();
        return row != getRecentsHeaderIndex();
    }

    bool RecentsModel::isFavoritesGroupButton(const QModelIndex& i) const
    {
        int r = i.row();
        return r == getFavoritesHeaderIndex();
    }

    bool RecentsModel::isFavoritesVisible() const
    {
        return FavoritesVisible_;
    }

    quint16 RecentsModel::getFavoritesCount() const
    {
        return FavoritesCount_;
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
        int i = 0;
        for (const auto& iter : Dialogs_)
        {
            if (iter.AimId_ == _aimId)
            {
                break;
            }
            ++i;
        }

        if (i < (int)Dialogs_.size())
        {
            if (FavoritesCount_)
            {
                if (i >= visibleContactsInFavorites())
                {
                    ++i;
                    if (!FavoritesVisible_)
                        i -= FavoritesCount_;
                }
                ++i;
            }

            return index(i);
        }

        return QModelIndex();
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

    int RecentsModel::recentsUnreads() const
    {
        int result = 0;
        for (const auto& dlg : Dialogs_)
        {
            if (!Logic::getContactListModel()->isMuted(dlg.AimId_) && dlg.FavoriteTime_ == -1)
                result += dlg.UnreadCount_;
        }
        return result;
    }

    int RecentsModel::favoritesUnreads() const
    {
        int result = 0;
        for (const auto& iter : Dialogs_)
        {
            if (!Logic::getContactListModel()->isMuted(iter.AimId_) && iter.FavoriteTime_ != -1)
                result += iter.UnreadCount_;
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
                    return Dialogs_[i + 1].AimId_;
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
                    return Dialogs_[i - 1].AimId_;
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

    int32_t RecentsModel::getTime(const QString & _aimId) const
    {
        if (const auto it = std::find_if(Dialogs_.begin(), Dialogs_.end(), isEqualDlgState(_aimId)); it != Dialogs_.end())
            return it->Time_;

        return -1;
    }

    int RecentsModel::correctIndex(int i) const
    {
        if (i == getFavoritesHeaderIndex() || i == getRecentsHeaderIndex())
            return i;

        if (FavoritesCount_ != 0)
        {
            if (i < getRecentsHeaderIndex())
                return i - 1;

            i -= getVisibleServiceItemInFavorites();
            if (!FavoritesVisible_)
                i += FavoritesCount_;
        }

        return i;
    }

    int RecentsModel::getVisibleIndex(int i) const
    {
        if (i < FavoritesCount_)
        {
            if (FavoritesVisible_)
                return i + 1;

            return -1;
        }

        auto result = i;
        if (FavoritesCount_ == 0)
            return result;

        result += 2;
        if (!FavoritesVisible_)
            result -= FavoritesCount_;

        return result;
    }

    int RecentsModel::visibleContactsInFavorites() const
    {
        return FavoritesVisible_ ? FavoritesCount_ : 0;
    }

    int RecentsModel::getFavoritesHeaderIndex() const
    {
        return FavoritesCount_ ? 0 : -1;
    }

    int RecentsModel::getRecentsHeaderIndex() const
    {
        if (!FavoritesCount_)
            return -1;

        return visibleContactsInFavorites() + 1;
    }

    int RecentsModel::getVisibleServiceItemInFavorites() const
    {
        return FavoritesCount_ ? 2 : 0;
    }

    bool RecentsModel::isServiceAimId(const QString& _aimId) const
    {
        return _aimId.startsWith(ql1c('~')) && _aimId.endsWith(ql1c('~'));
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

        if (it->FavoriteTime_ != -1)
            return;

        std::rotate(Dialogs_.begin() + getFavoritesCount(), it, it + 1);

        const auto dist = std::distance(Dialogs_.begin(), it);
        emit dataChanged(index(0), index(getVisibleIndex(dist)));
        emit updated();
    }

    int RecentsModel::dialogsCount() const
    {
        return Dialogs_.size();
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
        const std::chrono::milliseconds dur = (oneDay - now) + std::chrono::minutes(1);

        refreshTimer_->start(dur.count());
    }

    void RecentsModel::makeIndexes()
    {
        Indexes_.clear();
        int i = 0;
        for (const auto& iter : Dialogs_)
            Indexes_[iter.AimId_] = i++;
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
