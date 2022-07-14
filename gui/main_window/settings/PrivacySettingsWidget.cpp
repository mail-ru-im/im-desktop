#include "stdafx.h"

#include "PrivacySettingsWidget.h"
#include "core_dispatcher.h"

#include "main_window/MainPage.h"
#include "main_window/AppsPage.h"
#include "main_window/sidebar/SidebarUtils.h"
#include "main_window/contact_list/ContactListModel.h"
#include "main_window/contact_list/SelectionContactsForGroupChat.h"
#include "main_window/contact_list/ContactListUtils.h"
#include "main_window/contact_list/DisallowedInvitersModel.h"
#include "main_window/contact_list/DisallowedInvitersContactsModel.h"

#include "main_window/containers/PrivacySettingsContainer.h"

#include "controls/GeneralCreator.h"
#include "controls/SimpleListWidget.h"
#include "controls/RadioTextRow.h"
#include "controls/LabelEx.h"

#include "styles/ThemeParameters.h"
#include "utils/features.h"
#include "utils/InterConnector.h"
#include "previewer/toast.h"

namespace
{
    int privacyBottomSpacer() { return Utils::scale_value(40); }
    QString groupDisallowButtonCaption() { return QT_TRANSLATE_NOOP("settings", "Exceptions list"); }
    QString groupDisallowDialogCaption() { return QT_TRANSLATE_NOOP("settings", "Forbid to add me to groups"); }

    core::privacy_access_right indexToValue(int _index)
    {
        const auto& varList = Utils::getPrivacyAllowVariants();
        if (_index < 0 || _index >= int(varList.size()))
            return core::privacy_access_right::not_set;

        return varList[_index].second;
    }

    void sendStats(const QString& _name, core::privacy_access_right _value)
    {
        const auto statName = _name == u"groups"
                              ? core::stats::stats_event_names::privacyscr_grouptype_action
                              : core::stats::stats_event_names::privacyscr_calltype_action;

        std::string statVal;
        switch (_value)
        {
            case core::privacy_access_right::everybody:
                statVal = "everyone";
                break;
            case core::privacy_access_right::my_contacts:
                statVal = "friends";
                break;
            case core::privacy_access_right::nobody:
                statVal = "no_one";
                break;
            default:
                im_assert(!"invalid value");
                break;
        }
        Ui::GetDispatcher()->post_stats_to_core(statName, { {"type", statVal } });
    }

    void showErrorToast()
    {
        Utils::showTextToastOverContactDialog(QT_TRANSLATE_NOOP("settings", "Error occurred"));
    }

    bool shouldOptOutSms(core::privacy_access_right right)
    {
        return core::privacy_access_right::nobody == right;
    }
}

namespace Ui
{
    PrivacySettingOptions::PrivacySettingOptions(QWidget* _parent, const QString& _settingName, const QString& _caption)
        : QWidget(_parent)
        , value_(core::privacy_access_right::not_set)
        , name_(_settingName)
        , prevIndex_(-1)
    {
        auto headerWidget = new QWidget(this);
        auto headerLayout = Utils::emptyVLayout(headerWidget);
        Utils::grabTouchWidget(headerWidget);
        headerLayout->setAlignment(Qt::AlignTop);
        headerLayout->setContentsMargins(0, 0, 0, 0);

        GeneralCreator::addHeader(headerWidget, headerLayout, _caption, 20);

        auto mainLayout = Utils::emptyVLayout(this);

        mainLayout->addWidget(headerWidget);
        mainLayout->addSpacing(Utils::scale_value(8));

        auto innerWidget = new QWidget(this);
        Utils::grabTouchWidget(innerWidget);
        auto innerLayout = Utils::emptyVLayout(innerWidget);
        innerLayout->setAlignment(Qt::AlignTop);
        innerLayout->setSpacing(Utils::scale_value(12));

        list_ = new SimpleListWidget(Qt::Vertical);
        for (const auto& [name, type] : Utils::getPrivacyAllowVariants())
        {
            auto radioTextRow = new RadioTextRow(name);
            if (Testing::isEnabled())
            {
                auto typeQString = QString::fromStdString(to_string(type));
                if (!typeQString.isEmpty())
                    typeQString[0] = typeQString.at(0).toUpper();
                Testing::setAccessibleName(radioTextRow, qsl("AS PrivacyPage ") % _settingName % typeQString);
            }
            list_->addItem(radioTextRow);
        }

        innerLayout->addWidget(list_);
        mainLayout->addWidget(innerWidget);

        connect(list_, &SimpleListWidget::clicked, this, &PrivacySettingOptions::onClicked);

        Testing::setAccessibleName(this, qsl("AS PrivacyPage ") % _settingName);
    }

    int PrivacySettingOptions::getIndex() const noexcept
    {
        return list_->getCurrentIndex();
    }

    void PrivacySettingOptions::onValueChanged(const QString& _privacyGroup, const core::privacy_access_right _value)
    {
        if (_privacyGroup.isEmpty() || _privacyGroup == name_)
            updateValue(_value);
    }

