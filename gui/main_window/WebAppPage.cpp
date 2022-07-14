#include "stdafx.h"

#include "WebAppPage.h"
#include "js_bridge/AbstractCommand.h"
#include "AppsPage.h"
#include "sidebar/Sidebar.h"
#include "styles/ThemeParameters.h"
#include "styles/StyledWidget.h"
#include "../controls/WebPage.h"
#include "MainWindow.h"
#include "MainPage.h"
#include "main_window/ConnectionWidget.h"
#include "main_window/tasks/TaskEditWidget.h"
#include "main_window/contact_list/ServiceContacts.h"
#include "main_window/GroupChatOperations.h"
#include "main_window/contact_list/ContactListModel.h"
#include "main_window/containers/TaskContainer.h"
#include "main_window/mini_apps/MiniAppsUtils.h"
#include "main_window/mini_apps/MiniAppsContainer.h"
#include "../controls/GeneralDialog.h"
#include "../controls/TextWidget.h"
#include "controls/ContextMenu.h"
#include "../utils/utils.h"
#include "../utils/chromium.h"
#include "utils/features.h"
#include "previewer/toast.h"
#include "../common.shared/config/config.h"
#include "../styles/ThemesContainer.h"
#include "../core_dispatcher.h"
#include "../gui_settings.h"
#include "url_config.h"
#include "../utils/WebUtils.h"
#include "../my_info.h"


namespace
{
    auto getProgressAnimationSize() noexcept
    {
        return Utils::scale_value(32);
    }

    auto getProgressAnimationPenSize() noexcept
    {
        return Utils::fscale_value(1.3);
    }

    int getMargin() noexcept
    {
        return Utils::scale_value(16);
    }

    QMargins getWidgetMargins() noexcept
    {
        return QMargins(Utils::scale_value(32), Utils::scale_value(8), Utils::scale_value(32), Utils::scale_value(8));
    }

    int getSpacing() noexcept
    {
        return Utils::scale_value(4);
    }

    int getVerticalSpacerWidth() noexcept
    {
        return Utils::scale_value(20);
    }

    int getVerticalSpacerHeight() noexcept
    {
        return Utils::scale_value(40);
    }

    int getHorizontalSpacerWidth() noexcept
    {
        return Utils::scale_value(40);
    }

    int getHorizontalSpacerHeight() noexcept
    {
        return Utils::scale_value(20);
    }

    int defaultFontSize() noexcept
    {
        return Utils::scale_value(16);
    }

    int getTextSize() noexcept
    {
        return defaultFontSize();
    }

    QFont loadingLabelFont() { return Fonts::appFont(defaultFontSize()); }
    QFont errorLabelFont() { return Fonts::appFont(defaultFontSize(), Fonts::FontWeight::Medium); }

    auto themeColorKey()
    {
        return Styling::ThemeColorKey{ Styling::StyleVariable::TEXT_PRIMARY };
    }

    int getContextMenuMinWidth() noexcept
    {
        return Utils::scale_value(212);
    }
}


namespace Ui
{
    ReloadButton::ReloadButton(QWidget* _parent, int _radius)
        : RoundButton(_parent, _radius)
    {
    }

    ReloadButton::~ReloadButton() = default;

    QSize ReloadButton::minimumSizeHint() const
    {
        const auto ts = getTextSize();
        const auto margin = Utils::scale_value(12);
        return QSize(ts.width() + 4 * margin + getRadius(), ts.height() + 2 * margin);
    }

    WebView::WebView(QWidget* _parent)
        : QWebEngineView(_parent)
    {
        showTimer_.setSingleShot(true);
        connect(&showTimer_, &QTimer::timeout, this, &WebView::show);
    }

    void WebView::setVisible(bool _visible)
    {
        visible_ = _visible;
        if (reloading_)
            QWebEngineView::setVisible(false);
        else
            QWebEngineView::setVisible(_visible);
    }

    void WebView::beginReload()
    {
        reloading_ = true;
    }

    void WebView::endReload()
    {
        if (reloading_ && visible_ && !isVisible())
            showTimer_.start(500);
        reloading_ = false;
    }

    WebAppPage::WebAppPage(const QString& _type, QWidget* _parent)
        : QWidget(_parent)
        , type_(_type)
        , splitter_(new Splitter(Qt::Horizontal, this))
        , sidebar_(new Sidebar(_parent))
        , isCustomApp_ { Logic::GetAppsContainer()->isCustomApp(_type) }
    {}

    void WebAppPage::setUrl(const QUrl& _url, bool _force)
    {
        needForceChangeUrl_ = _force;
        urlToSet_ = _url;
        if ((!webView_ || webView_->page()->requestedUrl() != _url || _force) && isVisible())
            loadPreparedUrl();
    }

    QUrl WebAppPage::getUrl() const
    {
        return urlToSet_;
    }

    void WebAppPage::connectToPageSignals()
    {
        if (!webView_)
            return;
        connect(webView_->page(), &QWebEnginePage::loadFinished, this, &WebAppPage::onLoadFinished);
        connect(webView_->page(), &QWebEnginePage::loadStarted, this, &WebAppPage::onLoadStarted);
    }

    void WebAppPage::disconnectFromPageSignals()
    {
        if (webView_)
            webView_->page()->disconnect(this);
    }

    void WebAppPage::reloadPage()
    {
        if (webView_)
        {
            connectToPageSignals();
            stackedWidget_->setCurrentIndex(PageState::load);
            webView_->page()->triggerAction(QWebEnginePage::ReloadAndBypassCache);
        }
    }

