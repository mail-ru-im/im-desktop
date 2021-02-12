#include "SettingsForTesters.h"

#include <QDesktopServices>
#include <QDockWidget>
#include <set>

#include "utils/InterConnector.h"
#include "utils/features.h"
#include "main_window/MainWindow.h"
#include "controls/GeneralDialog.h"
#include "controls/GeneralCreator.h"
#include "controls/CustomButton.h"
#include "controls/DialogButton.h"
#include "controls/LineEditEx.h"
#include "../../controls/TransparentScrollBar.h"

#include "../contact_list/ContactListModel.h"

#include "../../styles/ThemeParameters.h"
#include "main_window/sidebar/SidebarUtils.h"
#include "../../memory_stats/gui_memory_monitor.h"

namespace
{
    int getLeftOffset()
    {
        return Utils::scale_value(20);
    }

    int getTextLeftOffset()
    {
        return Utils::scale_value(12);
    }

    int getMargin()
    {
        return Utils::scale_value(10);
    }

    int getItemHeight()
    {
        return Utils::scale_value(44);
    }

    int getItemWidth()
    {
        return Utils::scale_value(360);
    }

    int getIconSize()
    {
        return Utils::scale_value(20);
    }
}

namespace Ui
{
    SettingsForTesters::SettingsForTesters(QWidget* _parent)
        : QWidget(_parent)
        , openLogsBtn_(nullptr)
        , clearCacheBtn_(nullptr)
        , clearAvatarsBtn_(nullptr)
        , fullLogModeCheckbox_(nullptr)
        , devShowMsgIdsCheckbox_(nullptr)
        , devSaveCallRTPdumpsCheckbox_(nullptr)
        , devCustomIdCheckbox_(nullptr)
        , sendDevStatistic_(nullptr)
    {
        auto scrollArea = CreateScrollAreaAndSetTrScrollBarV(_parent);
        scrollArea->setWidgetResizable(true);
        scrollArea->setStyleSheet(ql1s("QWidget{ background: %1;}").arg(Styling::getParameters().getColorHex(Styling::StyleVariable::BASE_GLOBALWHITE)));
        Utils::grabTouchWidget(scrollArea->viewport(), true);

        auto layout = Utils::emptyHLayout(this);
        Testing::setAccessibleName(scrollArea, qsl("AS AdditionalSettingsPage scrollArea"));
        layout->addWidget(scrollArea);

        mainWidget_ = new QWidget(scrollArea);
        Utils::grabTouchWidget(mainWidget_);
        mainWidget_->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
        scrollArea->setWidget(mainWidget_);

        auto setupButton = [this](const auto& _icon, const auto& _text)
        {
            auto btn = new CustomButton(this, _icon, QSize(getIconSize(), getIconSize()));
            Styling::Buttons::setButtonDefaultColors(btn);
            btn->setText(_text);
            btn->setTextAlignment(Qt::AlignVCenter | Qt::AlignLeft);
            btn->setTextLeftOffset( getLeftOffset() + getIconSize() + getTextLeftOffset());
            btn->setNormalTextColor(Styling::getParameters().getColor(Styling::StyleVariable::TEXT_SOLID));
            btn->setSizePolicy(QSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed));
            btn->setMouseTracking(true);
            btn->setIconAlignment(Qt::AlignLeft);
            btn->setIconOffsets(getLeftOffset(), 0);
            btn->setFixedSize(getItemWidth(), getItemHeight());
            btn->setObjectName(qsl("menu_item"));

            Utils::grabTouchWidget(btn);

            mainLayout_->addWidget(btn);
            mainLayout_->setAlignment(btn, Qt::AlignLeft);

            return btn;
        };

        mainLayout_ = Utils::emptyVLayout(mainWidget_);
        mainLayout_->setContentsMargins(getMargin(), getMargin(), getMargin(), getMargin());
        mainLayout_->setSpacing(getMargin());

        const auto defaultIconName = qsl(":/background_icon");

