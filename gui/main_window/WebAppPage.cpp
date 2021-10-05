#include "stdafx.h"

#include "WebAppPage.h"
#include "AppsPage.h"
#include "sidebar/Sidebar.h"
#include "styles/ThemeParameters.h"
#include "../controls/WebPage.h"
#include "MainWindow.h"
#include "main_window/tasks/TaskEditWidget.h"
#include "main_window/contact_list/ServiceContacts.h"
#include "main_window/GroupChatOperations.h"
#include "main_window/contact_list/ContactListModel.h"
#include "main_window/containers/TaskContainer.h"
#include "../controls/GeneralDialog.h"
#include "../utils/utils.h"
#include "../common.shared/config/config.h"
#include "../styles/ThemesContainer.h"
#include "../core_dispatcher.h"
#include "url_config.h"


namespace
{
    int splitterHandleWidth() noexcept
    {
        return Utils::scale_value(1);
    }

    int getWebViewMinWidth() noexcept
    {
        return Utils::scale_value(380);
    }

    int getFrame2MinWith() noexcept
    {
        return Utils::scale_value(660);
    }

    int getFrame3MinWith()
    {
        return getFrame2MinWith() + Ui::Sidebar::getDefaultWidth();
    }
}


namespace Ui
{
    void Helper::postEvent(const QJsonObject& _action)
    {
        QString id;
        QString dataType;
        InfoType infoType = InfoType::person;

        if (dataType = qsl("event"); _action.contains(dataType) && _action[dataType].isString())
        {
            const auto event = _action[dataType].toString();
            if (event == u"createTask")
            {
                Q_EMIT createTask();
                return;
            }
            else if (event == u"editTask")
            {
                if (dataType = qsl("data"); _action.contains(dataType) && _action[dataType].isObject())
                {
                    const auto data = _action[dataType].toObject();
                    if (dataType = qsl("taskId"); data.contains(dataType) && data[dataType].isString())
                    {
                        id = data[dataType].toString();
                    }
                }
                Q_EMIT editTask(id);
                return;
            }
            else if (event == u"message")
            {
                if (dataType = qsl("data"); _action.contains(dataType) && _action[dataType].isObject())
                {
                    const auto data = _action[dataType].toObject();
                    if (dataType = qsl("userId"); data.contains(dataType) && data[dataType].isString())
                    {
                        Utils::InterConnector::instance().openDialog(data[dataType].toString());
                        return;
                    }
                }
            }
            else if (event == u"call")
            {
                if (dataType = qsl("data"); _action.contains(dataType) && _action[dataType].isObject())
                {
                    const auto data = _action[dataType].toObject();
                    if (dataType = qsl("userId"); data.contains(dataType) && data[dataType].isString())
                    {
                        auto sn = data[dataType].toString();
                        auto video = false;
                        if (dataType = qsl("video"); data.contains(dataType) && data[dataType].isBool())
                            video = data[dataType].toBool();

                        doVoipCall(sn, video ? CallType::Video : CallType::Audio, CallPlace::Profile);
                        return;
                    }
                }
            }
        }

        if (dataType = qsl("data"); _action.contains(dataType) && _action[dataType].isObject())
        {
            const auto data = _action[dataType].toObject();
            if (dataType = qsl("sn"); data.contains(dataType) && data[dataType].isString())
            {
                id = data[dataType].toString();
                infoType = InfoType::person;
            }

            if (dataType = qsl("threadId"); data.contains(dataType) && data[dataType].isString())
            {
                id = data[dataType].toString();
                infoType = InfoType::thread;
            }
        }

        if (id.isEmpty())
            return;

        Q_EMIT showInfo(id, infoType);
    }

    WebView::WebView(QWidget* _parent)
            : QWebEngineView(_parent)
    {
        showTimer_.setSingleShot(true);
        connect(&showTimer_, &QTimer::timeout, this, &WebView::show);
    }

