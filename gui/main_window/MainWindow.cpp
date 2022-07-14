#include "stdafx.h"
#include "MainWindow.h"

#include "LoginPage.h"
#include "MainPage.h"
#include "AppsPage.h"
#include "controls/GeneralDialog.h"
#include "ContactDialog.h"
#include "controls/ContextMenu.h"
#include "contact_list/ServiceContacts.h"
#include "contact_list/ContactListModel.h"
#include "contact_list/RecentsModel.h"
#include "contact_list/UnknownsModel.h"
#include "contact_list/MentionModel.h"
#include "contact_list/CallsModel.h"
#include "contact_list/AddContactDialogs.h"
#include "contact_list/ContactListUtils.h"
#include "contact_list/SelectionContactsForGroupChat.h"
#include "history_control/HistoryControlPage.h"
#include "history_control/MessagesScrollArea.h"
#include "history_control/feeds/FeedPage.h"
#include "input_widget/InputWidget.h"
#include "containers/PrivacySettingsContainer.h"
#include "containers/ThreadSubContainer.h"
#include "statuses/LocalStatuses.h"
#ifndef STRIP_AV_MEDIA
#include "mplayer/VideoPlayer.h"
#include "mplayer/LottieHandle.h"
#include "sounds/SoundsManager.h"
#endif // !STRIP_AV_MEDIA

#include "tray/TrayIcon.h"
#include "../gui_settings.h"
#include "../app_config.h"
#include "../url_config.h"
#include "../controls/MainStackedWidget.h"

#include "controls/TooltipWidget.h"
#include "main_window/contact_list/SearchWidget.h"
#include "main_window/containers/InputStateContainer.h"
#include "main_window/containers/TaskContainer.h"
#include "main_window/containers/FileSharingMetaContainer.h"
#include "main_window/input_widget/HistoryTextEdit.h"
#include "../controls/CustomButton.h"
#include "../previewer/GalleryWidget.h"
#include "../utils/utils.h"
#include "../utils/MimeDataUtils.h"
#include "../utils/features.h"
#include "../utils/InterConnector.h"
#include "../utils/gui_metrics.h"
#include "../utils/SearchPatternHistory.h"
#include "utils/periodic_gui_metrics.h"
#include "../cache/stickers/stickers.h"
#include "../my_info.h"
#include "main_window/ReleaseNotes.h"
#include "../../../common.shared/version_info_constants.h"
#include "user_activity/user_activity.h"
#include "../voip/quality/CallQualityStatsMgr.h"
#include "../memory_stats/gui_memory_monitor.h"
#include "ConnectionWidget.h"
#include "../utils/RegisterCustomSchemeTask.h"
#include "LocalPIN.h"
#include "styles/ThemesContainer.h"

#include "styles/ThemeParameters.h"
#include "styles/StyleSheetContainer.h"
#include "styles/StyleSheetGenerator.h"
#include "previewer/toast.h"

#include "../core_dispatcher.h"

#include "../../common.shared/config/config.h"
#include "TermsPrivacyWidget.h"
#include "statuses/SelectStatusWidget.h"
#ifndef STRIP_VOIP
#include "../voip/VideoWindow.h"
#endif

#ifdef HAS_WEB_ENGINE
#include "WebAppPage.h"
#endif

#ifdef _WIN32
#include <windowsx.h>
#include <Uxtheme.h>
#include <wtsapi32.h>
#else
#ifndef STRIP_CRASH_HANDLER
#include "../../common.shared/crash_report/crash_reporter.h"
#endif // !STRIP_CRASH_HANDLER
#endif //_WIN32

#ifdef __APPLE__
#include "../utils/macos/mac_support.h"
#include "../utils/macos/mac_titlebar.h"
#elif __linux__
#include <malloc.h>
#else
#include "../utils/win32/VirtualDesktopManager.h"
#endif //__APPLE__

#include "mini_apps/MiniAppsUtils.h"
#include "mini_apps/MiniAppsContainer.h"

namespace
{
#ifdef _WIN32
    constexpr int TITLE_CONTEXT_MENU_CMD = WM_USER + 0x100;

    constexpr int NativeWindowBorderWidth = 5;

    enum class Style : DWORD
    {
        // WS_THICKFRAME: without this the window cannot be resized and so aero snap, de-maximizing and minimizing won't work
        // WS_CAPTION: enables aero minimize animation/transition
        BasicBorderless = WS_POPUP | WS_THICKFRAME | WS_SYSMENU | WS_MINIMIZEBOX | WS_MAXIMIZEBOX | WS_CLIPCHILDREN,
        AeroBorderless = BasicBorderless | WS_CAPTION
    };

    int getWindowBorderWidth() noexcept
    {
        return Utils::scale_value(NativeWindowBorderWidth);
    }

    std::optional<QRect> getMonitorWorkRect(HWND _hwnd)
    {
        if (RECT rect; GetWindowRect(_hwnd, &rect))
        {
            if (auto monitor = MonitorFromRect(&rect, MONITOR_DEFAULTTONULL))
            {
                MONITORINFO info = { 0 };
                info.cbSize = sizeof(info);
                if (GetMonitorInfo(monitor, &info))
                    return QRect(info.rcWork.left, info.rcWork.top, info.rcWork.right - info.rcWork.left, info.rcWork.bottom - info.rcWork.top);
            }
        }

        return {};
    }

    std::optional<QRect> getMonitorWorkRect(const QPoint& _pos)
    {
        if (auto monitor = MonitorFromPoint({ _pos.x(), _pos.y() }, MONITOR_DEFAULTTONULL))
        {
            MONITORINFO info = { 0 };
            info.cbSize = sizeof(info);
            if (GetMonitorInfo(monitor, &info))
                return QRect(info.rcWork.left, info.rcWork.top, info.rcWork.right - info.rcWork.left, info.rcWork.bottom - info.rcWork.top);
        }

        return {};
    }

    bool isTheSameMonitor(const QPoint& _old, const QPoint& _new)
    {
        if (auto monitorOld = MonitorFromPoint({ _old.x(), _old.y() }, MONITOR_DEFAULTTONULL))
        {
            if (auto monitorNew = MonitorFromPoint({ _new.x(), _new.y() }, MONITOR_DEFAULTTONULL))
            {
                return monitorOld == monitorNew;
            }
        }
        return false;
    }

    bool getWindowTopLeftOnScreen(const QPoint& _posAbs, QPoint& _topLeft)
    {
        if (auto rect = getMonitorWorkRect(_posAbs); rect)
        {
            _topLeft = _posAbs - rect->topLeft();
            return true;
        }

        return false;
    }

    void updateSystemMenu(HWND _hWnd, HMENU& _menu)
    {
        if (!_hWnd || !_menu)
            return;

        auto maximized = IsZoomed(_hWnd);
        for (int i = 0; i < GetMenuItemCount(_menu); ++i)
        {
            MENUITEMINFO itemInfo = { 0 };
            itemInfo.cbSize = sizeof(itemInfo);
            itemInfo.fMask = MIIM_TYPE | MIIM_STATE | MIIM_ID;
            if (GetMenuItemInfo(_menu, i, TRUE, &itemInfo))
            {
                if (itemInfo.fType & MF_SEPARATOR)
                    continue;

                if (itemInfo.wID && !(itemInfo.fState & MF_DEFAULT))
                {
                    UINT fState = itemInfo.fState & ~(MF_DISABLED | MF_GRAYED);
                    if (itemInfo.wID == SC_CLOSE)
                        fState |= MF_DEFAULT;
                    else if ((maximized && (itemInfo.wID == SC_MAXIMIZE || itemInfo.wID == SC_SIZE || itemInfo.wID == SC_MOVE))
                        || (!maximized && itemInfo.wID == SC_RESTORE))
                        fState |= MF_DISABLED | MF_GRAYED;

                    itemInfo.fMask = MIIM_STATE;
                    itemInfo.fState = fState;
                    if (!SetMenuItemInfo(_menu, i, TRUE, &itemInfo))
                    {
                        DestroyMenu(_menu);
                        _menu = 0;
                        break;
                    }
                }
            }
            else
            {
                DestroyMenu(_menu);
                _menu = 0;
                break;
            }
        }
    }
#endif

    auto getWindowMarginOnScreen()
    {
        return Utils::scale_value(5);
    }

    auto getBgColor()
    {
        return Styling::getParameters().getColor(Styling::StyleVariable::BASE_GLOBALWHITE);
    }

    QSize getTitleButtonOriginalSize() noexcept
    {
        return { 44, 28 };
    }

    QSize getTitleButtonSize()
    {
        return Utils::scale_value(getTitleButtonOriginalSize());
    }

    constexpr std::chrono::milliseconds getOfflineTimeout() noexcept
    {
        return std::chrono::seconds(30);
    }

    constexpr std::chrono::milliseconds getUserActivityCheckTimeout() noexcept
    {
        return std::chrono::milliseconds(1000);
    }

    constexpr std::chrono::milliseconds getUserInactiveTimeout() noexcept
    {
        return std::chrono::minutes(1);
    }

    auto getErrorToastOffset() noexcept
    {
        return Utils::scale_value(10);
    }

    auto minimumWindowWidth()
    {
        const auto windowWidth = Utils::scale_value(380);
        return Features::isAppsNavigationBarVisible() ? (Ui::AppsNavigationBar::defaultWidth() + windowWidth) : windowWidth;
    }

    auto defaultWindowWidth()
    {
        const auto windowWidth = Utils::scale_value(1000);
        return Features::isAppsNavigationBarVisible() ? (Ui::AppsNavigationBar::defaultWidth() + windowWidth) : windowWidth;
    }

    auto defaultWindowHeight() noexcept
    {
        return Utils::scale_value(600);
    }

    QString makeTitle()
    {
        auto appTitle = Utils::getAppTitle();
        if constexpr (environment::is_alpha())
        {
            return appTitle % u" alpha version";
        }
        else if constexpr (environment::is_beta())
        {
            return appTitle % u" beta version";
        }
        else if constexpr (environment::is_develop())
        {
            const auto branchName = []() -> QLatin1String {
#ifdef GIT_BRANCH_NAME
                return ql1s(GIT_BRANCH_NAME);
#else
                return QLatin1String();
#endif
            }();
            const auto space = branchName.size() > 0 ? QStringView(u" ") : QStringView();
            const auto devWindowFlag = config::get().has_develop_cli_flag() ? QStringView(u" dev_window") : QStringView();
            return appTitle % u" develop version" % space % branchName % devWindowFlag;
        }
        return appTitle;
    }

#ifdef __linux__
    // force release of unused memory
    // https://codereview.qt-project.org/c/qt-creator/qt-creator/+/299132

    constexpr std::chrono::milliseconds getTrimTimerInterval() noexcept
    {
        return std::chrono::minutes(1);
    }

    class MemoryTrimmer : public QObject
    {
    public:
        MemoryTrimmer()
        {
            trimTimer_.setSingleShot(true);
            trimTimer_.setInterval(getTrimTimerInterval());
            // glibc may not actually free memory in free().
            connect(&trimTimer_, &QTimer::timeout, this, []() { malloc_trim(0); });
        }

        bool eventFilter(QObject* _o, QEvent* _e) override
        {
            if ((_e->type() == QEvent::MouseButtonPress || _e->type() == QEvent::KeyPress) && !trimTimer_.isActive())
                trimTimer_.start();

            return false;
        }

        QTimer trimTimer_;
    } memoryTrimmer;
#endif

#ifndef STRIP_CRASH_HANDLER
#ifdef __APPLE__
    void initCrashpad(std::string_view _baseUrl, std::string_view _login)
    {
        static std::once_flag flag;
        auto initImpl = [_baseUrl, _login]()
        {
            const auto productPath = config::get().string(config::values::product_path_mac);
            const QString dumpPath = QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation)
                % u'/' % QString::fromUtf8(productPath.data(), productPath.size()) % u"/Dumps";

            QDir().mkpath(dumpPath);

            crash_system::reporter::instance().init(dumpPath.toStdString(), _baseUrl, _login, MacSupport::bundleDir().toStdString());
        };
        std::call_once(flag, initImpl);
    }
#endif //STRIP_CRASH_HANDLER
#endif // __APPLE__
}

namespace Ui
{
    void memberAddFailed(const int _error)
    {
        QString errorText;
        switch (_error)
        {
        case 453:
            errorText = QT_TRANSLATE_NOOP("sidebar", "You cannot add this contact to the group due to its privacy settings");
            break;
        default:
            break;
        }

        if (!errorText.isEmpty())
            Utils::showToastOverMainWindow(errorText, getErrorToastOffset(), 2);
    };

    ShadowWindow::ShadowWindow(const int shadowWidth)
        : QWidget(nullptr, Qt::Tool | Qt::FramelessWindowHint | Qt::NoDropShadowWindowHint)
        , ShadowWidth_(shadowWidth)
        , IsActive_(true)
    {
        setAttribute(Qt::WA_TransparentForMouseEvents);
        setAttribute(Qt::WA_NoSystemBackground);
        setAttribute(Qt::WA_TranslucentBackground);
    }

    void ShadowWindow::setActive(const bool _value)
    {
        IsActive_ = _value;
        update();
    }

    void ShadowWindow::paintEvent(QPaintEvent * /*e*/)
    {
        QRect origin = rect();

        QRect right = QRect(QPoint(origin.width() - ShadowWidth_, origin.y() + ShadowWidth_ + 1), QPoint(origin.width(), origin.height() - ShadowWidth_ - 1));
        QRect left = QRect(QPoint(origin.x(), origin.y() + ShadowWidth_ + 1), QPoint(origin.x() + ShadowWidth_, origin.height() - ShadowWidth_ - 1));
        QRect top = QRect(QPoint(origin.x() + ShadowWidth_ + 1, origin.y()), QPoint(origin.width() - ShadowWidth_ - 1, origin.y() + ShadowWidth_));
        QRect bottom = QRect(QPoint(origin.x() + ShadowWidth_ + 1, origin.height() - ShadowWidth_), QPoint(origin.width() - ShadowWidth_ - 1, origin.height()));

        QRect topLeft = QRect(origin.topLeft(), QPoint(origin.x() + ShadowWidth_, origin.y() + ShadowWidth_));
        QRect topRight = QRect(QPoint(origin.width() - ShadowWidth_, origin.y()), QPoint(origin.width(), origin.y() + ShadowWidth_));
        QRect bottomLeft = QRect(QPoint(origin.x(), origin.height() - ShadowWidth_), QPoint(origin.x() + ShadowWidth_, origin.height()));
        QRect bottomRight = QRect(QPoint(origin.width() - ShadowWidth_, origin.height() - ShadowWidth_), origin.bottomRight());

        QPainter p(this);

        QLinearGradient lg = QLinearGradient(right.topLeft(), right.topRight());
        setGradientColor(lg);
        p.fillRect(right, QBrush(lg));

        lg = QLinearGradient(left.topRight(), left.topLeft());
        setGradientColor(lg);
        p.fillRect(left, QBrush(lg));

        lg = QLinearGradient(top.bottomLeft(), top.topLeft());
        setGradientColor(lg);
        p.fillRect(top, QBrush(lg));

        lg = QLinearGradient(bottom.topLeft(), bottom.bottomLeft());
        setGradientColor(lg);
        p.fillRect(bottom, QBrush(lg));

        QRadialGradient g = QRadialGradient(topLeft.bottomRight(), ShadowWidth_);
        setGradientColor(g);
        p.fillRect(topLeft, QBrush(g));

        g = QRadialGradient(topRight.bottomLeft(), ShadowWidth_);
        setGradientColor(g);
        p.fillRect(topRight, QBrush(g));

        g = QRadialGradient(bottomLeft.topRight(), ShadowWidth_);
        setGradientColor(g);
        p.fillRect(bottomLeft, QBrush(g));

        g = QRadialGradient(bottomRight.topLeft(), ShadowWidth_);
        setGradientColor(g);
        p.fillRect(bottomRight, QBrush(g));
    }

