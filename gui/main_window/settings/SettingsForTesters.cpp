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
    {
        auto scrollArea = CreateScrollAreaAndSetTrScrollBarV(_parent);
        scrollArea->setWidgetResizable(true);
        scrollArea->setStyleSheet(qsl("QWidget{ background: %1;}").arg(Styling::getParameters().getColorHex(Styling::StyleVariable::BASE_GLOBALWHITE)));
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

        openLogsBtn_ = setupButton(qsl(":/background_icon"), QT_TRANSLATE_NOOP("popup_window", "Open logs path"));
        Testing::setAccessibleName(openLogsBtn_, qsl("AS AdditionalSettingsPage openLogsButton"));
        clearCacheBtn_ = setupButton(qsl(":/background_icon"), QT_TRANSLATE_NOOP("popup_window", "Clear cache"));
        Testing::setAccessibleName(clearCacheBtn_, qsl("AS AdditionalSettingsPage clearCacheButton"));
        clearAvatarsBtn_ = setupButton(qsl(":/background_icon"), QT_TRANSLATE_NOOP("popup_window", "Clear avatars"));
        Testing::setAccessibleName(clearAvatarsBtn_, qsl("AS AdditionalSettingsPage clearAvatarsButton"));
        logCurrentMessagesModel_ = setupButton(qsl(":/background_icon"), QT_TRANSLATE_NOOP("popup_window", "Log messagesModel"));
        Testing::setAccessibleName(clearAvatarsBtn_, qsl("AS AdditionalSettingsPage logMessageModelButton"));

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

        if (Features::hasConnectByIpOption())
        {
            connectByIpCheckbox_ = GeneralCreator::addSwitcher(this, mainLayout_,
                QT_TRANSLATE_NOOP("popup_window", "Use internal DNS cache"),
                appConfig.connectByIp(),
                {},
                Utils::scale_value(36), qsl("AS AdditionalSettingsPage useInternalDNS"));

            connect(connectByIpCheckbox_, &Ui::SidebarCheckboxButton::checked, this, &SettingsForTesters::onToggleConnectByIp);
        }

        connect(fullLogModeCheckbox_, &Ui::SidebarCheckboxButton::checked, this, &SettingsForTesters::onToggleFullLogMode);
        connect(devShowMsgIdsCheckbox_, &Ui::SidebarCheckboxButton::checked, this, &SettingsForTesters::onToggleShowMsgIdsMenu);

        if (environment::is_develop())
        {
            updatebleCheckbox_ = GeneralCreator::addSwitcher(this, mainLayout_,
                                                             QT_TRANSLATE_NOOP("popup_window", "Updatable"),
                                                             appConfig.IsUpdateble(),
                                                             {},
                                                             Utils::scale_value(36), qsl("AS AdditionalSettingsPage updatableSetting"));

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

            connect(updatebleCheckbox_, &Ui::SidebarCheckboxButton::checked, this, &SettingsForTesters::onToggleUpdateble);
            connect(devSaveCallRTPdumpsCheckbox_, &Ui::SidebarCheckboxButton::checked, this, &SettingsForTesters::onToggleSaveRTPDumps);
            connect(devServerSearchCheckbox_, &Ui::SidebarCheckboxButton::checked, this, &SettingsForTesters::onToggleServerSearch);
            connect(devCustomIdCheckbox_, &Ui::SidebarCheckboxButton::checked, this, &SettingsForTesters::onToggleDevId);

            auto updateUrlLayout = Utils::emptyHLayout();
            updateUrlLayout->setContentsMargins(getMargin(), 0, getMargin(), 0);
            auto updateUrlEdit = new LineEditEx(this);
            updateUrlEdit->setPlaceholderText(QT_TRANSLATE_NOOP("popup_window", "Update url, empty for default"));
            auto checkUpdateButton = new DialogButton(this, QT_TRANSLATE_NOOP("popup_window", "Check"));
            Testing::setAccessibleName(checkUpdateButton, qsl("AS AdditionalSettingsPage checkUpdateButton"));
            Testing::setAccessibleName(updateUrlEdit, qsl("AS AdditionalSettingsPage updateUrlInput"));

            connect(checkUpdateButton, &DialogButton::clicked, this, [updateUrlEdit]()
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

    void SettingsForTesters::initViewElementsFrom(const AppConfig& appConfig)
    {
        if (fullLogModeCheckbox_ && fullLogModeCheckbox_->isChecked() != appConfig.IsFullLogEnabled())
            fullLogModeCheckbox_->setChecked(appConfig.IsFullLogEnabled());

        if (devShowMsgIdsCheckbox_ && devShowMsgIdsCheckbox_->isChecked() != appConfig.IsShowMsgIdsEnabled())
            devShowMsgIdsCheckbox_->setChecked(appConfig.IsShowMsgIdsEnabled());

        if (devSaveCallRTPdumpsCheckbox_ && devSaveCallRTPdumpsCheckbox_->isChecked() != appConfig.IsSaveCallRTPEnabled())
            devSaveCallRTPdumpsCheckbox_->setChecked(appConfig.IsSaveCallRTPEnabled());

        if (devCustomIdCheckbox_ && devCustomIdCheckbox_->isChecked() != appConfig.hasCustomDeviceId())
            devCustomIdCheckbox_->setChecked(appConfig.hasCustomDeviceId());

        if (connectByIpCheckbox_ && connectByIpCheckbox_->isChecked() != appConfig.connectByIp())
            connectByIpCheckbox_->setChecked(appConfig.connectByIp());
    }

    void SettingsForTesters::onOpenLogsPath(const QString &logsPath)
    {
        QDesktopServices::openUrl(QUrl::fromLocalFile(logsPath));
    }

    void SettingsForTesters::onToggleFullLogMode(bool checked)
    {
        AppConfig appConfig = GetAppConfig();
        appConfig.SetFullLogEnabled(checked);

        ModifyAppConfig(std::move(appConfig), [this](core::icollection* _coll) {
            initViewElementsFrom(GetAppConfig());
        }, this);
    }

    void SettingsForTesters::onToggleUpdateble(bool checked)
    {
        AppConfig appConfig = GetAppConfig();
        appConfig.SetUpdateble(checked);

        ModifyAppConfig(std::move(appConfig), [this](core::icollection* _coll) {
            initViewElementsFrom(GetAppConfig());
        }, this);
    }


    void SettingsForTesters::onToggleShowMsgIdsMenu(bool checked)
    {
        AppConfig appConfig = GetAppConfig();
        appConfig.SetShowMsgIdsEnabled(checked);

        ModifyAppConfig(std::move(appConfig), [this](core::icollection* _coll) {
            initViewElementsFrom(GetAppConfig());
        }, this);
    }

    void SettingsForTesters::onToggleSaveRTPDumps(bool checked)
    {
        AppConfig appConfig = GetAppConfig();
        appConfig.SetSaveCallRTPEnabled(checked);

        ModifyAppConfig(std::move(appConfig), [this](core::icollection* _coll) {
            initViewElementsFrom(GetAppConfig());
        }, this);
    }

    void SettingsForTesters::onToggleServerSearch(bool checked)
    {
        AppConfig appConfig = GetAppConfig();
        appConfig.SetServerSearchEnabled(checked);

        ModifyAppConfig(std::move(appConfig), [this](core::icollection* _coll) {
            initViewElementsFrom(GetAppConfig());
        }, this);
    }

    void SettingsForTesters::onToggleDevId(bool checked)
    {
        AppConfig appConfig = GetAppConfig();
        appConfig.SetCustomDeviceId(checked);

        ModifyAppConfig(std::move(appConfig), [this](core::icollection* _coll) {
            initViewElementsFrom(GetAppConfig());
        }, this);
        Utils::removeOmicronFile();
    }

    void SettingsForTesters::onToggleConnectByIp(bool checked)
    {
        AppConfig appConfig = GetAppConfig();
        appConfig.SetConnectByIp(checked);

        ModifyAppConfig(std::move(appConfig), [this](core::icollection* _coll) {
            initViewElementsFrom(GetAppConfig());
        }, this);

        const QString text = QT_TRANSLATE_NOOP("popup_window", "To change this option you must restart the application. Continue?");

        const auto confirmed = Utils::GetConfirmationWithTwoButtons(
            QT_TRANSLATE_NOOP("popup_window", "Cancel"),
            QT_TRANSLATE_NOOP("popup_window", "Yes"),
            text,
            QT_TRANSLATE_NOOP("popup_window", "Restart"),
            nullptr
        );

        if (confirmed)
        {
            Utils::restartApplication();

            return;
        }
    }

    SettingsForTesters::~SettingsForTesters()
    {
    }
}