    WebView::~WebView() = default;

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
        {
            showTimer_.start(500);
        }
        reloading_ = false;
    }

    WebAppPage::WebAppPage(AppPageType _type, QWidget* _parent)
        : QWidget(_parent)
        , type_(_type)
        , webContainer_(nullptr)
        , splitter_(new Splitter(Qt::Horizontal, this))
        , sidebar_(new Sidebar(_parent))
    {
    }

    WebAppPage::~WebAppPage() = default;

    void WebAppPage::setUrl(const QUrl& _url)
    {
        if (url_ != _url)
        {
            url_ = _url;
            if (isVisible())
                setUrlPrivate(_url);
            else
                urlChanged_ = true;
        }
    }

    QUrl WebAppPage::getUrl() const
    {
        return url_;
    }

    void WebAppPage::reloadPage()
    {
        if (webView_)
            webView_->page()->triggerAction(QWebEnginePage::ReloadAndBypassCache);
    }

    QUrl WebAppPage::webAppUrl(AppPageType _type)
    {
        auto composeUrl = [](QStringView _url) -> QUrl
        {
            if (_url.startsWith(u"https://", Qt::CaseInsensitive))
                return _url.toString();
            return QString(u"https://" % _url);
        };

        const auto isDarkTheme = Styling::getThemesContainer().getCurrentTheme()->isDark();
        switch (_type)
        {
        case Ui::AppPageType::messenger:
            im_assert("page is not webpage");
            return {};
        case Ui::AppPageType::org_structure:
            return composeUrl(getUrlConfig().getDi(isDarkTheme));
        case Ui::AppPageType::tasks:
            return composeUrl(getUrlConfig().getTasks() +
                qsl("?page=tasklist&config_host=%1&dark=%2&lang=%3&r=%4")
                .arg(QUrl(getUrlConfig().getConfigHost()).host()) // doing "u.myteam.vmailru.net" from the original configHost
                .arg(isDarkTheme ? qsl("1") : qsl("0"))
                .arg(Utils::GetTranslator()->getLang())
                .arg(rand() % 900 + 100));
        case AppPageType::calendar:
            return composeUrl(getUrlConfig().getCalendar() + qsl("?theme=%1").arg(isDarkTheme ? qsl("dark") : qsl("light")));
        default:
            break;
        }
        return {};
    }

    QString WebAppPage::webAppId(AppPageType _type)
    {
        switch (_type)
        {
        case Ui::AppPageType::org_structure:
            return qsl("di");
        case Ui::AppPageType::tasks:
            return qsl("tasks");
        case AppPageType::calendar:
            return qsl("calendar");
        default:
            break;
        }
        return {};
    }

    void WebAppPage::setUrlPrivate(const QUrl& _url)
    {
        urlChanged_ = false;
        initWebEngine();
        webView_->page()->setUrl(_url);
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
        sidebar_->hide();
        webView_ = new WebView;
        QWebEnginePage* p = new WebPage(webView_);
        p->setZoomFactor(Utils::getScaleCoefficient());
        webView_->setPage(p);
        webView_->setContextMenuPolicy(Qt::NoContextMenu);
        webView_->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
        webView_->setMinimumWidth(getWebViewMinWidth());
        p->setBackgroundColor(Styling::getParameters().getColor(Styling::StyleVariable::BASE_GLOBALWHITE));
        webView_->setStyleSheet(qsl("background:transparent;"));

        webChannel_ = new QWebChannel(p);
        p->setWebChannel(webChannel_);
        webChannel_->registerObject(qsl("client"), &helper_);

        connect(p, &QWebEnginePage::loadFinished, this, &WebAppPage::onLoadFinished);

        webContainer_ = new QWidget;
        webContainer_->setStyleSheet(qsl("background:transparent;"));

        auto webContainerLayout = Utils::emptyHLayout();
        webContainerLayout->addWidget(webView_);
        webContainer_->setLayout(webContainerLayout);

        splitter_->addWidget(webContainer_);
        splitter_->setOpaqueResize(true);
        splitter_->setHandleWidth(splitterHandleWidth());
        splitter_->setMouseTracking(true);

        addToLayout(splitter_);

        connect(Ui::GetDispatcher(), &Ui::core_dispatcher::getUserProxy, this, &WebAppPage::reloadPage);
        connect(&helper_, &Helper::showInfo, this, &WebAppPage::showInfo);
        connect(&helper_, &Helper::createTask, this, &WebAppPage::createTask);
        // We use QueuedConnection so that the "Edit Task" menu closes immediately when the edit dialog opens.
        // Otherwise, the "Edit task" will close together with the editing dialog.
        connect(&helper_, &Helper::editTask, this, &WebAppPage::editTask, Qt::QueuedConnection);
        connect(&Utils::InterConnector::instance(), &Utils::InterConnector::sidebarVisibilityChanged, this, &WebAppPage::onSidebarVisibilityChanged);
        connect(Logic::getContactListModel(), &Logic::ContactListModel::selectedContactChanged, this, &WebAppPage::showMessengerPage);
        connect(GetDispatcher(), &core_dispatcher::taskCreated, this, &WebAppPage::onTaskCreated);
        connect(&Utils::InterConnector::instance(), &Utils::InterConnector::profileSettingsShow, this, &WebAppPage::onShowProfile);
        connect(&Utils::InterConnector::instance(), &Utils::InterConnector::closeAnySemitransparentWindow, this, [this](const Utils::CloseWindowInfo& _info)
            {
                if (_info.reason_ != Utils::CloseWindowInfo::Reason::Keep_Sidebar)
                    onSidebarVisibilityChanged(false);
                //set focus to close by esc button
                sidebar_->setFocus();
            });

        connect(&Utils::InterConnector::instance(), &Utils::InterConnector::closeSidebar, this, [this]()
            {
                onSidebarVisibilityChanged(false);
            });
    }

    void WebAppPage::showEvent(QShowEvent* _event)
    {
        if ((!url_.isEmpty() && !webView_) || urlChanged_)
            setUrlPrivate(url_);
        QWidget::showEvent(_event);
    }

    void WebAppPage::showInfo(const QString& _aimId, const InfoType _infoType)
    {
        aimId_ = _aimId;
        infoType_ = _infoType;

        if (_infoType == InfoType::thread)
        {
            Logic::getContactListModel()->markAsThread(_aimId, {});
            using namespace ServiceContacts;
            sidebar_->preparePage(Sidebar::getThreadPrefix() % _aimId, { true, FrameCountMode::_2, contactId(ContactType::Tasks) }, false);
        }
        else if (_infoType == InfoType::person)
        {
            sidebar_->preparePage(aimId_, { false, FrameCountMode::_2 }, false);
        }

        sidebar_->showAnimated();
        //if window is small show sidebar with semitransparent window
        updateSidebarPos(width());
        //set focus to close by esc button. Order is important. You need to do this last.
        sidebar_->setFocus();
    }

    void WebAppPage::refreshPageIfNeeded()
    {
        if (false)
        {
            webView_->hide();
            webView_->beginReload();
            webView_->reload();
        }
    }

    void WebAppPage::onSidebarVisibilityChanged(bool _visible)
    {
        if (_visible)
        {
            sidebar_->showAnimated();
        }
        else
        {
            sidebar_->hideAnimated();
            sidebar_->setNeedShadow(false);
            splitter_->refresh();
            webContainer_->show();
        }
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
        gd.showInCenter();
    }

    void WebAppPage::createTask()
    {
        im_assert(taskCreateId_ == -1);

        auto taskWidget = new TaskEditWidget();
        connect(taskWidget, &TaskEditWidget::createTask, this, &WebAppPage::tryCreateTask);
        showTaskDialog(taskWidget);
    }

    void WebAppPage::editTask(const QString& _taskId)
    {
        // need for correct display semi-transparent overlay
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

    void WebAppPage::onTaskCreated(qint64 _seq, int _error)
    {
        im_assert(_seq > 0);

        if (taskCreateId_ != _seq)
            return;

        taskCreateId_ = -1;

        const auto isError = (_error != 0);
        if (!isError)
        {
            refreshPageIfNeeded();
        }
    }

    void WebAppPage::tryCreateTask(qint64 _seq)
    {
        taskCreateId_ = _seq;
    }

    void WebAppPage::onLoadFinished(bool /*_ok*/)
    {
        webView_->show();
        webView_->endReload();
    }

    bool WebAppPage::isSideBarInSplitter() const
    {
        return splitter_->indexOf(sidebar_) != -1;
    }

    void WebAppPage::insertSidebarToSplitter()
    {
        if (!isSideBarInSplitter())
        {
            splitter_->addWidget(sidebar_);
            if (auto handle = splitter_->handle(splitter_->count() - 1))
                handle->setDisabled(true);
        }
        sidebar_->setNeedShadow(false);
    }

    void WebAppPage::takeSidebarFromSplitter()
    {
        sidebar_->setNeedShadow(true);
    }

    void WebAppPage::resizeEvent(QResizeEvent* _event)
    {
        //change the height of the sidebar even if it is not visible
        if (height() != _event->oldSize().height())
            sidebar_->changeHeight();

        //we need to resize only if sidebar is visible
        if (!sidebar_->isVisible())
            return;

        //set focus to close by esc button
        sidebar_->setFocus();

        //save focus for thread
        auto prevFocused = qobject_cast<QWidget*>(qApp->focusObject());

        if (const int newWidth = width(); width() != _event->oldSize().width())
        {
            updateSidebarPos(newWidth);
            updateSplitterStretchFactors();
        }

        QMetaObject::invokeMethod(this, [prevFocused, this]() { resizeUpdateFocus(prevFocused); }, Qt::QueuedConnection);
    }

    void WebAppPage::updateSidebarPos(int _width)
    {
        if (_width >= getFrame3MinWith())
        {
            insertSidebarToSplitter();
        }
        else
        {
            sidebar_->updateSize();
            sidebar_->setFrameCountMode(FrameCountMode::_2);
            takeSidebarFromSplitter();
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

        if (isSideBarInSplitter() || sidebar_->isAncestorOf(_prevFocused))
            _prevFocused->setFocus();
        else if (sidebar_->isThreadOpen())
            Q_EMIT Utils::InterConnector::instance().setFocusOnInput(sidebar_->currentAimId(Sidebar::ResolveThread::Yes));
    }

    void WebAppPage::onShowProfile(const QString& _aimId)
    {
        if (!isVisible() || _aimId.isEmpty())
            return;

        showInfo(_aimId, InfoType::person);
    }
}