    void WebAppPage::loadPreparedUrl()
    {
        needForceChangeUrl_ = false;
        initWebEngine();
        if (!isUnavailable_)
        {
            disconnectFromPageSignals();
            onLoadStarted();
            initWebPage();
            connectToPageSignals();
            webView_->setUrl(urlToSet_);
            urlToSet_ = QUrl{};
        }
    }

    void WebAppPage::initWebEngine()
    {
        if (webView_)
            return;

        auto addToLayout = [this](auto w)
        {
            auto rootLayout = Utils::emptyHLayout(this);
            rootLayout->addWidget(w);
        };

        sidebar_->setFrameCountMode(FrameCountMode::_3);
        connect(sidebar_, &Sidebar::visibilityAnimationFinished, this, &WebAppPage::updatePageMaxWidth);
        connect(sidebar_, &Sidebar::hidden, this, [this]()
        {
            updateSplitterHandle(width());
            updatePageMaxWidth(false);
        });
        sidebar_->hide();
        stackedWidget_ = new QStackedWidget(this);
        stackedWidget_->addWidget(createLoadingContainer());
        stackedWidget_->addWidget(createWebContainer());
        stackedWidget_->addWidget(createErrorContainer(false));
        stackedWidget_->addWidget(createErrorContainer(true));
        stackedWidget_->setCurrentIndex(isUnavailable_? PageState::unavailable : PageState::load);

        splitter_->addWidget(stackedWidget_);
        splitter_->setOpaqueResize(true);
        splitter_->setHandleWidth(MainPage::splitterHandleWidth());
        splitter_->setMouseTracking(true);

        QObject::connect(splitter_, &Splitter::moved, this, [this, recentIdx = splitter_->count()](int _desiredPos, int _resultPos, int _index)
        {
            updatePageMaxWidth(sidebar_->isVisible());
            if (_index == recentIdx)
            {
                const auto newDesiredWidth = width() - _desiredPos;
                if (newDesiredWidth > Sidebar::getDefaultMaximumWidth() || newDesiredWidth < Sidebar::getDefaultWidth()
                    || newDesiredWidth + MainPage::pageMinWidth() + MainPage::splitterHandleWidth() >= width())
                {
                    return;
                }

                updateSidebarWidth(newDesiredWidth);
                if (sidebar_->getFrameCountMode() != FrameCountMode::_1)
                    sidebar_->saveWidth();
            }
            QTimer::singleShot(0, this, &WebAppPage::saveSplitterState);
        });

        addToLayout(splitter_);

        const std::chrono::milliseconds time = Features::webPageReloadInterval();
        reloadIntervals_ = { std::chrono::milliseconds::zero(), time / 2, time, 2 * time };
        reloadTimer_.setSingleShot(true);

        connect(Ui::GetDispatcher(), &Ui::core_dispatcher::getUserProxy, this, &WebAppPage::reloadPage);
        connect(Ui::GetDispatcher(), &Ui::core_dispatcher::connectionStateChanged, this, &WebAppPage::refreshPageIfConnectionStateOnline);
        connect(&webChannelProxy_, &WebChannelProxy::showInfo, this, &WebAppPage::showInfo);
        connect(&webChannelProxy_, &WebChannelProxy::createTask, this, &WebAppPage::createTask);
        connect(&webChannelProxy_, &WebChannelProxy::setTitle, this, &WebAppPage::onSetTitle);
        connect(&webChannelProxy_, &WebChannelProxy::setLeftButton, this, &WebAppPage::onSetLeftButton);
        connect(&webChannelProxy_, &WebChannelProxy::setRightButton, this, &WebAppPage::onSetRightButton);
        // We use QueuedConnection so that the "Edit Task" menu closes immediately when the edit dialog opens.
        // Otherwise, the "Edit task" will close together with the editing dialog.
        connect(&webChannelProxy_, &WebChannelProxy::editTask, this, &WebAppPage::editTask, Qt::QueuedConnection);
        connect(&webChannelProxy_, &WebChannelProxy::forwardTask, this, &WebAppPage::forwardTask, Qt::QueuedConnection);
        connect(&webChannelProxy_, &WebChannelProxy::pageLoadFinished, this, &WebAppPage::onPageLoadFinished);
        connect(&webChannelProxy_, &WebChannelProxy::functionCalled, this, &WebAppPage::onNativeClientFunctionCalled);
        connect(&reloadTimer_, &QTimer::timeout, this, &WebAppPage::onReloadPageTimeout);
        connect(&Utils::InterConnector::instance(), &Utils::InterConnector::sidebarVisibilityChanged, this, &WebAppPage::onSidebarVisibilityChanged);
        connect(Logic::getContactListModel(), &Logic::ContactListModel::selectedContactChanged, this, &WebAppPage::showMessengerPage);
        connect(GetDispatcher(), &core_dispatcher::taskCreated, this, &WebAppPage::onTaskCreated);
        connect(&Utils::InterConnector::instance(), &Utils::InterConnector::profileSettingsShow, this, &WebAppPage::onShowProfile);
        connect(&Utils::InterConnector::instance(), &Utils::InterConnector::closeAnySemitransparentWindow, this, [this](const Utils::CloseWindowInfo& _info)
        {
            if (_info.reason_ != Utils::CloseWindowInfo::Reason::Keep_Sidebar)
            {
                if (!(sidebar_->isVisible() && sidebar_->getFrameCountMode() == FrameCountMode::_1))
                    onSidebarVisibilityChanged(false);
            }
            //set focus to close by esc button
            sidebar_->setFocus();
        });

        connect(&Utils::InterConnector::instance(), &Utils::InterConnector::closeSidebar, this, [this]()
        {
            onSidebarVisibilityChanged(false);
        });

        connect(&Utils::InterConnector::instance(), &Utils::InterConnector::currentPageChanged, this, qOverload<>(&WebAppPage::updateHeaderVisibility));
        connect(webView_, &QWebEngineView::urlChanged, this, &WebAppPage::onUrlChanged);

        const auto scale = get_gui_settings()->get_value(settings_scale_coefficient, Utils::getBasicScaleCoefficient());
        const auto splitterScale = get_gui_settings()->get_value(settings_splitter_state_scale, 0.0);
        if (const auto val = QByteArray::fromStdString(get_gui_settings()->get_value<std::string>(settings_splitter_webpage_state, std::string())); !val.isEmpty() && scale == splitterScale)
            splitter_->restoreState(val);
    }