        openLogsBtn_ = setupButton(defaultIconName, QT_TRANSLATE_NOOP("popup_window", "Open logs path"));
        Testing::setAccessibleName(openLogsBtn_, qsl("AS AdditionalSettingsPage openLogsButton"));
        clearCacheBtn_ = setupButton(defaultIconName, QT_TRANSLATE_NOOP("popup_window", "Clear cache"));
        Testing::setAccessibleName(clearCacheBtn_, qsl("AS AdditionalSettingsPage clearCacheButton"));
        clearAvatarsBtn_ = setupButton(defaultIconName, QT_TRANSLATE_NOOP("popup_window", "Clear avatars"));
        Testing::setAccessibleName(clearAvatarsBtn_, qsl("AS AdditionalSettingsPage clearAvatarsButton"));
        logCurrentMessagesModel_ = setupButton(defaultIconName, QT_TRANSLATE_NOOP("popup_window", "Log messagesModel"));
        Testing::setAccessibleName(clearAvatarsBtn_, qsl("AS AdditionalSettingsPage logMessageModelButton"));
        logMemorySnapshot_ = setupButton(defaultIconName, QT_TRANSLATE_NOOP("popup_window", "Log Memory Report"));
        Testing::setAccessibleName(logMemorySnapshot_, qsl("AS AdditionalSettingsPage logMemoryReport"));

        if constexpr (environment::is_develop())
        {
            sendDevStatistic_ = setupButton(defaultIconName, QT_TRANSLATE_NOOP("popup_window", "Send Statistic"));
            Testing::setAccessibleName(sendDevStatistic_, qsl("AS AdditionalSettingsPage sendDevStatistic"));
        }

        connect(this, &SettingsForTesters::openLogsPath, this, &SettingsForTesters::onOpenLogsPath);
        connect(openLogsBtn_, &QPushButton::clicked, this, [this]() {
            GetDispatcher()->post_message_to_core("get_logs_path", nullptr, this,
                                                  [this](core::icollection* _coll) {
                Ui::gui_coll_helper coll(_coll, false);
                const QString logsPath = QString::fromUtf8(coll.get_value_as_string("path"));
                Q_EMIT openLogsPath(logsPath);
            });
        });

        connect(logCurrentMessagesModel_, &QPushButton::clicked, this, []() {
            if (const auto contact = Logic::getContactListModel()->selectedContact(); !contact.isEmpty())
                Q_EMIT Utils::InterConnector::instance().logHistory(contact);
        });

        connect(logMemorySnapshot_, &QPushButton::clicked, this, []() {
            GuiMemoryMonitor::instance().writeLogMemoryReport({});
        });

        connect(clearCacheBtn_, &QPushButton::clicked, this, [this]() {
            GetDispatcher()->post_message_to_core("remove_content_cache", nullptr, this,
                                                  [](core::icollection* _coll)
                                                  {
                                                      Ui::gui_coll_helper coll(_coll, false);
                                                      auto last_error = coll.get_value_as_int("last_error");
                                                      assert(!last_error);
                                                      Q_UNUSED(last_error);
                                                  });
        });

        connect(clearAvatarsBtn_, &QPushButton::clicked, this, [this]() {
            GetDispatcher()->post_message_to_core("clear_avatars", nullptr, this,
                                                  [](core::icollection* _coll)
                                                  {
                                                      Ui::gui_coll_helper coll(_coll, false);
                                                      auto last_error = coll.get_value_as_int("last_error");
                                                      assert(!last_error);
                                                      Q_UNUSED(last_error);
                                                  });
        });

        auto appConfig = GetAppConfig();

        fullLogModeCheckbox_ = GeneralCreator::addSwitcher(this, mainLayout_,
                                                           QT_TRANSLATE_NOOP("popup_window", "Enable full log mode"),
                                                           appConfig.IsFullLogEnabled(),
                                                           {},
                                                           Utils::scale_value(36), qsl("AS AdditionalSettingsPage enableFulllogSetting"));

        devShowMsgIdsCheckbox_ = GeneralCreator::addSwitcher(this, mainLayout_,
                                                             QT_TRANSLATE_NOOP("popup_window", "Display message IDs"),
                                                             appConfig.IsShowMsgIdsEnabled(),
                                                             {},
                                                             Utils::scale_value(36), qsl("AS AdditionalSettingsPage displayMsgIdsSetting"));

        watchGuiMemoryCheckbox_ = GeneralCreator::addSwitcher(this, mainLayout_,
                                                              QT_TRANSLATE_NOOP("popup_window", "Track internal cache"),
                                                              appConfig.WatchGuiMemoryEnabled(),
                                                              {},
                                                              Utils::scale_value(36), qsl("AS AdditionalSettingsPage trackInternalCache"));

