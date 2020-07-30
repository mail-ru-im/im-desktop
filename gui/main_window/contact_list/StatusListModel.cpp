#include "stdafx.h"

#include "StatusListModel.h"
#include "core_dispatcher.h"
#include "../../utils/features.h"
#include "../../gui_settings.h"
#include "../containers/StatusContainer.h"

namespace
{
    inline QString getDefaultJSONPath()
    {
        return (Ui::get_gui_settings()->get_value(settings_language, QString()) == u"ru") ? qsl(":/statuses/default_statuses") : qsl(":/statuses/default_statuses_eng");
    }

    auto statusesDelimeter()
    {
        return ql1c(';');
    }

    auto dataDelimeter()
    {
        return ql1c(':');
    }
}

namespace Logic
{
    StatusListModel::StatusListModel(QObject* _parent)
        : CustomAbstractListModel(_parent)
    {
        updateStatuses();
        connect(&Utils::InterConnector::instance(), &Utils::InterConnector::omicronUpdated, this, &StatusListModel::updateStatuses);
        connect(&Utils::InterConnector::instance(), &Utils::InterConnector::logout, this, &StatusListModel::clear);
    }

    void StatusListModel::updateCoreStatus(const QString &_aimId, const Statuses::Status &_status)
    {
        if (_aimId == Ui::MyInfo()->aimId())
            resetMyStatus(_status);
    }

    void Logic::StatusListModel::resetMyStatus(const Statuses::Status& _status)
    {
        updateStatus(_status.toString(), _status);
        setSelected(_status.toString());
        getStatusProxyModel()->invalidate();
    }

    void StatusListModel::updateStatuses()
    {
        clearData();
        selectedStatus_.clear();
        unserializeStatuses(Features::getStatusJson());
        statuses_.reserve(defaultStatuses_.size());
        for (const auto& s : defaultStatuses_)
        {
            statuses_.push_back(s);
            indexes_[s.toString()] = statuses_.size() - 1;
        }
        if (const auto currentStatus = Logic::GetStatusContainer()->getStatus(Ui::MyInfo()->aimId()); !currentStatus.isEmpty())
            resetMyStatus(currentStatus);
        updateResults();
    }

    int StatusListModel::rowCount(const QModelIndex&) const
    {
        return statuses_.size();
    }

    QVariant StatusListModel::data(const QModelIndex & _index, int _role) const
    {
        if (!_index.isValid() || (_role != Qt::DisplayRole && !Testing::isAccessibleRole(_role)))
            return QVariant();

        int cur = _index.row();
        if (cur >= (int)rowCount(_index))
            return QVariant();

        return QVariant::fromValue(statuses_[cur]);
    }
    void StatusListModel::clear()
    {
        Ui::get_gui_settings()->set_value(statuses_user_statuses, QString());
        clearData();
        updateResults();
    }

    void StatusListModel::clearData()
    {
        statuses_.clear();
        indexes_.clear();
    }

    std::optional<Statuses::Status> StatusListModel::getStatus(const QString& _status) const
    {
        const auto idx = getIndex(_status);
        if (idx == -1)
            return std::nullopt;

        return std::make_optional(statuses_[idx]);
    }

    void StatusListModel::setSelected(const QString& _status)
    {
        if (selectedStatus_ == _status)
            return;

        if (auto status = getStatus(selectedStatus_))
        {
            status->setSelected(false);
            updateStatus(selectedStatus_, std::move(*status));
        }

        if (auto status = getStatus(_status))
        {
            status->setSelected(true);
            updateStatus(_status, std::move(*status));
        }

        selectedStatus_ = _status;
        updateResults();
    }

    std::optional<Statuses::Status> StatusListModel::getSelected() const
    {
        return getStatus(selectedStatus_);
    }

    bool StatusListModel::isStatusSelected() const
    {
        return !selectedStatus_.isEmpty() && !getStatus(selectedStatus_)->isEmpty();
    }

    void StatusListModel::clearSelected()
    {
        setSelected(QString());
        updateResults();
        selectedStatus_.clear();
    }

    void StatusListModel::updateSelected()
    {
        if (isStatusSelected())
        {
            if (const auto idx = getIndex(selectedStatus_); idx != -1)
            {
                const auto i = index(idx);
                Q_EMIT dataChanged(i, i);
            }
        }
    }