    void PrivacySettingOptions::clearSelection()
    {
        list_->clearSelection();
    }

    void PrivacySettingOptions::updateValue(core::privacy_access_right _value)
    {
        auto idx = -1;
        if (_value != core::privacy_access_right::not_set)
        {
            const auto& varList = Utils::getPrivacyAllowVariants();
            const auto it = std::find_if(varList.begin(), varList.end(), [_value](const auto _p) { return _p.second == _value; });
            if (it != varList.end())
                idx = std::distance(varList.begin(), it);
        }
        else if (prevIndex_ >= 0)
        {
            idx = prevIndex_;
        }
        prevIndex_ = -1;

        value_ = indexToValue(idx);
        setIndex(idx);
    }

    void PrivacySettingOptions::setIndex(int _index)
    {
        if (list_->isValidIndex(_index))
            list_->setCurrentIndex(_index);
        else
            list_->clearSelection();
    }

    void PrivacySettingOptions::onClicked(int _index)
    {
        if (!list_->isValidIndex(_index))
            return;

        prevIndex_ = list_->getCurrentIndex();
        list_->setCurrentIndex(_index);

        const auto val = indexToValue(_index);
        sendStats(name_, val);
        Q_EMIT valueClicked(name_, val, QPrivateSignal());
    }


    PrivacySettingsWidget::PrivacySettingsWidget(QWidget* _parent)
        : QWidget(_parent)
        , invitersModel_(new Logic::DisallowedInvitersModel(this))
    {
        auto layout = Utils::emptyVLayout(this);

        optionsGroups_ = new PrivacySettingOptions(this, qsl("groups"), QT_TRANSLATE_NOOP("settings", "Who can add me to groups"));
        layout->addWidget(optionsGroups_, 0, Qt::AlignTop);

        {
            linkDisallow_ = new Ui::LabelEx(this);
            Testing::setAccessibleName(linkDisallow_, qsl("AS PrivacyPage Disallowed"));
            linkDisallow_->setFont(Fonts::appFontScaled(15));
            linkDisallow_->setColors(Styling::ThemeColorKey{ Styling::StyleVariable::TEXT_PRIMARY },
                Styling::ThemeColorKey{ Styling::StyleVariable::TEXT_PRIMARY_HOVER },
                Styling::ThemeColorKey{ Styling::StyleVariable::TEXT_PRIMARY_ACTIVE });
            linkDisallow_->setText(groupDisallowButtonCaption());
            linkDisallow_->setCursor(Qt::PointingHandCursor);
            Utils::grabTouchWidget(linkDisallow_);

            linkContainer_ = new QWidget(this);
            auto contLayout = Utils::emptyHLayout(linkContainer_);
            contLayout->setContentsMargins(Utils::scale_value(20), Utils::scale_value(20), 0, 0);
            contLayout->addWidget(linkDisallow_);
            layout->addWidget(linkContainer_, 0, Qt::AlignTop);
        }

        layout->addSpacing(privacyBottomSpacer());

        optionsCalls_ = new PrivacySettingOptions(this, qsl("calls"), QT_TRANSLATE_NOOP("settings", "Who can call me"));
        layout->addWidget(optionsCalls_, 0, Qt::AlignTop);

        layout->addSpacing(privacyBottomSpacer());

        if (Features::shouldShowSmsNotifySetting())
        {
            auto optOutSmsCheckbox = GeneralCreator::addSwitcher(
                this,
                layout,
                QT_TRANSLATE_NOOP("settings", "Disable SMS notifications"),
                shouldOptOutSms(Logic::GetPrivacySettingsContainer()->getCachedValue(qsl("smsNotify"))));
            Testing::setAccessibleName(static_cast<QWidget*>(optOutSmsCheckbox), qsl("AS PrivacyPage optOutSmsSetting"));

            QObject::connect(Logic::GetPrivacySettingsContainer(), &Logic::PrivacySettingsContainer::valueChanged, optOutSmsCheckbox,[optOutSmsCheckbox](const QString& _privacyGroup, core::privacy_access_right _value)
            {
                if (_privacyGroup == u"smsNotify")
                    optOutSmsCheckbox->setChecked(shouldOptOutSms(_value));
            });

            QObject::connect(optOutSmsCheckbox, &Ui::SidebarCheckboxButton::clicked, optOutSmsCheckbox, [optOutSmsCheckbox]()
            {
                const auto isChecked = optOutSmsCheckbox->isChecked();
                const auto newPrivacyValue = isChecked ? core::privacy_access_right::nobody : core::privacy_access_right::everybody;
                Ui::GetDispatcher()->post_stats_to_core(core::stats::stats_event_names::chatscr_block_sms_invite, { { "type", isChecked ? "yes" : "no" }, });
                Logic::GetPrivacySettingsContainer()->setValue(qsl("smsNotify"), newPrivacyValue);
            });

            layout->addSpacing(privacyBottomSpacer());
        };

        updateDisallowLinkVisibility();

        connect(optionsGroups_, &PrivacySettingOptions::valueClicked, Logic::GetPrivacySettingsContainer(), &Logic::PrivacySettingsContainer::setValue);
        connect(optionsCalls_, &PrivacySettingOptions::valueClicked, Logic::GetPrivacySettingsContainer(), &Logic::PrivacySettingsContainer::setValue);

        connect(Logic::GetPrivacySettingsContainer(), &Logic::PrivacySettingsContainer::valueChanged, optionsGroups_, &PrivacySettingOptions::onValueChanged);
        connect(Logic::GetPrivacySettingsContainer(), &Logic::PrivacySettingsContainer::valueChanged, optionsCalls_, &PrivacySettingOptions::onValueChanged);
        connect(Logic::GetPrivacySettingsContainer(), &Logic::PrivacySettingsContainer::valueChanged, this, &PrivacySettingsWidget::updateDisallowLinkVisibility);

        connect(&Utils::InterConnector::instance(), &Utils::InterConnector::omicronUpdated, this, &PrivacySettingsWidget::updateDisallowLinkVisibility);
        connect(GetDispatcher(), &core_dispatcher::externalUrlConfigUpdated, this, &PrivacySettingsWidget::updateDisallowLinkVisibility);
        connect(GetDispatcher(), &core_dispatcher::inviterBlacklistSearchError, showErrorToast);

        connect(linkDisallow_, &LabelEx::clicked, this, &PrivacySettingsWidget::onDisallowClicked);

        connect(GetDispatcher(), &core_dispatcher::inviterBlacklistRemoveResult, this, [this](const auto& _contacts, const bool _success)
        {
            if (_success)
                invitersModel_->remove(_contacts);
            else
                showErrorToast();
        });
        connect(GetDispatcher(), &core_dispatcher::inviterBlacklistAddResult, this, [this](const auto& _contacts, const bool _success)
        {
            if (_success)
                invitersModel_->add(_contacts);
            else
                showErrorToast();

            invitersModel_->hideSpinner();
        });
    }

