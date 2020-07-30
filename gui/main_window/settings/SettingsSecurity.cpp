#include "stdafx.h"
#include "GeneralSettingsWidget.h"

#include "PrivacySettingsManager.h"
#include "PageOpenerWidget.h"

#include "core_dispatcher.h"
#include "gui_settings.h"
#include "controls/GeneralCreator.h"
#include "controls/TextEmojiWidget.h"
#include "controls/TransparentScrollBar.h"
#include "controls/SimpleListWidget.h"
#include "controls/RadioTextRow.h"
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
    };

    QString getSessionsButtonCaption()
    {
        return QT_TRANSLATE_NOOP("settings", "Session List");
    }
}

static void initPrivacy(QVBoxLayout* _layout, QWidget* _parent)
{
    static auto privacyManager = new Logic::PrivacySettingsManager();

    const auto addPrivacySetting = [scrollArea = _parent, mainLayout = _layout](const auto& _settingName, const auto& _caption)
    {
        auto headerWidget = new QWidget(scrollArea);
        auto headerLayout = Utils::emptyVLayout(headerWidget);
        Utils::grabTouchWidget(headerWidget);
        headerLayout->setAlignment(Qt::AlignTop);
        headerLayout->setContentsMargins(0, 0, 0, 0);

        GeneralCreator::addHeader(headerWidget, headerLayout, _caption, 20);

        mainLayout->addWidget(headerWidget);
        mainLayout->addSpacing(Utils::scale_value(8));

        auto searchWidget = new QWidget(scrollArea);
        Utils::grabTouchWidget(searchWidget);
        auto searchLayout = Utils::emptyVLayout(searchWidget);
        searchLayout->setAlignment(Qt::AlignTop);
        searchLayout->setSpacing(Utils::scale_value(12));

        const auto& varList = Utils::getPrivacyAllowVariants();
        auto list = new SimpleListWidget(Qt::Vertical);
        for (const auto& [name, type] : varList)
        {
            auto radioTextRow = new RadioTextRow(name);
            if (Testing::isEnabled)
            {
                auto typeQString = QString::fromStdString(to_string(type));
                if (!typeQString.isEmpty())
                    typeQString[0] = typeQString.at(0).toUpper();
                Testing::setAccessibleName(radioTextRow, qsl("AS PrivacyPage ") % _settingName % typeQString);
            }
            list->addItem(radioTextRow);
        }

        const auto setIndex = [list](const auto _index)
        {
            if (list->isValidIndex(_index))
                list->setCurrentIndex(_index);
            else
                list->clearSelection();
        };

        QObject::connect(list, &SimpleListWidget::clicked, list, [_settingName, setIndex, &varList, list](int idx)
        {
            if (!list->isValidIndex(idx))
                return;

            const auto prevIdx = list->getCurrentIndex();
            list->setCurrentIndex(idx);

            privacyManager->setValue(_settingName, varList[idx].second, list, [setIndex, prevIdx](const bool _success)
            {
                if (!_success)
                    setIndex(prevIdx);
            });

            {
                const auto statName = _settingName == u"groups"
                    ? core::stats::stats_event_names::privacyscr_grouptype_action
                    : core::stats::stats_event_names::privacyscr_calltype_action;

                std::string statVal;
                switch (varList[idx].second)
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
                    assert(!"invalid value");
                    break;
                }
                Ui::GetDispatcher()->post_stats_to_core(statName, { {"type", statVal } });
            }

        }, Qt::UniqueConnection);

        QObject::connect(list, &SimpleListWidget::shown, list, [_settingName, setIndex, &varList, list]()
        {
            list->clearSelection();

            privacyManager->getValue(_settingName, list, [&varList, setIndex](const core::privacy_access_right _value)
            {
                auto idx = -1;
                if (_value != core::privacy_access_right::not_set)
                {
                    const auto it = std::find_if(varList.begin(), varList.end(), [_value](const auto _p) { return _p.second == _value; });
                    if (it != varList.end())
                        idx = std::distance(varList.begin(), it);
                }

                setIndex(idx);
            });
        });

        searchLayout->addWidget(list);
        mainLayout->addWidget(searchWidget);
        mainLayout->addSpacing(Utils::scale_value(40));
    };

    addPrivacySetting(qsl("groups"), QT_TRANSLATE_NOOP("settings", "Who can add me to groups"));
    addPrivacySetting(qsl("calls"), QT_TRANSLATE_NOOP("settings", "Who can call me"));
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
    hotkeyHintLabel->setStyleSheet(qsl("color: %1").arg(Styling::getParameters().getColorHex(Styling::StyleVariable::BASE_PRIMARY)));
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
    auto ignorelistLink = new LabelEx(_parent);
    Testing::setAccessibleName(ignorelistLink, qsl("AS PrivacyPage ignoreList"));
    ignorelistLink->setFont(Fonts::appFontScaled(15));
    ignorelistLink->setColors(Styling::getParameters().getColor(Styling::StyleVariable::TEXT_PRIMARY),
        Styling::getParameters().getColor(Styling::StyleVariable::TEXT_PRIMARY_HOVER),
        Styling::getParameters().getColor(Styling::StyleVariable::TEXT_PRIMARY_ACTIVE));
    ignorelistLink->setText(QT_TRANSLATE_NOOP("settings", "Ignore list"));
    ignorelistLink->setCursor(Qt::PointingHandCursor);
    Utils::grabTouchWidget(ignorelistLink);

    auto thirdLayout = Utils::emptyHLayout();
    thirdLayout->addSpacing(Utils::scale_value(20));
    thirdLayout->addWidget(ignorelistLink);
    _layout->addLayout(thirdLayout);

    QObject::connect(ignorelistLink, &LabelEx::clicked, _parent, [_parent]()
    {
        auto conn = std::make_shared<QMetaObject::Connection>();
        *conn = QObject::connect(GetDispatcher(), &core_dispatcher::recvPermitDeny, _parent, [conn, _parent](const bool _isEmpty)
        {
            QObject::disconnect(*conn);

            GetDispatcher()->post_stats_to_core(core::stats::stats_event_names::ignorelist_open);

            SelectContactsWidget* ignoredDialog = new SelectContactsWidget(
                Logic::getIgnoreModel(),
                Logic::MembersWidgetRegim::IGNORE_LIST,
                QT_TRANSLATE_NOOP("popup_window", "Ignore list"),
                QT_TRANSLATE_NOOP("popup_window", "Close"),
                _parent);

            ignoredDialog->UpdateViewForIgnoreList(_isEmpty);
            ignoredDialog->show();
            ignoredDialog->deleteLater();
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
            sessionsBtn->setText(getSessionsButtonCaption());
        else
            sessionsBtn->setText(getSessionsButtonCaption() % u" (" % QString::number(_sessions.size()) % u')');
    });

    _layout->addWidget(sessionsBtn);
}

void GeneralSettingsWidget::Creator::initSecurity(QWidget* _parent)
{
    auto scrollArea = CreateScrollAreaAndSetTrScrollBarV(_parent);
    scrollArea->setStyleSheet(qsl("QWidget{border: none; background-color: %1;}").arg(Styling::getParameters().getColorHex(Styling::StyleVariable::BASE_GLOBALWHITE)));
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
