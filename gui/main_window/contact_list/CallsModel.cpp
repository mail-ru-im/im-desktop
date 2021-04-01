#include "stdafx.h"
#include "CallsModel.h"
#include "core_dispatcher.h"
#include "../history_control/VoipEventInfo.h"
#include "../../utils/features.h"
#include "../../utils/InterConnector.h"
#include "../containers/StatusContainer.h"

namespace Logic
{
    std::unique_ptr<CallsModel> g_calls_model;

    CallsModel::CallsModel(QObject* _parent)
        : CustomAbstractListModel(_parent)
        , serviceItems_(GroupCall|BottomSpace)
    {
        updateServiceItems();

        connect(Ui::GetDispatcher(), &Ui::core_dispatcher::callLog, this, &CallsModel::callLog);
        connect(Ui::GetDispatcher(), &Ui::core_dispatcher::newCall, this, &CallsModel::newCall);
        connect(Ui::GetDispatcher(), &Ui::core_dispatcher::callRemoveMessages, this, &CallsModel::callRemoveMessages);
        connect(Ui::GetDispatcher(), &Ui::core_dispatcher::callDelUpTo, this, &CallsModel::callDelUpTo);
        connect(Ui::GetDispatcher(), &Ui::core_dispatcher::callRemoveContact, this, &CallsModel::callRemoveContact);
        connect(Ui::GetDispatcher(), &Ui::core_dispatcher::externalUrlConfigUpdated, this, &CallsModel::updateServiceItems);
        connect(&Utils::InterConnector::instance(), &Utils::InterConnector::omicronUpdated, this, &CallsModel::updateServiceItems);
        connect(Logic::GetStatusContainer(), &Logic::StatusContainer::statusChanged, this, [this]()
        {
            Q_EMIT dataChanged(QModelIndex(), QModelIndex());
        });
    }

    CallsModel::~CallsModel() = default;

    int CallsModel::rowCount(const QModelIndex& _parent) const
    {
        return calls_.size() + getServiceItemsSize();
    }

    QVariant CallsModel::data(const QModelIndex& _index, int _role) const
    {
        const auto i = _index.row();
        if (i >= 0 && i <= rowCount())
        {
            auto serviceItemsCount = getServiceItemsSize();
            if (i < serviceItemsCount)
            {
                Data::CallInfo info;
                if (i == getServiceItemIndex(TopSpace))
                {
                    info.ServiceAimid_ = qsl("~space~");
                }
                else if (i == getServiceItemIndex(GroupCall))
                {
                    info.ServiceAimid_ = qsl("~group_call~");
                    info.ButtonsText_ = QT_TRANSLATE_NOOP("calls", "Group call");
                }
                else if (i == getServiceItemIndex(CallByLink))
                {
                    info.ServiceAimid_ = qsl("~call_by_link~");
                    info.ButtonsText_ = QT_TRANSLATE_NOOP("calls", "Create call link");
                }
                else if (i == getServiceItemIndex(Webinar))
                {
                    info.ServiceAimid_ = qsl("~webinar~");
                    info.ButtonsText_ = QT_TRANSLATE_NOOP("calls", "Create webinar");
                }
                else if (i == getServiceItemIndex(BottomSpace))
                {
                    info.ServiceAimid_ = qsl("~space~");
                }
                else
                {
                    im_assert(!"unknown item");
                }

                if (Testing::isAccessibleRole(_role))
                    return info.ServiceAimid_;

                return QVariant::fromValue(std::make_shared<Data::CallInfo>(info));
            }

            if (Testing::isAccessibleRole(_role))
                return QString(u"AS CallLog " % calls_[i - serviceItemsCount].VoipSid_);

            return QVariant::fromValue(std::make_shared<Data::CallInfo>(calls_[i - serviceItemsCount]));
        }
        return QVariant();
    }

    bool CallsModel::isServiceItem(const QModelIndex& _index) const
    {
        return _index.row() < getServiceItemsSize();
    }

    void CallsModel::setTopSpaceVisible(bool _visible)
    {
        if (_visible)
            serviceItems_ |= TopSpace;
        else
            serviceItems_ &= ~TopSpace;

        Q_EMIT dataChanged(QModelIndex(), QModelIndex());
    }

    void CallsModel::callLog(const Data::CallInfoVec& _calls)
    {
        calls_ = _calls;
        sortCalls();
        groupCalls();

        Q_EMIT dataChanged(QModelIndex(), QModelIndex());
    }

    void CallsModel::newCall(const Data::CallInfo& _call)
    {
        calls_.push_back(_call);
        sortCalls();
        groupCalls();

        Q_EMIT dataChanged(QModelIndex(), QModelIndex());
    }