    void StatusListModel::unserializeStatuses(const QString& _statuses)
    {
        rapidjson::Document doc;
        auto json = _statuses.toUtf8();
        doc.Parse(json.constData(), json.size());

        if (doc.HasParseError())
        {
            assert(QFile::exists(getDefaultJSONPath()));

            QFile file(getDefaultJSONPath());
            if (!file.open(QFile::ReadOnly | QFile::Text))
                return;

            defaultStatuses_.clear();
            json = file.readAll();

            doc.Parse(json.constData(), json.size());
            if (doc.HasParseError())
            {
                assert(false);
                return;
            }
            file.close();
        }
        defaultStatuses_.clear();
        defaultStatuses_.push_back(Statuses::Status(QString(), QT_TRANSLATE_NOOP("status", "No status")));
        if (doc.IsArray())
        {
            for (auto& nodeIter : doc.GetArray())
            {
                Statuses::Status tmpStatus;
                tmpStatus.unserialize(nodeIter.GetObject());
                defaultStatuses_.push_back(std::move(tmpStatus));
            }
        }

        const auto storedStatuses = loadStoredStatuses();
        updateWithStored(defaultStatuses_, storedStatuses);
    }

    void StatusListModel::storeStatuses()
    {
        QStringList vStatuses;
        vStatuses.reserve(defaultStatuses_.size());
        std::unordered_map<QString, int64_t, Utils::QStringHasher> statuses;
        const auto storedStatuses = loadStoredStatuses();

        const auto updateVector = [&vStatuses](QString _status, int64_t _duration)
        {
            vStatuses.push_back(_status % dataDelimeter() % QString::number(_duration));
        };

        for (const auto& s : defaultStatuses_)
        {
            if (!s.isEmpty())
            {
                if (const auto& stat = getStatus(s.toString()); stat != std::nullopt && stat->getDefaultDuration() != s.getDefaultDuration())
                    statuses[stat->toString()] = stat->getDefaultDuration().count();
                else if (const auto& stored = storedStatuses.find(s.toString()); stored != storedStatuses.end())
                    statuses[stored->first] = stored->second;
            }
        }

        for (const auto& s : statuses)
            updateVector(s.first, s.second);

        Ui::get_gui_settings()->set_value<QString>(statuses_user_statuses, vStatuses.join(statusesDelimeter()));
    }

    StatusM StatusListModel::loadStoredStatuses() const
    {
        const auto statusesFromSettings = Ui::get_gui_settings()->get_value<QString>(statuses_user_statuses, QString());
        if (statusesFromSettings.isEmpty())
            return {};

        StatusM storedStatuses;
        const auto statuses = statusesFromSettings.splitRef(statusesDelimeter(), QString::SkipEmptyParts);

        for (const auto& s : boost::adaptors::reverse(statuses))
        {
            const auto& stat = s.split(dataDelimeter(), QString::SkipEmptyParts);
            if (!stat.isEmpty() && stat.size() == 2)
                storedStatuses[stat[0].toString()] = stat[1].toLongLong();
        }
        return storedStatuses;
    }

    void StatusListModel::updateWithStored(StatusV& _statuses, const StatusM& _storage)
    {
        if (!_storage.empty())
        {
            for (auto& s : _statuses)
            {
                if (const auto& stored = _storage.find(s.toString()); stored != _storage.end() && stored->second != s.getDefaultDuration().count())
                    s.setDuration(std::chrono::seconds(stored->second));
            }
        }
    }

    void StatusListModel::setTime(const QString& _status, std::chrono::seconds _time)
    {
        if (auto status = getStatus(_status); status && !status->isEmpty())
        {
            status->setDuration(_time);
            updateStatus(_status, std::move(*status));
        }
    }

    int Logic::StatusListModel::getIndex(const QString &_status) const
    {
        if (const auto item = indexes_.find(_status); item != indexes_.end())
            return item->second;

        return -1;
    }

    void StatusListModel::updateStatus(const QString& _statusKey, const Statuses::Status& _status)
    {
        if (_statusKey.isEmpty())
            return;

        if (const auto idx = getIndex(_statusKey); idx != -1)
        {
            statuses_[idx].update(_status);
            const auto i = index(idx);
            Q_EMIT dataChanged(i, i);
        }
    }

    void Logic::StatusListModel::updateResults()
    {
        Q_EMIT dataChanged(QModelIndex(), QModelIndex());
    }

    StatusListModel* getStatusModel()
    {
        static auto statusModel = std::make_unique<StatusListModel>(nullptr);
        if (statusModel->rowCount() == 0)
            statusModel->updateStatuses();
        return statusModel.get();
    }

    StatusProxyModel* getStatusProxyModel()
    {
        static auto statusProxyModel = std::make_unique<StatusProxyModel>(nullptr);
        statusProxyModel->setSourceModel(getStatusModel());
        return statusProxyModel.get();
    }
}
