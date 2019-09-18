#include "stdafx.h"
#include "SettingsTab.h"

#include "../contact_list/Common.h"
#include "../../core_dispatcher.h"
#include "../../gui_settings.h"
#include "../../utils/utils.h"
#include "../../utils/InterConnector.h"
#include "../../utils/gui_coll_helper.h"
#include "../../controls/CustomButton.h"
#include "../../controls/LabelEx.h"
#include "../../controls/TransparentScrollBar.h"
#include "../../controls/SimpleListWidget.h"
#include "../contact_list/TopPanel.h"
#include "../../utils/log/log.h"
#include "styles/StyleVariable.h"

#include "SimpleSettingsRow.h"
#include "ProfileSettingsRow.h"
#include "UpdaterSettingsRow.h"
#include "SpacerSettingsRow.h"

namespace
{
    constexpr int LEFT_OFFSET = 12;
    constexpr int BACK_WIDTH = 52;
    constexpr int TOP_PANEL_HEIGHT = 56;

    bool isClickableType(const Utils::CommonSettingsType _type)
    {
        return
            _type != Utils::CommonSettingsType::CommonSettingsType_Updater &&
            _type != Utils::CommonSettingsType::CommonSettingsType_None;
    }
}

namespace Ui
{
    class SettingsTab::UI
    {
    private:
        LabelEx* topicLabel_ = nullptr;
        HeaderTitleBar* topWidget_ = nullptr;

        SimpleListWidget* settingsList_ = nullptr;
        SimpleListItem* updatePlate_ = nullptr;
        SimpleListItem* updateSpacer_ = nullptr;

        std::vector<std::pair<int, Utils::CommonSettingsType>> indexToType_;

        friend class SettingsTab;

    public:
        void init(QWidget* _p, QLayout* _headerLayout, QLayout* _contentLayout)
        {
            topWidget_ = new HeaderTitleBar(_p);
            topWidget_->setTitle(QT_TRANSLATE_NOOP("head", "Settings"));
            Testing::setAccessibleName(topWidget_, qsl("AS settingstab topWidget_"));
            _headerLayout->addWidget(topWidget_);

            settingsList_ = new SimpleListWidget(Qt::Vertical);

            const auto addRow = [this](auto _row, const auto _type, const auto& _accName)
            {
                Testing::setAccessibleName(_row, qsl("AS settingsRow ") % _accName);
                indexToType_.emplace_back(settingsList_->addItem(_row), _type);
                return _row;
            };

            const auto addSpacer = [addRow]()
            {
                return addRow(new SpacerSettingsRow(), Utils::CommonSettingsType::CommonSettingsType_None, qsl("spacer"));
            };

            const auto addSimpleRow = [addRow](const auto& _icon, const auto _bg, const auto& _caption, const auto _type)
            {
                return addRow(new SimpleSettingsRow(qsl(":/settings/") % _icon, _bg, _caption), _type, _icon);
            };

            addRow(new ProfileSettingsRow(), Utils::CommonSettingsType::CommonSettingsType_Profile, qsl("Profile"));
            addSpacer();

            updatePlate_ = addRow(new UpdaterSettingsRow(), Utils::CommonSettingsType::CommonSettingsType_Updater, qsl("Updater"));
            updateSpacer_ = addSpacer();

            addSimpleRow(qsl("general"), Styling::StyleVariable::SECONDARY_RAINBOW_MINT, QT_TRANSLATE_NOOP("settings", "General"), Utils::CommonSettingsType::CommonSettingsType_General);
            addSimpleRow(qsl("notifications"), Styling::StyleVariable::SECONDARY_ATTENTION, QT_TRANSLATE_NOOP("settings", "Notifications"), Utils::CommonSettingsType::CommonSettingsType_Notifications);
            addSimpleRow(qsl("lock"), Styling::StyleVariable::BASE_SECONDARY, QT_TRANSLATE_NOOP("settings", "Privacy"), Utils::CommonSettingsType::CommonSettingsType_Security);
            addSimpleRow(qsl("appearance"), Styling::StyleVariable::SECONDARY_RAINBOW_COLD, QT_TRANSLATE_NOOP("settings", "Appearance"), Utils::CommonSettingsType::CommonSettingsType_Appearance);
            addSimpleRow(qsl("sticker"), Styling::StyleVariable::SECONDARY_RAINBOW_ORANGE, QT_TRANSLATE_NOOP("settings", "Stickers"), Utils::CommonSettingsType::CommonSettingsType_Stickers);
            addSpacer();

#ifndef STRIP_VOIP
            addSimpleRow(qsl("video"), Styling::StyleVariable::SECONDARY_RAINBOW_AQUA, QT_TRANSLATE_NOOP("settings", "Voice and video"), Utils::CommonSettingsType::CommonSettingsType_VoiceVideo);
#endif //STRIP_VOIP
            addSimpleRow(qsl("shortcuts"), Styling::StyleVariable::SECONDARY_RAINBOW_PURPLE, QT_TRANSLATE_NOOP("settings", "Shortcuts"), Utils::CommonSettingsType::CommonSettingsType_Shortcuts);
            addSimpleRow(qsl("language"), Styling::StyleVariable::SECONDARY_RAINBOW_PINK, QT_TRANSLATE_NOOP("settings", "Language"), Utils::CommonSettingsType::CommonSettingsType_Language);
            addSpacer();

            addSimpleRow(qsl("report"), Styling::StyleVariable::SECONDARY_RAINBOW_BLUE, QT_TRANSLATE_NOOP("settings", "Contact Us"), Utils::CommonSettingsType::CommonSettingsType_ContactUs);
            addSimpleRow(build::ProductNameShort(), Styling::StyleVariable::SECONDARY_RAINBOW_GREEN, QT_TRANSLATE_NOOP("settings", "About app"), Utils::CommonSettingsType::CommonSettingsType_About);

            Testing::setAccessibleName(settingsList_, qsl("AS settingstab settingsList"));
            _contentLayout->addWidget(settingsList_);
        }
    };

