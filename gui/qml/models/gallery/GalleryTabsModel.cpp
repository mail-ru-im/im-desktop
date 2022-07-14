#include "stdafx.h"
#include "GalleryTabsModel.h"

#include "../../../types/chat.h"
#include "../../../main_window/sidebar/GalleryList.h"
#include "../../../core_dispatcher.h"

Qml::GalleryTabsModel::GalleryTabsModel(QObject* _parent)
    : QAbstractListModel(_parent)
{
    types_.reserve(Ui::supportedMediaContentTypes.size());

    connect(Ui::GetDispatcher(), &Ui::core_dispatcher::dialogGalleryState, this, &GalleryTabsModel::setGalleryState);
    connect(this, &GalleryTabsModel::selectedTabChanged, this, &GalleryTabsModel::setSelectedTabToState);
}

void Qml::GalleryTabsModel::selectContact(const QString& _aimId)
{
    if (_aimId == selectedContact_)
        return;

    if (const auto it = allStates_.find(_aimId); it != allStates_.end())
    {
        selectedContact_ = _aimId;
        updateGalleryModelState(it->second);
    }
}

QString Qml::GalleryTabsModel::selectedContact() const
{
    return selectedContact_;
}

int Qml::GalleryTabsModel::rowCount(const QModelIndex& _index) const
{
    return types_.size();
}

QVariant Qml::GalleryTabsModel::data(const QModelIndex& _index, int _role) const
{
    const auto rowIndex = _index.row();
    const auto type = types_.at(rowIndex);
    switch (_role)
    {
    case TabRoles::Name:
        return Ui::getGalleryTitle(type);
    case TabRoles::CheckedState:
        return type == currentType_;
    case TabRoles::ContentType:
    case Qt::DisplayRole:
        return static_cast<int>(type);
    }
    if (Testing::isAccessibleRole(_role))
        return QString(u"AS GalleryTab " % Ui::getGalleryTitle(type));
    return {};
}

QHash<int, QByteArray> Qml::GalleryTabsModel::roleNames() const
{
    QHash<int, QByteArray> hash;
    hash[Name] = "tabName";
    hash[CheckedState] = "tabSelected";
    hash[ContentType] = "contentType";
    return hash;
}

Ui::MediaContentType Qml::GalleryTabsModel::selectedTab() const
{
    return currentType_;
}

void Qml::GalleryTabsModel::setSelectedTab(Ui::MediaContentType _currentType)
{
    if (currentType_ == _currentType)
    {
        tabClicked(_currentType);
        return;
    }

    if (const auto it = std::find(types_.begin(), types_.end(), _currentType); it != types_.end())
    {
        const auto previousTypeIt = std::find(types_.begin(), types_.end(), currentType_);

        currentType_ = _currentType;
        Q_EMIT selectedTabChanged(_currentType);

        const QVector<int> roles = { TabRoles::CheckedState };

        const auto newIndex = std::distance(types_.begin(), it);
        const auto changedIndex = createIndex(newIndex, 0);

        if (previousTypeIt != types_.end())
        {
            const auto previousIndex = std::distance(types_.begin(), previousTypeIt);
            const auto previousModelIndex = createIndex(previousIndex, 0);
            const auto& minIndex = previousIndex < newIndex ? previousModelIndex : changedIndex;
            const auto& maxIndex = previousIndex > newIndex ? previousModelIndex : changedIndex;
            Q_EMIT dataChanged(minIndex, maxIndex, roles);
        }
        else
        {
            Q_EMIT dataChanged(changedIndex, changedIndex, roles);
        }
    }
}

int Qml::GalleryTabsModel::tabIndex(Ui::MediaContentType _tabType) const
{
    if (const auto it = std::find(types_.cbegin(), types_.cend(), _tabType); it != types_.cend())
        return std::distance(types_.cbegin(), it);
    return -1;
}

void Qml::GalleryTabsModel::updateGalleryModelState(const Data::GalleryState& _state)
{
    const auto previousCount = rowCount();
    beginResetModel();
    currentType_ = _state.selectedTab_;

    types_.clear();
    std::copy_if(Ui::supportedMediaContentTypes.cbegin(), Ui::supportedMediaContentTypes.cend(), std::back_inserter(types_), [&_state](auto _type) { return Ui::countForType(_state.counters_, _type); });

    if (!Ui::countForType(_state.counters_, currentType_))
    {
        if (types_.empty())
        {
            currentType_ = Ui::MediaContentType::Invalid;
        }
        else
        {
            const auto previousType = std::exchange(currentType_, types_.front());
            for (auto type = ++types_.cbegin(); type != types_.cend(); ++type)
            {
                if (*type < previousType)
                    currentType_ = *type;
                else
                    break;
            }
        }
        Q_EMIT selectedTabChanged(currentType_);
    }

    endResetModel();

    if (previousCount != rowCount())
        Q_EMIT countChanged();
}

void Qml::GalleryTabsModel::setGalleryState(const QString& _aimId, const Data::DialogGalleryState& _state)
{
    allStates_[_aimId].counters_ = _state;
    if (_aimId == selectedContact_)
        updateGalleryModelState({ _state, currentType_ });
}


void Qml::GalleryTabsModel::setSelectedTabToState(Ui::MediaContentType _selectedTab)
{
    if (!selectedContact_.isEmpty())
        allStates_[selectedContact_].selectedTab_ = _selectedTab;
}
