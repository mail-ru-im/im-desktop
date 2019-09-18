#include "stdafx.h"
#include "GeneralSettingsWidget.h"

#include "../../controls/ConnectionSettingsWidget.h"
#include "../../controls/GeneralCreator.h"
#include "../../controls/TextEmojiWidget.h"
#include "../../controls/TransparentScrollBar.h"


#include "../../core_dispatcher.h"
#include "../../gui_settings.h"
#include "../../main_window/MainWindow.h"
#include "../../utils/InterConnector.h"
#include "../../utils/utils.h"
#include "../../utils/translator.h"
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

        GeneralCreator::addSwitcher(
                scrollArea,
                mainLayout,
                QT_TRANSLATE_NOOP("settings", "Consequently marking messages as read"),
                get_gui_settings()->get_value<bool>(settings_partial_read, settings_partial_read_deafult()),
                [](bool checked) {
                    if (get_gui_settings()->get_value<bool>(settings_partial_read, settings_partial_read_deafult()) !=
                        checked) {
                        get_gui_settings()->set_value<bool>(settings_partial_read, checked);

                        GetDispatcher()->post_stats_to_core(
                                core::stats::stats_event_names::settings_main_readall_action,
                                {{"Change_Position", std::string(checked ? "+" : "-")}});
                    }

                    return QString();
                });

        GeneralCreator::addSwitcher(
                scrollArea,
                mainLayout,
                QT_TRANSLATE_NOOP("settings", "Preview images and links"),
                get_gui_settings()->get_value<bool>(settings_show_video_and_images, true),
                [](bool checked) {
                    if (get_gui_settings()->get_value<bool>(settings_show_video_and_images, true) != checked)
                        get_gui_settings()->set_value<bool>(settings_show_video_and_images, checked);
                    return QString();
                });

        GeneralCreator::addSwitcher(
                scrollArea,
                mainLayout,
                QT_TRANSLATE_NOOP("settings", "Auto play videos"),
                get_gui_settings()->get_value<bool>(settings_autoplay_video, true),
                [](bool checked) {
                    if (get_gui_settings()->get_value<bool>(settings_autoplay_video, true) != checked)
                        get_gui_settings()->set_value<bool>(settings_autoplay_video, checked);
                    return QString();
                });

        GeneralCreator::addSwitcher(
                scrollArea,
                mainLayout,
                QT_TRANSLATE_NOOP("settings", "Auto play GIFs"),
                get_gui_settings()->get_value<bool>(settings_autoplay_gif, true),
                [](bool checked) {
                    if (get_gui_settings()->get_value<bool>(settings_autoplay_gif, true) != checked)
                        get_gui_settings()->set_value<bool>(settings_autoplay_gif, checked);
                    return QString();
                });

        GeneralCreator::addSwitcher(
                scrollArea,
                mainLayout,
                QT_TRANSLATE_NOOP("settings", "Show read status in groups"),
                get_gui_settings()->get_value<bool>(settings_show_groupchat_heads, true),
                [](bool checked) {
                    if (get_gui_settings()->get_value<bool>(settings_show_groupchat_heads, true) != checked)
                        get_gui_settings()->set_value<bool>(settings_show_groupchat_heads, checked);

                    if (checked)
                            emit Utils::InterConnector::instance().showHeads();
                    else
                            emit Utils::InterConnector::instance().hideHeads();

                    return QString();
                });

        GeneralCreator::addSwitcher(scrollArea, mainLayout,
                                    QT_TRANSLATE_NOOP("settings", "Show stickers suggests for emoji"),
                                    get_gui_settings()->get_value<bool>(settings_show_suggests_emoji, true),
                                    [](bool checked) {
                                        if (get_gui_settings()->get_value<bool>(settings_show_suggests_emoji, true) !=
                                            checked)
                                            get_gui_settings()->set_value<bool>(settings_show_suggests_emoji, checked);

                                        return QString();
                                    });

        GeneralCreator::addSwitcher(scrollArea, mainLayout,
                                    QT_TRANSLATE_NOOP("settings", "Show stickers suggests for words"),
                                    get_gui_settings()->get_value<bool>(settings_show_suggests_words, true),
                                    [](bool checked) {
                                        if (get_gui_settings()->get_value<bool>(settings_show_suggests_words, true) !=
                                            checked)
                                            get_gui_settings()->set_value<bool>(settings_show_suggests_words, checked);

                                        return QString();
                                    });

        GeneralCreator::addSwitcher(scrollArea, mainLayout,
                                    QT_TRANSLATE_NOOP("settings", "Automatically replace emojis"),
                                    get_gui_settings()->get_value<bool>(settings_autoreplace_emoji,
                                                                        settings_autoreplace_emoji_deafult()),
                                    [](bool checked) {
                                        if (get_gui_settings()->get_value<bool>(settings_autoreplace_emoji,
                                                                                settings_autoreplace_emoji_deafult()) !=
                                            checked)
                                            get_gui_settings()->set_value<bool>(settings_autoreplace_emoji, checked);

                                        return QString();
                                    });

        GeneralCreator::addSwitcher(scrollArea, mainLayout, QT_TRANSLATE_NOOP("settings", "Big emoji in chat"),
                                    get_gui_settings()->get_value<bool>(settings_allow_big_emoji,
                                                                        settings_allow_big_emoji_deafult()),
                                    [](bool checked) {
                                        if (get_gui_settings()->get_value<bool>(settings_allow_big_emoji,
                                                                                settings_allow_big_emoji_deafult()) !=
                                            checked) {
                                            get_gui_settings()->set_value<bool>(settings_allow_big_emoji, checked);
                                            emit Utils::InterConnector::instance().emojiSizeChanged();
                                        }

                                        return QString();
                                    });

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

        GeneralCreator::addSwitcher(
                scrollArea,
                mainLayout,
                QT_TRANSLATE_NOOP("settings", "Show popular contacts"),
                get_gui_settings()->get_value<bool>(settings_show_popular_contacts, true),
                [](bool checked) {
                    if (get_gui_settings()->get_value<bool>(settings_show_popular_contacts, true) != checked) {
                        get_gui_settings()->set_value<bool>(settings_show_popular_contacts, checked);
                        emit Utils::InterConnector::instance().clSortChanged();
                    }

                    return QString();
                });
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