    void PrivacySettingsWidget::showEvent(QShowEvent*)
    {
        optionsGroups_->clearSelection();
        optionsCalls_->clearSelection();
        Logic::GetPrivacySettingsContainer()->requestValues();
    }

    void PrivacySettingsWidget::onDisallowClicked()
    {
        invitersModel_->setForceSearch();
        invitersModel_->setSearchPattern(QString());

        SelectContactsWidgetOptions optionsList;
        optionsList.searchModel_ = invitersModel_;
        optionsList.withSemiwindow_ = false;

        SelectContactsWidget dialogList(
            invitersModel_,
            Logic::MembersWidgetRegim::DISALLOWED_INVITERS,
            groupDisallowDialogCaption(),
            QT_TRANSLATE_NOOP("popup_window", "Add"),
            this,
            optionsList);

        connect(invitersModel_, &Logic::DisallowedInvitersModel::showEmptyListPlaceholder, &dialogList, [&dialogList]()
        {
            dialogList.setEmptyLabelVisible(true);
        });

        connect(invitersModel_, &Logic::DisallowedInvitersModel::hideNoSearchResults, &dialogList, [&dialogList]()
        {
            dialogList.setEmptyLabelVisible(false);
        });

        std::function<void()> showList;
        showList = [&dialogList, this, &showList]()
        {
            if (dialogList.show())
            {
                SelectContactsWidgetOptions optionsAdd;
                optionsAdd.withSemiwindow_ = false;

                Logic::DisallowedInvitersContactsModel disModel;
                connect(&disModel, &Logic::DisallowedInvitersContactsModel::requestFailed, showErrorToast);

                SelectContactsWidget dialogAdd(
                    &disModel,
                    Logic::MembersWidgetRegim::DISALLOWED_INVITERS_ADD,
                    QT_TRANSLATE_NOOP("popup_window", "Add to list"),
                    QT_TRANSLATE_NOOP("popup_window", "Select"),
                    this,
                    optionsAdd);

                const auto added = dialogAdd.show();
                const auto selectedContacts = Logic::getContactListModel()->getCheckedContacts();
                Logic::getContactListModel()->clearCheckedItems();
                Logic::getContactListModel()->removeTemporaryContactsFromModel();

                if (added)
                {
                    Ui::GetDispatcher()->addToInvitersBlacklist(selectedContacts);
                    invitersModel_->showSpinner();
                }

                showList();
            }
        };

        Utils::InterConnector::instance().getAppsPage()->showSemiWindow();
        showList();
        Logic::GetPrivacySettingsContainer()->requestValues();
        Utils::InterConnector::instance().getAppsPage()->hideSemiWindow();
    }

    void PrivacySettingsWidget::updateDisallowLinkVisibility()
    {
        const auto visible =
            Features::isGroupInvitesBlacklistEnabled() &&
            optionsGroups_->getValue() != core::privacy_access_right::nobody &&
            optionsGroups_->getValue() != core::privacy_access_right::not_set;
        linkContainer_->setVisible(visible);
    }
}