    void ShadowWindow::setGradientColor(QGradient& _gradient)
    {
        QColor windowGradientColor(0, 0, 0);
        windowGradientColor.setAlphaF(0.2);
        _gradient.setColorAt(0, windowGradientColor);
        windowGradientColor.setAlphaF(IsActive_ ? 0.08 : 0.04);
        _gradient.setColorAt(0.2, windowGradientColor);
        windowGradientColor.setAlphaF(0.02);
        _gradient.setColorAt(0.6, IsActive_ ? windowGradientColor : Qt::transparent);
        _gradient.setColorAt(1, Qt::transparent);
    }

    TitleWidgetEventFilter::TitleWidgetEventFilter(QObject* parent)
        : QObject(parent)
        , dragging_(false)
    {
        mainWindow_ = static_cast<MainWindow*>(parent);

        iconTimer_ = new QTimer(this);
        iconTimer_->setSingleShot(true);
        connect(iconTimer_, &QTimer::timeout, this, &Ui::TitleWidgetEventFilter::showLogoContextMenu);
    }

    void TitleWidgetEventFilter::addIgnoredWidget(QWidget* _widget)
    {
        ignoredWidgets_.insert(_widget);
    }

    void TitleWidgetEventFilter::showContextMenu(QPoint _pos)
    {
#ifdef _WIN32
        auto handle = reinterpret_cast<HWND>(mainWindow_->winId());
        if (auto sysMenu = GetSystemMenu(handle, FALSE))
        {
            UINT enabledState = MF_BYCOMMAND | MF_ENABLED;
            UINT disabledState = MF_BYCOMMAND | (MF_DISABLED | MF_GRAYED);
            EnableMenuItem(sysMenu, SC_MAXIMIZE,mainWindow_->isMaximized()? disabledState: enabledState);
            EnableMenuItem(sysMenu, SC_SIZE,    mainWindow_->isMaximized()? disabledState: enabledState);
            EnableMenuItem(sysMenu, SC_MOVE,    mainWindow_->isMaximized()? disabledState: enabledState);
            EnableMenuItem(sysMenu, SC_RESTORE, mainWindow_->isMaximized()? enabledState: disabledState);

            int flag = TrackPopupMenu(sysMenu, TPM_LEFTALIGN|TPM_LEFTBUTTON|TPM_RETURNCMD, _pos.x(), _pos.y(), NULL, handle, nullptr);
            if (flag > 0)
                SendMessage(handle, WM_SYSCOMMAND, flag, TITLE_CONTEXT_MENU_CMD);
        }
#endif
    }

    void TitleWidgetEventFilter::showLogoContextMenu()
    {
        mainWindow_->activate();
        mainWindow_->updateMainMenu();

        auto logo = mainWindow_->getWindowLogo();
        showContextMenu(logo->mapToGlobal(logo->rect().center()));
    }

    bool TitleWidgetEventFilter::eventFilter(QObject* _obj, QEvent* _event)
    {
        if constexpr (platform::is_linux())
            return QObject::eventFilter(_obj, _event);

        bool ignore = std::any_of(ignoredWidgets_.cbegin(),
                                  ignoredWidgets_.cend(),
                                  [](QWidget* _wdg) { return _wdg->underMouse(); });
        if (!ignore && (_event->type() == QEvent::MouseButtonDblClick || _event->type() == QEvent::MouseButtonPress
            || _event->type() == QEvent::MouseMove || _event->type() == QEvent::MouseButtonRelease))
        {
            if (auto mouseEvent = static_cast<QMouseEvent*>(_event))
            {
                const bool logoUnderMouse = mainWindow_->getWindowLogo()->rect().contains(mouseEvent->pos());
                switch (_event->type())
                {
                case QEvent::MouseButtonDblClick:
                    if (logoUnderMouse)
                    {
                        iconTimer_->stop();
                        Q_EMIT logoDoubleClick();
                    }
                    else
                    {
                        Q_EMIT doubleClick();
                    }
                    dragging_ = false;

                    if constexpr (platform::is_windows())
                    {
                        _event->accept();
                        return true;
                    }

                    break;

                case QEvent::MouseButtonPress:
                    clickPos_ = mouseEvent->pos();
                    dragging_ = true;
                    _event->accept();
                    break;

                case QEvent::MouseMove:
                    if (dragging_)
                        Q_EMIT moveRequest(mouseEvent->globalPos() - clickPos_);
                    break;

                case QEvent::MouseButtonRelease:
                    dragging_ = false;
                    Q_EMIT checkPosition();
                    if (mouseEvent->button() == Qt::RightButton)
                        showContextMenu(mouseEvent->globalPos());
                    else if (logoUnderMouse)
                        iconTimer_->start(qApp->doubleClickInterval());

                    if constexpr (platform::is_windows())
                    {
                        _event->accept();
                        return true;
                    }

                    break;

                default:
                    break;
                }
            }
        }
        return QObject::eventFilter(_obj, _event);
    }

    void MainWindow::hideTaskbarIcon()
    {
#ifdef _WIN32
        auto hwnd = reinterpret_cast<HWND>(winId());
        if (!reinterpret_cast<HWND>(::GetWindowLong(hwnd, GWL_HWNDPARENT)))
            ::SetWindowLong(hwnd, GWL_HWNDPARENT, (LONG) fake_parent_window_);
#endif //_WIN32
        trayIcon_->forceUpdateIcon();
    }

    void MainWindow::showTaskbarIcon()
    {
#ifdef _WIN32
        ::SetWindowLong(reinterpret_cast<HWND>(winId()), GWL_HWNDPARENT, 0L);

        std::unique_ptr<QWidget> w(new QWidget(this));
        w->setWindowFlags(Qt::Tool | Qt::FramelessWindowHint);
        w->show();
        w->activateWindow();
        trayIcon_->forceUpdateIcon();
#endif //_WIN32
    }

    void MainWindow::showMenuBarIcon(bool _show)
    {
        trayIcon_->setVisible(_show);
    }

    int MainWindow::getTitleHeight() const
    {
        return platform::is_windows() ? titleWidget_->height() : 0;
    }

    bool MainWindow::isMaximized() const
    {
        return platform::is_windows() ? isMaximized_ : QMainWindow::isMaximized();
    }

    void MainWindow::lock()
    {
        if (!appsPage_) // init main page to show it faster after unlock
            initAppsPage();

        if (!localPINWidget_)
        {
            localPINWidget_ = new LocalPINWidget(LocalPINWidget::Mode::VerifyPIN, this);
            Testing::setAccessibleName(localPINWidget_, qsl("AS LocalPin"));
            stackedWidget_->addWidget(localPINWidget_);
        }

        setCurrentWidget(localPINWidget_, ForceLocked::Yes);

        closeGallery();
        Q_EMIT Utils::InterConnector::instance().applicationLocked();
#ifdef __APPLE__
        getMacSupport()->createMenuBar(true);
#endif
    }

    MainWindow::MainWindow(QApplication* _app, const bool _hasValidLogin, const bool _locked, const QString& _validOrFirstLogin)
        : appsPage_(nullptr)
        , loginPage_(nullptr)
        , gdprPage_(nullptr)
        , app_(_app)
        , eventFilter_(nullptr)
        , trayIcon_(nullptr)
        , Shadow_(nullptr)
        , ffplayer_(nullptr)
        , callStatsMgr_(nullptr)
        , connStateWatcher_(nullptr)
        , SkipRead_(false)
        , TaskBarIconHidden_(false)
        , isMaximized_(false)
        , sleeping_(false)
        , userActivityTimer_(nullptr)
        , updateWhenUserInactiveTimer_(nullptr)
#ifdef _WIN32
        , isAeroEnabled_(QtWin::isCompositionEnabled())
        , menu_(CreateMenu())
#endif
        , windowActive_(Testing::isAutoTestsMode())
        , uiActive_(Testing::isAutoTestsMode())
        , uiActiveMainWindow_(false)
        , maximizeLater_(false)
        , localPINWidget_(nullptr)
        , validOrFirstLogin_(_validOrFirstLogin)
    {
        auto scopedExit = Utils::ScopeExitT([this]() { inCtor_ = false; });
        Utils::InterConnector::instance().setMainWindow(this);

        eventFilter_ = new TitleWidgetEventFilter(this);
        trayIcon_ = new TrayIcon(this);
#ifndef STRIP_VOIP
        if (config::get().is_on(config::features::call_quality_stat))
            callStatsMgr_ = new CallQualityStatsMgr(this);
#endif

        connStateWatcher_ = new ConnectionStateWatcher(this);
#ifndef STRIP_CRASH_HANDLER
#if defined(__APPLE__) || defined(__linux__)
        if (!GetAppConfig().IsSysCrashHandleEnabled())
        {
#ifdef __APPLE__

            if (!config::get().is_on(config::features::external_url_config))
            {
                initCrashpad(config::get().url(config::urls::base_binary), validOrFirstLogin_.toStdString());
            }
            else if (const auto& baseBinary = getUrlConfig().getBaseBinary(); !baseBinary.isEmpty())
            {
                initCrashpad(baseBinary.toStdString(), validOrFirstLogin_.toStdString());
            }
            else
            {
                connect(Ui::GetDispatcher(), &Ui::core_dispatcher::externalUrlConfigUpdated, this, [validOrFirstLogin_ = validOrFirstLogin_]()
                {
                    if (const auto& baseBinary = getUrlConfig().getBaseBinary(); !baseBinary.isEmpty())
                        initCrashpad(baseBinary.toStdString(), validOrFirstLogin_.toStdString());
                });
            }
#elif __linux__
            crash_system::reporter::instance();
#endif // __APPLE__
        }
        else
        {
            crash_system::reporter::instance().uninstall();
        }
#endif //defined(__APPLE__) || defined(__linux__)
#endif // !STRIP_CRASH_HANDLER

#ifdef __APPLE__
        getMacSupport()->listenSystemEvents();
#endif // __APPLE__

        updateFormatActions();

        setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Minimum);
        setLayoutDirection(Qt::LeftToRight);
        Utils::setDefaultBackground(this);

        mainWidget_ = new QWidget(this);
        mainWidget_->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Minimum);
        Testing::setAccessibleName(mainWidget_, qsl("AS MainWindow mainWidget"));
        mainLayout_ = Utils::emptyVLayout(mainWidget_);
        mainLayout_->setSizeConstraint(QLayout::SetDefaultConstraint);
        titleWidget_ = new QWidget(mainWidget_);
        titleWidget_->setAutoFillBackground(true);
        Utils::updateBgColor(titleWidget_, Styling::getParameters().getAppTitlebarColor());
        titleWidget_->setFixedHeight(Utils::scale_value(28));
        titleWidget_->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);
        Testing::setAccessibleName(titleWidget_, qsl("AS MainWindow titleWidget"));
        Utils::ApplyStyle(titleWidget_, Styling::getParameters().getTitleQss());

        titleLayout_ = Utils::emptyHLayout(titleWidget_);

        logo_ = new QLabel(titleWidget_);
        auto pixmap = QPixmap(Utils::parse_image_name(qsl(":/logo/logo_16_100")));
        logo_->setPixmap(pixmap.isNull() ? Utils::renderSvgScaled(qsl(":/logo/logo_16"), QSize(16, 16)) : pixmap);
        logo_->setObjectName(qsl("windowIcon"));
        logo_->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
        logo_->setFocusPolicy(Qt::NoFocus);
        Testing::setAccessibleName(logo_, qsl("AS MainWindow logo"));
        titleLayout_->addWidget(logo_);

        title_ = new QLabel(titleWidget_);
        title_->setFocusPolicy(Qt::NoFocus);
        updateTitlePalette();

        title_->setFont(Fonts::appFontScaled(12, Fonts::FontWeight::SemiBold));
        title_->setContentsMargins(Utils::scale_value(8), 0, 0, 0);
        Testing::setAccessibleName(title_, qsl("AS MainWindow title"));
        titleLayout_->addWidget(title_);
        titleLayout_->addItem(new QSpacerItem(Utils::scale_value(20), 20, QSizePolicy::Fixed, QSizePolicy::Minimum));

        spacer_ = new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);
        titleLayout_->addItem(spacer_);

        const auto titleBtn = [p = titleWidget_](const auto& _path, const auto& _accName, const auto _hoverClr, const auto _pressClr) -> CustomButton*
        {
            auto btn = new CustomButton(p, _path, getTitleButtonOriginalSize());
            btn->setBackgroundNormal(Styling::ThemeColorKey{ Styling::StyleVariable::BASE_BRIGHT_INVERSE });
            btn->setBackgroundHovered(Styling::ThemeColorKey{ _hoverClr });
            btn->setBackgroundPressed(Styling::ThemeColorKey{ _pressClr });
            btn->setFixedSize(getTitleButtonSize());
            btn->setFocusPolicy(Qt::NoFocus);
            Testing::setAccessibleName(btn, _accName);
            Styling::Buttons::setButtonDefaultColors(btn);

            return btn;
        };

        hideButton_ = titleBtn(qsl(":/titlebar/minimize_button"), qsl("AS MainWindow hideButton"), Styling::StyleVariable::BASE_BRIGHT_HOVER, Styling::StyleVariable::BASE_BRIGHT_ACTIVE);
        titleLayout_->addWidget(hideButton_);

        maximizeButton_ = titleBtn(qsl(":/titlebar/expand_button"), qsl("AS MainWindow maximizeButton"), Styling::StyleVariable::BASE_BRIGHT_HOVER, Styling::StyleVariable::BASE_BRIGHT_ACTIVE);
        titleLayout_->addWidget(maximizeButton_);

        closeButton_ = titleBtn(qsl(":/titlebar/close_button"), qsl("AS MainWindow closeButton"), Styling::StyleVariable::SECONDARY_ATTENTION, Styling::StyleVariable::SECONDARY_ATTENTION);
        closeButton_->setHoverColor(Styling::ThemeColorKey{ Styling::StyleVariable::TEXT_SOLID_PERMANENT });
        closeButton_->setPressedColor(Styling::ThemeColorKey{ Styling::StyleVariable::TEXT_SOLID_PERMANENT });
        titleLayout_->addWidget(closeButton_);

        mainLayout_->addWidget(titleWidget_);
        stackedWidget_ = new MainStackedWidget(mainWidget_);

        Testing::setAccessibleName(stackedWidget_, qsl("AS MainWindow stackedWidget"));
        mainLayout_->addWidget(stackedWidget_);
        this->setCentralWidget(mainWidget_);

        stackedWidget_->setCurrentIndex(-1);

        QFont f = QApplication::font();
        f.setStyleStrategy(QFont::PreferAntialias);
        // FONT
        QApplication::setFont(f);

        if (_locked)
            LocalPIN::instance()->lock();
        else if (!get_gui_settings()->get_value(settings_keep_logged_in, settings_keep_logged_in_default()) || !_hasValidLogin)
            showLoginPage(false);
        else
            showAppsPage();

        auto filterAddIgnoreWidget = [this](auto _widget)
        {
            if (_widget)
                eventFilter_->addIgnoredWidget(_widget);
        };
        filterAddIgnoreWidget(hideButton_);
        filterAddIgnoreWidget(maximizeButton_);
        filterAddIgnoreWidget(closeButton_);
        titleWidget_->installEventFilter(eventFilter_);
        connect(eventFilter_, &Ui::TitleWidgetEventFilter::logoDoubleClick, this, &Ui::MainWindow::hideWindow);

        title_->setAttribute(Qt::WA_TransparentForMouseEvents);
        logo_->setAttribute(Qt::WA_TransparentForMouseEvents);

