#pragma once

#include <QAbstractListModel>
#include "../../../main_window/sidebar/GalleryList.h"
#include "../../../types/chat.h"

namespace Data
{
    struct GalleryState
    {
        DialogGalleryState counters_;
        Ui::MediaContentType selectedTab_ = Ui::MediaContentType::Invalid;
    };
}

namespace Qml
{
    class GalleryTabsModel : public QAbstractListModel
    {
        Q_OBJECT
        Q_PROPERTY(Ui::MediaContentType selectedTab READ selectedTab WRITE setSelectedTab NOTIFY selectedTabChanged)

    public:
        enum TabRoles
        {
            Name = Qt::UserRole + 1,
            CheckedState,
            ContentType
        };

        explicit GalleryTabsModel(QObject* _parent);

        void selectContact(const QString& _aimId);
        QString selectedContact() const;

        int rowCount(const QModelIndex& _index = QModelIndex()) const override;
        QVariant data(const QModelIndex& _index, int _role) const override;
        QHash<int, QByteArray> roleNames() const override;

        const std::vector<Ui::MediaContentType>& tabTypes() const { return types_; };

    public Q_SLOTS:
        Ui::MediaContentType selectedTab() const;
        void setSelectedTab(Ui::MediaContentType _currentType);

        int tabIndex(Ui::MediaContentType _tabType) const;

    Q_SIGNALS:
        void selectedTabChanged(Ui::MediaContentType _selectedTab);
        void countChanged();
        void tabClicked(Ui::MediaContentType _selectedTab);

    private:
        void updateGalleryModelState(const Data::GalleryState& _state);

    private Q_SLOTS:
        void setGalleryState(const QString& _aimId, const Data::DialogGalleryState& _state);
        void setSelectedTabToState(Ui::MediaContentType _selectedTab);

    private:
        std::unordered_map<QString, Data::GalleryState> allStates_;
        QString selectedContact_;

        std::vector<Ui::MediaContentType> types_;
        Ui::MediaContentType currentType_ = Ui::MediaContentType::Invalid;
    };
} // namespace Qml