    SettingsTab::SettingsTab(QWidget* _parent)
        : QWidget(_parent)
        , ui_(new UI())
        , currentSettingsItem_(Utils::CommonSettingsType::CommonSettingsType_None)
        , isCompact_(false)
        , scrollArea_(CreateScrollAreaAndSetTrScrollBarV(this))
    {
        Testing::setAccessibleName(scrollArea_, qsl("AS settingstab scrollArea"));
        scrollArea_->setObjectName(qsl("scroll_area"));
        scrollArea_->setWidgetResizable(true);
        scrollArea_->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        Utils::grabTouchWidget(scrollArea_->viewport(), true);

        auto scrollAreaContent = new QWidget(scrollArea_);
        scrollAreaContent->setObjectName(qsl("settings_view"));
        Utils::grabTouchWidget(scrollAreaContent);

        auto scrollAreaContentLayout = Utils::emptyVLayout(scrollAreaContent);
        scrollAreaContentLayout->setAlignment(Qt::AlignTop);

        scrollArea_->setWidget(scrollAreaContent);

        auto layout = Utils::emptyVLayout(this);
        ui_->init(scrollAreaContent, layout, scrollAreaContentLayout);
        layout->addWidget(scrollArea_);

        connect(&Utils::InterConnector::instance(), &Utils::InterConnector::compactModeChanged, this, &SettingsTab::compactModeChanged);
        connect(ui_->settingsList_, &SimpleListWidget::clicked, this, &SettingsTab::listClicked);

        connect(Ui::GetDispatcher(), &Ui::core_dispatcher::updateReady, this, &SettingsTab::appUpdateReady);

        ui_->updatePlate_->hide();
        ui_->updateSpacer_->hide();
    }

    SettingsTab::~SettingsTab()
    {
        //
    }

    void SettingsTab::selectSettings(Utils::CommonSettingsType _type)
    {
        switch (_type)
        {
        case Utils::CommonSettingsType::CommonSettingsType_Profile:
            settingsProfileClicked();
            break;
        case Utils::CommonSettingsType::CommonSettingsType_VoiceVideo:
            settingsVoiceVideoClicked();
            break;
        case Utils::CommonSettingsType::CommonSettingsType_Notifications:
            settingsNotificationsClicked();
            break;
        case Utils::CommonSettingsType::CommonSettingsType_Appearance:
            settingsAppearanceClicked();
            break;
        case Utils::CommonSettingsType::CommonSettingsType_About:
            settingsAboutAppClicked();
            break;
        case Utils::CommonSettingsType::CommonSettingsType_ContactUs:
            settingsContactUsClicked();
            break;
        case Utils::CommonSettingsType::CommonSettingsType_Language:
            settingsLanguageClicked();
            break;
        case Utils::CommonSettingsType::CommonSettingsType_Shortcuts:
            settingsHotkeysClicked();
            break;

        case Utils::CommonSettingsType::CommonSettingsType_Security:
            settingsSecurityClicked();
            break;

        case Utils::CommonSettingsType::CommonSettingsType_Stickers:
            settingsStickersClicked();
            break;

        case Utils::CommonSettingsType::CommonSettingsType_General:
        default:
            settingsGeneralClicked();
            break;
        }
    }

