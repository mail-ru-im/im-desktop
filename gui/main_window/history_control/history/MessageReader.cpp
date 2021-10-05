#include "MessageReader.h"
#include "History.h"
#include "../../../core_dispatcher.h"
#include "../../contact_list/ContactListModel.h"
#include "../../contact_list/RecentsModel.h"
#include "../../contact_list/UnknownsModel.h"
#include "../../../utils/gui_coll_helper.h"
#include "../../../gui_settings.h"

namespace
{
    auto idIsEqual = [](auto id)
    {
        return [id](const auto& msg)
        {
            return msg->Id_ == id;
        };
    };
}

namespace hist
{
    MessageReader::MessageReader(const QString& _aimId, QObject* _parent)
        : QObject(_parent)
        , aimId_(_aimId)
    {
        connect(Ui::GetDispatcher(), &Ui::core_dispatcher::mentionMe, this, &MessageReader::mentionMe);
    }

    MessageReader::~MessageReader() = default;

    void MessageReader::init()
    {
        Ui::gui_coll_helper collection(Ui::GetDispatcher()->create_collection(), true);
        collection.set_value_as_qstring("contact", aimId_);
        Ui::GetDispatcher()->post_message_to_core("archive/mentions/get", collection.get());
    }

    void MessageReader::setEnabled(bool _value)
    {
        enabled_ = _value;
    }

    bool MessageReader::isEnabled() const
    {
        return enabled_;
    }

    int MessageReader::getMentionsCount() const noexcept
    {
        return mentions_.size();
    }

    Data::MessageBuddySptr MessageReader::getNextUnreadMention() const
    {
        if (mentions_.empty())
            return nullptr;

        return mentions_.front();
    }

    void MessageReader::onMessageItemRead(const qint64 _messageId, const bool _visible)
    {
        if (!isEnabled())
            return;

        auto removeFromPending = [this](const qint64 _messageId)
        {
            if (const auto iter_message = pendingVisibleMessages_.find(_messageId); iter_message != pendingVisibleMessages_.end())
            {
                pendingVisibleMessages_.erase(iter_message);
                return true;
            }
            return false;
        };

        if (!_visible)
        {
            removeFromPending(_messageId);
        }
        else
        {
            if (const auto iter_message = pendingVisibleMessages_.find(_messageId); iter_message != pendingVisibleMessages_.end())
                return;

            pendingVisibleMessages_.insert(_messageId);

            constexpr auto readTimeout = std::chrono::milliseconds(300);

            QTimer::singleShot(readTimeout, this, [this, _messageId, removeFromPending]()
            {
                if (removeFromPending(_messageId))
                {
                    if (isEnabled())
                        onMessageItemReadVisible(_messageId);
                }
            });
        }
    }

    void MessageReader::onReadAllMentionsLess(const qint64 _messageId, const bool _visible)
    {
        const auto end = mentions_.end();
        const auto partIt = std::stable_partition(mentions_.begin(), end, [_messageId](const auto& msg)
        {
            return msg->Id_ > _messageId || _messageId <= 0;
        });

        const auto readIdsSize = std::distance(partIt, end);
        if (readIdsSize > 0)
        {
            std::vector<qint64> readIds;
            readIds.reserve(readIdsSize);

            for (auto it = partIt; it != end; ++it)
                readIds.push_back((*it)->Id_);

            mentions_.erase(partIt, end);

            for (auto id : readIds)
                Q_EMIT mentionRead(id, QPrivateSignal());
        }
    }

    void MessageReader::onReadAllMentions()
    {
        for (const auto& m : std::exchange(mentions_, {}))
            Q_EMIT mentionRead(m->Id_, QPrivateSignal());
    }

    void MessageReader::onMessageItemReadVisible(const qint64 _messageId)
    {
        if (_messageId != -1)
        {
            const auto dlgState = hist::getDlgState(aimId_);
            if (dlgState.LastMsgId_ == _messageId)
                onReadAllMentions();
            else
                onReadAllMentionsLess(_messageId, true);

            const auto lastReadMention = std::max(lastReads_.mention_, dlgState.LastReadMention_);
            const auto yoursLastRead = dlgState.YoursLastRead_;
            const auto needResetUnreadCount = (dlgState.UnreadCount_ != 0 && yoursLastRead == _messageId);
            if (yoursLastRead < _messageId || needResetUnreadCount)
            {
                if (std::min(lastReads_.mention_, lastReads_.text_) < _messageId || needResetUnreadCount)
                {
                    lastReads_.mention_ = std::max({ _messageId, lastReads_.mention_, lastReads_.text_ });
                    lastReads_.text_ = lastReads_.mention_;
                    sendPartialLastRead(lastReads_.text_);
                }
            }
            else if (lastReadMention < _messageId)
            {
                lastReads_.mention_ = _messageId;
                sendLastReadMention(_messageId);
            }
        }
    }

    void MessageReader::setDlgState(const Data::DlgState& _dlgState)
    {
        lastReads_.mention_ = std::max(lastReads_.mention_, _dlgState.LastReadMention_);
        lastReads_.text_ = _dlgState.YoursLastRead_;

        std::call_once(lastReads_.flag_, [this]()
        {
            if (lastReads_.mention_ == -1)
                lastReads_.mention_ = lastReads_.text_;
        });

        if (_dlgState.unreadMentionsCount_ == 0)
            onReadAllMentions();
    }

    void MessageReader::deleted(const Data::MessageBuddies& _messages)
    {
        if (mentions_.empty())
            return;

        for (const auto& message : _messages)
        {
            for (auto iter = mentions_.begin(); iter != mentions_.end(); ++iter)
            {
                auto id = (*iter)->Id_;
                if (id == message->Id_)
                {
                    mentions_.erase(iter);
                    Q_EMIT mentionRead(id, QPrivateSignal());
                    break;
                }
            }
        }
    }

    void MessageReader::mentionMe(const QString& _contact, const Data::MessageBuddySptr& _mention)
    {
        if (_contact != aimId_)
            return;

        if (const auto state = Logic::getRecentsModel()->getDlgState(_contact); state.YoursLastRead_ >= _mention->Id_ || state.LastReadMention_ >= _mention->Id_)
            return;

        if (std::none_of(mentions_.cbegin(), mentions_.cend(), idIsEqual(_mention->Id_)))
            mentions_.push_back(_mention);
    }

    void MessageReader::sendLastReadMention(qint64 _messageId)
    {
        Ui::gui_coll_helper collection(Ui::GetDispatcher()->create_collection(), true);
        collection.set_value_as_qstring("contact", aimId_);
        collection.set_value_as_int64("message", _messageId);
        Ui::GetDispatcher()->post_message_to_core("dlg_state/set_last_read_mention", collection.get());
    }

    void MessageReader::sendPartialLastRead(qint64 _messageId)
    {
        Ui::gui_coll_helper collection(Ui::GetDispatcher()->create_collection(), true);
        collection.set_value_as_qstring("contact", aimId_);
        collection.set_value_as_int64("message", _messageId);
        Ui::GetDispatcher()->post_message_to_core("dlg_state/set_last_read_partial", collection.get());
    }
}
