#pragma once

namespace Ui
{
    class WebPage : public QWebEnginePage
    {
        Q_OBJECT

    public:
        explicit WebPage(QWidget* parent = nullptr);

    protected:
        bool certificateError(const QWebEngineCertificateError&) override;
        void javaScriptConsoleMessage(JavaScriptConsoleMessageLevel level, const QString& message, int lineNumber, const QString& sourceID) override;
        bool acceptNavigationRequest(const QUrl& _url, NavigationType _type, bool _isMainFrame) override;
        QWebEnginePage* createWindow(WebWindowType) override;

    private Q_SLOTS:
        void onLinkHovered(const QString& _url);

    private:
        QString hoveredUrl_;
    };
}