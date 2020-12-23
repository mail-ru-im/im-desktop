#include "stdafx.h"
#include "GeneralSettingsWidget.h"

#include "PrivacySettingsWidget.h"

#include "PageOpenerWidget.h"

#include "core_dispatcher.h"
#include "gui_settings.h"
#include "controls/GeneralCreator.h"
#include "controls/TextEmojiWidget.h"
#include "controls/TransparentScrollBar.h"
#include "controls/GeneralDialog.h"
#include "controls/LabelEx.h"
#include "main_window/LocalPIN.h"
#include "main_window/MainWindow.h"
#include "main_window/sidebar/SidebarUtils.h"
#include "main_window/contact_list/IgnoreMembersModel.h"
#include "main_window/contact_list/ContactListModel.h"
#include "main_window/contact_list/SelectionContactsForGroupChat.h"
#include "main_window/contact_list/ContactListUtils.h"
#include "utils/InterConnector.h"
#include "utils/utils.h"
#include "styles/ThemeParameters.h"

using namespace Ui;

namespace
{
    QString getLockTimeoutStr(const std::chrono::minutes _timeout)
    {
        switch (_timeout.count())
        {
        case 0:
            return QT_TRANSLATE_NOOP("settings", "Off");
        case 1:
            return QT_TRANSLATE_NOOP("settings", "After 1 minute");
        case 5:
            return QT_TRANSLATE_NOOP("settings", "After 5 minutes");
        case 10:
            return QT_TRANSLATE_NOOP("settings", "After 10 minute");
        case 60:
            return QT_TRANSLATE_NOOP("settings", "After 1 hour");
        default:
            return QString();
        }
    }

    QString getSessionsButtonCaption()
    {
        return QT_TRANSLATE_NOOP("settings", "Session List");
    }

    LabelEx* addLink(QVBoxLayout* _layout, QWidget* _parent, const QString& _caption, const QString& _accName)
    {
        auto link = new LabelEx(_parent);
        Testing::setAccessibleName(link, _accName);
        link->setFont(Fonts::appFontScaled(15));
        link->setColors(Styling::getParameters().getColor(Styling::StyleVariable::TEXT_PRIMARY),
                                  Styling::getParameters().getColor(Styling::StyleVariable::TEXT_PRIMARY_HOVER),
                                  Styling::getParameters().getColor(Styling::StyleVariable::TEXT_PRIMARY_ACTIVE));
        link->setText(_caption);
        link->setCursor(Qt::PointingHandCursor);
        Utils::grabTouchWidget(link);

        auto thirdLayout = Utils::emptyHLayout();
        thirdLayout->addSpacing(Utils::scale_value(20));
        thirdLayout->addWidget(link);
        _layout->addLayout(thirdLayout);

        return link;
    }
}

static void initPrivacy(QVBoxLayout* _layout, QWidget* _parent)
{
    auto privacyWidget = new PrivacySettingsWidget(_parent);
    _layout->addWidget(privacyWidget);
}

