#include "stdafx.h"
#include "SettingsTab.h"

#include "../contact_list/Common.h"
#include "../contact_list/FavoritesUtils.h"
#include "../contact_list/ContactListModel.h"
#include "../../core_dispatcher.h"
#include "../../gui_settings.h"
#include "../../utils/utils.h"
#include "../../utils/InterConnector.h"
#include "../../utils/gui_coll_helper.h"
#include "../../utils/features.h"
#include "../../controls/CustomButton.h"
#include "../../controls/LabelEx.h"
#include "../../controls/TransparentScrollBar.h"
#include "../../controls/SimpleListWidget.h"
#include "../contact_list/TopPanel.h"
#include "../../utils/log/log.h"
#include "styles/StyleVariable.h"
#include "../common.shared/config/config.h"
#include"../../main_window/MainPage.h"

#include "SimpleSettingsRow.h"
#include "ProfileSettingsRow.h"
#include "SpacerSettingsRow.h"

namespace
{
    bool isClickableType(const Utils::CommonSettingsType _type)
    {
        return
            _type != Utils::CommonSettingsType::CommonSettingsType_Updater &&
            _type != Utils::CommonSettingsType::CommonSettingsType_None;
    }

    auto getContactUsText()
    {
        static const auto contactUs = Features::isContactUsViaBackend() ? QT_TRANSLATE_NOOP("main_page", "Report a problem") : QT_TRANSLATE_NOOP("main_page", "Contact Us");
        return contactUs;
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

        std::vector<std::pair<int, Utils::CommonSettingsType>> indexToType_;

        friend class SettingsTab;

    public:
        void init(QWidget* _p, QLayout* _headerLayout, QLayout* _contentLayout)
        {
            topWidget_ = new HeaderTitleBar(_p);
            topWidget_->setTitle(QT_TRANSLATE_NOOP("head", "Settings"));
            Testing::setAccessibleName(topWidget_, qsl("AS SettingsTab topWidget"));
            _headerLayout->addWidget(topWidget_);

            settingsList_ = new SimpleListWidget(Qt::Vertical);

            const auto addRow = [this](auto _row, const auto _type, QStringView _accName)
            {
                Testing::setAccessibleName(_row, u"AS SettingsRow " % _accName);
                indexToType_.emplace_back(settingsList_->addItem(_row), _type);
                return _row;
            };

            const auto addSpacer = [addRow]()
            {
                return addRow(new SpacerSettingsRow(), Utils::CommonSettingsType::CommonSettingsType_None, u"spacer");
            };

            const auto addSimpleRow = [addRow](const auto& _icon, const auto _bg, const auto& _caption, const auto _type, QStringView _accName)
            {
                return addRow(new SimpleSettingsRow(_icon, _bg, _caption), _type, _accName);
            };

            addRow(new ProfileSettingsRow(), Utils::CommonSettingsType::CommonSettingsType_Profile, u"profile");
            addSpacer();

            addSimpleRow(qsl(":/context_menu/favorites"), Styling::StyleVariable::SECONDARY_RAINBOW_YELLOW, QT_TRANSLATE_NOOP("settings", "Favorites"), Utils::CommonSettingsType::CommonSettingsType_Favorites, u"favorites");
            addSpacer();

            addSimpleRow(qsl(":/settings/general"), Styling::StyleVariable::SECONDARY_RAINBOW_MINT, QT_TRANSLATE_NOOP("settings", "General"), Utils::CommonSettingsType::CommonSettingsType_General, u"general");
            addSimpleRow(qsl(":/settings/notifications"), Styling::StyleVariable::SECONDARY_ATTENTION, QT_TRANSLATE_NOOP("settings", "Notifications"), Utils::CommonSettingsType::CommonSettingsType_Notifications, u"notifications");
            addSimpleRow(qsl(":/settings/lock"), Styling::StyleVariable::BASE_PRIMARY, QT_TRANSLATE_NOOP("settings", "Privacy"), Utils::CommonSettingsType::CommonSettingsType_Security, u"privacy");
            addSimpleRow(qsl(":/settings/appearance"), Styling::StyleVariable::SECONDARY_RAINBOW_COLD, QT_TRANSLATE_NOOP("settings", "Appearance"), Utils::CommonSettingsType::CommonSettingsType_Appearance, u"appearance");
            addSimpleRow(qsl(":/settings/sticker"), Styling::StyleVariable::SECONDARY_RAINBOW_ORANGE, QT_TRANSLATE_NOOP("settings", "Stickers"), Utils::CommonSettingsType::CommonSettingsType_Stickers, u"stickers");
            addSpacer();

#ifndef STRIP_VOIP
            addSimpleRow(qsl(":/settings/video"), Styling::StyleVariable::SECONDARY_RAINBOW_AQUA, QT_TRANSLATE_NOOP("settings", "Voice and video"), Utils::CommonSettingsType::CommonSettingsType_VoiceVideo, u"videoAndVoice");
#endif //STRIP_VOIP
            addSimpleRow(qsl(":/settings/shortcuts"), Styling::StyleVariable::SECONDARY_RAINBOW_PURPLE, QT_TRANSLATE_NOOP("settings", "Shortcuts"), Utils::CommonSettingsType::CommonSettingsType_Shortcuts, u"shortcuts");
            addSimpleRow(qsl(":/settings/language"), Styling::StyleVariable::SECONDARY_RAINBOW_PINK, QT_TRANSLATE_NOOP("settings", "Language"), Utils::CommonSettingsType::CommonSettingsType_Language, u"language");
            addSpacer();

            addSimpleRow(qsl(":/settings/report"), Styling::StyleVariable::SECONDARY_RAINBOW_BLUE, getContactUsText(), Utils::CommonSettingsType::CommonSettingsType_ContactUs, u"contactUs");
            addSimpleRow(qsl(":/settings/product"), Styling::StyleVariable::SECONDARY_RAINBOW_GREEN, QT_TRANSLATE_NOOP("settings", "About app"), Utils::CommonSettingsType::CommonSettingsType_About, u"aboutApp");

            addSimpleRow(qsl(":/settings/alert_icon"), Styling::StyleVariable::SECONDARY_ATTENTION, QT_TRANSLATE_NOOP("settings", "Advanced Settings"), Utils::CommonSettingsType::CommonSettingsType_Debug, u"advancedSettings");
            settingsList_->itemAt(getIndexByType(Utils::CommonSettingsType::CommonSettingsType_Debug))->setVisible(environment::is_develop());

            Testing::setAccessibleName(settingsList_, qsl("AS SettingsTab settingsList"));
            _contentLayout->addWidget(settingsList_);
        }

        int getIndexByType(Utils::CommonSettingsType _type) const
        {
            const auto it = std::find_if(indexToType_.begin(), indexToType_.end(), [_type](auto x) { return x.second == _type; });
            if (it != indexToType_.end())
                return it->first;
            im_assert(false);
            return 0;
        }
    };