    void WebAppPage::initWebPage()
    {
        const Utils::OpenUrlConfirm openUrlConfirm = (type_ == Utils::MiniApps::getMailId()) ? Utils::OpenUrlConfirm::No : Utils::OpenUrlConfirm::Yes;
        auto p = new WebPage(profile_, openUrlConfirm, this);
        p->setZoomFactor(Utils::getScaleCoefficient());
        p->setBackgroundColor(Styling::getParameters().getColor(Styling::StyleVariable::BASE_GLOBALWHITE));
        p->setWebChannel(webChannel_);
        p->initializeJsBridge();
        webView_->setPage(p);
        if (page_)
            page_->deleteLater();
        page_ = p;
    }

#if QT_CONFIG(menu)
    QMenu* WebAppPage::createContextMenu(const QPoint& _pos)
    {
        if (!webView_ || !webView_->page())
            return nullptr;
        auto menu = webView_->page()->createStandardContextMenu();
        if (!menu)
            return nullptr;

        Testing::setAccessibleName(menu, qsl("AS WebAppPage contextMenu"));
        ContextMenu::applyStyle(menu, false, Utils::scale_value(15), Utils::scale_value(36));
        menu->setMinimumWidth(getContextMenuMinWidth());
        menu->move(cursor().pos());
        menu->show();

        return menu;
    }
#endif

    QWidget* WebAppPage::createWebContainer()
    {
        webView_ = new WebView(this);
        profile_ = needsWebCache() ? Utils::InterConnector::instance().getLocalProfile() : QWebEngineProfile::defaultProfile();
        profile_->setHttpUserAgent(Ui::get_gui_settings()->get_value(settings_user_agent, QString()).arg(Ui::MyInfo()->aimId()));
        connect(profile_, &QWebEngineProfile::downloadRequested, this, &WebAppPage::downloadRequested);

        webView_->setContextMenuPolicy(Qt::NoContextMenu);
        webView_->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
        webView_->setMinimumWidth(MainPage::pageMinWidth());
        webView_->setStyleSheet(qsl("background:transparent;"));

        webView_->setContextMenuPolicy(Qt::CustomContextMenu);
        connect(webView_, &WebView::customContextMenuRequested, this, [this](const QPoint& _pos)
        {
#if QT_CONFIG(menu)
            createContextMenu(_pos);
#endif
        });

        webChannel_ = new QWebChannel(this);
        webChannel_->registerObject(qsl("client"), &webChannelProxy_);

        QWidget* webContainer = new QWidget(this);
        webContainer->setStyleSheet(qsl("background:transparent;"));

        auto webContainerLayout = Utils::emptyHLayout(this);
        webContainerLayout->addWidget(webView_);
        webContainer->setLayout(webContainerLayout);
        return webContainer;
    }

    QWidget* WebAppPage::createLoadingContainer()
    {
        auto loadingLayout = Utils::emptyVLayout(this);
        loadingLayout->setContentsMargins(QMargins());

        loadingLayout->addSpacerItem(new QSpacerItem(1, 1, QSizePolicy::Minimum, QSizePolicy::Expanding));

        progressAnimation_ = new ProgressAnimation(this);
        progressAnimation_->setProgressWidth(getProgressAnimationSize());
        progressAnimation_->setProgressPenColorKey(themeColorKey());
        loadingLayout->addWidget(progressAnimation_);
        loadingLayout->setAlignment(progressAnimation_, Qt::AlignCenter);

        auto label = new TextWidget(this, QT_TRANSLATE_NOOP("web_app", "Page is loading..."));
        label->init({ loadingLabelFont(), Styling::ThemeColorKey { Styling::StyleVariable::BASE_PRIMARY } });
        loadingLayout->addWidget(label, 0, Qt::AlignHCenter);

        loadingLayout->addSpacerItem(new QSpacerItem(1, 1, QSizePolicy::Minimum, QSizePolicy::Expanding));

        auto loadingContainer = new Styling::StyledWidget(this);
        loadingContainer->setDefaultBackground();
        loadingContainer->setLayout(loadingLayout);
        progressAnimation_->adjust();
        return loadingContainer;
    }

