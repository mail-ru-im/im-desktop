#include "stdafx.h"
#include "TaskAssigneeModel.h"
#include "../containers/LastseenContainer.h"

namespace
{
    struct NoAssigneSearchResult : Data::AbstractSearchResult
    {
        Data::SearchResultType getType() const override { return Data::SearchResultType::contact; }
        QString getAimId() const override
        {
            return Logic::TaskAssigneeModel::noAssigneeAimId();
        }
        QString getFriendlyName() const override
        {
            return QT_TRANSLATE_NOOP("task", "Unassigned");
        }
    };
}

namespace Logic
{
    TaskAssigneeModel::TaskAssigneeModel(QObject* _parent)
        : SearchModel(_parent)
    {
        setSearchInDialogs(false);
        setExcludeChats(Logic::SearchDataSource::localAndServer);
        setHideReadonly(true);
        setServerSearchEnabled(true);
        setCategoriesEnabled(false);
        setSortByTime(true);
        setFavoritesOnTop(false);
        setForceAddFavorites(false);
    }

    TaskAssigneeModel::~TaskAssigneeModel() = default;

    void TaskAssigneeModel::setSelectedContact(const QString& _aimId)
    {
        selectedContact_ = _aimId;
    }

    QString TaskAssigneeModel::noAssigneeAimId()
    {
        return qsl("_~no_assignee~_");
    }

    void TaskAssigneeModel::modifyResultsBeforeEmit(Data::SearchResultsV& _results)
    {

        _results.erase(std::remove_if(_results.begin(), _results.end(),
            [](const Data::AbstractSearchResultSptr& _result)
            {
                return Logic::GetLastseenContainer()->isBot(_result->getAimId());
            }),
            _results.end());

        if (!selectedContact_.isEmpty())
        {
            auto selected = std::find_if(_results.begin(), _results.end(), [this](const Data::AbstractSearchResultSptr& _result)
            {
                return _result->getAimId() == selectedContact_;
            });
            if (selected != _results.end())
                _results.erase(selected);

            auto emptyContact = std::make_shared<NoAssigneSearchResult>();

            _results.prepend(emptyContact);
        }
    }
}
