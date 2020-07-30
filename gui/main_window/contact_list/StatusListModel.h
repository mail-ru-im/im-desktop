#pragma once

#include <QtCore/QSortFilterProxyModel>
#include "AbstractSearchModel.h"
#include "../../types/chat.h"
#include "../../statuses/StatusUtils.h"
#include "../../statuses/Status.h"
#include "../../utils/InterConnector.h"

namespace Logic
{
    using StatusM = std::unordered_map<QString, int64_t, Utils::QStringHasher>;
    using StatusV = std::vector<Statuses::Status>;

    class StatusListModel : public CustomAbstractListModel
    {
      Q_OBJECT

    Q_SIGNALS:
          void StatusSelected(const QString&);
    public Q_SLOTS:
        void updateCoreStatus(const QString& _aimId, const Statuses::Status& _status);
    public:
        StatusListModel(QObject* _parent);

        int rowCount(const QModelIndex& _parent = QModelIndex()) const override;
        QVariant data(const QModelIndex& _index, int _role) const override;
        void clear();
        void clearData();

        std::optional<Statuses::Status> getStatus(const QString& _status) const;

        void setSelected(const QString& _status);
        std::optional<Statuses::Status> getSelected() const;
        bool isStatusSelected() const;
        void clearSelected();
        void updateSelected();

        void storeStatuses();
        void updateStatuses();

        void setTime(const QString& _status, std::chrono::seconds _time);
        const StatusV& getStatuses() const { return statuses_; }

    private:
        int getIndex(const QString& _status) const;
        void updateStatus(const QString& _statusKey, const Statuses::Status& _status);
        void updateResults();
        void unserializeStatuses(const QString& _statuses);
        StatusM loadStoredStatuses() const;
        void resetMyStatus(const Statuses::Status& _status);
        void updateWithStored(StatusV& _statuses, const StatusM& _storage);

    private:
        StatusV statuses_;
        StatusV defaultStatuses_;
        StatusM indexes_;
        QString selectedStatus_;
    };

    StatusListModel* getStatusModel();

    class StatusProxyModel : public QSortFilterProxyModel
    {
        Q_OBJECT

    public:
        StatusProxyModel(QObject *parent = nullptr)
            : QSortFilterProxyModel(parent)
        {
            connect(&Utils::InterConnector::instance(), &Utils::InterConnector::omicronUpdated, this, &StatusProxyModel::invalidate);
            connect(&Utils::InterConnector::instance(), &Utils::InterConnector::logout, this, &StatusProxyModel::invalidate);
        }

    protected:
        bool filterAcceptsRow(int sourceRow, const QModelIndex &sourceParent) const override
        {
            const auto model = qobject_cast<const Logic::StatusListModel*>(sourceModel());
            return (sourceRow != 0 || (sourceRow == 0 && model->isStatusSelected()));
        }
    };

    StatusProxyModel* getStatusProxyModel();
}