    QWidget* WebAppPage::createErrorContainer(bool _unavailable)
    {
        auto errorContainer = new Styling::StyledWidget(this);
        errorContainer->setDefaultBackground();

        auto verticalLayout_3 = new QVBoxLayout(errorContainer);
        verticalLayout_3->setContentsMargins(getMargin(), getMargin(), getMargin(), getMargin());
        auto gridLayout = new QGridLayout();
        auto verticalSpacer = new QSpacerItem(getVerticalSpacerWidth(), getVerticalSpacerHeight(), QSizePolicy::Minimum, QSizePolicy::Expanding);
        gridLayout->addItem(verticalSpacer, 0, 1);

        auto verticalSpacer_2 = new QSpacerItem(getVerticalSpacerWidth(), getVerticalSpacerHeight(), QSizePolicy::Minimum, QSizePolicy::Expanding);
        gridLayout->addItem(verticalSpacer_2, 2, 1);

        auto widget = new Styling::StyledWidget(errorContainer);
        widget->setBackgroundKey(Styling::ThemeColorKey{ Styling::StyleVariable::CHATEVENT_BACKGROUND });
        auto verticalLayout_2 = new QVBoxLayout(widget);
        verticalLayout_2->setContentsMargins(getWidgetMargins());
        auto verticalLayout = new QVBoxLayout();
        verticalLayout->setSpacing(getSpacing());

        auto label = new TextWidget(widget, _unavailable ? QT_TRANSLATE_NOOP("web_app", "The service is not available for your account. Contact technical support")
                                                         : QT_TRANSLATE_NOOP("web_app", "Unable to load"));
        label->init({ errorLabelFont(), Styling::ThemeColorKey{ Styling::StyleVariable::BASE_PRIMARY } });
        label->setAlignment(TextRendering::HorAligment::CENTER);
        if (_unavailable)
            unavailableLabel_ = label;

        auto gridLayout_2 = new QGridLayout();
        gridLayout_2->addWidget(label, 1, 1, 1, 1);
        const auto horizontalSpacer_1 = new QSpacerItem(0, 0, QSizePolicy::Minimum, QSizePolicy::Minimum);
        const auto horizontalSpacer_2 = new QSpacerItem(0, 0, QSizePolicy::Minimum, QSizePolicy::Minimum);
        gridLayout_2->addItem(horizontalSpacer_1, 1, 0);
        gridLayout_2->addItem(horizontalSpacer_2, 1, 2);

        verticalLayout->addLayout(gridLayout_2);

        auto reloadButton = new ReloadButton(widget);
        reloadButton->setText(QT_TRANSLATE_NOOP("web_app", "Retry"), getTextSize());
        reloadButton->setTextColor(Styling::ThemeColorKey{ Styling::StyleVariable::TEXT_PRIMARY });
        const auto bgNormal = Styling::ThemeColorKey{ Styling::StyleVariable::CHAT_PRIMARY };
        const auto bgHover = Styling::ThemeColorKey{ Styling::StyleVariable::CHAT_PRIMARY_HOVER };
        const auto bgActive = Styling::ThemeColorKey{ Styling::StyleVariable::CHAT_PRIMARY_ACTIVE };
        reloadButton->setColors(bgNormal, bgHover, bgActive);
        verticalLayout->addWidget(reloadButton);
        verticalLayout_2->addLayout(verticalLayout);

        reloadButton->setVisible(!_unavailable);

        gridLayout->addWidget(widget, 1, 1, 1, 1);
        const auto horizontalSpacer_3 = new QSpacerItem(getHorizontalSpacerWidth(), getHorizontalSpacerHeight(), QSizePolicy::Expanding, QSizePolicy::Minimum);
        gridLayout->addItem(horizontalSpacer_3, 1, 0);

        const auto horizontalSpacer_4 = new QSpacerItem(getHorizontalSpacerWidth(), getHorizontalSpacerHeight(), QSizePolicy::Expanding, QSizePolicy::Minimum);
        gridLayout->addItem(horizontalSpacer_4, 1, 2);
        verticalLayout_3->addLayout(gridLayout);

        connect(reloadButton, &RoundButton::clicked, this, &WebAppPage::refreshPage);

        return errorContainer;
    }

    void WebAppPage::showEvent(QShowEvent* _event)
    {
        if ((!urlToSet_.isEmpty() && !webView_) || (webView_ && webView_->page() && !urlToSet_.isEmpty() && urlToSet_ != webView_->page()->requestedUrl()) || needForceChangeUrl_)
            loadPreparedUrl();

        if (sidebar_ && sidebar_->wasVisible())
        {
            if (sidebar_->getFrameCountMode() == FrameCountMode::_1)
                onSidebarVisibilityChanged(true);
        }

        if (type_ == Utils::MiniApps::getAccountDeleteId())
            connectToPageSignals();
        QWidget::showEvent(_event);
    }

    void WebAppPage::showThreadInfo(const QString& _aimId)
    {
        showInfo(_aimId, SidebarContentType::threadInfo);
    }

    void WebAppPage::closeThreadInfo(const QString& _aimId)
    {
        using namespace ServiceContacts;
        sidebar_->preparePage(Sidebar::getThreadPrefix() % _aimId, { true, FrameCountMode::_2, contactId(ContactType::Tasks) }, false);
    }

    void WebAppPage::showInfo(const QString& _aimId, const SidebarContentType _infoType)
    {
        aimId_ = _aimId;
        infoType_ = _infoType;

        if (_infoType == SidebarContentType::thread || _infoType == SidebarContentType::task)
        {
            Logic::getContactListModel()->markAsThreadWithNoParentChat(_aimId, _infoType == SidebarContentType::task);
            using namespace ServiceContacts;
            sidebar_->preparePage(Sidebar::getThreadPrefix() % _aimId, { true, sidebar_->getFrameCountMode(), contactId(ContactType::Tasks) }, false);
        }
        else if (_infoType == SidebarContentType::person || _infoType == SidebarContentType::threadInfo)
        {
            sidebar_->preparePage(aimId_, { false, sidebar_->getFrameCountMode() }, false);
        }

        if (!sidebar_->isVisible())
            onSidebarVisibilityChanged(true);

        //set focus to close by esc button. Order is important. You need to do this last.
        sidebar_->setFocus();
        updateSplitterHandle(width());
    }

