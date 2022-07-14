#pragma once

#include "MiniApps.h"

namespace Logic
{
    using MiniAppMap = std::unordered_map<QString, Ui::MiniApp>;
    using MiniAppVector = std::vector<Ui::MiniApp>;

    enum class KeepWebPage
    {
        Yes,
        No
    };

    enum class UpdateCommand
    {
        None,
        Delete,
        Update
    };

    class MiniAppsContainer : public QObject
    {
        Q_OBJECT

    Q_SIGNALS:
        void appIconUpdated(const QString& _id);

        void updateApps(const QString& _selectedId);
        void updateAppFields(const QString& _id);
        void removeTab(const QString& _id);
        void removeApp(const QString& _id, KeepWebPage _keepAppPage = KeepWebPage::No);
        void removeHiddenPage(const QString& _id);

        void addFeedContact(const QString& _aimId);
        void removeFeedContact(const QString& _aimId);

    public:
        explicit MiniAppsContainer(QObject* _parent = nullptr);
        ~MiniAppsContainer() = default;

        void initApps();
        void updateAppsConfig();
        const std::vector<QString>& getAppsOrder();

        const Ui::MiniApp& getApp(const QString& _type) const;
        bool isAppEnabled(const QString& _id);
        bool isCustomApp(const QString& _id);

        void setCurrentApp(const QString& _id);

    public Q_SLOTS:
        void onAppHidden(const QString& _id);

    private:
        MiniAppMap getServiceApps();
        void initCustomApps();
        std::vector<QString> getEnabledMiniApps(const QString& _text);

        MiniAppMap createDefaultApps();

        void clearUpdatable();

    private:
        MiniAppMap apps_;
        std::vector<QString> appsOrder_;

        QString currentApp_;

        struct UpdatableApp
        {
            QString id_;
            UpdateCommand command_;
        } updatableApp_;
    };

    MiniAppsContainer* GetAppsContainer();
    void ResetAppsContainer();
}
