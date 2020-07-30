#include "stdafx.h"
#include "CallsSearchModel.h"
#include "CallsModel.h"
#include "../../utils/utils.h"
#include "../../core_dispatcher.h"
#include "../containers/StatusContainer.h"

namespace
{
    constexpr std::chrono::milliseconds typingTimeout = std::chrono::milliseconds(300);
}

namespace Logic
{
    CallsSearchModel::CallsSearchModel(QObject* _parent)
        : AbstractSearchModel(_parent)
        , timer_(new QTimer(this))
    {
        timer_->setSingleShot(true);
        timer_->setInterval(typingTimeout.count());
        connect(timer_, &QTimer::timeout, this, &CallsSearchModel::doSearch);
        connect(Logic::GetStatusContainer(), &Logic::StatusContainer::statusChanged, this, [this]()
        {
            Q_EMIT dataChanged(QModelIndex(), QModelIndex());
        });
    }

    CallsSearchModel::~CallsSearchModel() = default;

    int CallsSearchModel::rowCount(const QModelIndex& _parent) const
    {
        return results_.size();
    }

    QVariant CallsSearchModel::data(const QModelIndex& _index, int _role) const
    {
        const auto i = _index.row();
        if (i >= 0 && i <= rowCount())
        {
            return QVariant::fromValue(std::make_shared<Data::CallInfo>(results_[i]));
        }

        return QVariant();
    }

    void CallsSearchModel::setSearchPattern(const QString& _pattern)
    {
        searchPattern_ = _pattern;
        results_.clear();

        lastSearchPattern_ = _pattern;
        timer_->start();

        Q_EMIT showSearchSpinner();
        Q_EMIT hideNoSearchResults();
    }

    void CallsSearchModel::doSearch()
    {
        auto calls = results_.empty() ? Logic::GetCallsModel()->getCalls() : results_;
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

            for (auto c : calls)
            {
                if (c.getFriendly().contains(pattern, Qt::CaseInsensitive) || std::find_if(c.Members_.begin(), c.Members_.end(), [pattern](const auto& iter) { return iter.contains(pattern, Qt::CaseInsensitive); }) != c.Members_.end())
                {
                    c.Highlights_.push_back(pattern);
                    results_.push_back(c);
                }
            }
        }

        Q_EMIT dataChanged(QModelIndex(), QModelIndex());

        Q_EMIT hideSearchSpinner();
        if (results_.empty())
            Q_EMIT showNoSearchResults();
    }
}