    void WebAppPage::refreshPageIfConnectionStateOnline(const Ui::ConnectionState& _state)
    {
        if (_state == ConnectionState::stateOnline)
            refreshPage();
    }

    void WebAppPage::refreshPageIfNeeded()
    {
        if (isUnavailable_)
            return;

        reloadIntervalIdx_ = 0;
        //stop the timer if it was started before
        reloadTimer_.stop();
        refreshPage();
    }

    void WebAppPage::refreshPage()
    {
        if (!pageIsLoaded_)//each page is loaded once at the first launch, then it is updated itself
        {
            connectToPageSignals();
            if (webView_)
            {
                webView_->hide();
                webView_->beginReload();
                webView_->reload();
            }
        }
    }

    void WebAppPage::onSidebarVisibilityChanged(bool _visible)
    {
        if (!isVisible())
        {
            if (!_visible)
                sidebar_->setNeedShadow(false);
            return;
        }

        if( webView_ &&
            webView_->page() &&
            sidebar_->isThreadOpen() &&
            (infoType_ == SidebarContentType::task || infoType_ == SidebarContentType::thread) &&
            !aimId_.isEmpty())
        {
            const auto aimIdArg = _visible ? qsl("{") + aimId_ + qsl("}") : qsl("null");
            webView_->page()->runJavaScript(qsl("window.externalRouter.setOpenedThread('%1')").arg(aimIdArg));
        }

        if (_visible)
        {
            if (sidebar_->getFrameCountMode() != FrameCountMode::_1)
                sidebar_->saveWidth();

            sidebar_->setFrameCountMode(width() > MainPage::getFrame2MinWidth() ? FrameCountMode::_2 : FrameCountMode::_1);

            updateSidebarPos(width());
            restoreSidebar();
            sidebar_->showAnimated();
            if (sidebar_->getFrameCountMode() == FrameCountMode::_1)
                stackedWidget_->hide();

            Q_EMIT Utils::InterConnector::instance().currentPageChanged();

            QMetaObject::invokeMethod(this, [this]() { Q_EMIT Utils::InterConnector::instance().setFocusOnInput(aimId_);}, Qt::QueuedConnection);
        }
        else
        {
            stackedWidget_->setMaximumWidth(width());
            if (sidebar_->isVisible() && sidebar_->getFrameCountMode() != FrameCountMode::_1)
                sidebar_->saveWidth();

            removeSidebarPlaceholder();
            sidebar_->hideAnimated();
            splitter_->refresh();
            stackedWidget_->show();
        }

        updateSplitterHandle(width());
        updateHeaderVisibility();
    }

    void WebAppPage::showMessengerPage()
    {
        if (!isVisible())
            return;

        Utils::InterConnector::instance().getMainWindow()->showMessengerPage();
    }

    static void showTaskDialog(TaskEditWidget* _taskWidget)
    {
        GeneralDialog::Options opt;
        opt.ignoreKeyPressEvents_ = true;

        GeneralDialog gd(_taskWidget, Utils::InterConnector::instance().getMainWindow(), opt);
        _taskWidget->setFocusOnTaskTitleInput();
        gd.addLabel(_taskWidget->getHeader(), Qt::AlignCenter);
        gd.execute();
    }

    void WebAppPage::createTask()
    {
        im_assert(taskCreateId_ == -1);

        auto taskWidget = new TaskEditWidget();
        connect(taskWidget, &TaskEditWidget::createTask, this, &WebAppPage::tryCreateTask);
        showTaskDialog(taskWidget);
    }

    void WebAppPage::onSetTitle(const QString& _title)
    {
        Q_EMIT setTitle(type_, _title);
    }

    void WebAppPage::onSetLeftButton(const QString& _text, const QString& _color, bool _enabled)
    {
        activeColor_ = _color;
        Q_EMIT setLeftButton(type_, _text, activeColor_, _enabled);
    }

    void WebAppPage::onSetRightButton(const QString& _text, const QString& _color, bool _enabled)
    {
        Q_EMIT setRightButton(type_, _text, _color, _enabled);
    }

    void WebAppPage::editTask(const QString& _taskId)
    {
        // need for correct display semi-transparent overlay
        if (webView_ && webView_->page())
            webView_->page()->setBackgroundColor(Styling::getParameters().getColor(Styling::StyleVariable::BASE_GLOBALWHITE));

        const auto& taskData = Logic::GetTaskContainer()->getTask(_taskId);
        if (taskData.needToLoadData())
        {
            connect(Logic::GetTaskContainer(), &Logic::TaskContainer::taskChanged, this, &WebAppPage::editTask, Qt::UniqueConnection);
            return;
        }
        disconnect(Logic::GetTaskContainer(), &Logic::TaskContainer::taskChanged, this, &WebAppPage::editTask);

        auto taskWidget = new TaskEditWidget(taskData, Data::MentionMap());
        showTaskDialog(taskWidget);
    }

	void WebAppPage::forwardTask(const QString& _taskId)
	{
		const auto& taskData = Logic::GetTaskContainer()->getTask(_taskId);

		Data::Quote quote;
		quote.task_ = taskData;
		quote.mediaType_ = Ui::MediaType::mediaTypeTask;

		forwardMessage({quote}, false, QString(), QString(), false);
	}

