#include "stdafx.h"
#include "MainWindow.h"

#include "ContactDialog.h"
#include "LoginPage.h"
#include "MainPage.h"
#include "controls/GeneralDialog.h"
#include "contact_list/ContactListModel.h"
#include "contact_list/RecentsModel.h"
#include "contact_list/UnknownsModel.h"
#include "contact_list/MentionModel.h"
#include "contact_list/AddContactDialogs.h"
#include "history_control/HistoryControlPage.h"
#include "history_control/MessagesScrollArea.h"
#include "history_control/MentionCompleter.h"
#include "mplayer/VideoPlayer.h"
#include "sounds/SoundsManager.h"
#include "tray/TrayIcon.h"
#include "../gui_settings.h"
#include "../controls/MainStackedWidget.h"

#include "controls/TooltipWidget.h"
#include "../controls/CustomButton.h"
#include "../previewer/GalleryWidget.h"
#include "../utils/utils.h"
#include "../utils/InterConnector.h"
#include "../utils/gui_metrics.h"
#include "../utils/SearchPatternHistory.h"
#include "utils/periodic_gui_metrics.h"
#include "../cache/stickers/stickers.h"
#include "../my_info.h"
#include "main_window/ReleaseNotes.h"
#include "main_window/input_widget/AttachFilePopup.h"
#include "main_window/input_widget/HistoryTextEdit.h"
#include "../../../common.shared/version_info_constants.h"
#include "user_activity/user_activity.h"
#include "../voip/quality/CallQualityStatsMgr.h"
#include "../memory_stats/gui_memory_monitor.h"
#include "ConnectionWidget.h"
#include "../utils/RegisterCustomSchemeTask.h"
#include "LocalPIN.h"

#include "styles/ThemeParameters.h"
#include "../utils/InterConnector.h"
#include "previewer/toast.h"

#include "../core_dispatcher.h"

#ifdef _WIN32
#include "../../common.shared/win32/crash_handler.h"
#include "win32/WinNativeWindow.h"
#include <windowsx.h>
#include <wtsapi32.h>
#pragma comment(lib, "wtsapi32.lib")
#endif //_WIN32

#ifdef __APPLE__
#include "macos/AccountsPage.h"
#include "../utils/macos/mac_support.h"

#ifndef MAC_MIGRATION
#define MAC_MIGRATION
#include "../utils/macos/mac_migration.h"
#endif

#include "../utils/macos/mac_titlebar.h"
#endif //__APPLE__

namespace
{
#ifdef _WIN32
    constexpr int TITLE_CONTEXT_MENU_CMD = WM_USER + 0x100;

    int getWindowBorderWidth()
    {
        return Utils::scale_value(Ui::NativeWindowBorderWidth);
    }
#endif

    int getMinWindowWidth()
    {
        return Utils::scale_value(380);
    }

    int getMinWindowHeight()
    {
        return Utils::scale_value(550);
    }

    QRect getDefaultWindowSize()
    {
        return QRect(0, 0, Utils::scale_value(1000), Utils::scale_value(600));
    }

    QSize getTitleButtonOriginalSize()
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

