#include "stdafx.h"

#include "WebPage.h"
#include "app_config.h"
#include "url_config.h"
#include "../../common.shared/config/config.h"
#include "../utils/WebUtils.h"
#include "../utils/InterConnector.h"
#include "../main_window/MainWindow.h"

namespace
{
    constexpr std::chrono::milliseconds UNLOAD_TIMEOUT = std::chrono::minutes(1);

    enum FileChooserMode
    {
        Open,
        OpenMultiple,
        UploadFolder,
        Save
    };

    QString codeCallFunction() noexcept
    {
        return qsl("\n"
                   "let processingEvents = {};\n"
                   "new QWebChannel(qt.webChannelTransport, function(channel) {\n"
                   "    window.addEventListener(\"message\", function(evt) {\n"
                   "        processingEvents[evt.data.data.reqId] = evt;\n"
                   "        channel.objects.client.postEvent(evt.data);\n"
                   "    });\n"
                   "});\n");
    }

    QString sendFunctionCallReplyJs() noexcept
    {
        return qsl("window.sendFunctionCallReply");
    }

    QString codeReply() noexcept
    {
        return QChar::LineFeed % sendFunctionCallReplyJs() %
            qsl(" = replyData => {\n"
                "    processingEvents[replyData.reqId].source.postMessage({\n"
                "        type: \"functionCallReply\",\n"
                "        data: replyData,\n"
                "    }, processingEvents[replyData.reqId].origin);\n"
                "    delete processingEvents[replyData.reqId];\n"
                "}\n");
    }
}

namespace Ui
{
    WebPage::WebPage(QWebEngineProfile* _profile, Utils::OpenUrlConfirm _confirm, QWidget* _parent)
        : QWebEnginePage(_profile, _parent)
        , unloadTimer_(new QTimer(this))
        , type_(_confirm)
    {
        connect(this, &QWebEnginePage::loadStarted, this, [this]()
            {
                startTime_ = std::chrono::system_clock::now();
                WebUtils::writeLog(url().toString(), qsl("load started"));
            });

        connect(this, &QWebEnginePage::loadFinished, this, [this](bool _ok, int _err)
            {
                const auto loading = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now() - startTime_);
                auto str = _ok ? qsl("load finished") : qsl("load failed");
                str += qsl("; code %1; loading time: %2 ms").arg(_err).arg(loading.count());
                WebUtils::writeLog(url().toString(), str);
            });

        connect(this, &QWebEnginePage::linkHovered, this, &WebPage::onLinkHovered);

        unloadTimer_->setSingleShot(true);
        unloadTimer_->setInterval(UNLOAD_TIMEOUT);
        connect(unloadTimer_, &QTimer::timeout, this, &WebPage::unload);