    void WebAppPage::onTaskCreated(qint64 _seq, int _error)
    {
        im_assert(_seq > 0);

        if (taskCreateId_ != _seq)
            return;

        taskCreateId_ = -1;

        if (!_error)
            refreshPage();
    }

    void WebAppPage::onUrlChanged(const QUrl& _url)
    {
        if (isCustomApp_)
        {
            if (webView_->history()->canGoBack())
            {
                WebUtils::writeLog(_url.toString(), qsl("url changed to %1").arg(_url.toString()));
                Q_EMIT setLeftButton(type_, qsl("<"), activeColor_, true);
            }
            else
            {
                Q_EMIT setLeftButton(type_, qsl(""), {}, false);
            }
        }
    }

    void WebAppPage::headerLeftButtonClicked(const QString& _type)
    {
        if (_type != type_)
            return;

        if (isCustomApp_)
        {
            if (webView_)
                webView_->back();
            onUrlChanged({});
        }
        else
        {
            if (webView_ && webView_->page())
                webView_->page()->runJavaScript(qsl("window.externalRouter.leftTopButtonClick()"));
        }
    }

    void WebAppPage::headerRightButtonClicked(const QString& _type)
    {
        if (_type != type_)
            return;
        if (webView_ && webView_->page())
            webView_->page()->runJavaScript(qsl("window.externalRouter.rightTopButtonClick()"));
    }

    void WebAppPage::tryCreateTask(qint64 _seq)
    {
        taskCreateId_ = _seq;
    }

    void WebAppPage::onLoadFinished(bool _ok, int _err)
    {
        //_err > 0 for http code and _err < 0 for chromium error code(see utils/chromium.h)
        // if _ok waiting for a response from the server for tasks and contacts
        if (_ok && (type_ == Utils::MiniApps::getTasksId() || type_ == Utils::MiniApps::getOrgstructureId()))
            return;

        onPageLoadFinished(_ok);
    }

    void WebAppPage::onPageLoadFinished(bool _ok)
    {
        disconnectFromPageSignals();

        if (_ok)
        {
            pageIsLoaded_ = true;
            progressAnimation_->stop();
            reloadIntervalIdx_ = 0;
            if (webView_)
            {
                webView_->show();
                webView_->endReload();
            }
            stackedWidget_->setCurrentIndex(webView_->page()->requestedUrl().isEmpty() ? PageState::error : PageState::content);
        }
        else
        {
            if (reloadIntervalIdx_ < reloadIntervals_.size())
            {
                reloadTimer_.start(reloadIntervals_[reloadIntervalIdx_]);
                ++reloadIntervalIdx_;
            }
            else
            {
                stackedWidget_->setCurrentIndex(PageState::error);
            }
        }
    }

    void WebAppPage::onLoadStarted()
    {
        stackedWidget_->setCurrentIndex(PageState::load);
        progressAnimation_->start();
    }

    void WebAppPage::onReloadPageTimeout()
    {
        //do not make requests if the page is not visible
        if (isVisible())
            refreshPage();
    }

    void WebAppPage::onNativeClientFunctionCalled(const QString& _reqId, const QString& _name, const QJsonObject& _requestParams)
    {
        if (auto webCmd = JsBridge::makeCommand(_reqId, _name, _requestParams, this))
        {
            connect(webCmd, &JsBridge::AbstractCommand::commandReady, this, &WebAppPage::sendFunctionCallReply);
            connect(webCmd, &JsBridge::AbstractCommand::commandReady, webCmd, &JsBridge::AbstractCommand::deleteLater);
            webCmd->execute();
        }
    }

    void WebAppPage::sendFunctionCallReply(const QJsonObject& _reply)
    {
        if (auto page = qobject_cast<WebPage*>(webView_->page()))
            page->sendFunctionCallReply(_reply);
    }

    void WebAppPage::downloadRequested(QWebEngineDownloadItem* _download)
    {
        const auto name = _download->downloadFileName();
        if (activeDownloads_.contains(name))
            return;

        activeDownloads_.push_back(name);
        _download->setDownloadDirectory(Ui::getDownloadPath());
        _download->accept();

        Utils::showWebDownloadToast(name, false);

        connect(_download, &QWebEngineDownloadItem::finished, this, [name, guard = QPointer(this)]
        {
            if (!guard)
                return;
            guard->activeDownloads_.removeAll(name);
            Utils::showWebDownloadToast(name, true);
        });
    }

    bool WebAppPage::isSideBarInSplitter() const
    {
        return splitter_->indexOf(sidebar_) != -1;
    }

    void WebAppPage::insertSidebarToSplitter()
    {
        sidebar_->setNeedShadow(false);

        if (sidebar_->getFrameCountMode() != FrameCountMode::_2)
            removeSidebarPlaceholder();

        if (!isSideBarInSplitter())
        {
            splitter_->addWidget(sidebar_);
            if (auto handle = splitter_->handle(splitter_->count() - 1))
                handle->setDisabled(true);
        }
    }

    void WebAppPage::takeSidebarFromSplitter()
    {
        if (sidebar_->isVisible())
        {
            if (!sidebarPlaceholderWidget_)
                sidebarPlaceholderWidget_ = new QWidget();
            sidebarPlaceholderWidget_->setFixedWidth(sidebar_->getCurrentWidth() + MainPage::splitterHandleWidth());

            if (splitter_->indexOf(sidebarPlaceholderWidget_) == -1)
                splitter_->addWidget(sidebarPlaceholderWidget_);
        }
        sidebar_->setNeedShadow(true);
    }