    void CallsModel::callRemoveMessages(const QString& _aimid, const std::vector<qint64> _messages)
    {
        auto find = [_messages](qint64 _id)
        {
            return std::find_if(_messages.begin(), _messages.end(), [_id](const auto& _iter) { return _iter == _id; }) != _messages.end();
        };

        auto iter = calls_.begin();
        while (iter != calls_.end())
        {
            auto iter_messages = iter->Messages_.begin();
            while (iter_messages != iter->Messages_.end())
            {
                if (find(iter_messages->get()->Id_) && iter_messages->get()->AimId_ == _aimid)
                {
                    iter_messages = iter->Messages_.erase(iter_messages);
                    iter->calcCount();
                }
                else
                {
                    ++iter_messages;
                }
            }

            if (iter->Messages_.empty())
                iter = calls_.erase(iter);
            else
                ++iter;
        }

        Q_EMIT dataChanged(QModelIndex(), QModelIndex());
    }

    void CallsModel::callDelUpTo(const QString& _aimid, qint64 _del_up_to)
    {
        auto iter = calls_.begin();
        while (iter != calls_.end())
        {
            bool stillHasAimid_ = false;
            auto iter_messages = iter->Messages_.begin();
            while (iter_messages != iter->Messages_.end())
            {
                if (iter_messages->get()->AimId_ == _aimid)
                {
                    if (iter_messages->get()->Id_ <= _del_up_to)
                    {
                        iter_messages = iter->Messages_.erase(iter_messages);
                        iter->calcCount();
                    }
                    else
                    {
                        stillHasAimid_ = true;
                        ++iter_messages;
                    }
                }
                else
                {
                    ++iter_messages;
                }
            }

            if (iter->Messages_.empty())
                iter = calls_.erase(iter);
            else
                ++iter;
        }

        Q_EMIT dataChanged(QModelIndex(), QModelIndex());
    }

    void CallsModel::callRemoveContact(const QString& _contact)
    {
        auto iter = calls_.begin();
        while (iter != calls_.end())
        {
            auto iter_messages = iter->Messages_.begin();
            while (iter_messages != iter->Messages_.end())
            {
                if (iter_messages->get()->AimId_ == _contact)
                {
                    iter_messages = iter->Messages_.erase(iter_messages);
                    iter->calcCount();
                }
                else
                {
                    ++iter_messages;
                }
            }

            if (iter->Messages_.empty())
                iter = calls_.erase(iter);
            else
                ++iter;
        }

        Q_EMIT dataChanged(QModelIndex(), QModelIndex());
    }

    void CallsModel::updateServiceItems()
    {
        if (Features::isVcsCallByLinkEnabled())
            serviceItems_ |= CallByLink;
        else
            serviceItems_ &= ~CallByLink;

        if (Features::isVcsWebinarEnabled())
            serviceItems_ |= Webinar;
        else
            serviceItems_ &= ~Webinar;

        Q_EMIT dataChanged(QModelIndex(), QModelIndex());
    }

    void CallsModel::sortCalls()
    {
        std::sort(calls_.begin(), calls_.end(), [](const auto& _first, const auto& _second)
        {
            return _first.time() > _second.time();
        });

        Q_EMIT dataChanged(QModelIndex(), QModelIndex());
    }

    void CallsModel::groupCalls()
    {
        if (calls_.size() <= 1)
            return;

        const auto prevSize = calls_.size();

        auto iter = calls_.begin();
        auto next_iter = std::next(iter);

        while (next_iter != calls_.end())
        {
            auto cur_date = QDateTime::fromSecsSinceEpoch(iter->time());
            auto next_date = QDateTime::fromSecsSinceEpoch(next_iter->time());

            if (iter->isOutgoing() == next_iter->isOutgoing() &&
                iter->isVideo() == next_iter->isVideo() &&
                iter->isMissed() == next_iter->isMissed() &&
                cur_date.daysTo(next_date) == 0)
            {
                if ((iter->getAimId() == next_iter->getAimId() && iter->isSingleItem() && next_iter->isSingleItem()) ||
                    iter->VoipSid_ == next_iter->VoipSid_ || iter->Members_ == next_iter->Members_)
                {
                    iter->addMessages(next_iter->getMessages());
                    iter->mergeMembers(next_iter->Members_);
                    iter->calcCount();
                    calls_.erase(next_iter);
                }
                else
                {
                    ++iter;
                }
            }
            else
            {
                ++iter;
            }
            next_iter = std::next(iter);
        }

        if (prevSize != calls_.size())
            groupCalls();
    }

    Data::CallInfoVec CallsModel::getCalls() const
    {
        return calls_;
    }

    int CallsModel::getServiceItemsSize() const
    {
        unsigned int num = serviceItems_;
        auto count = 0;
        while (num)
        {
            count += num & 1;
            num >>= 1;
        }
        return count;
    }

    int CallsModel::getServiceItemIndex(const ServiceItems _item) const
    {
        if (!(serviceItems_ & _item))
            return -1;

        auto count = 0;
        unsigned int num = serviceItems_ & (~_item);
        for (auto i = 0; i < _item / 2; ++i)
        {
            count += num & 1;
            num >>= 1;
        }
        return count;
    }

    CallsModel* GetCallsModel()
    {
        if (!g_calls_model)
            g_calls_model = std::make_unique<CallsModel>(nullptr);

        return g_calls_model.get();
    }

    void ResetCallsModel()
    {
        g_calls_model.reset();
    }
}