        connect(fullLogModeCheckbox_, &Ui::SidebarCheckboxButton::checked, this, &SettingsForTesters::onToggleFullLogMode);
        connect(devShowMsgIdsCheckbox_, &Ui::SidebarCheckboxButton::checked, this, &SettingsForTesters::onToggleShowMsgIdsMenu);
        connect(watchGuiMemoryCheckbox_, &Ui::SidebarCheckboxButton::checked, this, &SettingsForTesters::onToggleWatchGuiMemory);

        if constexpr (environment::is_develop())
        {

            if constexpr (!build::is_pkg_msi())
            {
                updatebleCheckbox_ = GeneralCreator::addSwitcher(this, mainLayout_,
                                                                 QT_TRANSLATE_NOOP("popup_window", "Updatable"),
                                                                 appConfig.IsUpdateble(),
                                                                 {},
                                                                 Utils::scale_value(36), qsl("AS AdditionalSettingsPage updatableSetting"));
                connect(updatebleCheckbox_, &Ui::SidebarCheckboxButton::checked, this, &SettingsForTesters::onToggleUpdateble);
            }

            devSaveCallRTPdumpsCheckbox_ = GeneralCreator::addSwitcher(this, mainLayout_,
                                                                       QT_TRANSLATE_NOOP("popup_window", "Save call RTP dumps"),
                                                                       appConfig.IsSaveCallRTPEnabled());

            devServerSearchCheckbox_ = GeneralCreator::addSwitcher(this, mainLayout_,
                                                                   QT_TRANSLATE_NOOP("popup_window", "Server search"),
                                                                   appConfig.IsServerSearchEnabled(),
                                                                   {},
                                                                   Utils::scale_value(36), qsl("AS AdditionalSettingsPage serverSearchSetting"));

            devCustomIdCheckbox_ = GeneralCreator::addSwitcher(this, mainLayout_,
                                                               QT_TRANSLATE_NOOP("popup_window", "Set dev_id"),
                                                               appConfig.hasCustomDeviceId(),
                                                               {},
                                                               Utils::scale_value(36), qsl("AS AdditionalSettingsPage setDevIdSetting"));

            connect(devSaveCallRTPdumpsCheckbox_, &Ui::SidebarCheckboxButton::checked, this, &SettingsForTesters::onToggleSaveRTPDumps);
            connect(devServerSearchCheckbox_, &Ui::SidebarCheckboxButton::checked, this, &SettingsForTesters::onToggleServerSearch);
            connect(devCustomIdCheckbox_, &Ui::SidebarCheckboxButton::checked, this, &SettingsForTesters::onToggleDevId);
            connect(sendDevStatistic_, &QPushButton::clicked, this, []()
            {
                GetDispatcher()->post_stats_to_core(core::stats::stats_event_names::dev_statistic_event, { { "app_version", Utils::getVersionPrintable().toStdString()} });
            });

            if constexpr (!build::is_pkg_msi())
            {
                auto updateUrlLayout = Utils::emptyHLayout();
                updateUrlLayout->setContentsMargins(getMargin(), 0, getMargin(), 0);
                auto updateUrlEdit = new LineEditEx(this);
                updateUrlEdit->setPlaceholderText(QT_TRANSLATE_NOOP("popup_window", "Update url, empty for default"));
                auto checkUpdateButton = new DialogButton(this, QT_TRANSLATE_NOOP("popup_window", "Check"));
                Testing::setAccessibleName(checkUpdateButton, qsl("AS AdditionalSettingsPage checkUpdateButton"));
                Testing::setAccessibleName(updateUrlEdit, qsl("AS AdditionalSettingsPage updateUrlInput"));

                connect(checkUpdateButton, &DialogButton::clicked, updateUrlEdit, [updateUrlEdit]()
                {
                    gui_coll_helper collection(Ui::GetDispatcher()->create_collection(), true);
                    collection.set_value_as_qstring("url", updateUrlEdit->text());
                    GetDispatcher()->post_message_to_core("update/check", collection.get());
                });

                updateUrlLayout->addWidget(updateUrlEdit);
                updateUrlLayout->addSpacing(Utils::scale_value(15));
                updateUrlLayout->addWidget(checkUpdateButton);

                mainLayout_->addLayout(updateUrlLayout);
            }
        }
    }

    void SettingsForTesters::initViewElementsFrom(const AppConfig& _appConfig)
    {
        if (fullLogModeCheckbox_ && fullLogModeCheckbox_->isChecked() != _appConfig.IsFullLogEnabled())
            fullLogModeCheckbox_->setChecked(_appConfig.IsFullLogEnabled());

        if (devShowMsgIdsCheckbox_ && devShowMsgIdsCheckbox_->isChecked() != _appConfig.IsShowMsgIdsEnabled())
            devShowMsgIdsCheckbox_->setChecked(_appConfig.IsShowMsgIdsEnabled());

        if (devSaveCallRTPdumpsCheckbox_ && devSaveCallRTPdumpsCheckbox_->isChecked() != _appConfig.IsSaveCallRTPEnabled())
            devSaveCallRTPdumpsCheckbox_->setChecked(_appConfig.IsSaveCallRTPEnabled());

        if (devCustomIdCheckbox_ && devCustomIdCheckbox_->isChecked() != _appConfig.hasCustomDeviceId())
            devCustomIdCheckbox_->setChecked(_appConfig.hasCustomDeviceId());

        if (watchGuiMemoryCheckbox_ && watchGuiMemoryCheckbox_->isChecked() != _appConfig.WatchGuiMemoryEnabled())
            watchGuiMemoryCheckbox_->setChecked(_appConfig.WatchGuiMemoryEnabled());
    }

    void SettingsForTesters::onOpenLogsPath(const QString &_logsPath)
    {
        QDesktopServices::openUrl(QUrl::fromLocalFile(_logsPath));
    }

    void SettingsForTesters::onToggleFullLogMode(bool _checked)
    {
        AppConfig appConfig = GetAppConfig();
        appConfig.SetFullLogEnabled(_checked);

        ModifyAppConfig(std::move(appConfig), [this](core::icollection* _coll) {
            initViewElementsFrom(GetAppConfig());
        }, this);
    }

    void SettingsForTesters::onToggleUpdateble(bool _checked)
    {
        AppConfig appConfig = GetAppConfig();
        appConfig.SetUpdateble(_checked);

        ModifyAppConfig(std::move(appConfig), [this](core::icollection* _coll) {
            initViewElementsFrom(GetAppConfig());
        }, this);
    }


    void SettingsForTesters::onToggleShowMsgIdsMenu(bool _checked)
    {
        AppConfig appConfig = GetAppConfig();
        appConfig.SetShowMsgIdsEnabled(_checked);

        ModifyAppConfig(std::move(appConfig), [this](core::icollection* _coll) {
            initViewElementsFrom(GetAppConfig());
        }, this);
    }

    void SettingsForTesters::onToggleSaveRTPDumps(bool _checked)
    {
        AppConfig appConfig = GetAppConfig();
        appConfig.SetSaveCallRTPEnabled(_checked);

        ModifyAppConfig(std::move(appConfig), [this](core::icollection* _coll) {
            initViewElementsFrom(GetAppConfig());
        }, this);
    }

    void SettingsForTesters::onToggleServerSearch(bool _checked)
    {
        AppConfig appConfig = GetAppConfig();
        appConfig.SetServerSearchEnabled(_checked);

        ModifyAppConfig(std::move(appConfig), [this](core::icollection* _coll) {
            initViewElementsFrom(GetAppConfig());
        }, this);
    }

    void SettingsForTesters::onToggleDevId(bool _checked)
    {
        AppConfig appConfig = GetAppConfig();
        appConfig.SetCustomDeviceId(_checked);

        ModifyAppConfig(std::move(appConfig), [this](core::icollection* _coll) {
            initViewElementsFrom(GetAppConfig());
        }, this);
        Utils::removeOmicronFile();
    }

    void SettingsForTesters::onToggleWatchGuiMemory(bool _checked)
    {
        AppConfig appConfig = GetAppConfig();
        appConfig.SetWatchGuiMemoryEnabled(_checked);

        ModifyAppConfig(std::move(appConfig), [this](core::icollection* _coll) {
            initViewElementsFrom(GetAppConfig());
        }, this);
    }

    SettingsForTesters::~SettingsForTesters() = default;
}