        settings()->setAttribute(QWebEngineSettings::ErrorPageEnabled, false);
    }

    void WebPage::initializeJsBridge()
    {
        QFile jsFile(qsl("://resources/qwebchannel.js"));
        if (!jsFile.open(QIODevice::ReadOnly))
        {
            im_assert(!"Couldn't load Qt's QWebChannel API!");
            return;
        }
        QTextStream stream(&jsFile);
        QString webChannelJs = stream.readAll();

        webChannelJs += codeCallFunction();
        webChannelJs += codeReply();

        QWebEngineScript bridgeScript;
        bridgeScript.setSourceCode(webChannelJs);
        bridgeScript.setName(qsl("qwebchannel.js"));
        bridgeScript.setInjectionPoint(QWebEngineScript::DocumentCreation);
        bridgeScript.setWorldId(QWebEngineScript::MainWorld);
        bridgeScript.setRunsOnSubFrames(false);
        scripts().insert(bridgeScript);
    }

    void WebPage::sendFunctionCallReply(const QJsonObject& _reply)
    {
        im_assert(!_reply.isEmpty());
        const QString jsReply = sendFunctionCallReplyJs() % u'(' % QString::fromUtf8(QJsonDocument(_reply).toJson(QJsonDocument::Compact)) % u')';
        runJavaScript(jsReply);
    }

    bool WebPage::certificateError(const QWebEngineCertificateError& _error)
    {
        if (!Ui::GetAppConfig().IsSslVerificationEnabled())
            return true; // discard SSL error

        return QWebEnginePage::certificateError(_error);
    }

    void WebPage::javaScriptConsoleMessage(JavaScriptConsoleMessageLevel, const QString& _message, int, const QString& _sourceID)
    {
        WebUtils::writeLog(_sourceID, _message);
    }

    QWebEnginePage* WebPage::createWindow(QWebEnginePage::WebWindowType _type)
    {
        openHoveredUrl();
        return resetBackgroundPage();
    }

    void WebPage::onLinkHovered(const QString& _url)
    {
        hoveredUrl_ = _url;
    }

    void WebPage::unload()
    {
        if (backgroundPage_)
            backgroundPage_->deleteLater();

        printer_.reset();
    }

    void WebPage::printRequested()
    {
        if (!backgroundPage_)
            return;

        unloadTimer_->stop();

        printer_ = std::make_unique<QPrinter>();
        QPrintDialog* dialog = new QPrintDialog(printer_.get());
        dialog->setWindowTitle(QT_TRANSLATE_NOOP("web_app", "Print"));
        if (dialog->exec() == QDialog::Accepted)
        {
            backgroundPage_->print(printer_.get(), [guard = QPointer(this)](bool)
            {
                if (guard)
                    guard->unload();
            });
        }
        else
        {
            unload();
        }
    }

    QStringList WebPage::chooseFiles(FileSelectionMode _mode, const QStringList& _oldFiles, const QStringList& _acceptedMimeTypes)
    {
        Q_UNUSED(_acceptedMimeTypes);
        QWidget* parent = Utils::InterConnector::instance().getMainWindow();
        QStringList ret;
        QString str;
        switch (static_cast<FileChooserMode>(_mode))
        {
        case FileChooserMode::OpenMultiple:
            ret = QFileDialog::getOpenFileNames(parent, QString());
            break;
            // Chromium extension, not exposed as part of the public API for now.
        case FileChooserMode::UploadFolder:
            str = QFileDialog::getExistingDirectory(parent, tr("Select folder to upload"));
            if (!str.isNull())
                ret << str;
            break;
        case FileChooserMode::Save:
            str = QFileDialog::getSaveFileName(parent, QString(), (QStandardPaths::writableLocation(QStandardPaths::DownloadLocation) + _oldFiles.first()));
            if (!str.isNull())
                ret << str;
            break;
        case FileChooserMode::Open:
            str = QFileDialog::getOpenFileName(parent, QString(), _oldFiles.first());
            if (!str.isNull())
                ret << str;
            break;
        }
        return ret;
    }

    void WebPage::openHoveredUrl()
    {
        if (!hoveredUrl_.isEmpty())
        {
            WebUtils::writeLog(url().toString(), qsl("open external url: ") % hoveredUrl_);

            QString finalUrl;

            // check potential redirects
            QUrl url(hoveredUrl_);
            if (url.hasQuery())
            {
                const auto tryReplaceInternal = [](const QString& _url, QString& _out) -> bool
                {
                    if (Utils::isInternalScheme(_url) || _url.contains(getUrlConfig().getBase())
                           || Utils::isUrlVCS(_url) || _url.contains(getUrlConfig().getUrlStickerShare()))
                    {
                        _out = _url;
                        return true;
                    }

                    return false;
                };

                for (const auto& [_, item] : QUrlQuery(url.query()).queryItems())
                {
                    QUrl urlRedir = QUrl::fromUserInput(QUrl::fromPercentEncoding(item.toLatin1()));
                    if (urlRedir.hasQuery())
                    {
                        for (const auto& [_, link] : QUrlQuery(urlRedir.query()).queryItems())
                        {
                            const auto normalizedUrl = QUrl::fromUserInput(QUrl::fromPercentEncoding(link.toLatin1())).toString();
                            if (tryReplaceInternal(normalizedUrl, finalUrl))
                                break;
                        }
                    }
                    else if (tryReplaceInternal(urlRedir.toString(), finalUrl))
                        break;

                    if (!finalUrl.isEmpty())
                        break;
                }
            }

            if (finalUrl.isEmpty())
                finalUrl = hoveredUrl_;

            //do not warn about clicking on an external link from the Mail tab
            Utils::openUrl(finalUrl, type_);
        }

    }

    QWebEnginePage* WebPage::resetBackgroundPage()
    {
        unload();

        backgroundPage_ = new QWebEnginePage(this);
        connect(backgroundPage_, &QWebEnginePage::printRequested, this, &WebPage::printRequested);

        unloadTimer_->start();
        return backgroundPage_;
    }
}