    void SettingsTab::cleanSelection()
    {
        emit Utils::InterConnector::instance().popPagesToRoot();

        currentSettingsItem_ = Utils::CommonSettingsType::CommonSettingsType_None;
        updateSettingsState();
    }

    void SettingsTab::setCompactMode(bool isCompact, bool force)
    {
        if (isCompact == isCompact_ && !force)
            return;

        isCompact_ = isCompact;
        ui_->topWidget_->setVisible(!isCompact_);

        for (int i = 0, size = ui_->settingsList_->count(); i < size; ++i)
            ui_->settingsList_->itemAt(i)->setCompactMode(isCompact_);

        ui_->updateSpacer_->setVisible(ui_->updatePlate_->isVisible());
    }

    void SettingsTab::settingsProfileClicked()
    {
        setCurrentItem(Utils::CommonSettingsType::CommonSettingsType_Profile);

        emit Utils::InterConnector::instance().profileSettingsShow(QString());
        emit Utils::InterConnector::instance().showSettingsHeader(QT_TRANSLATE_NOOP("main_page", "My profile"));
    }

    void SettingsTab::settingsGeneralClicked()
    {
        setCurrentItem(Utils::CommonSettingsType::CommonSettingsType_General);

        emit Utils::InterConnector::instance().showSettingsHeader(QT_TRANSLATE_NOOP("main_page", "General settings"));
        emit Utils::InterConnector::instance().generalSettingsShow((int)Utils::CommonSettingsType::CommonSettingsType_General);
        GetDispatcher()->post_stats_to_core(core::stats::stats_event_names::settingsscr_settings_action);
    }

    void SettingsTab::settingsVoiceVideoClicked()
    {
        setCurrentItem(Utils::CommonSettingsType::CommonSettingsType_VoiceVideo);

        emit Utils::InterConnector::instance().showSettingsHeader(QT_TRANSLATE_NOOP("main_page", "Voice and video"));
        emit Utils::InterConnector::instance().generalSettingsShow((int)Utils::CommonSettingsType::CommonSettingsType_VoiceVideo);
        GetDispatcher()->post_stats_to_core(core::stats::stats_event_names::settingsscr_video_action);
    }

    void SettingsTab::settingsNotificationsClicked()
    {
        setCurrentItem(Utils::CommonSettingsType::CommonSettingsType_Notifications);

        emit Utils::InterConnector::instance().showSettingsHeader(QT_TRANSLATE_NOOP("main_page", "Notifications"));
        emit Utils::InterConnector::instance().generalSettingsShow((int)Utils::CommonSettingsType::CommonSettingsType_Notifications);
        GetDispatcher()->post_stats_to_core(core::stats::stats_event_names::settingsscr_notifications_action);
    }

    void SettingsTab::settingsAppearanceClicked()
    {
        setCurrentItem(Utils::CommonSettingsType::CommonSettingsType_Appearance);

        emit Utils::InterConnector::instance().showSettingsHeader(QT_TRANSLATE_NOOP("main_page", "Appearance"));
        emit Utils::InterConnector::instance().generalSettingsShow((int)Utils::CommonSettingsType::CommonSettingsType_Appearance);
        GetDispatcher()->post_stats_to_core(core::stats::stats_event_names::settingsscr_appearance_action);
    }

    void SettingsTab::settingsAboutAppClicked()
    {
        setCurrentItem(Utils::CommonSettingsType::CommonSettingsType_About);

        emit Utils::InterConnector::instance().showSettingsHeader(QT_TRANSLATE_NOOP("main_page", "About app"));
        emit Utils::InterConnector::instance().generalSettingsShow((int)Utils::CommonSettingsType::CommonSettingsType_About);
        GetDispatcher()->post_stats_to_core(core::stats::stats_event_names::settingsscr_aboutapp_action);
    }

