#include "stdafx.h"
#include "StatusSearchModel.h"
#include "../../core_dispatcher.h"

namespace
{
    constexpr std::chrono::milliseconds typingTimeout = std::chrono::milliseconds(300);
}

namespace Logic
{
    StatusSearchModel::StatusSearchModel(QObject* _parent)
        : AbstractSearchModel(_parent)
        , timer_(new QTimer(this))
    {
        timer_->setSingleShot(true);
        timer_->setInterval(typingTimeout);
        connect(timer_, &QTimer::timeout, this, &StatusSearchModel::doSearch);
    }

    StatusSearchModel::~StatusSearchModel() = default;

    int StatusSearchModel::rowCount(const QModelIndex& _parent) const
    {
        return results_.size();
    }

    QVariant StatusSearchModel::data(const QModelIndex& _index, int _role) const
    {
        if (!_index.isValid() || (_role != Qt::DisplayRole && !Testing::isAccessibleRole(_role)))
            return QVariant();

        int cur = _index.row();
        if (cur >= rowCount(_index))
            return QVariant();

        if (Testing::isAccessibleRole(_role))
            return results_[cur].toString();

        return QVariant::fromValue(results_[cur]);
    }

    void StatusSearchModel::setSearchPattern(const QString& _pattern)
    {
        searchPattern_ = _pattern.trimmed();
        results_.clear();

        timer_->start();

        Q_EMIT showSearchSpinner();
        Q_EMIT hideNoSearchResults();
    }

    void StatusSearchModel::updateTime(const QString& _status)
    {
        auto iter = std::find_if(results_.begin(), results_.end(), [&_status](const auto& r) { return r.toString() == _status; });
        if (iter != results_.end())
        {
            iter->setDuration(Logic::getStatusModel()->getStatus(_status)->getDefaultDuration());
            const auto idx = index(std::distance(results_.begin(), iter));
            Q_EMIT dataChanged(idx, idx);
        }
    }

    void StatusSearchModel::doSearch()
    {
        auto atExit = qScopeGuard([this]()
        {
          Q_EMIT dataChanged(QModelIndex(), QModelIndex());

          Q_EMIT hideSearchSpinner();
          if (results_.empty())
                  Q_EMIT showNoSearchResults();
        });

        auto statuses = Logic::getStatusModel()->getStatuses();
        if (searchPattern_.isEmpty())
        {
            results_ = std::move(statuses);
            return;
        }

        results_.clear();
        unsigned fixed_patterns_count = 0;
        auto searchPatterns = Utils::GetPossibleStrings(searchPattern_, fixed_patterns_count);
        unsigned i = 0;
        for (; i < fixed_patterns_count; ++i)
        {
            QString pattern;
            if (searchPatterns.empty())
            {
                pattern = searchPattern_;
            }
            else
            {
                pattern.reserve(searchPatterns.size());
                for (const auto& iter : searchPatterns)
                {
                    if (iter.size() > i)
                        pattern += iter.at(i);
                }
                pattern = std::move(pattern).trimmed();
            }

            for (const auto& s : statuses)
            {
                if(s.isEmpty())
                    continue;
                if ((s.getDescription().contains(pattern, Qt::CaseInsensitive) || s.toString().contains(pattern, Qt::CaseInsensitive))
                    && std::find(results_.begin(), results_.end(), s) == results_.end())
                    results_.push_back(s);
            }
        }
    }

    StatusSearchModel *getStatusSearchModel()
    {
        static auto statusModel = std::make_unique<StatusSearchModel>(nullptr);
        return statusModel.get();
    }
}
