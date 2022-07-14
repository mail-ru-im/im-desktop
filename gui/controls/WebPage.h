#pragma once

namespace Utils
{
    enum class OpenUrlConfirm;
}

namespace Ui
{
    class WebPage : public QWebEnginePage
    {
        Q_OBJECT

    public:
        explicit WebPage(QWebEngineProfile* _profile, Utils::OpenUrlConfirm _confirm, QWidget* _parent = nullptr);
        void initializeJsBridge();
        void sendFunctionCallReply(const QJsonObject& _reply);

    protected:
        bool certificateError(const QWebEngineCertificateError&) override;
        void javaScriptConsoleMessage(JavaScriptConsoleMessageLevel _level, const QString& _message, int _lineNumber, const QString& _sourceID) override;
        QWebEnginePage* createWindow(WebWindowType) override;
        QStringList chooseFiles(FileSelectionMode _mode, const QStringList& _oldFiles, const QStringList& _acceptedMimeTypes) override;

    private Q_SLOTS:
        void onLinkHovered(const QString& _url);
        void unload();
        void printRequested();

    private:
        void openHoveredUrl();
        QWebEnginePage* resetBackgroundPage();

    private:
        QString hoveredUrl_;
        QPointer<QWebEnginePage> backgroundPage_;
        std::unique_ptr<QPrinter> printer_;
        QTimer* unloadTimer_;
        Utils::OpenUrlConfirm type_;
        std::chrono::system_clock::time_point startTime_;
    };
}