    void WebAppPage::resizeEvent(QResizeEvent* _event)
    {
        //change the height of the sidebar even if it is not visible
        if (size() != _event->oldSize())
            sidebar_->changeHeight();

        if (unavailableLabel_)
        {
            const auto margins = getWidgetMargins();
            const auto horMargin = margins.left() + margins.right() + getMargin() * 2;

            if (unavailableLabel_->getDesiredWidth() > width() - horMargin)
                unavailableLabel_->setMaxWidthAndResize(width() - horMargin);
            else
                unavailableLabel_->setMaxWidthAndResize(unavailableLabel_->getDesiredWidth());
        }

        if (!stackedWidget_)
            return;

        //save focus for thread
        auto prevFocused = qobject_cast<QWidget*>(qApp->focusObject());

        if (const int newWidth = width(); width() != _event->oldSize().width())
        {
            if (newWidth < MainPage::getFrame2MinWidth())
            {
                stackedWidget_->setMaximumWidth(width());
                if (newWidth <= Sidebar::getDefaultMaximumWidth() && sidebar_->getFrameCountMode() != FrameCountMode::_1)
                    sidebar_->saveWidth();

                sidebar_->setFrameCountMode(FrameCountMode::_1);
                sidebar_->setWidth(newWidth);
                if (sidebar_->isVisible())
                {
                    Utils::InterConnector::instance().getAppsPage()->setHeaderVisible(false);
                    stackedWidget_->hide();
                    insertSidebarToSplitter();
                    sidebar_->show();
                    Q_EMIT Utils::InterConnector::instance().currentPageChanged();
                }
                else
                {
                    stackedWidget_->show();
                    removeSidebarPlaceholder();
                    Q_EMIT Utils::InterConnector::instance().currentPageChanged();
                }
            }
            else
            {
                sidebar_->setFrameCountMode((sidebar_->isVisible() || sidebar_->wasVisible()) && isSideBarInSplitter() ? FrameCountMode::_3 : FrameCountMode::_2);
                sidebar_->restoreWidth();
                Q_EMIT Utils::InterConnector::instance().currentPageChanged();
                updatePageMaxWidth(sidebar_->isVisible());
                stackedWidget_->show();
            }
        }

        if (width() != _event->oldSize().width())
        {
            updateSidebarPos(width());
            updateSplitterStretchFactors();
            updateHeaderVisibility();
        }

        saveSplitterState();

        QMetaObject::invokeMethod(this, [prevFocused, this]() { resizeUpdateFocus(prevFocused); }, Qt::QueuedConnection);
    }

    void Ui::WebAppPage::hideEvent(QHideEvent* _event)
    {
        if (!isVisible() && parentWidget()->isVisible())
            Q_EMIT pageIsHidden();
    }

    void WebAppPage::updateSidebarPos(int _width)
    {
        const auto updateSidebarPlaceholder = [this, _width]()
        {
            if (sidebarPlaceholderWidget_)
                sidebarPlaceholderWidget_->setFixedWidth(std::max(0, _width - MainPage::pageMinWidth()));
        };

        if (stackedWidget_ && stackedWidget_->width() < MainPage::pageMinWidth())
            stackedWidget_->resize(MainPage::pageMinWidth(), stackedWidget_->height());

        if (isWidthOutlying(_width)
            || (stackedWidget_->width() != MainPage::pageMinWidth() && _width < MainPage::getFrame2MinWidth()))
        {
            if (sidebar_->getFrameCountMode() != FrameCountMode::_1
            && _width < MainPage::pageMinWidth() + sidebar_->width() + MainPage::splitterHandleWidth())
            {
                if (sidebar_->width() < Sidebar::getDefaultMaximumWidth())
                {
                    sidebar_->saveWidth();
                    takeSidebarFromSplitter();
                    updateSidebarPlaceholder();
                }
                else
                {
                    insertSidebarToSplitter();
                    const auto newWidth = std::max(_width - (MainPage::pageMinWidth() + MainPage::splitterHandleWidth()), Sidebar::getDefaultMaximumWidth());
                    updateSidebarWidth(newWidth);
                    updateSidebarPlaceholder();
                }
            }
            else
            {
                removeSidebarPlaceholder();
                insertSidebarToSplitter();
            }
        }
        else if (isSideBarInSplitter())
        {
            if (stackedWidget_->width() == MainPage::pageMinWidth()
                || _width < MainPage::pageMinWidth() + sidebar_->width() + MainPage::splitterHandleWidth())
            {
                takeSidebarFromSplitter();
                if (_width <= Sidebar::getDefaultMaximumWidth())
                {
                    sidebar_->saveWidth();
                    updateSidebarWidth(std::min(sidebar_->width(), _width));
                }
                else
                {
                    sidebar_->restoreWidth();
                }
            }
        }
        else
        {
            sidebar_->updateSize();
            if (_width >= MainPage::pageMinWidth() + sidebar_->width() + MainPage::splitterHandleWidth())
            {
                sidebar_->restoreWidth();
                removeSidebarPlaceholder();
                insertSidebarToSplitter();
            }
            else
            {
                takeSidebarFromSplitter();
                updateSidebarPlaceholder();
            }
        }
        updateSplitterHandle(_width);
    }

