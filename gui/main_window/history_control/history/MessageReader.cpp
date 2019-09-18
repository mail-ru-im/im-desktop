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

            constexpr std::chrono::milliseconds readTimeout = std::chrono::milliseconds(300);

            QTimer::singleShot(readTimeout.count(), this, [this, _messageId, removeFromPending]()
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
                emit mentionRead(id, QPrivateSignal());
        }
    }

    void MessageReader::onMessageItemReadVisible(const qint64 _messageId)
    {
        if (_messageId != -1)
        {
            onReadAllMentionsLess(_messageId, true);

            if (Utils::isChat(aimId_) && !Logic::getContactListModel()->contains(aimId_))
                return;

            const auto dlgState = hist::getDlgState(aimId_);
            const auto lastReadMention = std::max(lastReads_.mention, dlgState.LastReadMention_);
            if (Ui::get_gui_settings()->get_value<bool>(settings_partial_read, settings_partial_read_deafult()))
            {
                const auto yoursLastRead = dlgState.YoursLastRead_;
                const auto needResetUnreadCount = (dlgState.UnreadCount_ != 0 && yoursLastRead == _messageId);
                if (yoursLastRead < _messageId || needResetUnreadCount)
                {
                    if (std::min(lastReads_.mention, lastReads_.text) < _messageId || needResetUnreadCount)
                    {
                        lastReads_.mention = std::max({ _messageId, lastReads_.mention, lastReads_.text });
                        lastReads_.text = lastReads_.mention;
                        sendPartialLastRead(lastReads_.text);
                    }
                }
                else if (lastReadMention < _messageId)
                {
                    lastReads_.mention = _messageId;
                    sendLastReadMention(_messageId);
                }
            }
            else if (lastReadMention < _messageId)
            {
                lastReads_.mention = _messageId;
                sendLastReadMention(_messageId);
            }
        }
    }

    void MessageReader::setDlgState(const Data::DlgState& _dlgState)
    {
        lastReads_.mention = _dlgState.LastReadMention_;
        lastReads_.text = _dlgState.YoursLastRead_;
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