    SettingsTab::SettingsTab(QWidget* _parent)
        : QWidget(_parent)
        , ui_(new UI())
        , currentSettingsItem_(Utils::CommonSettingsType::CommonSettingsType_None)
        , isCompact_(false)
        , scrollArea_(CreateScrollAreaAndSetTrScrollBarV(this))
    {
        Testing::setAccessibleName(scrollArea_, qsl("AS SettingsTab scrollArea"));
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
        connect(&Utils::InterConnector::instance(), &Utils::InterConnector::showDebugSettings, this, [this]()
        {
            if (auto debugSettings = ui_->settingsList_->itemAt(ui_->getIndexByType(Utils::CommonSettingsType::CommonSettingsType_Debug)))
            {
                debugSettings->setVisible(true);
                if (const auto page = Utils::InterConnector::instance().getMessengerPage())
                    page->settingsTabActivate(Utils::CommonSettingsType::CommonSettingsType_Debug);
            }
        });
    }

    SettingsTab::~SettingsTab() = default;

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

        case Utils::CommonSettingsType::CommonSettingsType_Debug:
            settingsDebugClicked();
            break;

        case Utils::CommonSettingsType::CommonSettingsType_Favorites:
            settingsFavoritesClicked();
            break;

        case Utils::CommonSettingsType::CommonSettingsType_General:
        default:
            settingsGeneralClicked();
            break;
        }
    }

    void SettingsTab::cleanSelection()
    {
        Q_EMIT Utils::InterConnector::instance().popPagesToRoot();

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
    }

    void SettingsTab::settingsProfileClicked()
    {
        setCurrentItem(Utils::CommonSettingsType::CommonSettingsType_Profile);

        Q_EMIT Utils::InterConnector::instance().profileSettingsShow(QString());
        Q_EMIT Utils::InterConnector::instance().showSettingsHeader(QT_TRANSLATE_NOOP("main_page", "My profile"));
    }

    void SettingsTab::settingsFavoritesClicked()
    {
        Logic::getContactListModel()->setCurrent(Favorites::aimId(), -1, true);
    }

    void SettingsTab::settingsGeneralClicked()
    {
        setCurrentItem(Utils::CommonSettingsType::CommonSettingsType_General);

        Q_EMIT Utils::InterConnector::instance().showSettingsHeader(QT_TRANSLATE_NOOP("main_page", "General settings"));
        Q_EMIT Utils::InterConnector::instance().generalSettingsShow((int)Utils::CommonSettingsType::CommonSettingsType_General);
        GetDispatcher()->post_stats_to_core(core::stats::stats_event_names::settingsscr_settings_action);
    }

    void SettingsTab::settingsVoiceVideoClicked()
    {
        setCurrentItem(Utils::CommonSettingsType::CommonSettingsType_VoiceVideo);

        Q_EMIT Utils::InterConnector::instance().showSettingsHeader(QT_TRANSLATE_NOOP("main_page", "Voice and video"));
        Q_EMIT Utils::InterConnector::instance().generalSettingsShow((int)Utils::CommonSettingsType::CommonSettingsType_VoiceVideo);
        GetDispatcher()->post_stats_to_core(core::stats::stats_event_names::settingsscr_video_action);
    }

    void SettingsTab::settingsNotificationsClicked()
    {
        setCurrentItem(Utils::CommonSettingsType::CommonSettingsType_Notifications);

        Q_EMIT Utils::InterConnector::instance().showSettingsHeader(QT_TRANSLATE_NOOP("main_page", "Notifications"));
        Q_EMIT Utils::InterConnector::instance().generalSettingsShow((int)Utils::CommonSettingsType::CommonSettingsType_Notifications);
        GetDispatcher()->post_stats_to_core(core::stats::stats_event_names::settingsscr_notifications_action);
    }

    void SettingsTab::settingsAppearanceClicked()
    {
        setCurrentItem(Utils::CommonSettingsType::CommonSettingsType_Appearance);

        Q_EMIT Utils::InterConnector::instance().showSettingsHeader(QT_TRANSLATE_NOOP("main_page", "Appearance"));
        Q_EMIT Utils::InterConnector::instance().generalSettingsShow((int)Utils::CommonSettingsType::CommonSettingsType_Appearance);
        GetDispatcher()->post_stats_to_core(core::stats::stats_event_names::settingsscr_appearance_action);
    }

    void SettingsTab::settingsAboutAppClicked()
    {
        setCurrentItem(Utils::CommonSettingsType::CommonSettingsType_About);

        Q_EMIT Utils::InterConnector::instance().showSettingsHeader(QT_TRANSLATE_NOOP("main_page", "About app"));
        Q_EMIT Utils::InterConnector::instance().generalSettingsShow((int)Utils::CommonSettingsType::CommonSettingsType_About);
        GetDispatcher()->post_stats_to_core(core::stats::stats_event_names::settingsscr_aboutapp_action);
    }

    void SettingsTab::settingsContactUsClicked()
    {
        setCurrentItem(Utils::CommonSettingsType::CommonSettingsType_ContactUs);

        Q_EMIT Utils::InterConnector::instance().showSettingsHeader(getContactUsText());
        Q_EMIT Utils::InterConnector::instance().generalSettingsShow((int)Utils::CommonSettingsType::CommonSettingsType_ContactUs);
        GetDispatcher()->post_stats_to_core(core::stats::stats_event_names::settingsscr_writeus_action);
    }

    void SettingsTab::settingsLanguageClicked()
    {
        setCurrentItem(Utils::CommonSettingsType::CommonSettingsType_Language);

        Q_EMIT Utils::InterConnector::instance().showSettingsHeader(QT_TRANSLATE_NOOP("main_page", "Language"));
        Q_EMIT Utils::InterConnector::instance().generalSettingsShow((int)Utils::CommonSettingsType::CommonSettingsType_Language);
        GetDispatcher()->post_stats_to_core(core::stats::stats_event_names::settingsscr_language_action);
    }

    void SettingsTab::settingsHotkeysClicked()
    {
        setCurrentItem(Utils::CommonSettingsType::CommonSettingsType_Shortcuts);

        Q_EMIT Utils::InterConnector::instance().showSettingsHeader(QT_TRANSLATE_NOOP("main_page", "Shortcuts"));
        Q_EMIT Utils::InterConnector::instance().generalSettingsShow((int)Utils::CommonSettingsType::CommonSettingsType_Shortcuts);
        GetDispatcher()->post_stats_to_core(core::stats::stats_event_names::settingsscr_hotkeys_action);
    }

    void SettingsTab::settingsSecurityClicked()
    {
        setCurrentItem(Utils::CommonSettingsType::CommonSettingsType_Security);

        Q_EMIT Utils::InterConnector::instance().showSettingsHeader(QT_TRANSLATE_NOOP("main_page", "Privacy"));
        Q_EMIT Utils::InterConnector::instance().generalSettingsShow((int)Utils::CommonSettingsType::CommonSettingsType_Security);
        GetDispatcher()->post_stats_to_core(core::stats::stats_event_names::settingsscr_privacy_action);
    }

    void SettingsTab::settingsStickersClicked()
    {
        setCurrentItem(Utils::CommonSettingsType::CommonSettingsType_Stickers);

        Q_EMIT Utils::InterConnector::instance().hideSettingsHeader();
        Q_EMIT Utils::InterConnector::instance().generalSettingsShow((int)Utils::CommonSettingsType::CommonSettingsType_Stickers);
        GetDispatcher()->post_stats_to_core(core::stats::stats_event_names::settingsscr_stickers_action);
    }

    void SettingsTab::settingsDebugClicked()
    {
        setCurrentItem(Utils::CommonSettingsType::CommonSettingsType_Debug);

        Q_EMIT Utils::InterConnector::instance().showSettingsHeader(QT_TRANSLATE_NOOP("settings", "Advanced Settings"));
        Q_EMIT Utils::InterConnector::instance().generalSettingsShow((int)Utils::CommonSettingsType::CommonSettingsType_Debug);
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

    Utils::CommonSettingsType SettingsTab::getTypeByIndex(int _index) const
    {
        const auto it = std::find_if(ui_->indexToType_.begin(), ui_->indexToType_.end(), [_index](auto x) { return x.first == _index; });
        if (it != ui_->indexToType_.end())
            return it->second;
        im_assert(false);
        return Utils::CommonSettingsType::CommonSettingsType_General;
    }

    int SettingsTab::getIndexByType(Utils::CommonSettingsType _type) const
    {
        return ui_->getIndexByType(_type);
    }

    void SettingsTab::compactModeChanged()
    {
        setCompactMode(isCompact_, true);
    }
}