    void WebAppPage::updateSplitterHandle(int _width)
    {
        if (auto handle = splitter_->handle(1))
        {
            const bool enableHandle = (_width > MainPage::pageMinWidth() && !sidebar_->isVisible())
                || (_width >= (MainPage::pageMinWidth() + Sidebar::getDefaultWidth()) && sidebar_->isVisible());
            handle->setEnabled(enableHandle);
        }
    }

    void Ui::WebAppPage::updateSidebarWidth(int _desiredWidth)
    {
        if (sidebar_)
            sidebar_->setFixedWidth(std::clamp(_desiredWidth, Sidebar::getDefaultWidth(), Sidebar::getDefaultMaximumWidth()));
    }

    void WebAppPage::removeSidebarPlaceholder()
    {
        if (sidebarPlaceholderWidget_)
        {
            sidebarPlaceholderWidget_->hide();
            sidebarPlaceholderWidget_->deleteLater();
            sidebarPlaceholderWidget_ = nullptr;
        }
    }

    void WebAppPage::updateSplitterStretchFactors()
    {
        for (int i = 0; i < splitter_->count(); ++i)
            splitter_->setStretchFactor(i, i > 0 ? 1 : 0);
    }

    void WebAppPage::resizeUpdateFocus(QWidget* _prevFocused)
    {
        if (!_prevFocused)
            return;

        if (GeneralDialog::isActive())
        {
            _prevFocused->setFocus();
            return;
        }

        if (sidebar_->isThreadOpen())
            Q_EMIT Utils::InterConnector::instance().setFocusOnInput(sidebar_->currentAimId(Sidebar::ResolveThread::Yes));
        else if (isSideBarInSplitter() || sidebar_->isAncestorOf(_prevFocused))
            _prevFocused->setFocus();
    }

    bool WebAppPage::isWidthOutlying(int _width) const
    {
        return _width >= MainPage::getFrame3MinWidth() || _width < MainPage::getFrame2MinWidth();
    }

    void WebAppPage::onShowProfile(const QString& _aimId)
    {
        showProfile(_aimId);
    }

    void WebAppPage::showProfile(const QString& _aimId)
    {
        if (!isVisible() || _aimId.isEmpty())
            return;

        showInfo(_aimId, SidebarContentType::person);
    }

    bool WebAppPage::needsWebCache() const
    {
        return type_ == Utils::MiniApps::getMailId() && Features::mailSelfAuth() ||
               type_ == Utils::MiniApps::getCalendarId() && Features::calendarSelfAuth() ||
               type_ == Utils::MiniApps::getCloudId() && Features::cloudSelfAuth();
    }

    void WebAppPage::showProfileFromThreadInfo(const QString& _aimId)
    {
        showProfileFromThreadInfo_ = true;
        showProfile(_aimId);
        showProfileFromThreadInfo_ = false;
    }

    bool WebAppPage::isProfileFromThreadInfo()
    {
        return showProfileFromThreadInfo_;
    }

    void WebAppPage::saveSplitterState()
    {
        static const auto scale = get_gui_settings()->get_value(settings_scale_coefficient, Utils::getBasicScaleCoefficient());
        get_gui_settings()->set_value(settings_splitter_webpage_state, splitter_->saveState().toStdString());
        if (get_gui_settings()->get_value(settings_splitter_state_scale, 0.0) != scale)
            get_gui_settings()->set_value(settings_splitter_state_scale, scale);
    }

    void WebAppPage::restoreSidebar()
    {
        if (sidebar_->getFrameCountMode() == FrameCountMode::_1)
        {
            sidebar_->setWidth(std::min(width(), MainPage::getFrame2MinWidth()));
        }
        else
        {
            sidebar_->restoreWidth();
            if (isWidthOutlying(width()) && width() >= MainPage::pageMinWidth() + sidebar_->getCurrentWidth() + MainPage::splitterHandleWidth())
                insertSidebarToSplitter();
            else
                updateSidebarPos(width());
        }
    }

    void WebAppPage::updatePageMaxWidth(bool _isSidebarVisible)
    {
        if (!stackedWidget_)
            return;
        const auto newSidebarWidth = _isSidebarVisible && isSideBarInSplitter() ? sidebar_->getCurrentWidth() : 0;
        stackedWidget_->setMaximumWidth(width() - newSidebarWidth);
        stackedWidget_->update();
        updateHeaderVisibility();
    }

    void WebAppPage::updateHeaderVisibility()
    {
        if (type_ == Utils::MiniApps::getAccountDeleteId())
            return;

        if (isVisible())
        {
            if (auto page = Utils::InterConnector::instance().getAppsPage())
                page->setHeaderVisible(!sidebar_->isVisible() || sidebar_->getFrameCountMode() != FrameCountMode::_1);
        }
    }

    bool WebAppPage::isPageLoaded()
    {
        return pageIsLoaded_;
    }

    void WebAppPage::markUnavailable(bool _isUnavailable)
    {
        if (_isUnavailable)
        {
            initWebEngine();
            if (webView_)
                webView_->stop();
            stackedWidget_->setCurrentIndex(PageState::unavailable);
        }
        else if (isUnavailable_)
        {
            onLoadStarted();
        }
        isUnavailable_ = _isUnavailable;
    }

    bool WebAppPage::isUnavailable() const
    {
        return isUnavailable_;
    }

    QString WebAppPage::getAimId() const
    {
        if (sidebar_)
            return sidebar_->currentAimId(Sidebar::ResolveThread::Yes);

        return {};
    }

    bool WebAppPage::isSidebarVisible() const
    {
        return sidebar_ && sidebar_->isVisible();
    }
}
