#include "stdafx.h"
#include "GeneralSettingsWidget.h"

#include "controls/ConnectionSettingsWidget.h"
#include "controls/GeneralCreator.h"
#include "controls/TextEmojiWidget.h"
#include "controls/TransparentScrollBar.h"

#include "core_dispatcher.h"
#include "gui_settings.h"
#include "main_window/MainWindow.h"
#include "main_window/sidebar/SidebarUtils.h"
#include "utils/InterConnector.h"
#include "utils/utils.h"
#include "utils/translator.h"
#include "utils/features.h"
#include "styles/ThemeParameters.h"
#include "statuses/StatusUtils.h"

using namespace Ui;

void GeneralSettingsWidget::Creator::initGeneral(GeneralSettings* _parent)
{
    auto scrollArea = CreateScrollAreaAndSetTrScrollBarV(_parent);
    scrollArea->setWidgetResizable(true);
    scrollArea->setStyleSheet(ql1s("QWidget{ background: transparent;}"));
    Utils::grabTouchWidget(scrollArea->viewport(), true);

    auto mainWidget = new QWidget(scrollArea);
    Utils::grabTouchWidget(mainWidget);

    auto mainLayout = Utils::emptyVLayout(mainWidget);
    mainLayout->setAlignment(Qt::AlignTop);
    mainLayout->setContentsMargins(0, Utils::scale_value(36), 0, Utils::scale_value(36));

    scrollArea->setWidget(mainWidget);

    auto layout = Utils::emptyHLayout(_parent);
    Testing::setAccessibleName(scrollArea, qsl("AS GeneralPage scrollArea"));
    layout->addWidget(scrollArea);

    const auto addSimpleSwitcher = [scrollArea, mainLayout](const auto& _caption, const char* _name, const bool _defaultValue, const QString& _accName = QString(), std::function<void(bool)> _func = {})
    {
        auto switcher = GeneralCreator::addSwitcher(
            scrollArea,
            mainLayout,
            _caption,
            get_gui_settings()->get_value<bool>(_name, _defaultValue),
            [_name, _defaultValue, _func = std::move(_func)](bool checked)
            {
                if (get_gui_settings()->get_value<bool>(_name, _defaultValue) != checked)
                    get_gui_settings()->set_value<bool>(_name, checked);

                if (_func)
                    _func(checked);
            }, -1, _accName);

        connect(get_gui_settings(), &qt_gui_settings::changed, switcher, [switcher, _name, _defaultValue](const QString& _key)
        {
            if (_key == ql1s(_name))
            {
                QSignalBlocker sb(switcher);
                switcher->setChecked(get_gui_settings()->get_value<bool>(_name, _defaultValue));
            }
        });

        return switcher;
    };

    {
        if constexpr (!platform::is_linux())
        {
            GeneralCreator::addHeader(scrollArea, mainLayout, QT_TRANSLATE_NOOP("settings", "Application"), 20);

            mainLayout->addSpacing(Utils::scale_value(12));
        }

        if constexpr (platform::is_windows())
        {
            GeneralCreator::addSwitcher(
                    scrollArea,
                    mainLayout,
                    QT_TRANSLATE_NOOP("settings", "Launch when system starts"),
                    Utils::isStartOnStartup(),
                    [_parent](bool checked) {
                        if (Utils::isStartOnStartup() != checked)
                            Utils::setStartOnStartup(checked);

                        if (_parent->launchMinimized_)
                        {
                            {
                                QSignalBlocker sb(_parent->launchMinimized_);

                                auto value = false;
                                if (checked) {
                                    value = get_gui_settings()->get_value<bool>(settings_start_minimazed, true);
                                }
                                _parent->launchMinimized_->setChecked(value);
                            }

                            _parent->launchMinimized_->setEnabled(checked);
                        }
                    }, -1, qsl("AS GeneralPage launchWenSystemStartSetting"));

            _parent->launchMinimized_ = GeneralCreator::addSwitcher(
                    scrollArea,
                    mainLayout,
                    QT_TRANSLATE_NOOP("settings", "Launch minimized"),
                    get_gui_settings()->get_value<bool>(settings_start_minimazed, true),
                    [](bool checked) { get_gui_settings()->set_value<bool>(settings_start_minimazed, checked); }, -1, qsl("AS GeneralPage launchMinimizedSetting"));

            if (!Utils::isStartOnStartup())
            {
                _parent->launchMinimized_->setChecked(false);
                _parent->launchMinimized_->setEnabled(false);
            }
            GeneralCreator::addSwitcher(
                    scrollArea,
                    mainLayout,
                    QT_TRANSLATE_NOOP("settings", "Show taskbar icon"),
                    get_gui_settings()->get_value<bool>(settings_show_in_taskbar, true),
                    [](bool checked)
                    {
                        Q_EMIT Utils::InterConnector::instance().showIconInTaskbar(checked);
                        get_gui_settings()->set_value<bool>(settings_show_in_taskbar, checked);
                    }, -1, qsl("AS GeneralPage launchWenSystemStartSetting"));
        }

        if constexpr (platform::is_apple())
        {
            GeneralCreator::addSwitcher(
                    scrollArea,
                    mainLayout,
                    QT_TRANSLATE_NOOP("settings", "Show in menu bar"),
                    get_gui_settings()->get_value(settings_show_in_menubar, false),
                    [](bool checked)
                    {
                        if (Utils::InterConnector::instance().getMainWindow())
                            Utils::InterConnector::instance().getMainWindow()->showMenuBarIcon(checked);
                        if (get_gui_settings()->get_value(settings_show_in_menubar, false) != checked)
                            get_gui_settings()->set_value(settings_show_in_menubar, checked);
                    }, -1, qsl("AS GeneralPage showInMenubarSetting"));
        }

        mainLayout->addSpacing(Utils::scale_value(20));
    }
    {
        GeneralCreator::addHeader(scrollArea, mainLayout, QT_TRANSLATE_NOOP("settings", "Chat"), 20);
        mainLayout->addSpacing(Utils::scale_value(12));

        _parent->previewLinks_ = addSimpleSwitcher(QT_TRANSLATE_NOOP("settings", "Preview links"), settings_show_video_and_images, true, qsl("AS GeneralPage previewLinksSetting"));

        addSimpleSwitcher(QT_TRANSLATE_NOOP("settings", "Auto play videos"), settings_autoplay_video, true, qsl("AS GeneralPage autoplayVideoSetting"));
        addSimpleSwitcher(QT_TRANSLATE_NOOP("settings", "Turn on video sound on hover"), settings_hoversound_video, settings_hoversound_video_default(), qsl("AS GeneralPage turnOnVideoSoundOnHoverSetting"));
        addSimpleSwitcher(QT_TRANSLATE_NOOP("settings", "Auto play GIFs"), settings_autoplay_gif, true, qsl("AS GeneralPage autoplayGIFSetting"));
        addSimpleSwitcher(QT_TRANSLATE_NOOP("settings", "Show read status in groups"), settings_show_groupchat_heads, true, qsl("AS GeneralPage showReadStatusInGroupSetting"), [](const bool _checked)
        {
            if (_checked)
                Q_EMIT Utils::InterConnector::instance().showHeads();
            else
                Q_EMIT Utils::InterConnector::instance().hideHeads();
        });

        _parent->smartreplySwitcher_ = addSimpleSwitcher(QT_TRANSLATE_NOOP("settings", "Show smart reply"), settings_show_smartreply, settings_show_smartreply_default(), qsl("AS GeneralPage showSmartReplySetting"), [](const bool _checked)
        {
            const core::stats::event_props_type props = { { "enabled", _checked ? "1" : "0" } };
            GetDispatcher()->post_im_stats_to_core(core::stats::im_stat_event_names::smartreply_settings_swtich, props);
            GetDispatcher()->post_stats_to_core(core::stats::stats_event_names::settingsscr_smartreply_action, props);
        });

        _parent->spellCheckerSwitcher_ = addSimpleSwitcher(QT_TRANSLATE_NOOP("settings", "Use system spell checker"), settings_spell_check, settings_spell_check_default(), qsl("AS GeneralPage useSpellCheckerSetting"));

        addSimpleSwitcher(QT_TRANSLATE_NOOP("settings", "Close search after result selection"), settings_fast_drop_search_results, settings_fast_drop_search_default(), qsl("AS GeneralPage closeSearchAfterResultSelectionSetting"));

        _parent->suggestsEmojiSwitcher_ = addSimpleSwitcher(QT_TRANSLATE_NOOP("settings", "Show stickers suggests for emoji"), settings_show_suggests_emoji, true, qsl("AS GeneralPage showStickerSuggestsForEmojiSetting"));
        _parent->suggestsWordsSwitcher_ = addSimpleSwitcher(QT_TRANSLATE_NOOP("settings", "Show stickers suggests for words"), settings_show_suggests_words, true, qsl("AS GeneralPage showStickerSuggestsForWordsSetting"));

        addSimpleSwitcher(QT_TRANSLATE_NOOP("settings", "Automatically replace emojis"), settings_autoreplace_emoji, settings_autoreplace_emoji_default(), qsl("AS GeneralPage autoReplaceEmojiSetting"));
        addSimpleSwitcher(QT_TRANSLATE_NOOP("settings", "Big emoji in chat"), settings_allow_big_emoji, settings_allow_big_emoji_default(), qsl("AS GeneralPage bigEmojiInChatSetting"), [](const bool)
        {
            Q_EMIT Utils::InterConnector::instance().emojiSizeChanged();
        });

        _parent->reactionsSwitcher_ = addSimpleSwitcher(QT_TRANSLATE_NOOP("settings", "Show reactions to messages"), settings_show_reactions, settings_show_reactions_default(), qsl("AS GeneralPage showReactionsSetting"));

        _parent->statusSwitcher_ = addSimpleSwitcher(QT_TRANSLATE_NOOP("settings", "Show statuses"), settings_allow_statuses, settings_allow_statuses_default(), qsl("AS GeneralPage showStatusesSetting"));

        mainLayout->addSpacing(Utils::scale_value(20));

        _parent->downloadPathChooser_ = GeneralCreator::addChooser(
            scrollArea,
            mainLayout,
            QT_TRANSLATE_NOOP("settings", "Save files to:"),
            Ui::getDownloadPath(),
            [_parent](TextEmojiWidget* path)
        {
            auto parentForDialog = platform::is_linux() ? nullptr : _parent;
            auto r = QFileDialog::getExistingDirectory(parentForDialog, QT_TRANSLATE_NOOP("settings", "Choose new path"), Ui::getDownloadPath(),
                QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks);
            if (r.length())
            {
                path->setText(r);
                Ui::setDownloadPath(QDir::toNativeSeparators(r));
            }
        }, qsl("AS GeneralPage downloadPath"));
        mainLayout->addSpacing(Utils::scale_value(20));
    }

    {
        GeneralCreator::addHeader(scrollArea, mainLayout, QT_TRANSLATE_NOOP("settings", "Calls"), 20);
        mainLayout->addSpacing(Utils::scale_value(12));

        addSimpleSwitcher(QT_TRANSLATE_NOOP("settings", "Warn about disabled microphone"), settings_warn_about_disabled_microphone, true, qsl("AS GeneralPage WarnAboutDisabledMicrophoneSetting"));

        mainLayout->addSpacing(Utils::scale_value(20));
    }

    {
        GeneralCreator::addHeader(scrollArea, mainLayout, QT_TRANSLATE_NOOP("settings", "Contacts"), 20);
        mainLayout->addSpacing(Utils::scale_value(12));

        addSimpleSwitcher(QT_TRANSLATE_NOOP("settings", "Show popular contacts"), settings_show_popular_contacts, true, qsl("AS GeneralPage showPopularContactsSetting"));

        mainLayout->addSpacing(Utils::scale_value(20));
    }

    {
        GeneralCreator::addHeader(scrollArea, mainLayout, QT_TRANSLATE_NOOP("settings", "Connection settings"), 20);
        mainLayout->addSpacing(Utils::scale_value(12));
        _parent->connectionTypeChooser_ = GeneralCreator::addChooser(
            scrollArea,
            mainLayout,
            QT_TRANSLATE_NOOP("settings", "Connection type:"),
            qsl("Auto"),
            [_parent](TextEmojiWidget* /*b*/)
        {
            auto connection_settings_widget_ = new ConnectionSettingsWidget(_parent);
            connection_settings_widget_->show();
            GetDispatcher()->post_stats_to_core(core::stats::stats_event_names::proxy_open);
        }, qsl("AS GeneralPage proxy"));
    }
}
