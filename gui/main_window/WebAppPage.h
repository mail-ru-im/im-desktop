#pragma once

#include "Splitter.h"
#include "../utils/InterConnector.h"


namespace Ui
{
    class Sidebar;
    enum class AppPageType;

    enum class InfoType
    {
        person,
        thread
    };

    enum class TabPage;
    enum class FrameCountMode;

    class Helper : public QObject
    {
        Q_OBJECT

    Q_SIGNALS:
        void showInfo(const QString& _aimId, const InfoType _infoType);
        void createTask();
        void editTask(const QString& _taskId);

    public:
        explicit Helper(QObject* _parent = nullptr)
            : QObject(_parent)
        {
        }

    public Q_SLOTS:
        void postEvent(const QJsonObject& _action);
    };

    class WebView : public QWebEngineView
    {
        Q_OBJECT

    public:
        WebView(QWidget* _parent = nullptr);
        virtual ~WebView();

        virtual void setVisible(bool _visible) override;

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

    public Q_SLOTS:
        void showInfo(const QString& _aimId, const InfoType _infoType);
        void createTask();
        void editTask(const QString& _taskId);
        void onShowProfile(const QString& _aimId);
        void onSidebarVisibilityChanged(bool _visible);
        void showMessengerPage();
        void onTaskCreated(qint64 _seq, int _error);
        void reloadPage();

    public:
        explicit WebAppPage(AppPageType _type, QWidget* _parent);
        ~WebAppPage();

        virtual void setUrl(const QUrl& _url);
        virtual QUrl getUrl() const;

        static QUrl webAppUrl(AppPageType _type);
        static QString webAppId(AppPageType _type);

        bool isSideBarInSplitter() const;
        void insertSidebarToSplitter();
        void takeSidebarFromSplitter();
        void refreshPageIfNeeded();

    private Q_SLOTS:
        void tryCreateTask(qint64 _seq);
        void onLoadFinished(bool _ok);

    protected:
        void resizeEvent(QResizeEvent* _event) override;

    private:
        void updateSplitterStretchFactors();
        void updateSidebarPos(int _width);
        void resizeUpdateFocus(QWidget* _prevFocused);

    private:
        void setUrlPrivate(const QUrl& _url);
        void initWebEngine();

    protected:
        void showEvent(QShowEvent* _event) override;

    private:
        AppPageType type_;
        QUrl url_;

        QWidget* webContainer_;
        WebView* webView_ = nullptr;
        QWebChannel* webChannel_;
        Splitter* splitter_;
        Sidebar* sidebar_;
        QString aimId_;
        InfoType infoType_;
        int64_t taskCreateId_ = -1;
        Helper helper_;
        bool urlChanged_ = false;
    };

}