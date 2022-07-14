#include "stdafx.h"
#include "GeneralSettingsWidget.h"
#include "PrivacySettingsWidget.h"
#include "SessionsPage.h"
#include "PageOpenerWidget.h"
#include "../PhoneWidget.h"
#include "../AppsPage.h"
#ifdef HAS_WEB_ENGINE
#include "../WebAppPage.h"
#endif

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
#include "styles/ThemeParameters.h"
#include "styles/ThemesContainer.h"
#include "utils/InterConnector.h"
#include "utils/utils.h"
#include "utils/features.h"
#include "../common.shared/config/config.h"
#include "../mini_apps/MiniAppsUtils.h"

using namespace Ui;

namespace
{
    constexpr std::chrono::milliseconds kWebViewOpenDelay(10);

    enum class SecuritySubPage
    {
        PrivacyPage,
        DeletionPage,
        SessionsPage
    };
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
        link->setColors(Styling::ThemeColorKey{ Styling::StyleVariable::TEXT_PRIMARY },
            Styling::ThemeColorKey{ Styling::StyleVariable::TEXT_PRIMARY_HOVER },
            Styling::ThemeColorKey{ Styling::StyleVariable::TEXT_PRIMARY_ACTIVE });
        link->setText(_caption);
        link->setCursor(Qt::PointingHandCursor);
        Utils::grabTouchWidget(link);

        auto thirdLayout = Utils::emptyHLayout();
        thirdLayout->addSpacing(Utils::scale_value(20));
        thirdLayout->addWidget(link);
        _layout->addLayout(thirdLayout);

        return link;
    }

    bool requestPhoneAttach()
    {
        auto phoneWidget = new PhoneWidget(0, PhoneWidgetState::ENTER_PHONE_STATE,
            QString(), QT_TRANSLATE_NOOP("phone_widget", "Enter your number"), true, Ui::AttachState::NEED_PHONE);
        GeneralDialog dialog(phoneWidget, Utils::InterConnector::instance().getMainWindow());
        QObject::connect(phoneWidget, &PhoneWidget::requestClose, &dialog, &GeneralDialog::acceptDialog);
        return dialog.execute();
    }

    void initDeleteAccountDialog(Ui::GeneralDialog& _dialog)
    {
        _dialog.addLabel(QT_TRANSLATE_NOOP("popup_window", "Delete Account"));
        if (Features::isDeleteAccountViaAdmin())
        {
            Testing::setAccessibleName(&_dialog, qsl("AS DeleteViaAdminDialog"));

            _dialog.addText(QT_TRANSLATE_NOOP("settings", "Your corporate account was created by a domain administrator. To remove, contact with your domain administrator"), Utils::scale_value(16));
            _dialog.addAcceptButton(QT_TRANSLATE_NOOP("settings", "Close"), true);
        }
        else
        {
            Testing::setAccessibleName(&_dialog, qsl("AS DeleteViaWebDialog"));
            _dialog.addText(QT_TRANSLATE_NOOP("settings", "All information associated with your profile will be deleted. It will be impossible to restore your account"), Utils::scale_value(16));
            auto buttons = _dialog.addButtonsPair(QT_TRANSLATE_NOOP("popup_window", "Cancel"), QT_TRANSLATE_NOOP("popup_window", "Delete"));
            const auto backgroundNormal = Styling::ThemeColorKey{ Styling::StyleVariable::SECONDARY_ATTENTION };
            const auto backgroundHover = Styling::ThemeColorKey{ Styling::StyleVariable::SECONDARY_ATTENTION_HOVER };
            const auto backgroundPressed = Styling::ThemeColorKey{ Styling::StyleVariable::SECONDARY_ATTENTION_ACTIVE };
            buttons.first->setBackgroundColor(backgroundNormal, backgroundHover, backgroundPressed);
            buttons.first->setBorderColor(backgroundNormal, backgroundHover, backgroundPressed);
        }
    }

    void initConfirmDialog(Ui::GeneralDialog& _dialog)
    {
        _dialog.addLabel(QT_TRANSLATE_NOOP("popup_window", "Delete Account"));
        _dialog.addText(QT_TRANSLATE_NOOP("popup_window", "To delete an account, you must attach a phone number"), Utils::scale_value(16));
        auto buttons = _dialog.addButtonsPair(QT_TRANSLATE_NOOP("popup_window", "Cancel"), QT_TRANSLATE_NOOP("popup_window", "Attach"));
    }

    void deleteAccount(
#if HAS_WEB_ENGINE
            WebAppPage* _webView, QStackedLayout* _layout
#endif
        )
    {
        if (!Features::isDeleteAccountEnabled())
            return;

        QString url = Features::deleteAccountUrl(Ui::MyInfo()->aimId());

        if (!url.startsWith(u"https://", Qt::CaseInsensitive))
            url = QString(u"https://" % url);

        if (Utils::isUin(Ui::MyInfo()->aimId()))
        {
            url += (u"?aimsid=" % Utils::InterConnector::instance().aimSid()
                    % u"&page=deleteAccount"
                    % u"&dark=" % (Styling::getThemesContainer().getCurrentTheme()->isDark() ? u'1' : u'0')
                    % u"&lang=" % Utils::GetTranslator()->getLang());
        }

#if HAS_WEB_ENGINE
        _webView->setUrl(url, true);
        _webView->reloadPage();
        _layout->setCurrentWidget(_webView);
#else
        Utils::openUrl(url);
#endif
    }

    struct AttachPhoneNotificationBlocker
    {
        AttachPhoneNotificationBlocker() : phoneAttached_(false)
        {
            MyInfo()->stopAttachPhoneNotifications();
        }

        ~AttachPhoneNotificationBlocker()
        {
            if (!phoneAttached_)
                MyInfo()->resumeAttachPhoneNotifications();
        }

        void setPhoneAttachResult(bool _result) { phoneAttached_ = _result; }

    private:
        bool phoneAttached_;
    };
}