static void initPasscode(QVBoxLayout* _layout, QWidget* _parent)
{
    auto autoLockWidget = new QWidget(_parent);
    autoLockWidget->setVisible(LocalPIN::instance()->enabled());
    Testing::setAccessibleName(autoLockWidget, qsl("AS PrivacyPage autoPinLock"));

    auto changePINWidget = new QWidget(_parent);
    changePINWidget->setVisible(LocalPIN::instance()->enabled());

    auto pinSwitcher = GeneralCreator::addSwitcher(
        _parent,
        _layout,
        QT_TRANSLATE_NOOP("settings", "Local passcode"),
        LocalPIN::instance()->enabled());
    Testing::setAccessibleName(pinSwitcher, qsl("AS PrivacyPage pinLockSetting"));

    QObject::connect(pinSwitcher, &Ui::SidebarCheckboxButton::checked, _parent, [pinSwitcher, changePINWidget, autoLockWidget](bool _checked)
    {
        QSignalBlocker blocker(pinSwitcher);
        pinSwitcher->setChecked(LocalPIN::instance()->enabled());

        auto codeWidget = new LocalPINWidget(_checked ? LocalPINWidget::Mode::SetPIN : LocalPINWidget::Mode::RemovePIN, nullptr);

        GeneralDialog d(codeWidget, Utils::InterConnector::instance().getMainWindow(), true);
        d.showInCenter();

        pinSwitcher->setChecked(LocalPIN::instance()->enabled());

        changePINWidget->setVisible(LocalPIN::instance()->enabled());
        autoLockWidget->setVisible(LocalPIN::instance()->enabled());
    });

    auto changePINLink = new TextEmojiWidget(_parent, Fonts::appFontScaled(15), Styling::getParameters().getColor(Styling::StyleVariable::TEXT_PRIMARY));
    Testing::setAccessibleName(changePINLink, qsl("AS PrivacyPage changePinLock"));
    Utils::grabTouchWidget(changePINLink);
    changePINLink->setCursor(QCursor(Qt::CursorShape::PointingHandCursor));
    changePINLink->setText(QT_TRANSLATE_NOOP("settings", "Change passcode"));
    QObject::connect(changePINLink, &TextEmojiWidget::clicked, []()
    {
        auto codeWidget = new LocalPINWidget(LocalPINWidget::Mode::ChangePIN, nullptr);
        GeneralDialog d(codeWidget, Utils::InterConnector::instance().getMainWindow(), true);
        d.showInCenter();
    });

    auto changePINLayout = Utils::emptyVLayout(changePINWidget);
    changePINLayout->addSpacing(Utils::scale_value(20));
    changePINLayout->addWidget(changePINLink);

    auto hotkeyHintLabel = new QLabel(_parent);
    hotkeyHintLabel->setWordWrap(true);
    hotkeyHintLabel->setFont(Fonts::appFontScaled(15, Fonts::FontWeight::Normal));
    hotkeyHintLabel->setStyleSheet(ql1s("color: %1").arg(Styling::getParameters().getColorHex(Styling::StyleVariable::BASE_PRIMARY)));
    const QString hotkey = (platform::is_apple() ? KeySymbols::Mac::command : qsl("Ctrl")) % u" + L";
    hotkeyHintLabel->setText(QT_TRANSLATE_NOOP("settings", "To manually lock the screen press %1").arg(hotkey));

    auto thirdLayout = Utils::emptyHLayout();
    thirdLayout->addSpacing(Utils::scale_value(20));
    auto extraLayout = Utils::emptyVLayout();
    extraLayout->addWidget(changePINWidget);
    extraLayout->addSpacing(Utils::scale_value(20));
    extraLayout->addWidget(hotkeyHintLabel);
    extraLayout->addSpacing(Utils::scale_value(36));
    extraLayout->addWidget(autoLockWidget);
    thirdLayout->addLayout(extraLayout);
    _layout->addLayout(thirdLayout);

    auto autoLockLayout = Utils::emptyVLayout(autoLockWidget);

    std::vector<std::chrono::minutes> lockTimeoutValues =
    {
        std::chrono::minutes(0),
        std::chrono::minutes(1),
        std::chrono::minutes(5),
        std::chrono::minutes(10),
        std::chrono::minutes(60)
    };

    std::vector<QString> lockTimeoutDescriptions;
    for (auto & _timeout : lockTimeoutValues)
        lockTimeoutDescriptions.push_back(getLockTimeoutStr(_timeout));

    auto it = std::find(lockTimeoutValues.begin(), lockTimeoutValues.end(), LocalPIN::instance()->lockTimeoutMinutes());
    if (it == lockTimeoutValues.end())
        it = lockTimeoutValues.begin();

    auto current = it - lockTimeoutValues.begin();
    GeneralCreator::addDropper(
        _parent,
        autoLockLayout,
        QT_TRANSLATE_NOOP("settings", "Auto lock"),
        true,
        lockTimeoutDescriptions,
        current /* selected index */,
        -1 /* width */,
        [values = std::move(lockTimeoutValues)](const QString&, int index, TextEmojiWidget*)
        {
            LocalPIN::instance()->setLockTimeout(values[index]);
        });

    autoLockLayout->addSpacing(Utils::scale_value(36));
}

static void initIgnored(QVBoxLayout* _layout, QWidget* _parent)
{
    auto ignorelistLink = addLink(_layout, _parent, QT_TRANSLATE_NOOP("settings", "Ignore list"), qsl("AS PrivacyPage ignoreList"));

    QObject::connect(ignorelistLink, &LabelEx::clicked, _parent, [_parent]()
    {
        auto conn = std::make_shared<QMetaObject::Connection>();
        *conn = QObject::connect(GetDispatcher(), &core_dispatcher::recvPermitDeny, _parent, [conn, _parent](const bool _isEmpty)
        {
            QObject::disconnect(*conn);

            GetDispatcher()->post_stats_to_core(core::stats::stats_event_names::ignorelist_open);

            SelectContactsWidget dialog(
                Logic::getIgnoreModel(),
                Logic::MembersWidgetRegim::IGNORE_LIST,
                QT_TRANSLATE_NOOP("popup_window", "Ignore list"),
                QT_TRANSLATE_NOOP("popup_window", "Close"),
                _parent);

            dialog.setEmptyLabelVisible(_isEmpty);
            dialog.show();
        });

        Logic::updateIgnoredModel({});
        Logic::ContactListModel::getIgnoreList();
    });
}

