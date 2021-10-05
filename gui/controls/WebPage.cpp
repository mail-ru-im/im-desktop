#include "stdafx.h"

#include "WebPage.h"
#include "app_config.h"
#include "../../common.shared/config/config.h"
#include "../utils/log/log.h"

namespace
{
    void writeLog(const QString& _url, const QString& _message)
    {
        auto dispatcher = Ui::GetDispatcher();
        core::coll_helper helper(dispatcher->create_collection(), true);
        helper.set<QString>("source", _url);
        helper.set<QString>("message", _message);
        dispatcher->post_message_to_core("web_log", helper.get());

        QString msg = qsl("source: ") % _url % QChar::LineFeed % qsl("message: ") % _message;
        Log::trace(qsl("web_page"), std::move(msg));
    }
}

namespace Ui
{
    WebPage::WebPage(QWidget* parent)
        : QWebEnginePage(parent)
    {
        connect(this, &QWebEnginePage::loadStarted, this, [this]()
        {
            writeLog(url().toString(), qsl("load started"));
        });

        connect(this, &QWebEnginePage::loadFinished, this, [this](bool _ok)
        {
            writeLog(url().toString(), _ok ? qsl("load finished") : qsl("load failed"));
        });

        connect(this, &QWebEnginePage::linkHovered, this, &WebPage::onLinkHovered);
    }

    bool WebPage::certificateError(const QWebEngineCertificateError& _error)
    {
        if (!Ui::GetAppConfig().IsSslVerificationEnabled())
            return true; // discard SSL error

        return QWebEnginePage::certificateError(_error);
    }

    void WebPage::javaScriptConsoleMessage(JavaScriptConsoleMessageLevel level, const QString& message, int lineNumber, const QString& sourceID)
    {
        writeLog(sourceID, message);
    }

    bool WebPage::acceptNavigationRequest(const QUrl& _url, NavigationType _type, bool _isMainFrame)
    {
        if (_type == QWebEnginePage::NavigationTypeLinkClicked)
            return false;

        return QWebEnginePage::acceptNavigationRequest(_url, _type, _isMainFrame);
    }

    QWebEnginePage* WebPage::createWindow(QWebEnginePage::WebWindowType _type)
    {
        Utils::openUrl(hoveredUrl_);
        return nullptr;
    }

    void WebPage::onLinkHovered(const QString& _url)
    {
        hoveredUrl_ = _url;
    }
}