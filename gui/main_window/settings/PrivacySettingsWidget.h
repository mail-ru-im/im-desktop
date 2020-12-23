#pragma once

namespace core
{
    enum class privacy_access_right;
}

namespace Logic
{
    class DisallowedInvitersModel;
}

namespace Ui
{
    class LabelEx;
    class SimpleListWidget;

    class PrivacySettingOptions : public QWidget
    {
        Q_OBJECT

    Q_SIGNALS:
        void valueClicked(const QString& _privacyGroup, core::privacy_access_right, QPrivateSignal) const;

    public:
        PrivacySettingOptions(QWidget* _parent, const QString& _settingName, const QString& _caption);

        core::privacy_access_right getValue() const noexcept { return value_; }
        int getIndex() const noexcept;
        void onValueChanged(const QString& _privacyGroup, core::privacy_access_right _value);

        void clearSelection();

    private:
        void updateValue(core::privacy_access_right _value);
        void setIndex(int _index);

        void onClicked(int _index);

    private:
        core::privacy_access_right value_;
        QString name_;

        int prevIndex_;

        SimpleListWidget* list_;
    };

    class PrivacySettingsWidget : public QWidget
    {
        Q_OBJECT

    public:
        PrivacySettingsWidget(QWidget* _parent);

    protected:
        void showEvent(QShowEvent*) override;

    private:
        void onDisallowClicked();
        void updateDisallowLinkVisibility();

    private:
        QWidget* linkContainer_;
        LabelEx* linkDisallow_;

        PrivacySettingOptions* optionsGroups_;
        PrivacySettingOptions* optionsCalls_;

        Logic::DisallowedInvitersModel* invitersModel_;
    };
}