static void initAccountSecurity(QVBoxLayout* _layout, QWidget* _parent)
{
    _layout->addSpacing(Utils::scale_value(40));
    GeneralCreator::addHeader(_parent, _layout, QT_TRANSLATE_NOOP("settings", "Account Security"), 20);
    _layout->addSpacing(Utils::scale_value(8));

    auto sessionsBtn = new PageOpenerWidget(_parent, getSessionsButtonCaption());
    Testing::setAccessibleName(sessionsBtn, qsl("AS PrivacyPage sessionsList"));
    QObject::connect(sessionsBtn, &PageOpenerWidget::clicked, _parent, []()
    {
        Q_EMIT Utils::InterConnector::instance().showSettingsHeader(QT_TRANSLATE_NOOP("settings", "Session List"));
        Q_EMIT Utils::InterConnector::instance().generalSettingsShow((int)Utils::CommonSettingsType::CommonSettingsType_Sessions);
    });
    QObject::connect(sessionsBtn, &PageOpenerWidget::shown, GetDispatcher(), &core_dispatcher::requestSessionsList);
    QObject::connect(GetDispatcher(), &core_dispatcher::activeSessionsList, sessionsBtn, [sessionsBtn](const std::vector<Data::SessionInfo>& _sessions)
    {
        if (_sessions.empty())
            sessionsBtn->setCaption(getSessionsButtonCaption());
        else
            sessionsBtn->setCaption(getSessionsButtonCaption() % u" (" % QString::number(_sessions.size()) % u')');
    });

    _layout->addWidget(sessionsBtn);
}

void GeneralSettingsWidget::Creator::initSecurity(QWidget* _parent)
{
    auto scrollArea = CreateScrollAreaAndSetTrScrollBarV(_parent);
    scrollArea->setStyleSheet(ql1s("QWidget{border: none; background-color: %1;}").arg(Styling::getParameters().getColorHex(Styling::StyleVariable::BASE_GLOBALWHITE)));
    scrollArea->setWidgetResizable(true);
    Utils::grabTouchWidget(scrollArea->viewport(), true);

    auto mainWidget = new QWidget(scrollArea);
    Utils::grabTouchWidget(mainWidget);
    Utils::ApplyStyle( mainWidget, Styling::getParameters().getGeneralSettingsQss());

    auto mainLayout = Utils::emptyVLayout(mainWidget);
    mainLayout->setAlignment(Qt::AlignTop);
    mainLayout->setContentsMargins(0, Utils::scale_value(36), 0, Utils::scale_value(36));

    scrollArea->setWidget(mainWidget);

    auto layout = Utils::emptyHLayout(_parent);
    Testing::setAccessibleName(scrollArea, qsl("AS PrivacyPage scrollArea"));
    layout->addWidget(scrollArea);

    initPrivacy(mainLayout, scrollArea);

    auto secondLayout = Utils::emptyVLayout();
    secondLayout->setAlignment(Qt::AlignTop);
    mainLayout->addLayout(secondLayout);

    if constexpr (platform::is_windows())
    {
        auto exeFileSwitcher = GeneralCreator::addSwitcher(
            scrollArea,
            secondLayout,
            QT_TRANSLATE_NOOP("settings", "Do not ask when running executable files"),
            Ui::get_gui_settings()->get_value<bool>(settings_exec_files_without_warning, settings_exec_files_without_warning_default()),
            [](bool checked) -> QString
        {
            Ui::get_gui_settings()->set_value<bool>(settings_exec_files_without_warning, checked);
            return {};
        });
        Testing::setAccessibleName(exeFileSwitcher, qsl("AS PrivacyPage askExucutableRunSetting"));

        connect(Ui::get_gui_settings(), &Ui::qt_gui_settings::changed, exeFileSwitcher, [exeFileSwitcher](const auto& key)
        {
            if (key == ql1s(settings_exec_files_without_warning))
            {
                QSignalBlocker sb(exeFileSwitcher);
                exeFileSwitcher->setChecked(get_gui_settings()->get_value<bool>(settings_exec_files_without_warning, settings_exec_files_without_warning_default()));
            }
        });
    }

    initPasscode(secondLayout, scrollArea);
    initIgnored(secondLayout, scrollArea);
    initAccountSecurity(secondLayout, scrollArea);
}
