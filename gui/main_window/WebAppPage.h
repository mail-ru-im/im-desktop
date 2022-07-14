#pragma once

#include "Splitter.h"
#include "../utils/InterConnector.h"
#include "../controls/CustomButton.h"
#include "WebChannelProxy.h"


namespace Ui
{
    class Sidebar;
    class ProgressAnimation;
    class TextWidget;
    enum class FrameCountMode;
    enum class ConnectionState;

    enum class SidebarContentType
    {
        person,
        thread,
        task,
        threadInfo
    };

    enum class TabPage;
    enum PageState
    {
        load,
        content,
        error,
        unavailable
    };

    class ReloadButton : public RoundButton
    {
        Q_OBJECT

    public:
        explicit ReloadButton(QWidget* _parent = nullptr, int _radius = 0);
        ~ReloadButton();

        QSize minimumSizeHint() const override;
    };


    class WebView : public QWebEngineView
    {
        Q_OBJECT

    public:
        WebView(QWidget* _parent = nullptr);

        void setVisible(bool _visible) override;

        void beginReload();
        void endReload();

    private:
        QTimer showTimer_;
        bool visible_ = false;
        bool reloading_ = false;
    };

    class WebAppPage : public QWidget
    {
        Q_OBJECT

    Q_SIGNALS:
        void setTitle(const QString& _type, const QString& _title);
        void setLeftButton(const QString& _type, const QString& _text, const QString& _color, bool _enabled);
        void setRightButton(const QString& _type, const QString& _text, const QString& _color, bool _enabled);
        void pageIsHidden();

    public Q_SLOTS:
        void showInfo(const QString& _aimId, const SidebarContentType _infoType);
        void createTask();
        void onSetTitle(const QString& _title);
        void onSetLeftButton(const QString& _text, const QString& _color, bool _enabled);
        void onSetRightButton(const QString& _text, const QString& _color, bool _enabled);
        void editTask(const QString& _taskId);
        void forwardTask(const QString& _taskId);
        void onShowProfile(const QString& _aimId);
        void onSidebarVisibilityChanged(bool _visible);
        void showMessengerPage();
        void onTaskCreated(qint64 _seq, int _error);
        void reloadPage();
        void refreshPageIfConnectionStateOnline(const Ui::ConnectionState& _state);
        void headerLeftButtonClicked(const QString& _type);
        void headerRightButtonClicked(const QString& _type);

    public:
        explicit WebAppPage(const QString& _type, QWidget* _parent);

        const QString& type() const { return type_; }

        void setUrl(const QUrl& _url, bool _force = false);
        QUrl getUrl() const;

        bool isSideBarInSplitter() const;
        void insertSidebarToSplitter();
        void takeSidebarFromSplitter();
        void refreshPageIfNeeded();
        bool isSidebarVisible() const;

        void showThreadInfo(const QString& _aimId);
        void closeThreadInfo(const QString& _aimId);
        void showProfile(const QString& _aimId);
        void showProfileFromThreadInfo(const QString& _aimId);
        bool isProfileFromThreadInfo();
        bool isPageLoaded();
        void markUnavailable(bool _isUnavailable);
        bool isUnavailable() const;

        QString getAimId() const;

    private Q_SLOTS:
        void tryCreateTask(qint64 _seq);
        void onLoadFinished(bool _ok, int _err);// reaction to QWebEnginePage signal
        void onPageLoadFinished(bool _ok);// reaction to the event from the server
        void onLoadStarted();
        void onReloadPageTimeout();
        void downloadRequested(QWebEngineDownloadItem* _download);
        void onNativeClientFunctionCalled(const QString& _reqId, const QString& _name, const QJsonObject& _requestParams);
        void sendFunctionCallReply(const QJsonObject& _reply);
        void onUrlChanged(const QUrl& _url);

    protected:
        void resizeEvent(QResizeEvent* _event) override;
        void hideEvent(QHideEvent* _event) override;

    private:
        void saveSplitterState();
        void restoreSidebar();
        void updateSplitterStretchFactors();
        void updateSidebarPos(int _width);
        void updateSplitterHandle(int _width);
        void updateSidebarWidth(int _desiredWidth);
        void removeSidebarPlaceholder();
        void updatePageMaxWidth(bool _isSidebarVisible);
        void updateHeaderVisibility();

        void resizeUpdateFocus(QWidget* _prevFocused);
        bool isWidthOutlying(int _width) const;

        void loadPreparedUrl();
        void initWebEngine();
        void initWebPage();
        void refreshPage();

        void connectToPageSignals();
        void disconnectFromPageSignals();

#if QT_CONFIG(menu)
        QMenu* createContextMenu(const QPoint& _pos);
#endif

        QWidget* createWebContainer();
        QWidget* createLoadingContainer();
        QWidget* createErrorContainer(bool _unavailable);

        bool needsWebCache() const;


    protected:
        void showEvent(QShowEvent* _event) override;

    private:
        const QString type_;
        QUrl urlToSet_;

        QStackedWidget* stackedWidget_ = nullptr;
        ProgressAnimation* progressAnimation_ = nullptr;
        WebView* webView_ = nullptr;
        QWebEngineProfile* profile_ = nullptr;
        QWebChannel* webChannel_ = nullptr;
        QWebEnginePage* page_ = nullptr;
        Splitter* splitter_ = nullptr;
        Sidebar* sidebar_ = nullptr;
        QWidget* sidebarPlaceholderWidget_ = nullptr;
        QString aimId_;
        SidebarContentType infoType_;
        WebChannelProxy webChannelProxy_;
        QTimer reloadTimer_;
        std::vector<std::chrono::milliseconds> reloadIntervals_;
        QString activeColor_;
        QStringList activeDownloads_;
        TextWidget* unavailableLabel_ = nullptr;
        int64_t taskCreateId_ = -1;
        size_t reloadIntervalIdx_ = 0;
        bool needsWebCache_ = false;
        bool needForceChangeUrl_ = false;
        bool pageIsLoaded_ = false;
        bool showProfileFromThreadInfo_ = false;
        bool isUnavailable_ = false;
        bool isCustomApp_ = false;
    };

}