static void initPrivacy(QVBoxLayout* _layout, QWidget* _parent)
{
    auto privacyWidget = new PrivacySettingsWidget(_parent);
    _layout->addWidget(privacyWidget);
}

static void initLinkSwitcher(QVBoxLayout* _layout, QWidget* _parent)
{
    auto externalLinkSwitcher = GeneralCreator::addSwitcher(
        _parent,
        _layout,
        QT_TRANSLATE_NOOP("settings", "Do not ask when opening external links"),
        Ui::get_gui_settings()->get_value<bool>(settings_open_external_link_without_warning, settings_open_external_link_without_warning_default()),
        [](bool checked) -> QString
        {
            Ui::get_gui_settings()->set_value<bool>(settings_open_external_link_without_warning, checked);
            return {};
        });
    Testing::setAccessibleName(externalLinkSwitcher, qsl("AS PrivacyPage askOpenExternalLinkSetting"));

    QObject::connect(Ui::get_gui_settings(), &Ui::qt_gui_settings::changed, externalLinkSwitcher, [externalLinkSwitcher](const auto& key)
    {
        if (key == ql1s(settings_open_external_link_without_warning))
        {
            QSignalBlocker sb(externalLinkSwitcher);
            externalLinkSwitcher->setChecked(get_gui_settings()->get_value<bool>(settings_open_external_link_without_warning, settings_open_external_link_without_warning_default()));
        }
    });
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
        GeneralDialog::Options opt;
        opt.ignoreKeyPressEvents_ = true;
        GeneralDialog d(codeWidget, Utils::InterConnector::instance().getMainWindow(), opt);
        d.execute();

        pinSwitcher->setChecked(LocalPIN::instance()->enabled());

        changePINWidget->setVisible(LocalPIN::instance()->enabled());
        autoLockWidget->setVisible(LocalPIN::instance()->enabled());
    });

    auto changePINLink = new TextEmojiWidget(_parent, Fonts::appFontScaled(15), Styling::ThemeColorKey{ Styling::StyleVariable::TEXT_PRIMARY });
    Testing::setAccessibleName(changePINLink, qsl("AS PrivacyPage changePinLock"));
    Utils::grabTouchWidget(changePINLink);
    changePINLink->setCursor(QCursor(Qt::CursorShape::PointingHandCursor));
    changePINLink->setText(QT_TRANSLATE_NOOP("settings", "Change passcode"));
    QObject::connect(changePINLink, &TextEmojiWidget::clicked, []()
    {
        GeneralDialog::Options opt;
        opt.ignoreKeyPressEvents_ = true;
        GeneralDialog d(new LocalPINWidget(LocalPINWidget::Mode::ChangePIN, nullptr), Utils::InterConnector::instance().getMainWindow(), opt);
        d.execute();
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

static void initAccountSecurity(QVBoxLayout* _layout, QWidget* _parent, 
#if HAS_WEB_ENGINE
    WebAppPage* _webView, 
 #endif
    SessionsPage* _sessionsPage, QStackedLayout* _stackedLayout)
{
    _layout->addSpacing(Utils::scale_value(40));
    GeneralCreator::addHeader(_parent, _layout, QT_TRANSLATE_NOOP("settings", "Account Security"), 20);
    _layout->addSpacing(Utils::scale_value(8));

    auto sessionsBtn = new PageOpenerWidget(_parent, getSessionsButtonCaption());
    Testing::setAccessibleName(sessionsBtn, qsl("AS PrivacyPage sessionsList"));
    QObject::connect(sessionsBtn, &PageOpenerWidget::clicked, _stackedLayout, [_stackedLayout, _sessionsPage]()
    {
        _stackedLayout->setCurrentWidget(_sessionsPage);
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

    QSpacerItem* spacer = new QSpacerItem(0, 0, QSizePolicy::Minimum, QSizePolicy::Fixed);
    _layout->addSpacerItem(spacer);
    int spacerIndex = _layout->indexOf(spacer);

    auto deleteAccBtn = addLink(_layout, _parent, QT_TRANSLATE_NOOP("settings", "Delete Account"), qsl("AS PrivacyPage deleteAccount"));
    deleteAccBtn->setColors(Styling::ThemeColorKey{ Styling::StyleVariable::SECONDARY_ATTENTION },
        Styling::ThemeColorKey{ Styling::StyleVariable::SECONDARY_ATTENTION_HOVER },
        Styling::ThemeColorKey{ Styling::StyleVariable::SECONDARY_ATTENTION_ACTIVE });

    auto updateUi = [delBtn = QPointer(deleteAccBtn), layout = QPointer(_layout), spacerIndex]()
    {
        if (delBtn)
            delBtn->setVisible(Features::isDeleteAccountEnabled());

        if (layout && layout->count() > 0)
        {
            if (QSpacerItem* sp = layout->itemAt(spacerIndex)->spacerItem())
                sp->changeSize(0, Features::isDeleteAccountEnabled() ? Utils::scale_value(50) : 0, QSizePolicy::Minimum, QSizePolicy::Fixed);
        }
    };

    updateUi();
    QObject::connect(&Utils::InterConnector::instance(), &Utils::InterConnector::omicronUpdated, updateUi);
    QObject::connect(GetDispatcher(), &core_dispatcher::externalUrlConfigUpdated, updateUi);

    QObject::connect(deleteAccBtn, &LabelEx::clicked, [
#if HAS_WEB_ENGINE
        _webView, _stackedLayout
#endif
    ]()
    {
        const bool isDeleteViaAdmin = Features::isDeleteAccountViaAdmin();
        if (!isDeleteViaAdmin && MyInfo()->phoneNumber().isEmpty())
        {
            Ui::GeneralDialog confirmDialog(nullptr, Utils::InterConnector::instance().getMainWindow());
            if (confirmDialog.isActive())
                return;

            AttachPhoneNotificationBlocker blocker;
            initConfirmDialog(confirmDialog);
            if (!confirmDialog.execute())
                return;

            if (!requestPhoneAttach())
                return;

            blocker.setPhoneAttachResult(true);
        }

        Ui::GeneralDialog generalDialog(nullptr, Utils::InterConnector::instance().getMainWindow());
        if (generalDialog.isActive())
            return;

        initDeleteAccountDialog(generalDialog);
        if (generalDialog.execute() && !isDeleteViaAdmin)
        {
            QTimer::singleShot(kWebViewOpenDelay, [
#if HAS_WEB_ENGINE
                _webView, _stackedLayout
#endif
            ]()
            {
                deleteAccount(
#if HAS_WEB_ENGINE
                    _webView, _stackedLayout
#endif
                );
            });
        }
    });
}


void GeneralSettingsWidget::Creator::initSecurity(QStackedWidget* _parent)
{
    auto mainWidget = _parent;
    QStackedLayout* stackedLayout = qobject_cast<QStackedLayout*>(mainWidget->layout());
    if (Q_LIKELY(stackedLayout))
    {
        stackedLayout->setContentsMargins(QMargins());
        stackedLayout->setSpacing(0);
    }

    auto scrollArea = CreateScrollAreaAndSetTrScrollBarV(mainWidget);
    scrollArea->setStyleSheet(ql1s("QWidget{border: none; background-color: transparent;}"));
    scrollArea->setWidgetResizable(true);
    Utils::grabTouchWidget(scrollArea->viewport(), true);


    SessionsPage* sessions = new SessionsPage(mainWidget);
    Testing::setAccessibleName(sessions, qsl("AS SessionsList"));
    sessions->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

#if HAS_WEB_ENGINE
    auto webView = new WebAppPage(Utils::MiniApps::getAccountDeleteId(), mainWidget);
#endif

    auto privacyWidget = new QWidget(mainWidget);
    Utils::grabTouchWidget(privacyWidget);
    Utils::ApplyStyle(privacyWidget, Styling::getParameters().getGeneralSettingsQss());

    auto mainLayout = Utils::emptyVLayout(privacyWidget);
    mainLayout->setAlignment(Qt::AlignTop);
    mainLayout->setContentsMargins(0, Utils::scale_value(36), 0, Utils::scale_value(36));

    scrollArea->setWidget(privacyWidget);
    Testing::setAccessibleName(scrollArea, qsl("AS PrivacyPage scrollArea"));

    initPrivacy(mainLayout, scrollArea);

    auto secondLayout = Utils::emptyVLayout();
    secondLayout->setAlignment(Qt::AlignTop);
    mainLayout->addLayout(secondLayout);

    if constexpr (!platform::is_apple())
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

    initLinkSwitcher(secondLayout, scrollArea);
    initPasscode(secondLayout, scrollArea);
    initIgnored(secondLayout, scrollArea);
    initAccountSecurity(secondLayout, scrollArea,
#if HAS_WEB_ENGINE
        webView, 
#endif
        sessions, stackedLayout);

    mainWidget->insertWidget((int)SecuritySubPage::PrivacyPage, scrollArea);
#if HAS_WEB_ENGINE
    mainWidget->insertWidget((int)SecuritySubPage::DeletionPage, webView);
#endif
    mainWidget->insertWidget((int)SecuritySubPage::SessionsPage, sessions);
    mainWidget->setCurrentWidget(scrollArea);

    connect(mainWidget, &QStackedWidget::currentChanged, [
#if HAS_WEB_ENGINE
        view = QPointer(webView)
#endif
    ](int index)
    {
#if HAS_WEB_ENGINE
        if (!view)
            return;
#endif

        switch (static_cast<SecuritySubPage>(index))
        {
        case SecuritySubPage::PrivacyPage:
            Q_EMIT Utils::InterConnector::instance().showSettingsHeader(QT_TRANSLATE_NOOP("main_page", "Privacy"));
            break;
        case SecuritySubPage::DeletionPage:
            Q_EMIT Utils::InterConnector::instance().showSettingsHeader(QT_TRANSLATE_NOOP("main_page", "Delete Account"));
            break;
        case SecuritySubPage::SessionsPage:
            Q_EMIT Utils::InterConnector::instance().showSettingsHeader(QT_TRANSLATE_NOOP("settings", "Session List"));
            break;
        }
    });
}