#ifdef _WIN32
        setWindowFlags(windowFlags() | Qt::Window | Qt::FramelessWindowHint | Qt::WindowMinMaxButtonsHint | Qt::WindowSystemMenuHint);

        auto hwnd = reinterpret_cast<HWND>(winId());
        SetMenu(hwnd, menu_);//avoids window resizing by Qt on display changing; if menu exists Qt forces NCCALCSIZE instead of AdjustWindowRectEx (which adds margins for caption, box and other stuff which is virtual and actually doesn't exist)
        SetWindowLongPtr(hwnd, GWL_STYLE, static_cast<LONG>(isAeroEnabled_ ? Style::AeroBorderless : Style::BasicBorderless));

        if (isAeroEnabled_)
            QtWin::extendFrameIntoClientArea(this, 1, 1, 1, 1);

        fake_parent_window_ = Utils::createFakeParentWindow();

        VirtualDesktops::initVirtualDesktopManager();
#else
        titleWidget_->hide();
#endif

        title_->setMouseTracking(true);

        connect(hideButton_, &QPushButton::clicked, this, &MainWindow::minimize);
        connect(maximizeButton_, &QPushButton::clicked, this, &MainWindow::maximize);
        connect(closeButton_, &QPushButton::clicked, this, &MainWindow::hideWindow);

        if constexpr (!platform::is_windows())
        {
            connect(eventFilter_, &Ui::TitleWidgetEventFilter::doubleClick, this, &MainWindow::maximize);
            connect(eventFilter_, &Ui::TitleWidgetEventFilter::checkPosition, this, &MainWindow::checkPosition);
            connect(eventFilter_, &Ui::TitleWidgetEventFilter::moveRequest, this, &MainWindow::moveRequest);
        }

#ifndef STRIP_VOIP
        connect(&Ui::GetDispatcher()->getVoipController(), &voip_proxy::VoipController::onVoipResetComplete, this, &MainWindow::onVoipResetComplete, Qt::QueuedConnection);
        if (callStatsMgr_)
            connect(&Ui::GetDispatcher()->getVoipController(), &voip_proxy::VoipController::onVoipCallEndedStat, callStatsMgr_, &CallQualityStatsMgr::onVoipCallEndedStat);
#endif

        connect(Ui::GetDispatcher(), &core_dispatcher::connectionStateChanged, connStateWatcher_, &ConnectionStateWatcher::setState);

        connect(Ui::GetDispatcher(), &core_dispatcher::needLogin, this, &MainWindow::showLoginPage, Qt::DirectConnection);
        connect(&Utils::InterConnector::instance(), &Utils::InterConnector::showIconInTaskbar, this, &MainWindow::showIconInTaskbar);

        connect(&Utils::InterConnector::instance(), &Utils::InterConnector::activateNextUnread, this, &MainWindow::activateNextUnread);

        connect(&Utils::InterConnector::instance(), &Utils::InterConnector::noPagesInDialogHistory, this, [this]() {
            if (auto mainPage = MainPage::instance(); mainPage->isOneFrameTab())
            {
                mainPage->update();
                setFocus();
            }
        });

        if constexpr (platform::is_windows())
        {
            connect(&Utils::InterConnector::instance(), &Utils::InterConnector::incomingCallAlert, this, [this]() {
                QApplication::alert(this, 500);
            });
        }
        
        connect(&Utils::InterConnector::instance(), &Utils::InterConnector::openStatusPicker, this, &MainWindow::openStatusPicker);

        connect(this, &MainWindow::needActivate, this, &MainWindow::activate);
        connect(this, &MainWindow::needActivateWith, this, &MainWindow::activateWithReason);

        connect(get_gui_settings(), &Ui::qt_gui_settings::changed, this, &MainWindow::guiSettingsChanged);

        connect(Ui::GetDispatcher(), &Ui::core_dispatcher::memberAddFailed, this, Ui::memberAddFailed);
        connect(Ui::GetDispatcher(), &Ui::core_dispatcher::needExternalUserAgreement, this, &MainWindow::showUserAgreementPage, Qt::UniqueConnection);

        connect(&Utils::InterConnector::instance(), &Utils::InterConnector::omicronUpdated, this, &MainWindow::omicronUpdated);
        connect(&Utils::InterConnector::instance(), &Utils::InterConnector::startSearchHashtag, this, [](const QString& _pattern)
        {
            if (const auto& aimId = Logic::getContactListModel()->selectedContact();
                !aimId.isEmpty() && !ServiceContacts::isServiceContact(aimId))
            {
                Q_EMIT Utils::InterConnector::instance().startSearchInDialog(aimId, _pattern);
            }
        });
#if 0
        if constexpr (platform::is_windows())
        {
            const auto shadowWidth = get_gui_settings()->get_shadow_width();
            Shadow_ = new ShadowWindow(shadowWidth);
            QPoint pos = mapToGlobal(QPoint(rect().x(), rect().y()));
            Shadow_->move(pos.x() - shadowWidth, pos.y() - shadowWidth);
            Shadow_->resize(rect().width() + 2 * shadowWidth, rect().height() + 2 * shadowWidth);

            Shadow_->setActive(true);
            Shadow_->show();
        }
#endif

#ifdef _WIN32
        DragAcceptFiles(hwnd, TRUE);
#endif //_WIN32

        if (!get_gui_settings()->get_value<bool>(settings_show_in_taskbar, true))
            hideTaskbarIcon();

#ifdef __APPLE__
        getMacSupport()->enableMacUpdater();
        getMacSupport()->enableMacPreview(winId());

        getMacSupport()->windowTitle()->setup();
#endif

        updateWindowTitle();

        QTimer::singleShot(2000, this, &MainWindow::checkNewVersion);

        setMouseTracking(true);

        userActivityTimer_ = new QTimer(this);
        QObject::connect(userActivityTimer_, &QTimer::timeout, this, &MainWindow::userActivityTimer);
        userActivityTimer_->start(getUserActivityCheckTimeout());

        if constexpr (!platform::is_apple())
        {
            updateWhenUserInactiveTimer_ = new QTimer(this);
            updateWhenUserInactiveTimer_->setInterval(getUserInactiveTimeout());
            updateWhenUserInactiveTimer_->setSingleShot(true);
            connect(updateWhenUserInactiveTimer_, &QTimer::timeout, &Utils::InterConnector::instance(), &Utils::InterConnector::updateWhenUserInactive);
        }

        // request privacy settings only after creating a connection
        connect(Ui::GetDispatcher(), &Ui::core_dispatcher::im_created, Logic::GetPrivacySettingsContainer(), &Logic::PrivacySettingsContainer::requestValues);


#ifndef STRIP_AV_MEDIA
        // initialize cache in main thread
        LottieHandleCache::instance();
#endif

        connect(&Styling::getThemesContainer(), &Styling::ThemesContainer::globalThemeChanged, this, &MainWindow::onThemeChanged, Qt::QueuedConnection);
    }

    void MainWindow::onDestroy()
    {
        Tooltip::resetDefaultTooltip();
        Tooltip::resetDefaultMultilineTooltip();

        app_->removeNativeEventFilter(this);
        app_->removeEventFilter(this);
#ifdef __linux__
        app_->removeEventFilter(&memoryTrimmer);
#endif

#ifdef _WIN32
        auto hwnd = reinterpret_cast<HWND>(winId());
        if (!GetMenu(hwnd))//according to msdn there is no need to destroy menu which is assigned to a window
            DestroyMenu(menu_);

        if (fake_parent_window_)
        {
            ::SetWindowLong(hwnd, GWL_HWNDPARENT, 0L);
            ::DestroyWindow(fake_parent_window_);
        }

        VirtualDesktops::resetVirtualDesktopManager();
#endif

    }

    MainWindow::~MainWindow()
    {
    }

    void MainWindow::init(bool _needShow)
    {
        app_->installNativeEventFilter(this);
        app_->installEventFilter(this);
#ifdef __linux__
        app_->installEventFilter(&memoryTrimmer);
#endif

        initSettings(_needShow);
        initStats();

        updateMainMenu();

#ifdef _WIN32
        // redraw frame
        SetWindowPos(reinterpret_cast<HWND>(winId()), nullptr, 0, 0, 0, 0, SWP_FRAMECHANGED | SWP_NOMOVE | SWP_NOSIZE);
#endif
    }

    void MainWindow::onOpenChat(const std::string& _contact)
    {
        raise();
        activate();

        if (!_contact.empty())
            Utils::InterConnector::instance().openDialog(QString::fromStdString(_contact));
    }

    void MainWindow::activateWithReason(ActivateReason _reason)
    {
        if (_reason != ActivateReason::ByMailClick)
            activate();
        if (_reason == ActivateReason::ByNotificationClick)
            statistic::getGuiMetrics().eventForegroundByNotification();
    }

    void MainWindow::activate()
    {
        if constexpr (platform::is_apple())
        {
            // workaround - remove blinking after hide/minimize the window
            setVisible(false);
            activateWindow();
            updateMainMenu();
        }
        setVisible(true);
        activateWindow();
#ifdef _WIN32
        auto hwnd = reinterpret_cast<HWND>(winId());
        ShowWindow(hwnd, SW_SHOW);
        if (isMaximized())
            showMaximized();
        else if (IsIconic(hwnd))
            SendMessage(hwnd, WM_SYSCOMMAND, SC_RESTORE, 0);
        else
            showNormal();

        if (Shadow_)
            SetWindowPos((HWND)Shadow_->winId(), (HWND)winId(), 0, 0, 0, 0, SWP_NOACTIVATE | SWP_NOOWNERZORDER | SWP_NOMOVE | SWP_NOSIZE | SWP_SHOWWINDOW);
#endif //_WIN32
        if constexpr (platform::is_windows())
        {
            showIconInTaskbar(get_gui_settings()->get_value<bool>(settings_show_in_taskbar, true));
        }
#ifdef __APPLE__
        getMacSupport()->activateWindow(winId());
        updateMainMenu();
#endif //__APPLE__
#ifdef __linux__
        isMaximized() ? showMaximized() : showNormal();
#endif //__linux__

        trayIcon_->hideAlerts();
        showHideGallery();

        for (auto page : Utils::InterConnector::instance().getVisibleHistoryPages())
            page->updateItems();

        updateMainMenu();
    }

    void MainWindow::openGallery(const Utils::GalleryData& _data)
    {
        if (gallery_)
            return;

        statistic::getGuiMetrics().eventOpenGallery();

        gallery_ = new Previewer::GalleryWidget(this); // will delete itself on close
        connect(gallery_, &Previewer::GalleryWidget::finished, this, &MainWindow::exit);
        connect(gallery_, &Previewer::GalleryWidget::closed, this, &MainWindow::galeryClosed);
        gallery_->openGallery(_data);
    }

    void MainWindow::showHideGallery()
    {
        if (gallery_ && gallery_->isHidden() && !gallery_->isClosing())
            gallery_->showFullScreen();
    }

    void MainWindow::openAvatar(const QString &_aimId)
    {
        if (gallery_)
            return;

        gallery_ = new Previewer::GalleryWidget(this); // will delete itself on close
        connect(gallery_, &Previewer::GalleryWidget::finished, this, &MainWindow::exit);
        connect(gallery_, &Previewer::GalleryWidget::closed, this, &MainWindow::galeryClosed);
        gallery_->openAvatar(_aimId);
    }

    void MainWindow::closeGallery()
    {
        if (gallery_)
            gallery_->closeGallery();
    }

    void MainWindow::activateFromEventLoop(ActivateReason _reason)
    {
        Q_EMIT activateWithReason(_reason);
    }

    bool MainWindow::isActive() const
    {
#ifdef _WIN32
        // Check the absence of statuses isMinimized or hided, because native window can be focused in this state.
        return GetForegroundWindow() == reinterpret_cast<HWND>(winId()) && !isMinimized();
#else
        return isActiveWindow();
#endif //_WIN32
    }

    bool MainWindow::isUIActive() const
    {
        return uiActive_;
    }

    bool MainWindow::isUIActiveMainWindow() const
    {
        return uiActiveMainWindow_;
    }

    bool MainWindow::isAppsPage() const
    {
        return appsPage_ && stackedWidget_->currentWidget() == appsPage_;
    }

    bool MainWindow::isMessengerPage() const
    {
        return isAppsPage() && appsPage_->isMessengerPage();
    }

    bool MainWindow::isMessengerPageContactDialog() const
    {
        return isAppsPage() && appsPage_->isContactDialog();
    }

    bool MainWindow::isWebAppTasksPage() const
    {
        return isAppsPage() && appsPage_->isTasksPage();
    }

    bool MainWindow::isFeedAppPage() const
    {
        return isAppsPage() && appsPage_->isFeedPage();
    }

    int MainWindow::getScreen() const
    {
        return QApplication::desktop()->screenNumber(this);
    }

    QRect MainWindow::screenGeometry() const
    {
        return QApplication::desktop()->screenGeometry(getScreen());
    }

    QRect MainWindow::screenAvailableGeometry() const
    {
        return QApplication::desktop()->availableGeometry(getScreen());
    }

    QRect MainWindow::availableVirtualGeometry() const
    {
        return QGuiApplication::primaryScreen()->availableVirtualGeometry();
    }

    int MainWindow::getMinWindowWidth() const
    {
        const auto screenWidth = screenAvailableGeometry().width();
        return std::min(minimumWindowWidth(), screenWidth - getWindowMarginOnScreen() * 2);
    }

    int MainWindow::getMinWindowHeight() const
    {
        const auto screenHeight = screenAvailableGeometry().height();
        return std::min(Utils::scale_value(580), screenHeight - getWindowMarginOnScreen());
    }

    QRect MainWindow::getDefaultWindowSize() const
    {
        auto available = screenAvailableGeometry();
        const auto defaultSize = QRect(available.x(), available.y(), defaultWindowWidth(), defaultWindowHeight());
        if (!available.contains(defaultSize))
        {
            const auto margin = getWindowMarginOnScreen();
            return available.marginsRemoved(QMargins(margin, 0, margin, margin));
        }

        return defaultSize;
    }

    QRect MainWindow::getDefaultWindowGeometry() const
    {
        const auto center = screenAvailableGeometry().center();
        auto defaultWindowRect = getDefaultWindowSize();
        defaultWindowRect.moveTo(center.x() - defaultWindowRect.width() / 2, center.y() - defaultWindowRect.height() / 2);

        return defaultWindowRect;
    }

    void MainWindow::skipRead()
    {
        SkipRead_ = true;
    }

    void MainWindow::closePopups(const Utils::CloseWindowInfo& _info)
    {
        // FIXME: check if works && find actual fix
        if (Ui::MyInfo()->isGdprAgreementInProgress())
            return;

        closeGallery();

        Q_EMIT Utils::InterConnector::instance().closeAnyPopupMenu();
        Q_EMIT Utils::InterConnector::instance().closeAnyPopupWindow(_info);
        Q_EMIT Utils::InterConnector::instance().closeAnySemitransparentWindow(_info);
    }

    MainPage* MainWindow::getMessengerPage() const
    {
        return appsPage_ ? appsPage_->messengerPage() : nullptr;
    }

#if HAS_WEB_ENGINE
    Ui::WebAppPage* MainWindow::getTasksPage() const
    {
        return appsPage_ ? appsPage_->tasksPage() : nullptr;
    }
