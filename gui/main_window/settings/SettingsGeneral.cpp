#include "stdafx.h"
#include "GeneralSettingsWidget.h"

#include "../../controls/ConnectionSettingsWidget.h"
#include "../../controls/GeneralCreator.h"
#include "../../controls/TextEmojiWidget.h"
#include "../../controls/TransparentScrollBar.h"

#include "../../../common.shared/config/config.h"

#include "../../core_dispatcher.h"
#include "../../gui_settings.h"
#include "../../main_window/MainWindow.h"
#include "../../utils/InterConnector.h"
#include "../../utils/utils.h"
#include "../../utils/translator.h"
#include "../../utils/features.h"
#include "../../styles/ThemeParameters.h"

using namespace Ui;

void GeneralSettingsWidget::Creator::initGeneral(GeneralSettings* _parent)
{
    auto scrollArea = CreateScrollAreaAndSetTrScrollBarV(_parent);
    scrollArea->setWidgetResizable(true);
    scrollArea->setStyleSheet(qsl("QWidget{ background: %1;}").arg(Styling::getParameters().getColorHex(Styling::StyleVariable::BASE_GLOBALWHITE)));
    Utils::grabTouchWidget(scrollArea->viewport(), true);

    auto mainWidget = new QWidget(scrollArea);
    Utils::grabTouchWidget(mainWidget);

    auto mainLayout = Utils::emptyVLayout(mainWidget);
    mainLayout->setAlignment(Qt::AlignTop);
    mainLayout->setContentsMargins(0, Utils::scale_value(36), 0, Utils::scale_value(36));

    scrollArea->setWidget(mainWidget);

    auto layout = Utils::emptyHLayout(_parent);
    Testing::setAccessibleName(scrollArea, qsl("AS settings scrollArea"));
    layout->addWidget(scrollArea);

    const auto addSimpleSwitcher = [scrollArea, mainLayout](const auto& _caption, const char* _name, const bool _defaultValue, const std::function<void(bool)> _func = {})
    {
        return GeneralCreator::addSwitcher(
            scrollArea,
            mainLayout,
            _caption,
            get_gui_settings()->get_value<bool>(_name, _defaultValue),
            [_name, _defaultValue, _func](bool checked)
            {
                if (get_gui_settings()->get_value<bool>(_name, _defaultValue) != checked)
                    get_gui_settings()->set_value<bool>(_name, checked);

                if (_func)
                    _func(checked);

                return QString();
            });
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
                    [_parent](bool checked) -> QString {
                        if (Utils::isStartOnStartup() != checked)
                            Utils::setStartOnStartup(checked);


                        if (_parent->launchMinimized_ != nullptr) {
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

                        return QString();
                    });

            _parent->launchMinimized_ = GeneralCreator::addSwitcher(
                    scrollArea,
                    mainLayout,
                    QT_TRANSLATE_NOOP("settings", "Launch minimized"),
                    get_gui_settings()->get_value<bool>(settings_start_minimazed, true),
                    [](bool checked) -> QString {
                        get_gui_settings()->set_value<bool>(settings_start_minimazed, checked);
                        return QString();
                    });

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
                    [](bool checked) -> QString {
                        emit Utils::InterConnector::instance().showIconInTaskbar(checked);
                        get_gui_settings()->set_value<bool>(settings_show_in_taskbar, checked);
                        return QString();
                    });
        }

        if constexpr (platform::is_apple())
        {
            GeneralCreator::addSwitcher(
                    scrollArea,
                    mainLayout,
                    QT_TRANSLATE_NOOP("settings", "Show in menu bar"),
                    get_gui_settings()->get_value(settings_show_in_menubar, false),
                    [](bool checked) -> QString {
                        if (Utils::InterConnector::instance().getMainWindow())
                            Utils::InterConnector::instance().getMainWindow()->showMenuBarIcon(checked);
                        if (get_gui_settings()->get_value(settings_show_in_menubar, false) != checked)
                            get_gui_settings()->set_value(settings_show_in_menubar, checked);
                        return QString();
                    });
        }

        mainLayout->addSpacing(Utils::scale_value(20));
    }
    {
        GeneralCreator::addHeader(scrollArea, mainLayout, QT_TRANSLATE_NOOP("settings", "Chat"), 20);
        mainLayout->addSpacing(Utils::scale_value(12));


        addSimpleSwitcher(QT_TRANSLATE_NOOP("settings", "Consequently marking messages as read"), settings_partial_read, settings_partial_read_default(), [](const bool _checked)
        {
            GetDispatcher()->post_stats_to_core(core::stats::stats_event_names::settings_main_readall_action, { {"Change_Position", std::string(_checked ? "+" : "-")} });
        });

        if (config::get().is_on(config::features::snippet_in_chat))
            addSimpleSwitcher(QT_TRANSLATE_NOOP("settings", "Preview links"), settings_show_video_and_images, true);

        addSimpleSwitcher(QT_TRANSLATE_NOOP("settings", "Auto play videos"), settings_autoplay_video, true);
        addSimpleSwitcher(QT_TRANSLATE_NOOP("settings", "Turn on video sound on hover"), settings_hoversound_video, settings_hoversound_video_default());
        addSimpleSwitcher(QT_TRANSLATE_NOOP("settings", "Auto play GIFs"), settings_autoplay_gif, true);
        addSimpleSwitcher(QT_TRANSLATE_NOOP("settings", "Show read status in groups"), settings_show_groupchat_heads, true, [](const bool _checked)
        {
            if (_checked)
                emit Utils::InterConnector::instance().showHeads();
            else
                emit Utils::InterConnector::instance().hideHeads();
        });

        if (Features::isSmartreplyEnabled())
        {
            auto switcher = addSimpleSwitcher(QT_TRANSLATE_NOOP("settings", "Show smart reply"), settings_show_smartreply, settings_show_smartreply_default(), [](const bool _checked)
            {
                if (_checked)
                    emit Utils::InterConnector::instance().showSmartreplies();
                else
                    emit Utils::InterConnector::instance().hideSmartReplies(QString());

                const core::stats::event_props_type props = { { "enabled", _checked ? "1" : "0" } };
                GetDispatcher()->post_im_stats_to_core(core::stats::im_stat_event_names::smartreply_settings_swtich, props);
                GetDispatcher()->post_stats_to_core(core::stats::stats_event_names::settingsscr_smartreply_action, props);
            });

            connect(&Utils::InterConnector::instance(), &Utils::InterConnector::smartReplySettingShowChanged, switcher, [switcher]()
            {
                QSignalBlocker sb(switcher);
                switcher->setChecked(get_gui_settings()->get_value<bool>(settings_show_smartreply, settings_show_smartreply_default()));
            });
        }
        addSimpleSwitcher(QT_TRANSLATE_NOOP("settings", "Close search after result selection"), settings_fast_drop_search_results, settings_fast_drop_search_default());

        if (Features::isSuggestsEnabled())
        {
            addSimpleSwitcher(QT_TRANSLATE_NOOP("settings", "Show stickers suggests for emoji"), settings_show_suggests_emoji, true);
            addSimpleSwitcher(QT_TRANSLATE_NOOP("settings", "Show stickers suggests for words"), settings_show_suggests_words, true);
        }

        addSimpleSwitcher(QT_TRANSLATE_NOOP("settings", "Automatically replace emojis"), settings_autoreplace_emoji, settings_autoreplace_emoji_default());
        addSimpleSwitcher(QT_TRANSLATE_NOOP("settings", "Big emoji in chat"), settings_allow_big_emoji, settings_allow_big_emoji_default());

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
        });
        mainLayout->addSpacing(Utils::scale_value(20));
    }
    {
        GeneralCreator::addHeader(scrollArea, mainLayout, QT_TRANSLATE_NOOP("settings", "Contacts"), 20);
        mainLayout->addSpacing(Utils::scale_value(12));

        addSimpleSwitcher(QT_TRANSLATE_NOOP("settings", "Show popular contacts"), settings_show_popular_contacts, true);

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
        });
    }
}