    void SettingsTab::settingsContactUsClicked()
    {
        setCurrentItem(Utils::CommonSettingsType::CommonSettingsType_ContactUs);

        emit Utils::InterConnector::instance().showSettingsHeader(QT_TRANSLATE_NOOP("main_page", "Contact Us"));
        emit Utils::InterConnector::instance().generalSettingsShow((int)Utils::CommonSettingsType::CommonSettingsType_ContactUs);
        GetDispatcher()->post_stats_to_core(core::stats::stats_event_names::settingsscr_writeus_action);
    }

    void SettingsTab::settingsLanguageClicked()
    {
        setCurrentItem(Utils::CommonSettingsType::CommonSettingsType_Language);

        emit Utils::InterConnector::instance().showSettingsHeader(QT_TRANSLATE_NOOP("main_page", "Language"));
        emit Utils::InterConnector::instance().generalSettingsShow((int)Utils::CommonSettingsType::CommonSettingsType_Language);
        GetDispatcher()->post_stats_to_core(core::stats::stats_event_names::settingsscr_language_action);
    }

    void SettingsTab::settingsHotkeysClicked()
    {
        setCurrentItem(Utils::CommonSettingsType::CommonSettingsType_Shortcuts);

        emit Utils::InterConnector::instance().showSettingsHeader(QT_TRANSLATE_NOOP("main_page", "Shortcuts"));
        emit Utils::InterConnector::instance().generalSettingsShow((int)Utils::CommonSettingsType::CommonSettingsType_Shortcuts);
        GetDispatcher()->post_stats_to_core(core::stats::stats_event_names::settingsscr_hotkeys_action);
    }

    void SettingsTab::settingsSecurityClicked()
    {
        setCurrentItem(Utils::CommonSettingsType::CommonSettingsType_Security);

        emit Utils::InterConnector::instance().showSettingsHeader(QT_TRANSLATE_NOOP("main_page", "Privacy"));
        emit Utils::InterConnector::instance().generalSettingsShow((int)Utils::CommonSettingsType::CommonSettingsType_Security);
        GetDispatcher()->post_stats_to_core(core::stats::stats_event_names::settingsscr_privacy_action);
    }

    void SettingsTab::settingsStickersClicked()
    {
        setCurrentItem(Utils::CommonSettingsType::CommonSettingsType_Stickers);

        emit Utils::InterConnector::instance().hideSettingsHeader();
        emit Utils::InterConnector::instance().generalSettingsShow((int)Utils::CommonSettingsType::CommonSettingsType_Stickers);
        GetDispatcher()->post_stats_to_core(core::stats::stats_event_names::settingsscr_stickers_action);
    }

    void SettingsTab::setCurrentItem(const Utils::CommonSettingsType _item)
    {
        currentSettingsItem_ = _item;
        updateSettingsState();
    }

    void SettingsTab::updateSettingsState()
    {
        const auto index = getIndexByType(currentSettingsItem_);
        ui_->settingsList_->setCurrentIndex(index);
        if (auto w = ui_->settingsList_->itemAt(index))
            scrollArea_->ensureWidgetVisible(w, 0, 0);
    }

    void SettingsTab::listClicked(int _index)
    {
        if (const auto type = getTypeByIndex(_index); isClickableType(type))
            selectSettings(type);
    }

    void SettingsTab::appUpdateReady()
    {
        if (auto plate = qobject_cast<UpdaterSettingsRow*>(ui_->updatePlate_))
        {
            plate->setUpdateReady(true);
            if (ui_->updateSpacer_)
                ui_->updateSpacer_->show();
        }
    }

    Utils::CommonSettingsType SettingsTab::getTypeByIndex(int _index) const
    {
        const auto it = std::find_if(ui_->indexToType_.begin(), ui_->indexToType_.end(), [_index](auto x) { return x.first == _index; });
        if (it != ui_->indexToType_.end())
            return it->second;
        assert(false);
        return Utils::CommonSettingsType::CommonSettingsType_General;
    }

    int SettingsTab::getIndexByType(Utils::CommonSettingsType _type) const
    {
        const auto it = std::find_if(ui_->indexToType_.begin(), ui_->indexToType_.end(), [_type](auto x) { return x.second == _type; });
        if (it != ui_->indexToType_.end())
            return it->first;
        assert(false);
        return 0;
    }

    void SettingsTab::compactModeChanged()
    {
        setCompactMode(isCompact_, true);
    }

}