#endif

    AppsPage* MainWindow::getAppsPage() const
    {
        return appsPage_;
    }

    QLabel* MainWindow::getWindowLogo() const
    {
        return logo_;
    }

    void MainWindow::notifyWindowActive(const bool _active)
    {
        if (appsPage_->isContactDialog())
        {
            MainPage::instance()->notifyApplicationWindowActive(_active);
        }

        windowActive_ = _active;

        needChangeState();

        updateMainMenu();
        statistic::getGuiMetrics().eventMainWindowActive();
    }

    void MainWindow::saveWindowGeometryAndState(QSize size)
    {
        if (inCtor_)
            return;
        if (isMaximized())
        {
            get_gui_settings()->set_value(settings_window_maximized, true);
        }
        else
        {
            QRect rc = Ui::get_gui_settings()->get_value(settings_main_window_rect, QRect());
            bool changeWindowSize = true;

            if constexpr (platform::is_windows())
                if (isMinimized())
                    changeWindowSize = false;

            if (changeWindowSize)
                rc.setSize(size);

            rc.moveTo(!platform::is_windows() ? pos() : geometry().topLeft());

            get_gui_settings()->set_value(settings_main_window_rect, rc);
            get_gui_settings()->set_value(settings_desktop_rect, screenAvailableGeometry());
            get_gui_settings()->set_value(settings_available_geometry, availableVirtualGeometry());
            get_gui_settings()->set_value(settings_window_maximized, false);
        }
    }

    void MainWindow::onEscPressed(const QString& _aimId, const bool _autorepeat)
    {
        auto mainPage = MainPage::instance();
        if (!mainPage)
            return;

        if (mainPage->isNavigatingInRecents())
        {
            mainPage->stopNavigatingInRecents();

            if (_aimId.isEmpty())
                setFocus();
            else
                Q_EMIT Utils::InterConnector::instance().setFocusOnInput(_aimId);
        }
        else if (auto contactDialog = mainPage->getContactDialog(); mainPage->isInRecentsTab() && contactDialog->escCancel_->canCancel())
        {
            if constexpr (build::is_debug())
                contactDialog->escCancel_->dumpTree(0);

            contactDialog->escCancel_->cancel();

            if (mainPage->isSearchActive() && mainPage->getFrameCount() == FrameCountMode::_1)
                mainPage->setSearchFocus();
        }
        else if (!_autorepeat)
        {
            auto cur = static_cast<QWidget*>(qApp->focusObject());
            if (cur != this && (qobject_cast<Ui::ClickableWidget*>(cur) || qobject_cast<QAbstractButton*>(cur)))
            {
                cur->clearFocus();
                Q_EMIT Utils::InterConnector::instance().setFocusOnInput(_aimId);
            }
            else if (mainPage->isSearchActive())
            {
                mainPage->openRecents();
            }
            else if (_aimId.isEmpty())
            {
                 if (get_gui_settings()->get_value<bool>(settings_fast_drop_search_results, settings_fast_drop_search_default()))
                    mainPage->setSearchFocus();
            }
        }
    }

    void MainWindow::initAppsPage()
    {
        appsPage_ = new AppsPage(this);
        connect(Logic::GetAppsContainer(), &Logic::MiniAppsContainer::updateApps, this, &MainWindow::onMiniAppsUpdated, Qt::UniqueConnection);
        Testing::setAccessibleName(appsPage_, qsl("AS AppsPage"));
        stackedWidget_->addWidget(appsPage_);
    }

    void MainWindow::updateFormatActions()
    {
        const auto normalIconColor = Styling::getParameters().getColor(Styling::StyleVariable::BASE_SECONDARY);
        const auto disabledIconColor = Styling::getParameters().getColor(Styling::StyleVariable::BASE_TERTIARY);
        auto loadIcon = [&normalIconColor, &disabledIconColor](const QString& _name)
        {
            auto icon = QIcon(Utils::renderSvgScaled(_name, ContextMenu::defaultIconSize(), normalIconColor));
            icon.addPixmap(Utils::renderSvgScaled(_name, ContextMenu::defaultIconSize(), disabledIconColor), QIcon::Mode::Disabled);
            return icon;
        };
        auto makeFormatActionData = [](const QString& _command)
        {
            QMap<QString, QVariant> result;
            result[qsl("command")] = _command;
            return result;
        };

        const auto actionsData = std::array
        {
            std::make_tuple(core::data::format_type::bold, QT_TRANSLATE_NOOP("context_menu", "Bold"), qsl("Ctrl+B"), loadIcon(qsl(":/context_menu/bold")), qsl("format/bold")),
            std::make_tuple(core::data::format_type::italic, QT_TRANSLATE_NOOP("context_menu", "Italic"), qsl("Ctrl+I"), loadIcon(qsl(":/context_menu/italic")), qsl("format/italic")),
            std::make_tuple(core::data::format_type::underline, QT_TRANSLATE_NOOP("context_menu", "Underline"), qsl("Ctrl+U"), loadIcon(qsl(":/context_menu/underline")), qsl("format/underline")),
            std::make_tuple(core::data::format_type::strikethrough, QT_TRANSLATE_NOOP("context_menu", "Strike through"), qsl("Ctrl+Shift+X"), loadIcon(qsl(":/context_menu/strikethrough")), qsl("format/strikethrough")),
            std::make_tuple(core::data::format_type::monospace, QT_TRANSLATE_NOOP("context_menu", "Monospace"), qsl("Ctrl+Shift+M"), loadIcon(qsl(":/context_menu/monospace")), qsl("format/monospace")),
            std::make_tuple(core::data::format_type::link, QT_TRANSLATE_NOOP("context_menu", "Create link"), qsl("Ctrl+Shift+K"), loadIcon(qsl(":/context_menu/link")), qsl("format/link")),
            std::make_tuple(core::data::format_type::unordered_list, QT_TRANSLATE_NOOP("context_menu", "List"), qsl("Ctrl+Shift+8"), loadIcon(qsl(":/context_menu/unordered_list")), qsl("format/unordered_list")),
            std::make_tuple(core::data::format_type::ordered_list, QT_TRANSLATE_NOOP("context_menu", "Ordered list"), qsl("Ctrl+Shift+7"), loadIcon(qsl(":/context_menu/ordered_list")), qsl("format/ordered_list")),
            std::make_tuple(core::data::format_type::quote, QT_TRANSLATE_NOOP("context_menu", "Quote"), qsl("Ctrl+Shift+9"), loadIcon(qsl(":/context_menu/quote")), qsl("format/quote")),
            std::make_tuple(core::data::format_type::pre, QT_TRANSLATE_NOOP("context_menu", "Code"), qsl("Ctrl+Shift+Alt+C"), loadIcon(qsl(":/context_menu/code")), qsl("format/code")),
            std::make_tuple(core::data::format_type::none, QT_TRANSLATE_NOOP("context_menu", "Clear format"), qsl("Ctrl+Shift+N"), loadIcon(qsl(":/context_menu/close")), qsl("format/clear")),
        };

        static Styling::ThemeChecker checker;
        if (checker.checkAndUpdateHash())
            formatActions_.clear();

        for (const auto& [type, name, shortcut, icon, command] : actionsData)
        {
            if (formatActions_.count(type))
            {
                formatActions_[type]->setIcon(icon);
                return;
            }
            QAction* action = new QAction(name, this);

            action->setShortcut(QKeySequence(shortcut));
            action->setShortcutVisibleInContextMenu(true);
            action->setIcon(icon);
            action->setData(makeFormatActionData(command));
            formatActions_[type] = action;
        }
    }

    void MainWindow::updateTitlePalette()
    {
        QPalette pal = title_->palette();
        pal.setColor(QPalette::Foreground, Styling::getParameters().getColor(Styling::StyleVariable::TEXT_SOLID));
        title_->setPalette(pal);
    }

    void MainWindow::updateWindowTitle()
    {
        auto title = makeTitle();
        if (stackedWidget_->currentWidget() == appsPage_ && get_gui_settings()->get_value<bool>(settings_show_unreads_in_title, false))
        {
            const auto unreads = Logic::getRecentsModel()->totalUnreads() + Logic::getUnknownsModel()->totalUnreads();
            if (unreads > 0)
                title += u" (" % Utils::getUnreadsBadgeStr(unreads) % u')';
        }

        title_->setText(title);
        setWindowTitle(title);
#ifdef __APPLE__
        getMacSupport()->windowTitle()->setTitleText(title);
#endif
    }

    void MainWindow::setCurrentWidget(QWidget* _widget, ForceLocked _forceLocked)
    {
        stackedWidget_->setCurrentWidget(_widget, _forceLocked);
        if (localPINWidget_ && _widget != localPINWidget_)
        {
            stackedWidget_->removeWidget(localPINWidget_);
            localPINWidget_->deleteLater();
            localPINWidget_ = nullptr;
        }

        updateWindowTitle();
    }

    bool MainWindow::nativeEventFilter(const QByteArray& _data, void* _message, long* _result)
    {
#ifdef _WIN32
        Q_UNUSED(_data);

        static bool checkGeometry = false;
        static QRect restoreGeometry;

        static std::optional<QRect> prevMonitorRect;
        static bool monitorChanged;

        auto msg = static_cast<MSG*>(_message);
        auto hwnd = reinterpret_cast<HWND>(winId());

        if (msg->message == WM_NCCREATE && msg->hwnd == hwnd)
        {
            auto userdata = reinterpret_cast<LPCREATESTRUCTW>(msg->lParam)->lpCreateParams;
            // store window instance pointer in window user data
            SetWindowLongPtrW(msg->hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(userdata));
        }
        else if (msg->message == WM_ERASEBKGND && msg->hwnd == hwnd)
        {
            const auto bgColor = getBgColor();
            if (auto brush = CreateSolidBrush(RGB(bgColor.red(), bgColor.green(), bgColor.blue())))
            {
                SetClassLongPtr(msg->hwnd, GCLP_HBRBACKGROUND, reinterpret_cast<LONG_PTR>(brush));
                DeleteObject(brush);
            }
        }
        else if (msg->message == WM_NCACTIVATE && msg->hwnd == hwnd)
        {
            if (!isAeroEnabled_ && _result)
            {
                // Prevents window frame reappearing on window activation
                // in "basic" theme, where no aero shadow is present.
                *_result = 1;
                return true;
            }
        }
        else if (msg->message == WM_NCHITTEST && msg->hwnd == hwnd && _result)
        {
            const int borderWidth = getWindowBorderWidth();
            const int x = GET_X_LPARAM(msg->lParam);
            const int y = GET_Y_LPARAM(msg->lParam);

            const auto topLeft = QWidget::mapToGlobal(rect().topLeft());
            const auto bottomRight = QWidget::mapToGlobal(rect().bottomRight());

            if (x <= topLeft.x() + borderWidth)
            {
                if (y <= topLeft.y() + borderWidth)
                    *_result = HTTOPLEFT;
                else if (y >= bottomRight.y() - borderWidth)
                    *_result = HTBOTTOMLEFT;
                else
                    *_result = HTLEFT;
            }
            else if (x >= bottomRight.x() - borderWidth)
            {
                if (y <= topLeft.y() + borderWidth)
                    *_result = HTTOPRIGHT;
                else if (y >= bottomRight.y() - borderWidth)
                    *_result = HTBOTTOMRIGHT;
                else
                    *_result = HTRIGHT;
            }
            else
            {
                if (y <= topLeft.y() + borderWidth)
                    *_result = HTTOP;
                else if (y >= bottomRight.y() - borderWidth)
                    *_result = HTBOTTOM;
                else
                    *_result = HTCLIENT;
            }

            if (*_result == HTCLIENT)
            {
                auto curPos = QWidget::mapFromGlobal(QPoint(x, y));

                // workaround for correcting the mapped cursor position:
                // when the main window is maximized its borders are off-screen,
                // so adjust the position by this offset value
                if (IsZoomed(msg->hwnd))
                {
                    RECT wRect;
                    GetWindowRect(msg->hwnd, &wRect);

                    if (auto monitor = MonitorFromRect(&wRect, MONITOR_DEFAULTTONEAREST))
                    {
                        MONITORINFO info = { 0 };
                        info.cbSize = sizeof(info);
                        if (GetMonitorInfo(monitor, &info))
                            curPos += QPoint(wRect.left - info.rcWork.left, wRect.top - info.rcWork.top);
                    }
                }

                // checking the caption area: between the logo and system buttons
                if (titleWidget_->geometry().contains(curPos) && !logo_->geometry().contains(curPos) && !hideButton_->geometry().contains(curPos)
                    && !maximizeButton_->geometry().contains(curPos) && !closeButton_->geometry().contains(curPos))
                    *_result = HTCAPTION;
            }

            return true;
        }
        else if (msg->message == WM_NCCALCSIZE && msg->hwnd == hwnd && _result)
        {
            if (IsZoomed(msg->hwnd))
            {
                auto params = reinterpret_cast<LPNCCALCSIZE_PARAMS>(msg->lParam);
                auto& rect = (msg->wParam == TRUE) ? params->rgrc[0] : *reinterpret_cast<LPRECT>(msg->lParam);

                // adjust client rect to not spill over monitor edges when maximized
                if (auto monitor = MonitorFromRect(&rect, MONITOR_DEFAULTTONEAREST))
                {
                    MONITORINFO info = { 0 };
                    info.cbSize = sizeof(info);
                    if (GetMonitorInfo(monitor, &info))
                        rect = std::move(info.rcWork);
                }
            }

            *_result = 0;
            return true;
        }
        else if (msg->message == WM_GETMINMAXINFO && msg->hwnd == hwnd && _result)
        {
            auto minMaxInfo = reinterpret_cast<MINMAXINFO*>(msg->lParam);
            RECT rect;
            if (GetWindowRect(msg->hwnd, &rect))
            {
                if (auto monitor = MonitorFromRect(&rect, MONITOR_DEFAULTTONULL))
                {
                    MONITORINFO monitorInfo = { 0 };
                    monitorInfo.cbSize = sizeof(monitorInfo);
                    if (GetMonitorInfo(monitor, &monitorInfo))
                    {
                        auto& work_area = monitorInfo.rcWork;
                        auto& monitor_rect = monitorInfo.rcMonitor;

                        minMaxInfo->ptMaxPosition.x = std::abs(work_area.left - monitor_rect.left);
                        minMaxInfo->ptMaxPosition.y = std::abs(work_area.top - monitor_rect.top);
                        minMaxInfo->ptMaxSize.x = std::abs(work_area.right - work_area.left);
                        minMaxInfo->ptMaxSize.y = std::abs(work_area.bottom - work_area.top);
                        minMaxInfo->ptMinTrackSize.x = minimumWidth();
                        minMaxInfo->ptMinTrackSize.y = minimumHeight();
                        minMaxInfo->ptMaxTrackSize = minMaxInfo->ptMaxSize;

                        *_result = 0;
                        return true;
                    }
                }
            }
        }
        else if (msg->message == WM_SIZE && msg->hwnd == hwnd && msg->wParam != SIZE_MINIMIZED)
        {
            isMaximized_ = IsZoomed(msg->hwnd);
            updateState();
        }
        else if (((msg->message == WM_SYSCOMMAND && msg->wParam == SC_RESTORE) || (msg->message == WM_SHOWWINDOW && msg->wParam == TRUE)) && msg->hwnd == hwnd)
        {
            unhideWindow();
        }
        else if (msg->message == WM_SYSCOMMAND && msg->wParam == SC_CLOSE)
        {
            hideWindow();
            return true;
        }
        else if (msg->message == WM_SYSCOMMAND && msg->wParam == SC_MINIMIZE)
        {
            minimize();
            return true;
        }
        else if (msg->message == WM_SYSCOMMAND && msg->wParam == SC_MAXIMIZE)
        {
            maximize();
            return true;
        }
#if 0
        else if (msg->message == WM_WINDOWPOSCHANGING || msg->message == WM_WINDOWPOSCHANGED)
        {
            if (msg->hwnd == (HANDLE)winId())
            {
                WINDOWPOS* pos = (WINDOWPOS*)msg->lParam;
                if (pos->flags == 0x8170 || pos->flags == 0x8130)
                {
                    if (Shadow_)
                        SetWindowPos((HWND)Shadow_->winId(), (HWND)winId(), 0, 0, 0, 0, SWP_NOACTIVATE | SWP_NOOWNERZORDER | SWP_NOMOVE | SWP_NOSIZE | SWP_HIDEWINDOW);
                }
                else if (Shadow_)
                {
                    if (!(pos->flags & SWP_NOSIZE) && !(pos->flags & SWP_NOMOVE) && !(pos->flags & SWP_DRAWFRAME))
                    {
                        int shadowWidth = get_gui_settings()->get_shadow_width();
                        SetWindowPos((HWND)Shadow_->winId(), (HWND)winId(), pos->x - shadowWidth, pos->y - shadowWidth, pos->cx + shadowWidth * 2, pos->cy + shadowWidth * 2, SWP_NOACTIVATE | SWP_NOOWNERZORDER);
                    }
                    else if (!(pos->flags & SWP_NOZORDER))
                    {
                        UINT flags = SWP_NOACTIVATE | SWP_NOOWNERZORDER | SWP_NOMOVE | SWP_NOSIZE;
                        if (pos->flags & SWP_SHOWWINDOW)
                            flags |= SWP_SHOWWINDOW;
                        if (pos->flags & SWP_HIDEWINDOW)
                            flags |= SWP_HIDEWINDOW;

                        SetWindowPos((HWND)Shadow_->winId(), (HWND)winId(), 0, 0, 0, 0, flags);
                    }
                }
            }
            return false;
        }
        else if (msg->message == WM_ACTIVATE)
        {
            if (!Shadow_)
            {
                return false;
            }

            const auto isInactivate = (msg->wParam == WA_INACTIVE);
            const auto isShadowWindow = (msg->hwnd == (HWND)Shadow_->winId());
            if (isShadowWindow && !isInactivate)
            if (msg->wParam != WA_INACTIVE && msg->hwnd == (HANDLE)winId())
            {
                activate();
                return false;
            }
        }
#endif
        else if (msg->message == WM_DISPLAYCHANGE)
        {
            // when the native window is minimized it has negative coordinates
            // and checkPosition() maximizes it, so prevent this
            if (!isMinimized())
                checkPosition();

            saveWindowGeometryAndState(geometry().size());
        }
        else if (msg->message == WM_POWERBROADCAST)
        {
            if (msg->wParam == PBT_APMSUSPEND)
            {
                gotoSleep();
            }
            else if (msg->wParam == PBT_APMRESUMESUSPEND)
            {
                gotoWakeup();
            }
        }
        else if (msg->message == WM_WTSSESSION_CHANGE)
        {
            if (msg->wParam == WTS_SESSION_LOCK)
            {
                gotoSleep();

                if (LocalPIN::instance()->autoLockEnabled())
                    LocalPIN::instance()->lock();
            }
            else if (msg->wParam == WTS_SESSION_UNLOCK)
            {
                gotoWakeup();
            }
        }
        else if (msg->message == WM_NCRBUTTONUP && msg->hwnd == hwnd && _result)
        {
            const auto mousePos = MAKEPOINTS(msg->lParam);

            auto sysMenu = GetSystemMenu(msg->hwnd, FALSE);
            updateSystemMenu(msg->hwnd, sysMenu);
            if (sysMenu)
            {
                SetForegroundWindow(msg->hwnd);
                if (int cmd = TrackPopupMenu(sysMenu, TPM_LEFTALIGN | TPM_TOPALIGN | TPM_LEFTBUTTON | TPM_RETURNCMD, mousePos.x, mousePos.y, 0, msg->hwnd, nullptr))
                {
                    SendMessage(msg->hwnd, WM_SYSCOMMAND, cmd, 0);
                    *_result = 0;
                    return true;
                }
            }
        }
        else if (msg->message == WM_NCLBUTTONDOWN && msg->hwnd == hwnd)
        {
            // remember the current geometry for possible correction when restoring the maximized main window
            if (!IsZoomed(msg->hwnd))
                restoreGeometry = geometry();
        }
        else if (msg->message == WM_ENTERSIZEMOVE)
        {
            prevMonitorRect = getMonitorWorkRect(msg->hwnd);
            monitorChanged = false;
        }
        else if (msg->message == WM_SYSKEYDOWN && (msg->wParam & VK_MENU) && msg->hwnd == hwnd) //https://jira.mail.ru/browse/IMDESKTOP-14805
        {
            SetMenu(hwnd, NULL);
        }
        else if (msg->message == WM_SYSKEYUP && (msg->wParam & VK_MENU) && msg->hwnd == hwnd) //https://jira.mail.ru/browse/IMDESKTOP-14805
        {
            SetMenu(hwnd, menu_);
        }
        else if (msg->message == WM_EXITSIZEMOVE && msg->hwnd == hwnd)
        {
            prevMonitorRect = getMonitorWorkRect(msg->hwnd);
            monitorChanged = false;

            // it is necessary to check the geometry after restoration if the main window is maximized by dragging it to the top of the screen
            if (IsZoomed(msg->hwnd))
            {
                checkGeometry = true;

                auto geometry = restoreGeometry;
                if (const auto screenRect = getMonitorWorkRect(msg->hwnd); screenRect)
                {
                    // change the window position for proper recovery after restarting if most of it is off-screen
                    QPoint topLeft;
                    if (!screenRect->contains(geometry.center()) && getWindowTopLeftOnScreen(geometry.topLeft(), topLeft))
                    {
                        // if the window size is larger than the screen then fix it with the margins (x, 0, x, x)
                        // x - getWindowMarginOnScreen()
                        if (screenRect->width() < geometry.width())
                            geometry.setWidth(screenRect->width() - 2 * getWindowMarginOnScreen());
                        if (screenRect->height() < geometry.height())
                            geometry.setHeight(screenRect->height() - getWindowMarginOnScreen());

                        geometry.moveTo(screenRect->topLeft() + topLeft);

                        // shift window if it is not fully displayed on the screen
                        if (!screenRect->contains(geometry))
                        {
                            int dx = 0;
                            int dy = 0;
                            const auto winBottomRight = geometry.bottomRight();
                            const auto screenBottomRight = screenRect->bottomRight();

                            if (screenBottomRight.x() < winBottomRight.x())
                                dx = screenBottomRight.x() - winBottomRight.x() - getWindowMarginOnScreen();
                            if (screenBottomRight.y() < winBottomRight.y())
                                dy = screenBottomRight.y() - winBottomRight.y() - getWindowMarginOnScreen();

                            geometry.translate(dx, dy);
                        }
                    }
                }
                if (!inCtor_)
                {
                    get_gui_settings()->set_value(settings_main_window_rect, geometry);
                    get_gui_settings()->set_value(settings_desktop_rect, screenAvailableGeometry());
                    get_gui_settings()->set_value(settings_available_geometry, availableVirtualGeometry());
                }
            }
        }
        else if (msg->message == WM_WINDOWPOSCHANGING && msg->hwnd == hwnd)
        {
            auto pos = reinterpret_cast<LPWINDOWPOS>(msg->lParam);
            if (checkGeometry && (!IsIconic(msg->hwnd) && !IsZoomed(msg->hwnd)))
            {
                if (restoreGeometry != QRect(pos->x, pos->y, pos->cx, pos->cy))
                {
                    pos->x = restoreGeometry.x();
                    pos->y = restoreGeometry.y();
                    pos->cx = restoreGeometry.width();
                    pos->cy = restoreGeometry.height();
                }

                checkGeometry = false;
            }
            else if (IsZoomed(hwnd))
            {
                if (!pos->cx && !pos->cy)
                    return false;

                auto screenRect = getMonitorWorkRect(msg->hwnd);
                if (!screenRect)
                {
                    auto newPoint = QPoint((pos->x + pos->cx) / 2, (pos->y + pos->cy) / 2);
                    if (isTheSameMonitor(QPoint(pos->x, pos->y), newPoint))
                        screenRect = getMonitorWorkRect(newPoint);
                }
                if (screenRect)
                {
                    if (!isAeroEnabled_ || pos->x > screenRect->left() || pos->y > screenRect->top()
                        || pos->cx < screenRect->width() || pos->cy < screenRect->height())
                    {
                        pos->x = screenRect->left();
                        pos->y = screenRect->top();
                        pos->cx = screenRect->width();
                        pos->cy = screenRect->height();
                    }
                }
            }
            else if (!(pos->flags & SWP_NOSIZE) && (pos->cx != width() || pos->cy != height()))
            {
                auto screenRect = getMonitorWorkRect(msg->hwnd);
                if (screenRect != prevMonitorRect)
                    monitorChanged = true;

                if (monitorChanged)
                    pos->flags |= SWP_FRAMECHANGED;
            }
        }
        else if (msg->message == WM_WINDOWPOSCHANGED && msg->hwnd == hwnd)
        {
            if (IsZoomed(hwnd))
            {
                if (const auto screenRect = getMonitorWorkRect(msg->hwnd); screenRect)
                {
                    const auto pos = reinterpret_cast<LPWINDOWPOS>(msg->lParam);
                    if (pos->x > screenRect->left() || pos->y > screenRect->top()
                        || pos->cx < screenRect->width() || pos->cy < screenRect->height())
                        SetWindowPos(msg->hwnd, nullptr, screenRect->left(), screenRect->top(), screenRect->width(), screenRect->height(), SWP_NOOWNERZORDER | SWP_FRAMECHANGED);
                }
            }
        }
        else if (msg->message == WM_DPICHANGED && msg->hwnd == hwnd)
        {
            *_result = 0;
            return true;
        }
        else if (msg->message == WM_CLOSE && msg->hwnd == hwnd)
        {
            hideWindow();
            return true;
        }
#else

#ifdef __APPLE__
        return MacSupport::nativeEventFilter(_data, _message, _result);
#endif

#endif //_WIN32
        return false;
    }

    bool MainWindow::eventFilter(QObject* _obj, QEvent* _event)
    {
        switch (_event->type())
        {
            case QEvent::KeyPress:
            {
                const auto keyEvent = static_cast<QKeyEvent*>(_event);
#ifndef STRIP_VOIP
                if (MainPage::hasInstance() && MainPage::instance()->getVideoWindow() && MainPage::instance()->isVideoWindowOn())
                {
                    const auto videoWindow = MainPage::instance()->getVideoWindow();
                    if ((keyEvent->modifiers() & Qt::ControlModifier) && (keyEvent->modifiers() & Qt::ShiftModifier))
                    {
                        switch (keyEvent->key())
                        {
                            case Qt::Key_S:
                            {
                                videoWindow->switchScreenSharingEnableByShortcut();
                                return true;
                            };
                            case Qt::Key_V:
                            {
                                videoWindow->switchVideoEnableByShortcut();
                                return true;
                            };
                            case Qt::Key_A:
                            {
                                videoWindow->switchMicrophoneEnableByShortcut();
                                return true;
                            };
                            case Qt::Key_D:
                            {
                                videoWindow->switchSoundEnableByShortcut();
                                return true;
                            };
                        }
                    }
                }
#endif
                userActivity();
                Q_EMIT anyKeyPressed(QPrivateSignal());

                auto& interConnector = Utils::InterConnector::instance();

                if (isAppsPage())
                {
                    const auto mainPage = MainPage::instance();
                    const bool infoOnlyVisible = interConnector.isSidebarVisible()
                                                 && mainPage && (!mainPage->isSideBarInSplitter() || mainPage->isOneFrameSidebar())
                                                 && !mainPage->isSidebarWithThreadPage();
                    const auto isSearchWidgetActive = (qobject_cast<Ui::SearchEdit*>(QApplication::focusWidget()) != nullptr);
                    const auto isGalleryActive = gallery_ && gallery_->isActiveWindow();
                    const auto contact = getCurrentAimId();
                    auto page = interConnector.getHistoryPage(contact);
                    if (mainPage && (!mainPage->isSemiWindowVisible() || mainPage->isSidebarWithThreadPage()))
                    {
                        if (!isSearchWidgetActive && !isGalleryActive)
                        {
                            if (interConnector.isMultiselect(contact) && !infoOnlyVisible)
                            {
                                bool handled = true;
                                if (keyEvent->key() == Qt::Key_Right)
                                    interConnector.multiselectNextElementRight();
                                else if (keyEvent->key() == Qt::Key_Left)
                                    interConnector.multiselectNextElementLeft();
                                else if (keyEvent->key() == Qt::Key_Up)
                                    interConnector.multiSelectCurrentMessageUp(keyEvent->modifiers() & Qt::ShiftModifier);
                                else if (keyEvent->key() == Qt::Key_Down)
                                    interConnector.multiSelectCurrentMessageDown(keyEvent->modifiers() & Qt::ShiftModifier);
                                else if (keyEvent->key() == Qt::Key_Tab)
                                    interConnector.multiselectNextElementTab();
                                else if (keyEvent->key() == Qt::Key_Escape)
                                    interConnector.setMultiselect(false, contact);
                                else if (keyEvent->key() == Qt::Key_Space)
                                    interConnector.multiselectSpace();
                                else if (keyEvent->key() == Qt::Key_Enter || keyEvent->key() == Qt::Key_Return)
                                    interConnector.multiselectEnter();
                                else
                                    handled = false;

                                if (handled)
                                    return true;
                            }
                            else if (keyEvent->key() == Qt::Key_Up && (keyEvent->modifiers() & Qt::ShiftModifier && keyEvent->modifiers() & Qt::AltModifier))
                            {
                                interConnector.setMultiselect(true, contact, true);
                                if (page)
                                    page->setFocusOnArea();

                                return true;
                            }
                        }
                    }

                    page = interConnector.getHistoryPage(Logic::getContactListModel()->selectedContact());

                    if ((keyEvent->key() == Qt::Key_Tab || keyEvent->key() == Qt::Key_Backtab) && !(keyEvent->modifiers() & Qt::ControlModifier))
                    {
                        if (page && page->isPttRecordingHoldByMouse())
                            return true;

                        const auto cur = static_cast<QWidget*>(qApp->focusObject());
                        if (cur == this)
                        {
                            const bool tabForward = keyEvent->key() == Qt::Key_Tab;
                            const auto setToInput = tabForward && !Logic::getContactListModel()->selectedContact().isEmpty() && page && page->canSetFocusOnInput();

                            if (setToInput)
                                page->setFocusOnInputFirstFocusable();
                            else
                                MainPage::instance()->setSearchTabFocus();

                            return true;
                        }
                    }
                    else if (keyEvent->key() == Qt::Key_Escape)
                    {
                        if (page && page->isRecordingPtt())
                        {
                            page->closePttRecording();
                            return true;
                        }
                        else if (lastPttInput_ && lastPttInput_->isRecordingPtt() && lastPttInput_->isActive())
                        {
                            lastPttInput_->closePttPanel();
                            lastPttInput_->stopPttRecording();
                        }
                    }
                    else if (keyEvent->key() == Qt::Key_Space)
                    {
                        if (page && (page->tryPlayPttRecord() || page->tryPausePttRecord()))
                            return true;
                        else if (lastPttInput_ && lastPttInput_->isActive())
                            return lastPttInput_->tryPlayPttRecord() || lastPttInput_->tryPausePttRecord();
                    }
                }
                if (Statuses::isStatusEnabled() && isMessengerPage())
                {
                    if (keyEvent->key() == Qt::Key_S && (keyEvent->modifiers() & Qt::ControlModifier) && !(keyEvent->modifiers() & Qt::ShiftModifier))
                    {
                        Q_EMIT interConnector.openStatusPicker();
                        return true;
                    }
                }
                break;
            }
            case QEvent::MouseButtonPress:
                Q_EMIT mousePressed(QPrivateSignal());
                [[fallthrough]];
            case QEvent::TouchBegin:
            case QEvent::Wheel:
            case QEvent::MouseMove:
            case QEvent::ApplicationActivate:
            {
                if (_event->type() == QEvent::MouseMove)
                {
                    const auto e = static_cast<QMouseEvent*>(_event);
                    Q_EMIT mouseMoved(e->pos(), QPrivateSignal());
                }
                userActivity();
                break;
            }
            case QEvent::MouseButtonRelease:
            {
                Q_EMIT mouseReleased(QPrivateSignal());
                break;
            }
            case QEvent::FocusOut:
            {
                if (auto w = qobject_cast<QWidget*>(_obj))
                    previousFocusedWidget_ = w;
                break;
            }
            default:
                break;
        }

        return QMainWindow::eventFilter(_obj, _event);
    }

    void MainWindow::enterEvent(QEvent* _event)
    {
        QMainWindow::enterEvent(_event);

        if (platform::is_apple() &&  qApp->activeWindow() != this && (!MainPage::hasInstance() || !MainPage::instance()->isVideoWindowActive()))
            qApp->setActiveWindow(this);
        updateMainMenu();
        Q_EMIT Utils::InterConnector::instance().historyControlPageFocusIn(Logic::getContactListModel()->selectedContact());
    }

    void MainWindow::resizeEvent(QResizeEvent* _event)
    {
        Q_EMIT Utils::InterConnector::instance().mainWindowResized();

        Q_EMIT Utils::InterConnector::instance().closeAnyPopupMenu();
        Q_EMIT Utils::InterConnector::instance().closeAnyPopupWindow({ Utils::CloseWindowInfo::Initiator::MainWindow, Utils::CloseWindowInfo::Reason::MW_Resizing });

        if (isVisible())
            saveWindowGeometryAndState(_event->size());

#ifdef _WIN32
        // workaround for correcting maximized window size:
        // for some reason, after fixing the window size in the WM_NCCALCSIZE and WM_GETMINMAXINFO message handlers,
        // it does not change for QMainWindow, so fix it by changing the window margins
        if (isMaximized())
        {
            const auto winSize = geometry().size();
            const auto screenSize = QApplication::desktop()->availableGeometry(getScreen()).size();
            if (winSize.width() > screenSize.width() && winSize.height() > screenSize.height())
                QMainWindow::setContentsMargins(QMargins(0, 0, winSize.width() - screenSize.width(), winSize.height() - screenSize.height()));
        }
        else
        {
            QMainWindow::setContentsMargins(QMargins());
        }
#endif // _WIN32

        updateMainMenu();
    }

    void MainWindow::moveEvent(QMoveEvent* _event)
    {
        if (inCtor_)
            return;
        if (!isMaximized())
        {
            auto rc = get_gui_settings()->get_value(settings_main_window_rect, QRect());
            rc.moveTo(!platform::is_windows() ? pos() : geometry().topLeft());

#ifdef _WIN32
            // when the main window is snapped to the left/right half of the screen
            // save his normal position for the next restart
            WINDOWPLACEMENT placement;
            placement.length = sizeof(WINDOWPLACEMENT);
            if (GetWindowPlacement(reinterpret_cast<HWND>(winId()), &placement))
            {
                const auto& r = placement.rcNormalPosition;
                auto normalRect = QRect(r.left, r.top, r.right - r.left, r.bottom - r.top);

                // fix the normlRect position when the taskbar is located on the left or top of the screen
                const auto screenArea = QDesktopWidget().screenGeometry(this);
                const auto workArea = QDesktopWidget().availableGeometry(this);
                if (workArea.topLeft() != screenArea.topLeft())
                {
                    const auto diff = workArea.topLeft() - screenArea.topLeft();
                    normalRect.translate(diff);
                }

                if (rc != normalRect)
                    rc = std::move(normalRect);
            }
#endif
            if (isVisible())
                get_gui_settings()->set_value(settings_main_window_rect, rc);
            get_gui_settings()->set_value(settings_desktop_rect, screenAvailableGeometry());
        }
    }

    void MainWindow::changeEvent(QEvent* _event)
    {
        bool active = isActive();

        if (_event->type() == QEvent::ActivationChange)
        {
            if (Shadow_)
                Shadow_->setActive(active);

            notifyWindowActive(active && !isMinimized());

#ifdef __APPLE__
            getMacSupport()->windowTitle()->updateTitleBgColor();
            getMacSupport()->windowTitle()->updateTitleTextColor();
#endif
            Q_EMIT Utils::InterConnector::instance().activationChanged(active);
        }
        else if (_event->type() == QEvent::ApplicationStateChange)
        {
            if (Shadow_)
                Shadow_->setActive(active);
        }
        else if (_event->type() == QEvent::WindowStateChange)
        {
            QWindowStateChangeEvent* event = static_cast<QWindowStateChangeEvent*>(_event);

            if (event->oldState() & Qt::WindowMinimized)
            {
                showHideGallery();
                notifyWindowActive(active);
            }

            saveWindowGeometryAndState(geometry().size());
        }

        QMainWindow::changeEvent(_event);
    }

    void MainWindow::closeEvent(QCloseEvent* _event)
    {
        if constexpr (!platform::is_windows())
        {
            if (_event->spontaneous())
            {
                _event->ignore();

                hideWindow();
            }
        }

        Q_EMIT windowClose(QPrivateSignal());
    }

    void MainWindow::hideEvent(QHideEvent* _event)
    {
        Q_EMIT windowHide(QPrivateSignal());
        QMainWindow::hideEvent(_event);
    }

    void MainWindow::keyPressEvent(QKeyEvent* _event)
    {
        if (isMessengerPage())
        {
            const auto aimId = getCurrentAimId(true);
            if (MainPage::hasInstance())
            {
                if (_event->modifiers() & Qt::ControlModifier && _event->key() == Qt::Key_F)
                {
                    if (isSearchInDialog(_event))
                    {
                        if (!aimId.isEmpty())
                        {
                            if (Logic::getContactListModel()->isThread(aimId))
                                Q_EMIT Utils::InterConnector::instance().startSearchInThread(aimId);
                            else if (aimId != ServiceContacts::getTasksName())
                                Q_EMIT Utils::InterConnector::instance().startSearchInDialog(aimId);
                        }
                    }
                    else
                    {
                        Q_EMIT Utils::InterConnector::instance().setSearchFocus();
                    }
                }
            }

            if (MainPage::hasInstance() && _event->modifiers() == Qt::ControlModifier && _event->key() == Qt::Key_W)
            {
                activateShortcutsClose();
            }
            else if (MainPage::hasInstance() && MainPage::instance()->isInRecentsTab() && _event->modifiers() == Qt::ControlModifier && _event->key() == Qt::Key_R)
            {
                Logic::getRecentsModel()->markAllRead();
            }
            else if (_event->key() == Qt::Key_Escape)
            {
                if (MainPage::hasInstance() && MainPage::instance()->isInSettingsTab())
                {
                    if (MainPage::instance()->isOneFrameTab())
                    {
                        showMessengerPage();
                        MainPage::instance()->selectRecentsTab();
                    }
                    else
                    {
                        Q_EMIT Utils::InterConnector::instance().myProfileBack();
                    }
                }
                else
                {
                    onEscPressed(aimId, _event->isAutoRepeat());
                }
            }
            else if (_event->key() == Qt::Key_Enter || _event->key() == Qt::Key_Return)
            {
                auto page = Logic::getContactListModel()->isThread(aimId) ? Utils::InterConnector::instance().getHistoryPage(aimId)
                                                                          : qobject_cast<HistoryControlPage*>(Utils::InterConnector::instance().getPage(aimId));
                if (page && page->isVisible() && page->isRecordingPtt() && page->isInputActive())
                    page->sendPttRecord();
                else if (lastPttInput_ && lastPttInput_->isRecordingPtt() && lastPttInput_->isActive())
                    lastPttInput_->sendPtt();
            }
            else if (_event->key() == Qt::Key_Space)
            {
                Q_EMIT Utils::InterConnector::instance().stopPttRecord();
            }
            else if (_event->modifiers() == Qt::ControlModifier && _event->key() == Qt::Key_T)
            {
                Q_EMIT Utils::InterConnector::instance().startPttRecord();
            }

            if (MainPage::hasInstance() && !MainPage::instance()->isSearchActive() && qApp->focusObject() == this)
            {
                if (_event->key() == Qt::Key_Down)
                    Q_EMIT downKeyPressed(QPrivateSignal());
                else if (_event->key() == Qt::Key_Up)
                    Q_EMIT upKeyPressed(QPrivateSignal());
                else if (_event->key() == Qt::Key_Enter || _event->key() == Qt::Key_Return)
                    Q_EMIT enterKeyPressed(QPrivateSignal());
            }

            if (MainPage::hasInstance() && !MainPage::instance()->isSemiWindowVisible())
            {
                auto nextChatSequence = false;
                auto prevChatSequence = false;
                const auto modifiers = _event->modifiers();
                const auto key = _event->key();

                if constexpr (platform::is_apple())
                {
                    nextChatSequence = (modifiers == (Qt::ControlModifier | Qt::ShiftModifier)) && (key == Qt::Key_BracketRight || key == Qt::Key_BraceRight);
                    prevChatSequence = !nextChatSequence && (modifiers == (Qt::ControlModifier | Qt::ShiftModifier)) && (key == Qt::Key_BracketLeft || key == Qt::Key_BraceLeft);
                }
                else
                {
                    nextChatSequence = _event->matches(QKeySequence::NextChild) || (modifiers == Qt::ControlModifier && key == Qt::Key_PageDown);
                    prevChatSequence = !nextChatSequence && (_event->matches(QKeySequence::PreviousChild) || (modifiers == Qt::ControlModifier && key == Qt::Key_PageUp));
                }

                if (nextChatSequence)
                    activateNextChat();
                else if (prevChatSequence)
                    activatePrevChat();
            }

            if constexpr (platform::is_windows())
            {
                if (_event->modifiers() == Qt::ControlModifier && _event->key() == Qt::Key_F4)
                    Logic::getRecentsModel()->hideChat(aimId);
            }
        }

        if (isAppsPage())
        {
            if (_event->modifiers() == Qt::ControlModifier && _event->key() == Qt::Key_L && LocalPIN::instance()->enabled())
                LocalPIN::instance()->lock();
        }

        if (isWebAppTasksPage())
        {
            const auto aimId = getCurrentAimId();
            if (_event->key() == Qt::Key_Escape)
                Q_EMIT Utils::InterConnector::instance().closeSidebar();
            else if (_event->modifiers() & Qt::ControlModifier && _event->key() == Qt::Key_F)
            {
                if (isSearchInDialog(_event) && !aimId.isEmpty())
                    Q_EMIT Utils::InterConnector::instance().startSearchInThread(aimId);
            }
        }

        if constexpr (platform::is_linux() || platform::is_windows())
        {
            if (_event->modifiers() == Qt::ControlModifier && _event->key() == Qt::Key_Q)
                exit();
        }

        QMainWindow::keyPressEvent(_event);
    }

    void MainWindow::mouseReleaseEvent(QMouseEvent* _event)
    {
        //Q_EMIT mouseReleased(QPrivateSignal());
        QMainWindow::mouseReleaseEvent(_event);
    }

    void MainWindow::showEvent(QShowEvent* _event)
    {
        if (maximizeLater_)
        {
            maximizeLater_ = false;
            showMaximized();
        }
        QMainWindow::showEvent(_event);
    }

    void MainWindow::placeRectOnDesktopEntirely(QRect& _rect, QMargins _margins)
    {
        auto desktopRect = availableVirtualGeometry();
        if (!_margins.isNull())
            desktopRect.adjust(_margins.left(), _margins.top(), -_margins.right(), -_margins.bottom());

        if (desktopRect.contains(_rect))
            return;

        if (auto intersectedRect = desktopRect.intersected(_rect); !intersectedRect.isEmpty())
        {
            if (auto fitWidth = _rect.width() - desktopRect.width(); fitWidth > 0)
                _rect.setWidth(_rect.width() - fitWidth);
            if (auto fitHeight = _rect.height() - desktopRect.height(); fitHeight > 0)
                _rect.setHeight(_rect.height() - fitHeight);

            auto getAxisDelta = [](int _point1, int _lenght1, int _point2, int _lenght2) -> int
            {
                auto delta = 0;

                if (_point1 < _point2)
                    delta = _point2 - _point1;
                else if ((_point1 + _lenght1) > (_point2 + _lenght2))
                    delta = _lenght2 - _lenght1;

                return delta;
            };
            const auto dx = getAxisDelta(_rect.x(), _rect.width(), intersectedRect.x(), intersectedRect.width());
            const auto dy = getAxisDelta(_rect.y(), _rect.height(), intersectedRect.y(), intersectedRect.height());
            _rect.translate(dx, dy);
        }
        else
        {
            _rect = getDefaultWindowGeometry();
        }
    }

    void MainWindow::initSizes(bool _needShow)
    {
        setMinimumHeight(getMinWindowHeight());
        setMinimumWidth(getMinWindowWidth());

        const auto defaultWindowRect = getDefaultWindowGeometry();
        auto mainRect = Ui::get_gui_settings()->get_value(settings_main_window_rect, QRect());
        if (mainRect.isNull())
        {
            mainRect = defaultWindowRect;
            get_gui_settings()->set_value(settings_main_window_rect, mainRect);
        }

        if constexpr (!platform::is_windows())
        {
            bool intersects = false;
            const auto screens = QGuiApplication::screens();
            for (const auto screen : screens)
            {
                auto screenGeometry = screen->availableGeometry();
                auto intersectedRect = screenGeometry.intersected(mainRect);
                if (intersectedRect.width() > Utils::scale_value(50) && intersectedRect.height() > Utils::scale_value(50))
                {
                    intersects = true;
                    break;
                }
            }

            if (!intersects)
                mainRect = defaultWindowRect;

            move(mainRect.left(), mainRect.top());
            resize(mainRect.width(), mainRect.height());
        }
        else
        {
            const auto desktopGeometry = availableVirtualGeometry();

            if (Ui::get_gui_settings()->get_value(settings_available_geometry, desktopGeometry) != desktopGeometry)
            {
                get_gui_settings()->set_value(settings_available_geometry, desktopGeometry);

                auto prevDesktopGeometry = Ui::get_gui_settings()->get_value(settings_desktop_rect, screenAvailableGeometry());
                auto bestScreen = 0;
                for (auto i = 0; i < QApplication::desktop()->screenCount(); ++i)
                {
                    auto screen = QApplication::desktop()->screenGeometry(i);
                    if (screen.width() >= mainRect.width() && screen.height() >= mainRect.height())
                    {
                        bestScreen = i;
                        break;
                    }
                }

                auto screen = QApplication::desktop()->availableGeometry(bestScreen);
                Ui::get_gui_settings()->set_value(settings_desktop_rect, screen);

                if (screen.width() < mainRect.width())
                    mainRect.setWidth(mainRect.width() * (double)screen.width() / prevDesktopGeometry.width());
                if (screen.height() < mainRect.height())
                    mainRect.setHeight(mainRect.width() * (double)screen.height() / prevDesktopGeometry.height());

                mainRect.moveLeft(screen.x() + (screen.width() - mainRect.width()) / 2);
                mainRect.moveTop(screen.y() + (screen.height() - mainRect.height()) / 2);
            }

            const auto margin = getWindowMarginOnScreen();
            placeRectOnDesktopEntirely(mainRect, QMargins(margin, 0, margin, margin));

            if (!_needShow && !get_gui_settings()->get_value<bool>(settings_show_in_taskbar, true))
                // when starts without showing the main window and without the application icon in the taskbar,
                // the main window receives a resize event from QGuiApplicationPrivate::processGeometryChangeEvent(...)
                // with initial sizes and overrides the current geometry setting, so set the geometry on the next cycle
                QTimer::singleShot(0, this, [this, mainRect]() { setGeometry(mainRect); });
            else
                setGeometry(mainRect);
        }
    }

    void MainWindow::initSettings(bool _needShow)
    {
        initSizes(_needShow);

        const bool isMaximized = get_gui_settings()->get_value<bool>(settings_window_maximized, false);
        const bool showTaskBarIcon = get_gui_settings()->get_value<bool>(settings_show_in_taskbar, true);

        if (!_needShow)
        {
            if (platform::is_windows() && !showTaskBarIcon)
                hide();
            else
                QMainWindow::showMinimized();
        }
        else
        {
            isMaximized ? showMaximized() : show();
        }

        if constexpr (platform::is_windows())
        {
            updateMaximizeButton(isMaximized);
            maximizeButton_->setStyle(QApplication::style());
            showIconInTaskbar(showTaskBarIcon);
        }

#ifdef _WIN32
        if (isMaximized && Shadow_)
            SetWindowPos((HWND)Shadow_->winId(), (HWND)winId(), 0, 0, 0, 0, SWP_NOACTIVATE | SWP_NOOWNERZORDER | SWP_NOMOVE | SWP_NOSIZE | SWP_HIDEWINDOW);

        WTSRegisterSessionNotification((HWND)winId(), NOTIFY_FOR_THIS_SESSION);
#endif //_WIN32
    }

    void MainWindow::initStats()
    {
        statistics::getPeriodicGuiMetrics().onPoll();
    }

    QWidget* MainWindow::getWidget() const
    {
        return mainWidget_;
    }

    void MainWindow::resize(int _w, int _h)
    {
        const auto geometry = screenAvailableGeometry();

        if (geometry.width() < _w)
            _w = geometry.width();

        if (geometry.height() < _h)
            _h = geometry.height();

#ifdef __APPLE__
        // workaround for setting the correct maximized window size (not full screen) after initialization
        // if try to set the correct size the window will expand beyond the boundaries of the screen,
        // but in this case the final window size will be correct
        const auto windowTitleHeight = MacSupport::getWidgetHeaderHeight(*this);
        const auto availableClientAreaHeight = geometry.height() - windowTitleHeight;
        if (_h >= availableClientAreaHeight)
            _h = availableClientAreaHeight - 1;
#endif

        return QMainWindow::resize(_w, _h);
    }

    void MainWindow::showMaximized()
    {
        if constexpr (platform::is_linux())
        {
            QMainWindow::showMaximized();
        }
        else
        {
            isMaximized_ = true;

            if constexpr (platform::is_windows())
            {
                setWindowState(Qt::WindowMaximized);
                QMainWindow::showMaximized();
            }
            else
            {
                QMainWindow::showNormal();

                const auto screen = getScreen();
                const auto screenGeometry = QApplication::desktop()->availableGeometry(screen);
                setGeometry(screenGeometry);
            }
        }

        updateState();
    }

    void MainWindow::showNormal()
    {
        isMaximized_ = false;

        if constexpr (platform::is_windows())
        {
            setWindowState(Qt::WindowNoState);
        }
        else
        {
            const auto mainRect = Ui::get_gui_settings()->get_value(settings_main_window_rect, getDefaultWindowSize());
            if constexpr (platform::is_linux())
            {
                move(mainRect.left(), mainRect.top());
                resize(mainRect.width(), mainRect.height());
            }
            else
            {
                setGeometry(mainRect);
            }
        }
        QMainWindow::showNormal();

        updateState();
    }

    void MainWindow::showMinimized()
    {
#ifdef _WIN32
        ShowWindow((HWND)winId(), SW_MINIMIZE);
        return;
#endif //_WIN32
        QMainWindow::showMinimized();
    }

    void MainWindow::updateState()
    {
        if constexpr (platform::is_windows())
            updateMaximizeButton(isMaximized());

        get_gui_settings()->set_value<bool>(settings_window_maximized, isMaximized());
    }

    void MainWindow::checkPosition()
    {
        QRect desktopRect;
        const auto screens = QGuiApplication::screens();
        for (const auto screen : screens)
            desktopRect |= screen->availableGeometry();

        QRect main = rect();
        QPoint mainP = mapToGlobal(main.topLeft());
        main.moveTo(mainP);

        if constexpr (!platform::is_windows())
        {
            if (main.y() < desktopRect.y())
                maximize();
        }
    }

    void MainWindow::userActivity()
    {
        if (!isMinimized())
            UserActivity::raiseUserActivity();
    }

    void MainWindow::updateMaximizeButton(bool _isMaximized)
    {
        if (_isMaximized)
            maximizeButton_->setDefaultImage(qsl(":/titlebar/restore_button"));
        else
            maximizeButton_->setDefaultImage(qsl(":/titlebar/expand_button"));

        Styling::Buttons::setButtonDefaultColors(maximizeButton_);
    }

    void MainWindow::gotoSleep()
    {
        if (sleeping_)
            return;

        sleeping_ = true;

        //needChangeState(true);
    }

    void MainWindow::gotoWakeup()
    {
        sleeping_ = false;
    }

    void MainWindow::activateShortcutsClose()
    {
        const auto closeCode = get_gui_settings()->get_value<int>(settings_shortcuts_close_action, static_cast<int>(ShortcutsCloseAction::Default));
        const auto closeAct = static_cast<ShortcutsCloseAction>(closeCode);

        if constexpr (platform::is_apple())
        {
            if (gallery_ && gallery_->isActiveWindow())
            {
                gallery_->escape();
                return;
            }

            if (MainPage::hasInstance() && MainPage::instance()->isVideoWindowActive())
                return;
        }

        switch (closeAct)
        {
        case ShortcutsCloseAction::RollUpWindowAndChat:
            Q_EMIT Utils::InterConnector::instance().closeAnyPopupMenu();
            Q_EMIT Utils::InterConnector::instance().closeAnyPopupWindow(Utils::CloseWindowInfo());
            if (MainPage::hasInstance())
            {
                Utils::InterConnector::instance().closeDialog();
                MainPage::instance()->update();
                setFocus();
            }
            [[fallthrough]];
        case ShortcutsCloseAction::RollUpWindow:
            hideWindow();
            break;
        case ShortcutsCloseAction::CloseChat:
            Q_EMIT Utils::InterConnector::instance().closeAnyPopupMenu();
            Q_EMIT Utils::InterConnector::instance().closeAnyPopupWindow(Utils::CloseWindowInfo());
            closeCurrent();
            break;
        default:
            break;
        }
    }

    void MainWindow::onThemeChanged()
    {
#ifdef __APPLE__
        getMacSupport()->windowTitle()->updateTitleBgColor();
        getMacSupport()->windowTitle()->updateTitleTextColor();
#endif
        updateFormatActions();
        updateTitlePalette();
        Utils::setDefaultBackground(this);
        Utils::updateBgColor(titleWidget_, Styling::getParameters().getAppTitlebarColor());
        update();
    }

    void MainWindow::needChangeState()
    {
        const bool mainActive = windowActive_ && !sleeping_ && UserActivity::getLastActivityMs() < getOfflineTimeout() && !LocalPIN::instance()->locked();

        const bool voipActive = []()
        {
#ifndef STRIP_VOIP
            return !Ui::GetDispatcher()->getVoipController().currentCallContacts().empty() && Ui::GetDispatcher()->getVoipController().isConnectionEstablished();
#else
            return false;
#endif
        }();

        const bool newState = mainActive || voipActive;
        const bool prevState = std::exchange(uiActive_, newState);
        uiActiveMainWindow_ = mainActive;

        static bool isFirst = true;

        const bool stateChanged = (isFirst || uiActive_ != prevState);

        if (uiActive_)
            GetDispatcher()->postUiActivity();

        if (stateChanged)
        {
            if (uiActive_)
            {
                if (updateWhenUserInactiveTimer_)
                    updateWhenUserInactiveTimer_->stop();

                trayIcon_->hideAlerts();

                if (!SkipRead_ && isMessengerPageContactDialog())
                {
                    Logic::getRecentsModel()->sendLastRead();
                    Logic::getUnknownsModel()->sendLastRead();
                }

                SkipRead_ = false;
            }
            else
            {
                if (updateWhenUserInactiveTimer_)
                    updateWhenUserInactiveTimer_->start();
            }

            if (MainPage::hasInstance())
                MainPage::instance()->notifyUIActive(uiActive_);
        }

        isFirst = false;
    }

    void MainWindow::userActivityTimer()
    {
        needChangeState();
    }

    void MainWindow::onShowAddContactDialog(const Utils::AddContactDialogs::Initiator& _initiator)
    {
        Utils::showAddContactsDialog(_initiator);
    }

    void MainWindow::maximize()
    {
        if (isMaximized())
        {
            showNormal();
#if 0
            auto mainRect = Ui::get_gui_settings()->get_value(settings_main_window_rect, getDefaultWindowSize());

            resize(mainRect.width(), mainRect.height());
            move(mainRect.x(), mainRect.y() < 0 ? 0 : mainRect.y());
#endif
#ifdef _WIN32
            if (Shadow_)
                SetWindowPos((HWND)Shadow_->winId(), (HWND)winId(), 0, 0, 0, 0, SWP_NOACTIVATE | SWP_NOOWNERZORDER | SWP_NOMOVE | SWP_NOSIZE | SWP_SHOWWINDOW);
#endif //_WIN32
        }
        else
        {
#ifdef _WIN32
            if (Shadow_)
                SetWindowPos((HWND)Shadow_->winId(), (HWND)winId(), 0, 0, 0, 0, SWP_NOACTIVATE | SWP_NOOWNERZORDER | SWP_NOMOVE | SWP_NOSIZE | SWP_HIDEWINDOW);
#endif //_WIN32
            showMaximized();
        }
        GetDispatcher()->post_stats_to_core(core::stats::stats_event_names::main_window_fullscreen);
        updateMainMenu();
    }

    void MainWindow::minimize()
    {
        if constexpr (platform::is_apple())
        {
            if (MainPage::hasInstance() && MainPage::instance()->isVideoWindowActive() && !MainPage::instance()->isVideoWindowMinimized())
            {
                MainPage::instance()->minimizeVideoWindow();
                return;
            }

            if (gallery_)
            {
                if (isFullScreen())
                    gallery_->closeGallery();
                else
                    gallery_->showMinimized();
            }
        }

        if (get_gui_settings()->get_value<bool>(settings_show_in_taskbar, true))
        {
#ifdef _WIN32
            if (maximizeLater_)
            {
                maximizeLater_ = false;
                showMinimized();
                maximizeLater_ = true;
            }
            else
            {
                showMinimized();
            }

            if (Shadow_)
                SetWindowPos((HWND)Shadow_->winId(), (HWND)winId(), 0, 0, 0, 0, SWP_NOACTIVATE | SWP_NOOWNERZORDER | SWP_NOMOVE | SWP_NOSIZE | SWP_HIDEWINDOW);
#elif __APPLE__
            if (isFullScreen())
                toggleFullScreen();
            else
                MacSupport::minimizeWindow(winId());
#else
            showMinimized();
#endif //_WIN32
        }
        else
        {
            hideWindow();
        }

        notifyWindowActive(false);

        if (MainPage::hasInstance() && MainPage::instance()->isVideoWindowOn() && !MainPage::instance()->isVideoWindowMinimized())
            MainPage::instance()->showVideoWindow();

        updateMainMenu();
    }

    void MainWindow::moveRequest(QPoint _point)
    {
        if (isMaximized())
            maximize();
        else
            move(_point);
    }

    void MainWindow::guiSettingsChanged(const QString& _key)
    {
        if (_key == ql1s(settings_language) || _key == ql1s(settings_scale_coefficient))
        {
            //showLoginPage();
            //showMainPage();
        }
        else if (_key == ql1s(settings_notify_new_mail_messages))
        {
            trayIcon_->updateEmailIcon();
        }
        else if (_key == ql1s(settings_show_unreads_in_title))
        {
            updateWindowTitle();
        }
    }

    void MainWindow::clear_global_objects()
    {
        // delete messenger & super app page page
        if (appsPage_)
        {
            appsPage_->resetPages();
            appsPage_ = nullptr;
        }

        Logic::ResetMentionModel();
        Logic::ResetContactListModel();
        Logic::ResetRecentsModel();
        Logic::ResetUnknownsModel();
        Logic::ResetCallsModel();
        Logic::resetLastSearchPatterns();

        Logic::InputStateContainer::instance().clearAll();
        Logic::ResetPrivacySettingsContainer();
        Logic::ThreadSubContainer::instance().clear();

        Ui::ResetMyInfo();
        Statuses::LocalStatuses::instance()->resetToDefaults();

        Utils::InterConnector::instance().clearInternalCaches();
        Logic::ResetTaskContainer();
        Logic::ResetFileSharingMetaContainer();
        Logic::ResetAppsContainer();

        Ui::Stickers::resetCache();

        trayIcon_->resetState();
    }

    void MainWindow::showLoginPage(const bool _is_auth_error)
    {
        closePopups({ Utils::CloseWindowInfo::Initiator::MainWindow, Utils::CloseWindowInfo::Reason::ShowLoginPage });
        Ui::GetDispatcher()->unsubscribeTasksCounter();

        auto showLoginPageInternal = [this, _is_auth_error]()
        {
#ifdef __APPLE__
            getMacSupport()->createMenuBar(true);

            getMacSupport()->forceEnglishInputSource();
#endif

            if (!loginPage_)
            {
                loginPage_ = new LoginPage(this);
                Testing::setAccessibleName(loginPage_, qsl("AS LoginPage"));
                stackedWidget_->addWidget(loginPage_);

                connect(loginPage_, &LoginPage::loggedIn, this, &MainWindow::showAppsPage);

#ifndef STRIP_VOIP
                if (callStatsMgr_)
                    connect(loginPage_, &LoginPage::loggedIn, callStatsMgr_, &CallQualityStatsMgr::reconnectToMyInfo);
#endif
            }

            setCurrentWidget(loginPage_);

            if (_is_auth_error)
                Q_EMIT Utils::InterConnector::instance().authError(core::le_wrong_login);

            clear_global_objects();
        };

        if (GeneralDialog::isActive())
            QTimer::singleShot(0, this, showLoginPageInternal);
        else
            showLoginPageInternal();
    }

    void MainWindow::showAppsPage()
    {
        connect(MyInfo(), &my_info::needGDPR, this, &MainWindow::showGDPRPage, Qt::UniqueConnection);

        Styling::getThemesContainer().chooseAndSetTheme();

        if (!appsPage_)
            initAppsPage();
        else
            appsPage_->prepareForShow();

#ifdef __APPLE__
        getMacSupport()->createMenuBar(false);
#endif
        setCurrentWidget(appsPage_);
    }

    void MainWindow::showMessengerPage()
    {
        showAppsPage();
        appsPage_->showMessengerPage();
    }

    void MainWindow::showGDPRPage()
    {
        closePopups({ Utils::CloseWindowInfo::Initiator::MainWindow, Utils::CloseWindowInfo::Reason::ShowGDPRPage });

        auto showGDPRPageInternal = [this]()
        {
#ifdef __APPLE__
            getMacSupport()->createMenuBar(true);
#endif

            if (!gdprPage_)
            {
                TermsPrivacyWidget::Options options;
                options.isGdprUpdate_ = true;
                const auto product = config::get().translation(config::translations::gdpr_welcome);

                const auto title = QT_TRANSLATE_NOOP("terms_privacy_widget", product.data());
                const auto description = QT_TRANSLATE_NOOP("terms_privacy_widget",
                                                           "By clicking \"Accept and agree\", you confirm that you have read carefully "
                                                           "and agree to our <a href=\"%1\">Terms</a> "
                                                           "and <a href=\"%2\">Privacy Policy</a>").arg(legalTermsUrl(), privacyPolicyUrl());

                gdprPage_ = new TermsPrivacyWidget(title, description, options);
                if (gdprPage_)
                {
                    gdprPage_->init();

                    Testing::setAccessibleName(gdprPage_, qsl("AS GDPR"));
                    stackedWidget_->addWidget(gdprPage_);

                    connect(gdprPage_, &TermsPrivacyWidget::agreementAccepted, this, &MainWindow::showAppsPage);
                }
            }

            if (gdprPage_)
                setCurrentWidget(gdprPage_);

            GetDispatcher()->post_stats_to_core(core::stats::stats_event_names::gdpr_show_update);
        };

        if (GeneralDialog::isActive())
            QTimer::singleShot(0, this, showGDPRPageInternal);
        else
            showGDPRPageInternal();
    }

