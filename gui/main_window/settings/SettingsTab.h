#pragma once

namespace Utils
{
    enum class CommonSettingsType;
}

namespace Ui
{
    class ScrollAreaWithTrScrollBar;

    class SettingsTab : public QWidget
    {
        class UI;

        Q_OBJECT;

    private Q_SLOTS:
        void compactModeChanged();

    private:
        std::unique_ptr< UI > ui_;
        Utils::CommonSettingsType currentSettingsItem_;
        bool isCompact_;
        ScrollAreaWithTrScrollBar* scrollArea_;

    public:
        explicit SettingsTab(QWidget* _parent = nullptr);
        ~SettingsTab();

        Utils::CommonSettingsType currentSettingsItem() const noexcept { return currentSettingsItem_; }

        void selectSettings(Utils::CommonSettingsType _type);

        void cleanSelection();
        void setCompactMode(bool isCompact, bool force = false);

    private:
        void settingsProfileClicked();
        void settingsGeneralClicked();
        void settingsVoiceVideoClicked();
        void settingsNotificationsClicked();
        void settingsAppearanceClicked();
        void settingsAboutAppClicked();
        void settingsContactUsClicked();
        void settingsLanguageClicked();
        void settingsHotkeysClicked();
        void settingsSecurityClicked();
        void settingsStickersClicked();

        void setCurrentItem(const Utils::CommonSettingsType _item);

        void updateSettingsState();

        void listClicked(int _index);

        void appUpdateReady();

        Utils::CommonSettingsType getTypeByIndex(int _index) const;
        int getIndexByType(Utils::CommonSettingsType _type) const;
    };
}