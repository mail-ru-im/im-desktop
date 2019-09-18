#include "Heads.h"

#include "../../../core_dispatcher.h"
#include "../../../utils/InterConnector.h"
#include "../../../gui_settings.h"

#include <boost/range/adaptor/reversed.hpp>

Q_LOGGING_CATEGORY(heads, "heads")

namespace Heads
{
    HeadContainer::HeadContainer(const QString& _aimId, QObject* _parent)
        : QObject(_parent)
        , aimId_(_aimId)
    {
        connect(&Utils::InterConnector::instance(), &Utils::InterConnector::hideHeads, this, &Heads::HeadContainer::onHideHeads);
        connect(&Utils::InterConnector::instance(), &Utils::InterConnector::showHeads, this, &Heads::HeadContainer::onShowHeads);

        if (Ui::get_gui_settings()->get_value<bool>(settings_show_groupchat_heads, true))
            onShowHeads();
    }

    HeadContainer::~HeadContainer() = default;

    const Data::HeadsById& HeadContainer::headsById() const
    {
        return chatHeads_.heads_;
    }

    bool HeadContainer::hasHeads(qint64 _id) const
    {
        if (const auto hit = chatHeads_.heads_.constFind(_id); hit != chatHeads_.heads_.constEnd())
            return !hit->isEmpty();
        return false;
    }

    void HeadContainer::onShowHeads()
    {
        connect(Ui::GetDispatcher(), &Ui::core_dispatcher::chatHeads, this, &HeadContainer::onHeads, Qt::UniqueConnection);
    }

    void HeadContainer::onHideHeads()
    {
        disconnect(Ui::GetDispatcher(), &Ui::core_dispatcher::chatHeads, this, &HeadContainer::onHeads);
        chatHeads_ = {};
        emit hide(QPrivateSignal());
    }

    void HeadContainer::onHeads(const Data::ChatHeads& _chatHeads)
    {
        if (_chatHeads.aimId_ != aimId_)
            return;

        if constexpr (build::is_debug())
        {
            qCDebug(heads) << aimId_ << "reset" << _chatHeads.resetState_;

            for (auto iter = _chatHeads.heads_.begin(), end = _chatHeads.heads_.end(); iter != end; ++iter)
            {
                qCDebug(heads) << iter.key();
                for (const auto& h : iter.value())
                    qCDebug(heads) << h.aimid_ << h.friendly_;
            }
        }

        if (_chatHeads.resetState_)
        {
            chatHeads_ = _chatHeads;
        }
        else
        {
            for (auto iter = _chatHeads.heads_.begin(), end = _chatHeads.heads_.end(); iter != end; ++iter)
            {
                for (auto& h : chatHeads_.heads_)
                {
                    for (const auto& _h : iter.value())
                        h.removeAll(_h);
                }

                const auto& newHeads = iter.value();
                auto& currentHeads = chatHeads_.heads_[iter.key()];

                for (const auto& h : boost::adaptors::reverse(newHeads))
                    currentHeads.push_back(h);

                std::rotate(currentHeads.rbegin(), currentHeads.rbegin() + newHeads.size(), currentHeads.rend());
            }
        }

        emit headChanged(_chatHeads, QPrivateSignal());
    }
}