void MainWindow::showUserAgreementPage(const QString& _termsOfUseUrl, const QString& _privacyPolicyUrl)
{
    closePopups({ Utils::CloseWindowInfo::Initiator::MainWindow, Utils::CloseWindowInfo::Reason::ShowUserAgreementPage });
    this->loginPage_->openUserAgreement(_termsOfUseUrl, _privacyPolicyUrl);
}


void MainWindow::checkForUpdates()
    {
#ifdef __APPLE__
        getMacSupport()->runMacUpdater();
#endif
    }

    void MainWindow::checkForUpdatesInBackground()
    {
#ifdef __APPLE__
        if (getMacSupport()->isUpdateRequested())
            Q_EMIT Utils::InterConnector::instance().onMacUpdateInfo(Utils::MacUpdateState::Requested);
        getMacSupport()->checkForUpdatesInBackground();
#endif
    }

    void MainWindow::installUpdates()
    {
#ifdef __APPLE__
        getMacSupport()->installUpdates();
#endif
    }

    void MainWindow::showIconInTaskbar(bool _show)
    {
        if (_show)
            showTaskbarIcon();
        else
            hideTaskbarIcon();
    }

    void MainWindow::copy()
    {
        if (auto focused = QApplication::focusWidget())
        {
            bool handled = false;

            if constexpr (platform::is_apple())
            {
                const auto area = qobject_cast<Ui::MessagesScrollArea*>(focused);
                if (area)
                    handled = true;

                if (!area || area && !MimeData::copyMimeData(*area))
                {
                    const auto text = Utils::InterConnector::instance().getSidebarSelectedText();
                    if (!text.isEmpty())
                    {
                        QApplication::clipboard()->setText(text);
                        Utils::showCopiedToast();
                        handled = true;
                    }
                }

                if (!Utils::InterConnector::instance().isMultiselect())
                {
                    const auto aimId = area ? area->getAimid() : QString{};
                    Q_EMIT Utils::InterConnector::instance().setFocusOnInput(aimId);
                }
            }

            if (!handled)
            {
                QApplication::postEvent(focused, new QKeyEvent(QEvent::KeyPress, Qt::Key_C, Qt::ControlModifier));
                QApplication::postEvent(focused, new QKeyEvent(QEvent::KeyRelease, Qt::Key_C, Qt::ControlModifier));
            }
        }
        else if (gallery_)
        {
            gallery_->onCopy();
        }
    }

    void MainWindow::cut()
    {
        if (auto focused = QApplication::focusWidget(); focused && focused->inputMethodHints() != Qt::ImhNone)
        {
            if (Utils::InterConnector::instance().isRecordingPtt())
                return;

            QApplication::postEvent(focused, new QKeyEvent(QEvent::KeyPress, Qt::Key_X, Qt::ControlModifier));
            QApplication::postEvent(focused, new QKeyEvent(QEvent::KeyRelease, Qt::Key_X, Qt::ControlModifier));
        }
    }

    void MainWindow::paste()
    {
        if (auto focused = QApplication::focusWidget(); focused && focused->inputMethodHints() != Qt::ImhNone)
        {
            if (Utils::InterConnector::instance().isRecordingPtt())
                return;

            QApplication::postEvent(focused, new QKeyEvent(QEvent::KeyPress, Qt::Key_V, Qt::ControlModifier));
            QApplication::postEvent(focused, new QKeyEvent(QEvent::KeyRelease, Qt::Key_V, Qt::ControlModifier));
        }
    }

    void MainWindow::undo()
    {
        if (auto focused = QApplication::focusWidget(); focused && focused->inputMethodHints() != Qt::ImhNone)
        {
            if (Utils::InterConnector::instance().isRecordingPtt())
                return;

            QApplication::postEvent(focused, new QKeyEvent(QEvent::KeyPress, Qt::Key_Z, Qt::ControlModifier));
            QApplication::postEvent(focused, new QKeyEvent(QEvent::KeyRelease, Qt::Key_Z, Qt::ControlModifier));
        }
    }

    void MainWindow::redo()
    {
        if (auto focused = QApplication::focusWidget(); focused && focused->inputMethodHints() != Qt::ImhNone)
        {
            if (Utils::InterConnector::instance().isRecordingPtt())
                return;

            QApplication::postEvent(focused, new QKeyEvent(QEvent::KeyPress, Qt::Key_Z, Qt::ControlModifier|Qt::ShiftModifier));
            QApplication::postEvent(focused, new QKeyEvent(QEvent::KeyRelease, Qt::Key_Z, Qt::ControlModifier|Qt::ShiftModifier));
        }
    }

    void MainWindow::activateNextUnread()
    {
        if (MainPage::hasInstance())
        {
            activateIfNeeded();
            closePopups({ Utils::CloseWindowInfo::Initiator::Unknown, Utils::CloseWindowInfo::Reason::Keep_Sidebar });
            MainPage::instance()->recentsTabActivate(true);
        }
    }

    void MainWindow::activateNextChat()
    {
        if (MainPage::hasInstance())
        {
            activateIfNeeded();
            MainPage::instance()->nextChat();
        }
    }

    void MainWindow::activatePrevChat()
    {
        if (MainPage::hasInstance())
        {
            activateIfNeeded();
            MainPage::instance()->prevChat();
        }
    }

    void MainWindow::activateAbout()
    {
        if (MainPage::hasInstance())
        {
            activateIfNeeded();
            MainPage::instance()->settingsTabActivate(Utils::CommonSettingsType::CommonSettingsType_About);
        }
    }

    void MainWindow::activateSettings()
    {
        if (MainPage::hasInstance())
        {
            activateIfNeeded();
            MainPage::instance()->settingsClicked();
        }
    }

    void MainWindow::closeCurrent()
    {
        activate();

        if (const auto& selectedContact = Logic::getContactListModel()->selectedContact(); selectedContact.isEmpty())
            hideWindow();
        else
            Logic::getRecentsModel()->hideChat(selectedContact);
    }

    void MainWindow::toggleFullScreen()
    {
#ifdef __APPLE__
        if (!Utils::InterConnector::instance().isDragOverlay())
            MacSupport::toggleFullScreen();
#endif
    }

    void MainWindow::updateMainMenu()
    {
#ifdef __APPLE__
        getMacSupport()->updateMainMenu();
#endif
    }

    void MainWindow::exit()
    {
        Utils::InterConnector::instance().startDestroying();

#ifdef STRIP_VOIP
        QApplication::exit();
#else

#ifdef _WIN32
        if (Shadow_)
            SetWindowPos((HWND)Shadow_->winId(), (HWND)winId(), 0, 0, 0, 0, SWP_NOACTIVATE | SWP_NOOWNERZORDER | SWP_NOMOVE | SWP_NOSIZE | SWP_HIDEWINDOW);
#endif

        Ui::GetDispatcher()->getVoipController().voipReset();
#endif //STRIP_VOIP
    }

    void MainWindow::onVoipResetComplete()
    {
        QApplication::exit();
    }

    void MainWindow::hideWindow()
    {
        Q_EMIT windowHiddenToTray();

        TaskBarIconHidden_ = true;

#if defined __APPLE__
        MacSupport::closeWindow(winId());
        updateMainMenu();
#elif defined (_WIN32)
        ShowWindow(reinterpret_cast<HWND>(winId()), SW_HIDE);
#else
        hide();
#endif

#ifdef _WIN32
        if (Shadow_)
            SetWindowPos((HWND)Shadow_->winId(), (HWND)winId(), 0, 0, 0, 0, SWP_NOACTIVATE | SWP_NOOWNERZORDER | SWP_NOMOVE | SWP_NOSIZE | SWP_HIDEWINDOW);
#endif //_WIN32
    }

    void MainWindow::unhideWindow()
    {
        Q_EMIT windowUnhiddenFromTray();

#ifdef _WIN32
        setVisible(true);
        if (Shadow_)
            SetWindowPos((HWND)Shadow_->winId(), (HWND)winId(), 0, 0, 0, 0, SWP_NOACTIVATE | SWP_NOOWNERZORDER | SWP_NOMOVE | SWP_NOSIZE | SWP_SHOWWINDOW);
        trayIcon_->hideAlerts();
        if (!SkipRead_ && isMessengerPageContactDialog())
        {
            Logic::getRecentsModel()->sendLastRead();
            Logic::getUnknownsModel()->sendLastRead();
        }

        notifyWindowActive(isActive());

        if (!TaskBarIconHidden_)
            SkipRead_ = false;
        TaskBarIconHidden_ = false;
#endif
    }

    void MainWindow::pasteEmoji()
    {
        if (!Logic::getContactListModel()->selectedContact().isEmpty())
        {
#ifdef __APPLE__
            MacSupport::showEmojiPanel();
#else
            Q_EMIT Utils::InterConnector::instance().showStickersPicker();
#endif
        }
    }

    void MainWindow::checkNewVersion()
    {
        const auto savedHash = get_gui_settings()->get_value(release_notes_sha1, QString());
        const auto currentHash = Ui::ReleaseNotes::releaseNotesHash();

        const auto lastVersion = get_gui_settings()->get_value(last_version, QString());
        const auto currentVersion = QString::fromUtf8(VERSION_INFO_STR);

        const auto last = lastVersion.splitRef(ql1c('.'));
        const auto current = currentVersion.splitRef(ql1c('.'));

        const auto currentIsGreater = std::lexicographical_compare(last.cbegin(), last.cend(), current.cbegin(), current.cend(), [](const auto & v1, const auto & v2)
        {
            return v1.toInt() < v2.toInt();
        });

        const auto firstRun = get_gui_settings()->get_value<bool>(first_run, true);

        get_gui_settings()->set_value(release_notes_sha1, currentHash);

        if (currentIsGreater && platform::is_linux() && !build::is_store())
        {
            auto task = new Utils::RegisterCustomSchemeTask();
            task->setAutoDelete(true);

            connect(task, &Utils::RegisterCustomSchemeTask::finished, this, [currentVersion]() { get_gui_settings()->set_value(last_version, currentVersion); });
            QThreadPool::globalInstance()->start(task);
        }
        else
        {
            get_gui_settings()->set_value(last_version, currentVersion);
        }

        if (savedHash != currentHash && currentIsGreater && !firstRun)
            Ui::ReleaseNotes::showReleaseNotes();
    }

    void MainWindow::stateChanged(const Qt::WindowState& _state)
    {
        userActivity();
    }