    auto getErrorToastOffset()
    {
        return Utils::scale_value(10);
    }
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
        HWND handle = mainWindow_->getParentWindow();
        HMENU sysMenu = GetSystemMenu(handle, FALSE);
        if (sysMenu)
        {
            UINT enabledState = MF_BYCOMMAND | MF_ENABLED;
            UINT disabledState = MF_BYCOMMAND | (MF_DISABLED | MF_GRAYED);
            EnableMenuItem(sysMenu, SC_MAXIMIZE,mainWindow_->isMaximized()? disabledState: enabledState);
            EnableMenuItem(sysMenu, SC_SIZE,    mainWindow_->isMaximized()? disabledState: enabledState);
            EnableMenuItem(sysMenu, SC_MOVE,    mainWindow_->isMaximized()? disabledState: enabledState);
            EnableMenuItem(sysMenu, SC_RESTORE, mainWindow_->isMaximized()? enabledState: disabledState);

            int flag = TrackPopupMenu(sysMenu, TPM_LEFTALIGN|TPM_LEFTBUTTON|TPM_RETURNCMD, _pos.x(), _pos.y(), NULL, handle, nullptr);
            if(flag > 0)
                SendMessage(handle, WM_SYSCOMMAND, flag, TITLE_CONTEXT_MENU_CMD);
        }
#endif
    }

    void TitleWidgetEventFilter::showLogoContextMenu()
    {
        mainWindow_->activate();
        mainWindow_->updateMainMenu();
#ifdef _WIN32
        SendMessage(mainWindow_->getParentWindow(), WM_SYSCOMMAND, SC_KEYMENU, VK_SPACE);
#else
        auto logo = mainWindow_->getWindowLogo();
        showContextMenu(logo->mapToGlobal(logo->rect().center()));
#endif
    }

    bool TitleWidgetEventFilter::eventFilter(QObject* _obj, QEvent* _event)
    {
        if constexpr (platform::is_linux())
            return QObject::eventFilter(_obj, _event);

        bool ignore = std::any_of(ignoredWidgets_.begin(),
                                  ignoredWidgets_.end(),
                                  [](QWidget* _wdg) { return _wdg->underMouse(); });
        if (!ignore)
        {
            QMouseEvent* mouseEvent = dynamic_cast<QMouseEvent*>(_event);
            if (mouseEvent)
            {
                const bool logoUnderMouse = mainWindow_->getWindowLogo()->rect().contains(mouseEvent->pos());
                switch (_event->type())
                {
                case QEvent::MouseButtonDblClick:
                    if (logoUnderMouse)
                    {
                        iconTimer_->stop();
                        emit logoDoubleClick();
                    }
                    else
                    {
                        emit doubleClick();
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
                        emit moveRequest(mouseEvent->globalPos() - clickPos_);
                    break;

                case QEvent::MouseButtonRelease:
                    dragging_ = false;
                    emit checkPosition();
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
        HWND parent = (HWND)::GetWindowLong(parentNativeWindowHandle_, GWL_HWNDPARENT);
        if (!parent)
            ::SetWindowLong(parentNativeWindowHandle_, GWL_HWNDPARENT, (LONG) fake_parent_window_);
#endif //_WIN32
        trayIcon_->forceUpdateIcon();
    }

    void MainWindow::showTaskbarIcon()
    {
#ifdef _WIN32
        ::SetWindowLong(parentNativeWindowHandle_, GWL_HWNDPARENT, 0L);

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

    void MainWindow::setFocusOnInput()
    {
        if (mainPage_)
            mainPage_->setFocusOnInput();
    }

    void MainWindow::onSendMessage(const QString& contact)
    {
        if (mainPage_)
            mainPage_->onSendMessage(contact);
    }

    int MainWindow::getTitleHeight() const
    {
        return platform::is_windows() ? titleWidget_->height() : 0;
    }

    bool MainWindow::isMaximized() const
    {
        return platform::is_windows() ? isMaximized_ : QMainWindow::isMaximized();
    }

    bool MainWindow::isMinimized() const
    {
#ifdef _WIN32
        return isNativeWindowShowCmd(getParentWindow(), SW_SHOWMINIMIZED) || isNativeWindowShowCmd(getParentWindow(), SW_MINIMIZE);
#else
        return QMainWindow::isMinimized();
#endif
    }

    bool MainWindow::hasMemoryDockWidget() const
    {
        return memoryDockWidget_;
    }

    void MainWindow::addMemoryDockWidget(Qt::DockWidgetArea _area, QDockWidget *_dockWidget)
    {
        if (hasMemoryDockWidget())
        {
            assert(!"Already have a dock memory widget");
            return;
        }

        addDockWidget(_area, _dockWidget);
        memoryDockWidget_ = _dockWidget;

        connect(memoryDockWidget_, &QObject::destroyed,
                this, [this](QObject*){
            removeMemoryDockWidget();
        });
    }

    bool MainWindow::removeMemoryDockWidget()
    {
        if (!memoryDockWidget_)
            return false;

        memoryDockWidget_->deleteLater();
        memoryDockWidget_ = nullptr;

        return true;
    }

    void MainWindow::lock()
    {
        if (!mainPage_) // init mainpage to show it faster after unlock
        {
            mainPage_ = MainPage::instance(this);
            Testing::setAccessibleName(mainPage_, qsl("AS mainPage_"));
            stackedWidget_->addWidget(mainPage_);
        }

        if (!localPINWidget_)
        {
            localPINWidget_ = new LocalPINWidget(LocalPINWidget::Mode::VerifyPIN, this);
            stackedWidget_->addWidget(localPINWidget_);
        }

        stackedWidget_->setCurrentWidget(localPINWidget_, ForceLocked::Yes);

        closeGallery();
        emit Utils::InterConnector::instance().applicationLocked();
#ifdef __APPLE__
        getMacSupport()->createMenuBar(true);
#endif
    }

#ifdef _WIN32
    HWND MainWindow::getParentWindow() const
    {
        return parentNativeWindowHandle_;
    }

    void MainWindow::hideParentWindow()
    {
        ignoreActivateEvent_ = true;
        ShowWindow(parentNativeWindowHandle_, SW_HIDE);
        QTimer::singleShot(0, this, [this]() {ignoreActivateEvent_ = false; });
    }

    std::unique_ptr<WinNativeWindow> MainWindow::takeNativeWindow()
    {
        return std::move(parentNativeWindow_);
    }
#endif

    QRect MainWindow::nativeGeometry() const
    {
#ifdef _WIN32
        RECT winRect;
        GetWindowRect(getParentWindow(), &winRect);
        return QRect(winRect.left, winRect.top, winRect.right - winRect.left, winRect.bottom - winRect.top);
#else
        return geometry();
#endif
    }

    MainWindow::MainWindow(QApplication* app, const bool _has_valid_login, const bool _locked)
        : mainPage_(nullptr)
        , loginPage_(nullptr)
        , app_(app)
        , eventFilter_(new TitleWidgetEventFilter(this))
        , trayIcon_(new TrayIcon(this))
        , backgroundPixmap_(QPixmap())
        , Shadow_(nullptr)
        , ffplayer_(nullptr)
        , callStatsMgr_(build::is_dit() ? nullptr : new CallQualityStatsMgr(this))
        , memoryDockWidget_(nullptr)
        , connStateWatcher_(new ConnectionStateWatcher(this))
        , SkipRead_(false)
        , TaskBarIconHidden_(false)
        , isMaximized_(false)
        , sleeping_(false)
        , userActivityTimer_(nullptr)
#ifdef _WIN32
        , parentNativeWindow_(std::make_unique<WinNativeWindow>())
        , parentNativeWindowHandle_(nullptr)
        , ignoreActivateEvent_(false)
#endif
#ifdef __APPLE__
        , accounts_page_(nullptr)
        , migrationManager_(new MacMigrationManager(this))
#endif //__APPLE__
        , windowActive_(false)
        , uiActive_(false)
        , localPINWidget_(nullptr)
    {
        Utils::InterConnector::instance().setMainWindow(this);

#ifdef _WIN32
        const auto productDataPath = ::common::get_user_profile() + build::GetProductVariant(product_path_icq_w, product_path_agent_w, product_path_biz_w, product_path_dit_w);
        core::dump::crash_handler chandler("icq.desktop", productDataPath, false);
        chandler.set_process_exception_handlers();
        chandler.set_thread_exception_handlers();
#endif //_WIN32

#ifdef __APPLE__
        getMacSupport()->enableMacCrashReport();
        getMacSupport()->postCrashStatsIfNedded();
        getMacSupport()->listenSleepAwakeEvents();
#endif
        setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Minimum);
        setLayoutDirection(Qt::LeftToRight);
        setAutoFillBackground(false);

        Utils::ApplyStyle(this, Styling::getParameters().getScrollBarQss());

        mainWidget_ = new QWidget(this);
        mainWidget_->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Minimum);
        Testing::setAccessibleName(mainWidget_, qsl("AS mainWidget_"));
        mainLayout_ = Utils::emptyVLayout(mainWidget_);
        mainLayout_->setSizeConstraint(QLayout::SetDefaultConstraint);
        titleWidget_ = new QWidget(mainWidget_);
        titleWidget_->setAutoFillBackground(true);
        Utils::updateBgColor(titleWidget_, Styling::getParameters().getColor(Styling::StyleVariable::BASE_BRIGHT_INVERSE));
        titleWidget_->setFixedHeight(Utils::scale_value(28));
        titleWidget_->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);
        Testing::setAccessibleName(titleWidget_, qsl("AS titleWidget_"));
        Utils::ApplyStyle(titleWidget_, Styling::getParameters().getTitleQss());

        titleLayout_ = Utils::emptyHLayout(titleWidget_);

        logo_ = new QLabel(titleWidget_);
        logo_->setPixmap(Utils::renderSvgScaled(build::GetProductVariant(qsl(":/logo/logo_icq"), qsl(":/logo/logo_agent"), qsl(":/logo/logo_biz_16"), qsl(":/logo/logo_dit")), QSize(16, 16)));
        logo_->setObjectName(qsl("windowIcon"));
        logo_->setProperty(build::GetProductVariant("icq", "agent", "biz", "dit"), true);
        logo_->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
        logo_->setFocusPolicy(Qt::NoFocus);
        Testing::setAccessibleName(logo_, qsl("AS logo_"));
        titleLayout_->addWidget(logo_);

        title_ = new QLabel(titleWidget_);
        title_->setFocusPolicy(Qt::NoFocus);
        QPalette pal = title_->palette();
        pal.setColor(QPalette::Foreground, Styling::getParameters().getColor(Styling::StyleVariable::TEXT_SOLID));
        title_->setPalette(pal);

        title_->setFont(Fonts::appFontScaled(12, Fonts::FontWeight::SemiBold));
        title_->setContentsMargins(Utils::scale_value(8), 0, 0, 0);
        Testing::setAccessibleName(title_, qsl("AS title_"));
        titleLayout_->addWidget(title_);
        titleLayout_->addItem(new QSpacerItem(Utils::scale_value(20), 20, QSizePolicy::Fixed, QSizePolicy::Minimum));

        spacer_ = new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);
        titleLayout_->addItem(spacer_);

        const auto connectStates = [this](CustomButton* _btn, const auto _hoverClr, const auto _pressClr)
        {
            const auto onHover = [_btn, _hoverClr](const bool _hovered) { _btn->setBackgroundColor(Styling::getParameters().getColor(_hovered ? _hoverClr : Styling::StyleVariable::BASE_BRIGHT_INVERSE)); };
            const auto onPress = [_btn, _pressClr](const bool _pressed) { _btn->setBackgroundColor(Styling::getParameters().getColor(_pressed ? _pressClr : Styling::StyleVariable::BASE_BRIGHT_INVERSE)); };
            connect(_btn, &CustomButton::changedHover, this, onHover);
            connect(_btn, &CustomButton::changedPress, this, onPress);
        };

        const auto titleBtn = [p = titleWidget_, connectStates](const auto& _path, const auto& _accName, const auto _hoverClr, const auto _pressClr)
        {
            auto btn = new CustomButton(p, _path, getTitleButtonOriginalSize());
            btn->setBackgroundColor(Styling::getParameters().getColor(Styling::StyleVariable::BASE_BRIGHT_INVERSE));
            btn->setFixedSize(getTitleButtonSize());
            btn->setFocusPolicy(Qt::NoFocus);
            Testing::setAccessibleName(btn, _accName);
            Styling::Buttons::setButtonDefaultColors(btn);

            connectStates(btn, _hoverClr, _pressClr);
            return btn;
        };

        hideButton_ = titleBtn(qsl(":/titlebar/minimize_button"), qsl("AS hideButton_"), Styling::StyleVariable::BASE_BRIGHT_HOVER, Styling::StyleVariable::BASE_BRIGHT_ACTIVE);
        titleLayout_->addWidget(hideButton_);

        maximizeButton_ = titleBtn(qsl(":/titlebar/expand_button"), qsl("AS maximizeButton_"), Styling::StyleVariable::BASE_BRIGHT_HOVER, Styling::StyleVariable::BASE_BRIGHT_ACTIVE);
        titleLayout_->addWidget(maximizeButton_);

        closeButton_ = titleBtn(qsl(":/titlebar/close_button"), qsl("AS closeButton_"), Styling::StyleVariable::SECONDARY_ATTENTION, Styling::StyleVariable::SECONDARY_ATTENTION);
        closeButton_->setHoverColor(Styling::getParameters().getColor(Styling::StyleVariable::TEXT_SOLID_PERMANENT));
        closeButton_->setPressedColor(Styling::getParameters().getColor(Styling::StyleVariable::TEXT_SOLID_PERMANENT));
        titleLayout_->addWidget(closeButton_);

        Testing::setAccessibleName(titleWidget_, qsl("AS titleWidget_"));
        mainLayout_->addWidget(titleWidget_);
        stackedWidget_ = new MainStackedWidget(mainWidget_);
        Testing::setAccessibleName(stackedWidget_, qsl("AS stackedWidget_"));

        mainLayout_->addWidget(stackedWidget_);
        this->setCentralWidget(mainWidget_);

        stackedWidget_->setCurrentIndex(-1);

        QFont f = QApplication::font();
        f.setStyleStrategy(QFont::PreferAntialias);
        // FONT
        QApplication::setFont(f);

        if (_locked)
            LocalPIN::instance()->lock();
        else if (!get_gui_settings()->get_value(settings_keep_logged_in, true) || !_has_valid_login)
            showLoginPage(false);
        else
            showMainPage();

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

        QString appTitle = Utils::getAppTitle();
        if constexpr (build::is_alpha())
            appTitle += qsl(" alpha version");

        title_->setText(appTitle);
        title_->setAttribute(Qt::WA_TransparentForMouseEvents);
        logo_->setAttribute(Qt::WA_TransparentForMouseEvents);

        setWindowTitle(title_->text());
#ifdef _WIN32
        if (parentNativeWindow_->init(appTitle))
            parentNativeWindowHandle_ = parentNativeWindow_->hwnd();

        setWindowFlags(Qt::Window | Qt::FramelessWindowHint | Qt::NoDropShadowWindowHint | Qt::WindowMinMaxButtonsHint | Qt::WindowSystemMenuHint);
        setProperty("_q_embedded_native_parent_handle", (WId)parentNativeWindowHandle_);

        SetParent((HWND)winId(), parentNativeWindowHandle_);
        SetWindowLong((HWND)winId(), GWL_STYLE, WS_CHILD | WS_CLIPCHILDREN | WS_CLIPSIBLINGS);
        QEvent e(QEvent::EmbeddingControl);
        QApplication::sendEvent(this, &e);

        parentNativeWindow_->setChildWindow((HWND)winId());
        parentNativeWindow_->setChildWidget(this);

        fake_parent_window_ = Utils::createFakeParentWindow();
#else
        titleWidget_->hide();
#endif

        title_->setMouseTracking(true);

        connect(hideButton_, &QPushButton::clicked, this, &MainWindow::minimize);
        connect(maximizeButton_, &QPushButton::clicked, this, &MainWindow::maximize);
        connect(closeButton_, &QPushButton::clicked, this, &MainWindow::hideWindow);

        // prevent the MainWindow from moving inside the parentNativeWindow
        if constexpr (!platform::is_windows())
        {
            connect(eventFilter_, &Ui::TitleWidgetEventFilter::doubleClick, this, &MainWindow::maximize);
            connect(eventFilter_, &Ui::TitleWidgetEventFilter::checkPosition, this, &MainWindow::checkPosition);
            connect(eventFilter_, &Ui::TitleWidgetEventFilter::moveRequest, this, &MainWindow::moveRequest);
        }

        connect(&Ui::GetDispatcher()->getVoipController(), &voip_proxy::VoipController::onVoipResetComplete, this, &MainWindow::onVoipResetComplete, Qt::QueuedConnection);
        if (callStatsMgr_)
            connect(&Ui::GetDispatcher()->getVoipController(), &voip_proxy::VoipController::onVoipCallEndedStat, callStatsMgr_, &CallQualityStatsMgr::onVoipCallEndedStat);

        connect(Ui::GetDispatcher(), &core_dispatcher::connectionStateChanged, connStateWatcher_, &ConnectionStateWatcher::setState);

        connect(Ui::GetDispatcher(), &core_dispatcher::needLogin, this, &MainWindow::showLoginPage, Qt::DirectConnection);
        connect(&Utils::InterConnector::instance(), &Utils::InterConnector::showIconInTaskbar, this, &MainWindow::showIconInTaskbar);

        connect(&Utils::InterConnector::instance(), &Utils::InterConnector::activateNextUnread, this, &MainWindow::activateNextUnread);

        connect(&Utils::InterConnector::instance(), &Utils::InterConnector::noPagesInDialogHistory, this, [this]() {
            if (mainPage_ && mainPage_->isOneFrameTab())
            {
                mainPage_->update();
                setFocus();
            }
        });

        connect(this, &MainWindow::needActivate, this, &MainWindow::activate);
        connect(this, &MainWindow::needActivateWith, this, &MainWindow::activateWithReason);

        connect(get_gui_settings(), &Ui::qt_gui_settings::changed, this, &MainWindow::guiSettingsChanged);

        connect(Ui::GetDispatcher(), &Ui::core_dispatcher::memberAddFailed, this, Ui::memberAddFailed);
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
        initSettings();
        initStats();
#ifdef _WIN32
        DragAcceptFiles((HWND)winId(), TRUE);
#endif //_WIN32

        if (!get_gui_settings()->get_value<bool>(settings_show_in_taskbar, true))
            hideTaskbarIcon();

#ifdef __APPLE__
        getMacSupport()->enableMacUpdater();
        getMacSupport()->enableMacPreview(winId());

        getMacSupport()->windowTitle()->setup();
        getMacSupport()->windowTitle()->setTitleText(title_->text());
#endif

        if constexpr(!platform::is_apple())
            QTimer::singleShot(2000, this, &MainWindow::checkNewVersion);

        setMouseTracking(true);

        userActivityTimer_ = new QTimer(this);
        QObject::connect(userActivityTimer_, &QTimer::timeout, this, &MainWindow::userActivityTimer);
        userActivityTimer_->start(getUserActivityCheckTimeout().count());

        app_->installNativeEventFilter(this);
        app_->installEventFilter(this);

        updateMainMenu();
#ifdef _WIN32
        // Send the parent native window a WM_SIZE message to update the widget size
        SendMessage(parentNativeWindowHandle_, WM_SIZE, 0, 0);
#endif
    }

    MainWindow::~MainWindow()
    {
        Tooltip::resetDefaultTooltip();
        resetMainPage();

        app_->removeNativeEventFilter(this);
        app_->removeEventFilter(this);

#ifdef _WIN32
        if (fake_parent_window_)
            ::DestroyWindow(fake_parent_window_);
#endif
    }

    void MainWindow::show()
    {
#ifdef _WIN32
        ShowWindow(parentNativeWindowHandle_, SW_SHOW);
#endif
        QMainWindow::show();
    }

    void MainWindow::onOpenChat(const std::string& _contact)
    {
        raise();
        activate();

        if (!_contact.empty())
        {
            Logic::getContactListModel()->setCurrent(QString::fromStdString(_contact), -1, true);
        }
    }

    void MainWindow::activateWithReason(ActivateReason _reason)
    {
        activate();

        if (_reason == ActivateReason::ByNotificationClick)
            statistic::getGuiMetrics().eventForegroundByNotification();
    }

    void MainWindow::activate()
    {
        activateMainWindow();
    }

    void MainWindow::activateMainWindow(bool _activate)
    {
#ifndef _WIN32
        Q_UNUSED(_activate);
#endif
        if constexpr (platform::is_apple())
        {
            // workaround - remove blinking after hide/minimize the window
            setVisible(false);
            activateWindow();
            updateMainMenu();
        }
        setVisible(true);
#ifdef _WIN32
        if (!isActive())
        {
            activateWindow();
            isMaximized() ? showMaximized() : showNormal(_activate);
            ShowWindow(parentNativeWindowHandle_, SW_SHOW);
        }
        if (Shadow_)
            SetWindowPos((HWND)Shadow_->winId(), (HWND)winId(), 0, 0, 0, 0, SWP_NOACTIVATE | SWP_NOOWNERZORDER | SWP_NOMOVE | SWP_NOSIZE | SWP_SHOWWINDOW);
#else
        activateWindow();
#endif //_WIN32
#ifdef __APPLE__
        getMacSupport()->activateWindow(winId());
        updateMainMenu();
#endif //__APPLE__
#ifdef __linux__
        isMaximized() ? showMaximized() : showNormal();
#endif //__linux__

        trayIcon_->hideAlerts();
        showHideGallery();

        auto currentHistoryPage = Utils::InterConnector::instance().getHistoryPage(Logic::getContactListModel()->selectedContact());
        if (currentHistoryPage)
            currentHistoryPage->updateItems();
        updateMainMenu();
    }

    void MainWindow::openGallery(const QString &_aimId, const QString &_link, int64_t _msgId, DialogPlayer* _attachedPlayer)
    {
        if (gallery_)
            return;

        statistic::getGuiMetrics().eventOpenGallery();

        gallery_ = new Previewer::GalleryWidget(this); // will delete itself on close
        connect(gallery_, &Previewer::GalleryWidget::finished, this, &MainWindow::exit);
        gallery_->openGallery(_aimId, _link, _msgId, _attachedPlayer);
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
        emit activateWithReason(_reason);
    }

    bool MainWindow::isActive() const
    {
#ifdef _WIN32
        // Check the absence of statuses isMinimized or hided, because native window can be focused in this state.
        return isActiveWindow() && !isMinimized() && IsWindowVisible(getParentWindow()) > 0;
#else
        return isActiveWindow();
#endif //_WIN32
    }

    bool MainWindow::isUIActive() const
    {
        return uiActive_;
    }

    bool MainWindow::isMainPage() const
    {
        if (mainPage_ == nullptr)
            return false;

        return mainPage_->isContactDialog();
    }

    int MainWindow::getScreen() const
    {
#ifdef _WIN32
        RECT winRect;
        GetWindowRect(parentNativeWindowHandle_, &winRect);
        return QApplication::desktop()->screenNumber(QPoint(winRect.left + (winRect.right - winRect.left) / 2, winRect.top + (winRect.bottom - winRect.top) / 2));
#else
        return QApplication::desktop()->screenNumber(this);
#endif
    }

    QRect MainWindow::screenGeometry() const
    {
        return QApplication::desktop()->screenGeometry(getScreen());
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

        emit Utils::InterConnector::instance().closeAnyPopupMenu();
        emit Utils::InterConnector::instance().closeAnyPopupWindow(_info);
        emit Utils::InterConnector::instance().closeAnySemitransparentWindow(_info);
    }

    HistoryControlPage* MainWindow::getHistoryPage(const QString& _aimId) const
    {
        if (mainPage_)
            return mainPage_->getHistoryPage(_aimId);
        else
            return nullptr;
    }

    MainPage* MainWindow::getMainPage() const
    {
        return mainPage_;
    }

    QLabel* MainWindow::getWindowLogo() const
    {
        return logo_;
    }

    void MainWindow::showSidebar(const QString& _aimId)
    {
        if (mainPage_)
            mainPage_->showSidebar(_aimId);
    }

    void MainWindow::showMembersInSidebar(const QString& _aimId)
    {
        if (mainPage_)
            mainPage_->showMembersInSidebar(_aimId);
    }

    void MainWindow::setSidebarVisible(const Utils::SidebarVisibilityParams& _params)
    {
        if (mainPage_)
            mainPage_->setSidebarVisible(_params);
    }

    bool MainWindow::isSidebarVisible() const
    {
        return mainPage_ && mainPage_->isSidebarVisible();
    }

    void MainWindow::restoreSidebar()
    {
        if (mainPage_)
            mainPage_->restoreSidebar();
    }

    void MainWindow::notifyWindowActive(const bool _active)
    {
        if (isMainPage() && mainPage_)
        {
            mainPage_->notifyApplicationWindowActive(_active);
        }

        windowActive_ = _active;

        needChangeState();

        updateMainMenu();
        statistic::getGuiMetrics().eventMainWindowActive();
    }

    void MainWindow::saveWindowSize(QSize size)
    {
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

            rc.moveTo(platform::is_apple() ? pos() : geometry().topLeft());

            get_gui_settings()->set_value(settings_main_window_rect, rc);
            get_gui_settings()->set_value(settings_window_maximized, false);
        }
    }

    bool MainWindow::nativeEventFilter(const QByteArray& _data, void* _message, long* _result)
    {
#ifdef _WIN32
        if (!_result)
            return false;

        MSG* msg = (MSG*)(_message);

        if (msg->message == WM_NCHITTEST)
        {
            if (msg->hwnd != (HANDLE)winId())
                return false;

            RECT winRect;
            GetWindowRect(msg->hwnd, &winRect);

            const int borderWidth = getWindowBorderWidth();
            const int x = GET_X_LPARAM(msg->lParam) - winRect.left;
            const int y = GET_Y_LPARAM(msg->lParam) - winRect.top;

            auto internalArea = QRect(QPoint(borderWidth, borderWidth),
                                      QPoint(winRect.right - winRect.left - borderWidth, winRect.bottom - winRect.top - borderWidth));

            if (x >= borderWidth && x <= winRect.right - winRect.left - borderWidth
                && y >= borderWidth && y <= getTitleHeight())
            {
                // This allows for buttons in the toolbar and logo to keep the mouse interaction
                if (QApplication::widgetAt(QCursor::pos()) != titleWidget_ || getWindowLogo()->rect().contains(QPoint(x, y)))
                    return false;

                *_result = HTTRANSPARENT;
                return true;
            }
            else if (!internalArea.contains(x, y))
            {
                *_result = HTTRANSPARENT;
                return true;
            }
        }
        else if ((msg->message == WM_SYSCOMMAND && msg->wParam == SC_RESTORE && msg->hwnd == (HWND)winId()) || (msg->message == WM_SHOWWINDOW && msg->hwnd == (HWND)winId() && msg->wParam == TRUE))
        {
            setVisible(true);
            if (Shadow_)
                SetWindowPos((HWND)Shadow_->winId(), (HWND)winId(), 0, 0, 0, 0, SWP_NOACTIVATE | SWP_NOOWNERZORDER | SWP_NOMOVE | SWP_NOSIZE | SWP_SHOWWINDOW);
            trayIcon_->hideAlerts();
            if (!SkipRead_ && isMainPage())
            {
                Logic::getRecentsModel()->sendLastRead();
                Logic::getUnknownsModel()->sendLastRead();
            }

            notifyWindowActive(isActive());

            if (!TaskBarIconHidden_)
                SkipRead_ = false;
            TaskBarIconHidden_ = false;
#if 0
            if (msg->lParam == TITLE_CONTEXT_MENU_CMD)
                maximize();
#endif
        }
        else if (msg->message == WM_SYSCOMMAND && msg->wParam == SC_CLOSE)
        {
            if (hasMemoryDockWidget() && msg->hwnd == reinterpret_cast<HWND>(memoryDockWidget_->winId()))
            {
                memoryDockWidget_->close();
                return true;
            }

            hideWindow();
            return true;
        }
#if 0
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
#endif
        else if (msg->message == WM_ACTIVATE)
        {
#if 0
            if (!Shadow_)
            {
                return false;
            }

            const auto isInactivate = (msg->wParam == WA_INACTIVE);
            const auto isShadowWindow = (msg->hwnd == (HWND)Shadow_->winId());
            if (isShadowWindow && !isInactivate)
#endif
            if (!ignoreActivateEvent_ && msg->wParam != WA_INACTIVE && msg->hwnd == (HANDLE)winId())
            {
                activateMainWindow(false);
                return false;
            }
        }
        else if (msg->message == WM_DISPLAYCHANGE)
        {
            checkPosition();
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
#if 0
        else if (msg->message == WM_NCLBUTTONDOWN && msg->hwnd == (HWND)winId())
        {
            BYTE bFlag = 0;
            switch (msg->wParam)
            {
            case HTTOP:
                bFlag = WMSZ_TOP;
                break;

            case HTTOPLEFT:
                bFlag = WMSZ_TOPLEFT;
                break;

            case HTTOPRIGHT:
                bFlag = WMSZ_TOPRIGHT;
                break;

            case HTLEFT:
                bFlag = WMSZ_LEFT;
                break;

            case HTRIGHT:
                bFlag = WMSZ_RIGHT;
                break;

            case HTBOTTOM:
                bFlag = WMSZ_BOTTOM;
                break;

            case HTBOTTOMLEFT:
                bFlag = WMSZ_BOTTOMLEFT;
                break;

            case HTBOTTOMRIGHT:
                bFlag = WMSZ_BOTTOMRIGHT;
                break;
            }

            if (bFlag)
            {
                DefWindowProc(msg->hwnd, WM_SYSCOMMAND, (SC_SIZE | bFlag), msg->lParam);
                return true;
            }
        }
        else if (msg->message == WM_SETCURSOR && msg->hwnd == (HWND)winId())
        {
            LPCTSTR pszCur = NULL;
            switch (LOWORD(msg->lParam))
            {
            case HTTOP:
            case HTBOTTOM:
                pszCur = IDC_SIZENS;
                break;

            case HTTOPLEFT:
            case HTBOTTOMRIGHT:
                pszCur = IDC_SIZENWSE;
                break;

            case HTLEFT:
            case HTRIGHT:
                pszCur = IDC_SIZEWE;
                break;

            case HTTOPRIGHT:
            case HTBOTTOMLEFT:
                pszCur = IDC_SIZENESW;
                break;
            }

            if (pszCur)
            {
                SetCursor(LoadCursor(NULL, pszCur));
                return true;
            }
        }
#endif
        else if (msg->message == WM_SIZE)
        {
            isMaximized_ = isNativeWindowShowCmd(getParentWindow(), SW_MAXIMIZE);
            updateState();
        }
        else if (msg->message == WM_SYSKEYDOWN && msg->wParam == VK_SPACE)
        {
            // when Alt+Space is pressed tell the native window to show the context menu
            SendMessage(parentNativeWindowHandle_, WM_SYSCOMMAND, SC_KEYMENU, VK_SPACE);
            return true;
        }
#else

#ifdef __APPLE__
        return MacSupport::nativeEventFilter(_data, _message, _result);
#endif

#endif //_WIN32
        return false;
    }

    bool MainWindow::eventFilter(QObject* obj, QEvent* event)
    {
        switch (event->type())
        {
            case QEvent::KeyPress:
            {
                userActivity();

                if (mainPage_ && stackedWidget_->currentWidget() == mainPage_ && !mainPage_->isSemiWindowVisible())
                {
                    const auto keyEvent = static_cast<QKeyEvent*>(event);
                    if ((keyEvent->key() == Qt::Key_Tab || keyEvent->key() == Qt::Key_Backtab) && !(keyEvent->modifiers() & Qt::ControlModifier))
                    {
                        const auto cur = static_cast<QWidget*>(qApp->focusObject());
                        if (cur == this)
                        {
                            const bool tabForward = keyEvent->key() == Qt::Key_Tab;
                            const auto setToInput =
                                tabForward &&
                                !Logic::getContactListModel()->selectedContact().isEmpty() &&
                                mainPage_->getContactDialog()->canSetFocusOnInput();

                            if (setToInput)
                                mainPage_->getContactDialog()->setFocusOnInputFirstFocusable();
                            else
                                mainPage_->setSearchTabFocus();

                            return true;
                        }
                    }
                }

                break;
            }
            case QEvent::MouseButtonPress:
                emit mousePressed(QPrivateSignal());
                [[fallthrough]];
            case QEvent::TouchBegin:
            case QEvent::Wheel:
            case QEvent::MouseMove:
            case QEvent::ApplicationActivate:
            {
                userActivity();
                break;
            }
            case QEvent::MouseButtonRelease:
            {
                emit mouseReleased(QPrivateSignal());
                break;
            }

            default:
                break;
        }

        return QMainWindow::eventFilter(obj, event);
    }

    void MainWindow::customEvent(QEvent* _event)
    {
#ifdef _WIN32
        if (_event->type() == NativeWindowMoved)
        {
            if (!isMaximized())
            {
                auto wndPos = static_cast<NativeWindowMovedEvent*>(_event);
                auto rc = get_gui_settings()->get_value(settings_main_window_rect, QRect());
                rc.moveTo(wndPos->getPoint());
                get_gui_settings()->set_value(settings_main_window_rect, rc);
            }
        }
#endif
    }

    void MainWindow::enterEvent(QEvent* _event)
    {
        QMainWindow::enterEvent(_event);

        if (platform::is_apple() &&  qApp->activeWindow() != this && (mainPage_ == nullptr || !mainPage_->isVideoWindowActive()))
            qApp->setActiveWindow(this);
        updateMainMenu();
        emit Utils::InterConnector::instance().historyControlPageFocusIn(Logic::getContactListModel()->selectedContact());
    }

    void MainWindow::resizeEvent(QResizeEvent* _event)
    {
        emit Utils::InterConnector::instance().mainWindowResized();

        emit Utils::InterConnector::instance().closeAnyPopupMenu();
        emit Utils::InterConnector::instance().closeAnyPopupWindow({ Utils::CloseWindowInfo::Initiator::MainWindow, Utils::CloseWindowInfo::Reason::MW_Resizing });

        saveWindowSize(_event->size());

        updateMainMenu();
    }

    void MainWindow::moveEvent(QMoveEvent* _event)
    {
        if (!isMaximized())
        {
            auto rc = get_gui_settings()->get_value(settings_main_window_rect, QRect());
            rc.moveTo(platform::is_apple() ? pos() : geometry().topLeft());
            get_gui_settings()->set_value(settings_main_window_rect, rc);
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
            emit Utils::InterConnector::instance().activationChanged(active);
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

            saveWindowSize(geometry().size());
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

        emit windowClose(QPrivateSignal());
    }

    void MainWindow::hideEvent(QHideEvent* _event)
    {
        emit windowHide(QPrivateSignal());
        QMainWindow::hideEvent(_event);
    }

    void MainWindow::keyPressEvent(QKeyEvent* _event)
    {
        if (qobject_cast<MainPage*>(stackedWidget_->currentWidget()))
        {
            const auto aimId = Logic::getContactListModel()->selectedContact();

            if (mainPage_ && !mainPage_->isSemiWindowVisible())
            {
                const auto searchCode = get_gui_settings()->get_value<int>(settings_shortcuts_search_action, static_cast<int>(ShortcutsSearchAction::Default));
                const auto searchAct = static_cast<ShortcutsSearchAction>(searchCode);

                if (_event->matches(QKeySequence::Find))
                {
                    if (searchAct == ShortcutsSearchAction::GlobalSearch || aimId.isEmpty())
                        emit Utils::InterConnector::instance().setSearchFocus();
                    else if (!aimId.isEmpty())
                        emit Utils::InterConnector::instance().startSearchInDialog(aimId);
                }
                else if (_event->modifiers() == (Qt::ControlModifier | Qt::ShiftModifier) && _event->key() == Qt::Key_F)
                {
                    if (searchAct == ShortcutsSearchAction::GlobalSearch && !aimId.isEmpty())
                        emit Utils::InterConnector::instance().startSearchInDialog(aimId);
                    else
                        emit Utils::InterConnector::instance().setSearchFocus();
                }
            }

            if (mainPage_ && _event->modifiers() == Qt::ControlModifier && _event->key() == Qt::Key_W)
                activateShortcutsClose();

            if (_event->key() == Qt::Key_Escape)
            {
                const auto histPage = Utils::InterConnector::instance().getHistoryPage(aimId);
                const auto contDialog = Utils::InterConnector::instance().getContactDialog();

                if (mainPage_ && mainPage_->isNavigatingInRecents())
                {
                    mainPage_->stopNavigatingInRecents();

                    if (aimId.isEmpty())
                        setFocus();
                    else
                        setFocusOnInput();
                }
                else if (mainPage_ && mainPage_->isSuggestVisible())
                {
                    mainPage_->hideSuggest();
                }
                else if (contDialog && contDialog->isShowingSmileMenu())
                {
                    contDialog->hideSmilesMenu();
                }
                else if (histPage && histPage->getMentionCompleter() && histPage->getMentionCompleter()->completerVisible())
                {
                    emit Utils::InterConnector::instance().hideMentionCompleter();
                }
                else if (AttachFilePopup::isOpen())
                {
                    AttachFilePopup::hidePopup();
                }
                else if (contDialog && contDialog->isRecordingPtt())
                {
                    contDialog->closePttRecording();
                }
                else if (contDialog && contDialog->isEditingMessage())
                {
                    emit Utils::InterConnector::instance().cancelEditing();
                }
                else if (contDialog && contDialog->isReplyingToMessage())
                {
                    contDialog->dropReply();
                }
                else if (mainPage_ && mainPage_->isSidebarVisible())
                {
                    mainPage_->setSidebarVisible(false);
                }
                else if (!_event->isAutoRepeat())
                {
                    auto cur = static_cast<QWidget*>(qApp->focusObject());
                    if (cur != this && (qobject_cast<Ui::ClickableWidget*>(cur) || qobject_cast<QAbstractButton*>(cur)))
                    {
                        cur->clearFocus();
                        setFocusOnInput();
                    }
                    else if (mainPage_)
                    {
                        if (aimId.isEmpty())
                            mainPage_->setSearchFocus();
                        else
                            mainPage_->closeAndHighlightDialog();
                    }
                }
            }
            else if (_event->key() == Qt::Key_Enter || _event->key() == Qt::Key_Return)
            {
                const auto contDialog = Utils::InterConnector::instance().getContactDialog();
                if (contDialog && contDialog->isRecordingPtt())
                    contDialog->sendPttRecord();
            }
            else if (_event->key() == Qt::Key_Space)
            {
                const auto contDialog = Utils::InterConnector::instance().getContactDialog();
                if (contDialog && contDialog->isRecordingPtt())
                    contDialog->stopPttRecord();
            }
            else if (_event->modifiers() == Qt::ControlModifier && _event->key() == Qt::Key_R)
            {
                const auto contDialog = Utils::InterConnector::instance().getContactDialog();
                if (contDialog && !contDialog->isRecordingPtt())
                    contDialog->startPttRecordingLock();
            }

            if (mainPage_ && !mainPage_->isSearchActive() && qApp->focusObject() == this)
            {
                if (_event->key() == Qt::Key_Down)
                    emit downKeyPressed(QPrivateSignal());
                else if (_event->key() == Qt::Key_Up)
                    emit upKeyPressed(QPrivateSignal());
                else if (_event->key() == Qt::Key_Enter || _event->key() == Qt::Key_Return)
                    emit enterKeyPressed(QPrivateSignal());
            }

            if (mainPage_ && !mainPage_->isSemiWindowVisible())
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
                    mainPage_->nextChat();
                else if (prevChatSequence)
                    mainPage_->prevChat();
            }

            if constexpr (platform::is_windows())
            {
                if (_event->modifiers() == Qt::ControlModifier && _event->key() == Qt::Key_F4)
                    Logic::getRecentsModel()->hideChat(aimId);
            }

            if (_event->modifiers() == Qt::ControlModifier && _event->key() == Qt::Key_L && LocalPIN::instance()->enabled())
                LocalPIN::instance()->lock();
        }

        if constexpr (platform::is_linux() || platform::is_windows())
            if (_event->modifiers() == Qt::ControlModifier && _event->key() == Qt::Key_Q)
                exit();

        QMainWindow::keyPressEvent(_event);
    }

    void MainWindow::mouseReleaseEvent(QMouseEvent* _event)
    {
        //emit mouseReleased(QPrivateSignal());
        QMainWindow::mouseReleaseEvent(_event);
    }

    void MainWindow::initSizes()
    {
        auto mainRect = Ui::get_gui_settings()->get_value(settings_main_window_rect, getDefaultWindowSize());

        const int screenCount = QDesktopWidget().screenCount();

        bool intersects = false;

        for (int i = 0; i < screenCount; ++i)
        {
            QRect screenGeometry = QDesktopWidget().screenGeometry(i);

            QRect intersectedRect = screenGeometry.intersected(mainRect);

            if (intersectedRect.width() > Utils::scale_value(50) && intersectedRect.height() > Utils::scale_value(50))
            {
                intersects = true;
                break;
            }
        }

        if (!intersects)
        {
            mainRect = getDefaultWindowSize();
        }

        resize(mainRect.width(), mainRect.height());

        setMinimumHeight((qApp->desktop()->screenGeometry().height() >= Utils::scale_value(800)) ? getMinWindowHeight() : qApp->desktop()->screenGeometry().height() * 0.7);
        setMinimumWidth(getMinWindowWidth());

#ifdef _WIN32
        parentNativeWindow_->setMinimumSize(minimumWidth(), minimumHeight());
#endif
        if (mainRect.left() == 0 && mainRect.top() == 0)
        {
            QRect desktopRect = QDesktopWidget().availableGeometry(this);

            QPoint center = desktopRect.center();

            move(center.x() - width() / 2, center.y() - height() / 2);

            get_gui_settings()->set_value(settings_main_window_rect, geometry());

#ifdef _WIN32
            parentNativeWindow_->setGeometry(geometry());
#endif
        }
        else
        {
            move(mainRect.left(), mainRect.top());
#ifdef _WIN32
            parentNativeWindow_->setGeometry(mainRect);
#endif
        }
    }

    void MainWindow::initSettings()
    {
        initSizes();

        bool isMaximized = get_gui_settings()->get_value<bool>(settings_window_maximized, false);

        isMaximized ? showMaximized() : show();

        if constexpr (platform::is_windows())
        {
            updateMaximizeButton(isMaximized);
            maximizeButton_->setStyle(QApplication::style());
        }

#ifdef _WIN32
        if (isMaximized && Shadow_)
            SetWindowPos((HWND)Shadow_->winId(), (HWND)winId(), 0, 0, 0, 0, SWP_NOACTIVATE | SWP_NOOWNERZORDER | SWP_NOMOVE | SWP_NOSIZE | SWP_HIDEWINDOW);

        WTSRegisterSessionNotification( (HWND)winId(), NOTIFY_FOR_THIS_SESSION);
#endif //_WIN32
    }

    void MainWindow::initStats()
    {
        statistics::getPeriodicGuiMetrics().onPoll();
    }

    QWidget* MainWindow::getWidget()
    {
        return mainWidget_;
    }

    void MainWindow::resize(int w, int h)
    {
        const auto geometry = screenGeometry();

        if (geometry.width() < w)
            w = geometry.width();

        if (geometry.height() < h)
            h = geometry.height();

        return QMainWindow::resize(w, h);
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

#ifdef _WIN32
            setVisible(true);
            SendMessage(parentNativeWindowHandle_, WM_SYSCOMMAND, SC_MAXIMIZE, 0);
#else
            QMainWindow::showNormal();

            const auto screen = getScreen();
            const auto screenGeometry = QApplication::desktop()->availableGeometry(screen);
            setGeometry(screenGeometry);
#endif
        }

        updateState();
    }

    void MainWindow::showNormal(bool _activate)
    {
#ifndef _WIN32
        Q_UNUSED(_activate);
#endif
        isMaximized_ = false;
#ifdef _WIN32
        const auto hWnd = getParentWindow();
        if (IsWindowVisible(hWnd) == 0 || !isNativeWindowShowCmd(hWnd, SW_NORMAL))
            SendMessage(hWnd, WM_SYSCOMMAND, SC_RESTORE, 0);
        else if (_activate)
            SendMessage(hWnd, WM_ACTIVATE, WA_CLICKACTIVE, 0);
#else
        QMainWindow::showNormal();

        auto mainRect = Ui::get_gui_settings()->get_value(settings_main_window_rect, getDefaultWindowSize());
        setGeometry(mainRect);
#endif
        updateState();
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
        for (int i = 0; i < QDesktopWidget().screenCount(); ++i)
            desktopRect |= QDesktopWidget().availableGeometry(i);

        QRect main = rect();
        QPoint mainP = mapToGlobal(main.topLeft());
        main.moveTo(mainP);
        int y = main.y();
        if (y < desktopRect.y())
            maximize();
    }

    void MainWindow::userActivity()
    {
        if (!isMinimized())
            UserActivity::raiseUserActivity();
    }

    void MainWindow::updateMaximizeButton(bool _isMaximized)
    {
        if (_isMaximized)
        {
            maximizeButton_->setDefaultImage(qsl(":/titlebar/restore_button"));
            //maximizeButton_->setHoverImage(qsl(":/titlebar/restore_button_hover"));
            //maximizeButton_->setPressedImage(qsl(":/titlebar/restore_button_active"));
        }
        else
        {
            maximizeButton_->setDefaultImage(qsl(":/titlebar/expand_button"));
            //maximizeButton_->setHoverImage(qsl(":/titlebar/expand_button_hover"));
            //maximizeButton_->setPressedImage(qsl(":/titlebar/expand_button_active"));
        }
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

            if (mainPage_ && mainPage_->isVideoWindowActive())
            {
                mainPage_->closeVideoWindow();
                return;
            }
        }

        switch (closeAct)
        {
        case ShortcutsCloseAction::RollUpWindowAndChat:
            emit Utils::InterConnector::instance().closeAnyPopupMenu();
            emit Utils::InterConnector::instance().closeAnyPopupWindow(Utils::CloseWindowInfo());
            if (mainPage_)
            {
                Logic::getContactListModel()->setCurrent(QString(), -1);
                mainPage_->update();
                setFocus();
            }
            [[fallthrough]];
        case ShortcutsCloseAction::RollUpWindow:
            hideWindow();
            break;
        case ShortcutsCloseAction::CloseChat:
            emit Utils::InterConnector::instance().closeAnyPopupMenu();
            emit Utils::InterConnector::instance().closeAnyPopupWindow(Utils::CloseWindowInfo());
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
    }

    void MainWindow::needChangeState()
    {
        const auto lastUserActivity = UserActivity::getLastActivityMs();

        const bool prevState = uiActive_;

        if ((windowActive_ && lastUserActivity < getOfflineTimeout() && !sleeping_ && !LocalPIN::instance()->locked())
            || Ui::GetDispatcher()->getVoipController().isConnectionEstablished())
        {
            uiActive_ = true;
        }
        else
        {
            uiActive_ = false;
        }

        static bool isFirst = true;

        const bool stateChanged = (isFirst || uiActive_ != prevState);

        if (uiActive_)
        {

            GetDispatcher()->postUiActivity();
        }

        if (stateChanged)
        {
            if (uiActive_)
            {
                trayIcon_->hideAlerts();

                if (!SkipRead_ && isMainPage())
                {
                    Logic::getRecentsModel()->sendLastRead();
                    Logic::getUnknownsModel()->sendLastRead();
                }

                SkipRead_ = false;
            }

            if (mainPage_)
                mainPage_->notifyUIActive(uiActive_);
        }


        isFirst = false;
    }

    void MainWindow::resetMainPage()
    {
        MainPage::reset();
        mainPage_ = nullptr;
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
            if (mainPage_ && mainPage_->isVideoWindowActive() && !mainPage_->isVideoWindowMinimized())
            {
                mainPage_->minimizeVideoWindow();
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
            SendMessage(parentNativeWindowHandle_, WM_SYSCOMMAND, SC_MINIMIZE, 0);
#if 0
            showMinimized();
            if (Shadow_)
                SetWindowPos((HWND)Shadow_->winId(), (HWND)winId(), 0, 0, 0, 0, SWP_NOACTIVATE | SWP_NOOWNERZORDER | SWP_NOMOVE | SWP_NOSIZE | SWP_HIDEWINDOW);
#endif
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

        if (mainPage_ && mainPage_->isVideoWindowOn() && !mainPage_->isVideoWindowMinimized())
            mainPage_->showVideoWindow();
        updateMainMenu();
    }

    void MainWindow::moveRequest(QPoint _point)
    {
        if (isMaximized())
        {
            maximize();
        }
        else
        {
            if constexpr (platform::is_windows())
            {
                if (hasMemoryDockWidget() && !memoryDockWidget_->isFloating())
                    _point.setX(_point.x() - memoryDockWidget_->width());
            }

            move(_point);
        }
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
    }

    void MainWindow::clear_global_objects()
    {
        // delete main page
        if (mainPage_)
        {
            stackedWidget_->removeWidget(mainPage_);
            resetMainPage();
        }

        Logic::ResetMentionModel();
        Logic::ResetContactListModel();
        Logic::ResetRecentsModel();
        Logic::ResetUnknownsModel();
        Logic::resetLastSearchPatterns();
        Ui::ResetMyInfo();

        Ui::GetDispatcher()->getVoipController().resetMaskManager();

        Ui::Stickers::resetCache();

        trayIcon_->resetState();
    }

    void MainWindow::showMigrateAccountPage(const QString& _accountId)
    {
#ifdef __APPLE__
        migrationManager_->init(_accountId);

        if (migrationManager_->getProfiles().size() == 1)
        {
            MacProfile profile = migrationManager_->getProfiles()[0];

            migrationManager_->loginProfile(profile, MacProfile());

            showMainPage();
        }
        else
        {
            bool migrated = false;
            if (migrationManager_->getProfiles().size() == 2)
            {
                MacProfile profile1 = migrationManager_->getProfiles()[0];
                MacProfile profile2 = migrationManager_->getProfiles()[1];

                if (profile1.type() != profile2.type())
                {
                    MacProfile & main = profile1.type() == MacProfileType::Agent?profile1:profile2;
                    MacProfile & slave = profile1.type() == MacProfileType::ICQ?profile1:profile2;

                    migrationManager_->loginProfile(main, slave);

                    migrated = true;
                }

                showMainPage();
            }

            if (!migrated)
            {
                if (!accounts_page_)
                {
                    accounts_page_ = new AccountsPage(this, migrationManager_);
                    connect(accounts_page_, &AccountsPage::account_selected, this, &MainWindow::showMainPage, Qt::QueuedConnection); // strict order
                    accounts_page_->summon();
                    Testing::setAccessibleName(accounts_page_, qsl("AS accounts_page_"));
                    stackedWidget_->addWidget(accounts_page_);
                }

                stackedWidget_->setCurrentWidget(accounts_page_);

                clear_global_objects();
            }
            else
            {
                showMainPage();
            }
        }
#endif
    }

    void MainWindow::showLoginPage(const bool _is_auth_error)
    {
        closePopups({ Utils::CloseWindowInfo::Initiator::MainWindow, Utils::CloseWindowInfo::Reason::ShowLoginPage });

        auto showLoginPageInternal = [this, _is_auth_error]()
        {
#ifdef __APPLE__
            getMacSupport()->createMenuBar(true);

            getMacSupport()->forceEnglishInputSource();

            if (!get_gui_settings()->get_value<bool>(settings_mac_accounts_migrated, false))
            {
                if (!MacMigrationManager::canMigrateAccount().isEmpty())
                {
                    resetMainPage();
                    upgradeStuff();
                    return;
                }
                else
                {
                    Ui::get_gui_settings()->set_value<bool>(settings_mac_accounts_migrated, true);
                }
            }
#endif

            if (!loginPage_)
            {
                loginPage_ = new LoginPage(this, true /* is_login */);
                Testing::setAccessibleName(loginPage_, qsl("AS loginPage_"));
                stackedWidget_->addWidget(loginPage_);

                connect(loginPage_, &LoginPage::loggedIn, this, &MainWindow::showMainPage, Qt::QueuedConnection);
                connect(loginPage_, &LoginPage::loggedIn, this, [this]() {
                    if (callStatsMgr_)
                        callStatsMgr_->reconnectToMyInfo();
                });
            }

            stackedWidget_->setCurrentWidget(loginPage_);
            GetDispatcher()->post_stats_to_core(core::stats::stats_event_names::reg_page_phone);

            if (_is_auth_error)
            {
                emit Utils::InterConnector::instance().authError(core::le_wrong_login);
            }

            clear_global_objects();
        };

        if (GeneralDialog::isActive())
            QTimer::singleShot(0, this, showLoginPageInternal);
        else
            showLoginPageInternal();
    }

    void MainWindow::showMainPage()
    {
        if (!mainPage_)
        {
            mainPage_ = MainPage::instance(this);
            Testing::setAccessibleName(mainPage_, qsl("AS mainPage_"));
            stackedWidget_->addWidget(mainPage_);
        }
#ifdef __APPLE__
        getMacSupport()->createMenuBar(false);
#endif
        stackedWidget_->setCurrentWidget(mainPage_);
    }

    void MainWindow::upgradeStuff()
    {
#ifdef __APPLE__
        const auto profiles = MacMigrationManager::canMigrateAccount();
        showMigrateAccountPage(profiles);

        if (!profiles.isEmpty())
        {
            return;
        }
#endif

        if (!get_gui_settings()->get_value(settings_keep_logged_in, true))// || !get_gui_settings()->contains_value(settings_keep_logged_in))
        {
            showLoginPage(false);
        }
        else
        {
            showMainPage();
        }
    }

    void MainWindow::checkForUpdates()
    {
#ifdef __APPLE__
        getMacSupport()->runMacUpdater();
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
        if(auto focused = QApplication::focusWidget())
        {
            bool handled = false;

            if constexpr (platform::is_apple())
            {
                if (auto area = qobject_cast<Ui::MessagesScrollArea*>(focused))
                {
#ifdef __APPLE__
                    if (QString text = area->getSelectedText(); !text.isEmpty())
                        MacSupport::replacePasteboard(text);
#endif
                    handled = true;
                }
            }

            if (!handled)
            {
                QApplication::postEvent(focused, new QKeyEvent(QEvent::KeyPress, Qt::Key_C, Qt::ControlModifier));
                QApplication::postEvent(focused, new QKeyEvent(QEvent::KeyRelease, Qt::Key_C, Qt::ControlModifier));
            }
        }
    }

    void MainWindow::cut()
    {
        if(auto focused = QApplication::focusWidget(); focused && focused->inputMethodHints() != Qt::ImhNone)
        {
            const auto contDialog = Utils::InterConnector::instance().getContactDialog();
            if (contDialog && contDialog->isRecordingPtt())
                return;

            QApplication::postEvent(focused, new QKeyEvent(QEvent::KeyPress, Qt::Key_X, Qt::ControlModifier));
            QApplication::postEvent(focused, new QKeyEvent(QEvent::KeyRelease, Qt::Key_X, Qt::ControlModifier));
        }
    }

    void MainWindow::paste()
    {
        if(auto focused = QApplication::focusWidget(); focused && focused->inputMethodHints() != Qt::ImhNone)
        {
            const auto contDialog = Utils::InterConnector::instance().getContactDialog();
            if (contDialog && contDialog->isRecordingPtt())
                return;

            QApplication::postEvent(focused, new QKeyEvent(QEvent::KeyPress, Qt::Key_V, Qt::ControlModifier));
            QApplication::postEvent(focused, new QKeyEvent(QEvent::KeyRelease, Qt::Key_V, Qt::ControlModifier));
        }
    }


    void MainWindow::undo()
    {
        if(auto focused = QApplication::focusWidget(); focused && focused->inputMethodHints() != Qt::ImhNone)
        {
            const auto contDialog = Utils::InterConnector::instance().getContactDialog();
            if (contDialog && contDialog->isRecordingPtt())
                return;

            QApplication::postEvent(focused, new QKeyEvent(QEvent::KeyPress, Qt::Key_Z, Qt::ControlModifier));
            QApplication::postEvent(focused, new QKeyEvent(QEvent::KeyRelease, Qt::Key_Z, Qt::ControlModifier));
        }
    }


    void MainWindow::redo()
    {
        if(auto focused = QApplication::focusWidget(); focused && focused->inputMethodHints() != Qt::ImhNone)
        {
            const auto contDialog = Utils::InterConnector::instance().getContactDialog();
            if (contDialog && contDialog->isRecordingPtt())
                return;

            QApplication::postEvent(focused, new QKeyEvent(QEvent::KeyPress, Qt::Key_Z, Qt::ControlModifier|Qt::ShiftModifier));
            QApplication::postEvent(focused, new QKeyEvent(QEvent::KeyRelease, Qt::Key_Z, Qt::ControlModifier|Qt::ShiftModifier));
        }
    }

    void MainWindow::activateNextUnread()
    {
        activate();
        closePopups({ Utils::CloseWindowInfo::Initiator::Unknown, Utils::CloseWindowInfo::Reason::Keep_Sidebar });
        mainPage_->recentsTabActivate(true);
    }

    void MainWindow::activateNextChat()
    {
        activate();
        mainPage_->recentsTabActivate(false);
        mainPage_->selectRecentChat(Logic::getRecentsModel()->nextAimId(Logic::getContactListModel()->selectedContact()));
    }

    void MainWindow::activatePrevChat()
    {
        activate();
        mainPage_->recentsTabActivate(false);
        mainPage_->selectRecentChat(Logic::getRecentsModel()->prevAimId(Logic::getContactListModel()->selectedContact()));
    }

    void MainWindow::activateAbout()
    {
        activate();
        mainPage_->settingsTabActivate(Utils::CommonSettingsType::CommonSettingsType_About);
    }

    void MainWindow::activateProfile()
    {
        activate();
        mainPage_->settingsTabActivate(Utils::CommonSettingsType::CommonSettingsType_Profile);
    }

    void MainWindow::activateSettings()
    {
        activate();
        mainPage_->settingsClicked();
    }


    void MainWindow::closeCurrent()
    {
        activate();

        const auto selectedContact = Logic::getContactListModel()->selectedContact();
        if (selectedContact.isEmpty())
        {
            hideWindow();
        }
        else
        {
            Logic::getRecentsModel()->hideChat(selectedContact);
        }
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

        hideParentWindow();
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
        TaskBarIconHidden_ = true;

#if defined __APPLE__
        MacSupport::closeWindow(winId());
        updateMainMenu();
#elif defined (_WIN32)
        hideParentWindow();
#else
        hide();
#endif

#ifdef _WIN32
        if (Shadow_)
            SetWindowPos((HWND)Shadow_->winId(), (HWND)winId(), 0, 0, 0, 0, SWP_NOACTIVATE | SWP_NOOWNERZORDER | SWP_NOMOVE | SWP_NOSIZE | SWP_HIDEWINDOW);
#endif //_WIN32
    }

    void MainWindow::pasteEmoji()
    {
        if (!Logic::getContactListModel()->selectedContact().isEmpty())
        {
#ifdef __APPLE__
            MacSupport::showEmojiPanel();
#else
            getMainPage()->getContactDialog()->onSmilesMenu();
#endif
        }
    }


    void MainWindow::onShowVideoWindow()
    {
        if (mainPage_)
        {
            mainPage_->showVideoWindow();
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

        if (currentIsGreater && platform::is_linux())
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
            if (mainPage_->isVideoWindowOn())
                mainPage_->showVideoWindow();
            activate();
        }
        else
        {
            activate();
            if (mainPage_->isVideoWindowOn())
                mainPage_->showVideoWindow();
        }

    }

    void MainWindow::zoomWindow()
    {
#ifdef __APPLE__
        if (mainPage_ && mainPage_->isVideoWindowActive())
        {
            if (mainPage_->isVideoWindowMaximized())
                mainPage_->showVideoWindow();
            else
                mainPage_->maximizeVideoWindow();
            return;
        }

        getMacSupport()->zoomWindow(winId());
        updateMainMenu();
#endif
    }
}