#ifdef __APPLE__
    MacSupport* MainWindow::getMacSupport()
    {
        static MacSupport mac_support(this);

        return &mac_support;
    }
#endif //__APPPLE__

    void MainWindow::bringToFront()
    {
        if (isActive())
        {
            if (MainPage::hasInstance() && MainPage::instance()->isVideoWindowOn())
                MainPage::instance()->showVideoWindow();
            activate();
        }
        else
        {
            activate();
            if (MainPage::hasInstance() && MainPage::instance()->isVideoWindowOn())
                MainPage::instance()->showVideoWindow();
        }

    }

    void MainWindow::zoomWindow()
    {
#ifdef __APPLE__
        if (MainPage::hasInstance() && MainPage::instance()->isVideoWindowActive())
        {
            auto mainPage = MainPage::instance();
            if (mainPage->isVideoWindowMaximized())
                mainPage->showVideoWindow();
            else
                mainPage->maximizeVideoWindow();
            return;
        }

        getMacSupport()->zoomWindow(winId());
        updateMainMenu();
#endif
    }

    void MainWindow::openStatusPicker()
    {
        auto w = new Statuses::SelectStatusWidget();
        Testing::setAccessibleName(w, qsl("AS SelectStatusWidget"));

        Ui::GeneralDialog::Options opt;
        opt.fixedSize_ = false;
        Ui::GeneralDialog dialog(w, this, opt);
        dialog.execute();
        setFocus();
    }

    void MainWindow::omicronUpdated()
    {
        setMinimumWidth(getMinWindowWidth());
    }

    void MainWindow::onMiniAppsUpdated()
    {
        if (Logic::GetAppsContainer()->isAppEnabled(Utils::MiniApps::getTasksId()))
            Ui::GetDispatcher()->subscribeTasksCounter();
        else
            Ui::GetDispatcher()->unsubscribeTasksCounter();

        if (Logic::GetAppsContainer()->isAppEnabled(Utils::MiniApps::getMailId()))
            Ui::GetDispatcher()->subscribeMailsCounter();
        else
            Ui::GetDispatcher()->unsubscribeMailsCounter();
    }

    QString MainWindow::getCurrentAimId(bool _checkSearchFocus) const
    {
        const auto mainPage = MainPage::instance();
        auto aimId = Logic::getContactListModel()->selectedContact();
        if (mainPage && mainPage->isSidebarWithThreadPage() && mainPage->isSidebarInFocus(_checkSearchFocus))
        {
            aimId = mainPage->getSidebarAimid(Sidebar::ResolveThread::Yes);
        }
#ifdef HAS_WEB_ENGINE
        else if (isWebAppTasksPage())
        {
            if (auto tasks = appsPage_->tasksPage(); tasks && tasks->isSidebarVisible())
                aimId = tasks->getAimId();
        }
#endif
        return aimId;
    }

    bool MainWindow::isSearchInDialog(QKeyEvent* _event) const
    {
        const auto searchCode = get_gui_settings()->get_value<int>(settings_shortcuts_search_action, static_cast<int>(ShortcutsSearchAction::Default));
        const auto searchAct = static_cast<ShortcutsSearchAction>(searchCode);
        const auto isGlobalAct = searchAct == ShortcutsSearchAction::GlobalSearch;
        const bool isGlobalHotkey = _event->modifiers() & Qt::ShiftModifier;
        return isGlobalAct == isGlobalHotkey;
    }

    void MainWindow::activateIfNeeded()
    {
        if (!isActive())
            activate();
    }
}
