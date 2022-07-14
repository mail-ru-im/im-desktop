#include "stdafx.h"
#include "OAuth2Dialog.h"

#include "../core/core.h"
#include "../utils/features.h"
#include "url_config.h"

namespace
{
    constexpr QStringView responseType() noexcept { return u"code"; }
    constexpr QSize kMinimalSize(512, 610);
    constexpr size_t kMaxRetryCount = 3;
}

namespace Ui
{

class OAuth2DialogPrivate
{
public:
    QString authUrl_;
    QString clientId_;
    QUrl redirectUrl_;
    QString scope_;
    QWebEngineView* view_;
    int result_;
    size_t retryCount_;

    OAuth2DialogPrivate()
        : view_(nullptr)
        , result_(QDialog::Rejected)
        , retryCount_(kMaxRetryCount)
    {}

    void init(OAuth2Dialog* _q)
    {
        // Change the default profile: we care about possble security
        // issues here. If that will cause errors or will no longer be
        // the case - this code can be safely removed.
        setupWebProfile(QWebEngineProfile::defaultProfile());

        view_ = new QWebEngineView(_q);
        view_->setContextMenuPolicy(Qt::NoContextMenu); // disable context menu
        for (int i = 0; i < QWebEnginePage::WebActionCount; ++i) // disable all web actions
            view_->pageAction((QWebEnginePage::WebAction)i)->setEnabled(false);

        setupWebProfile(view_->page()->profile());

        QObject::connect(view_, &QWebEngineView::urlChanged, _q, &OAuth2Dialog::onUrlChanged);
        QObject::connect(view_, &QWebEngineView::loadFinished, _q, &OAuth2Dialog::onLoadFinished);

        QVBoxLayout* layout = new QVBoxLayout(_q);
        layout->setContentsMargins(QMargins());
        layout->addWidget(view_);
    }

    void setupWebProfile(QWebEngineProfile* _profile)
    {
        _profile->setPersistentCookiesPolicy(QWebEngineProfile::NoPersistentCookies);
        _profile->setHttpCacheType(QWebEngineProfile::NoCache);
    }

    void cleanupWebProfile(QWebEngineProfile* _profile)
    {
        _profile->cookieStore()->deleteSessionCookies();
        _profile->cookieStore()->deleteAllCookies();
        _profile->clearAllVisitedLinks();
        _profile->clearHttpCache();
    }

    QUrl buildUrl() const
    {
        im_assert(!authUrl_.isEmpty());
        im_assert(!clientId_.isEmpty());
        im_assert(!redirectUrl_.isEmpty());
        im_assert(!scope_.isEmpty());

        QString url = authUrl_ % u'?' %
              u"client_id=" % clientId_ %
              u"&response_type=" % responseType() %
              u"&scope=" % scope_ %
              u"&redirect_uri=" % redirectUrl_.toString();

        return QUrl(url);
    }
};


OAuth2Dialog::OAuth2Dialog(QWidget* _parent, Qt::WindowFlags _flags)
    : QDialog(_parent, _flags)
    , d(std::make_unique<OAuth2DialogPrivate>())
{
    d->init(this);
    setMinimumSize(kMinimalSize);
}

OAuth2Dialog::~OAuth2Dialog()
{
    d->cleanupWebProfile(d->view_->page()->profile());

    // Cleanup the default profile: we care about possble security
    // issues here. If that will cause errors or will no longer be
    // the case - this code can be safely removed.
    d->cleanupWebProfile(QWebEngineProfile::defaultProfile());
}

void OAuth2Dialog::setClientId(const QString& _clientId)
{
    d->clientId_ = _clientId;
}

QString OAuth2Dialog::clientId() const
{
    return d->clientId_;
}

void OAuth2Dialog::setAuthUrl(const QString& _url)
{
    d->authUrl_ = _url;
}

QString OAuth2Dialog::authUrl() const
{
    return d->authUrl_;
}

void OAuth2Dialog::setRedirectUrl(const QString& _url)
{
    d->redirectUrl_ = _url;
}

QString OAuth2Dialog::redirectUrl() const
{
    return d->redirectUrl_.toString();
}

void OAuth2Dialog::setScope(const QString& _scope)
{
    d->scope_ = _scope;
}

QString OAuth2Dialog::scope() const
{
    return d->scope_;
}

void OAuth2Dialog::start()
{
    d->view_->load(d->buildUrl());
}

void OAuth2Dialog::onUrlChanged(const QUrl& _url)
{
    if (_url.host() == d->redirectUrl_.host())
    {
        QUrlQuery query(_url);
        QString code = query.queryItemValue(ql1s("code"));
        if (!code.isEmpty())
        {
            Q_EMIT authCodeReceived(code, QPrivateSignal{});
            d->result_ = QDialog::Accepted;
        }

        d->view_->triggerPageAction(QWebEnginePage::Stop);
        done(d->result_);
    }
}

void OAuth2Dialog::onLoadFinished(bool _success)
{
    if (_success)
    {
        d->retryCount_ = kMaxRetryCount;
        return;
    }

    if (!d->view_->url().isEmpty() || (--d->retryCount_ == 0))
    {
        emitError();
        return;
    }
    d->view_->triggerPageAction(QWebEnginePage::Reload);
}

void OAuth2Dialog::emitError()
{
    Q_EMIT errorOccured(core::le_network_error, QPrivateSignal{});
    close();
}

void OAuth2Dialog::closeEvent(QCloseEvent* _event)
{
    d->view_->triggerPageAction(QWebEnginePage::Stop);
    QDialog::closeEvent(_event);
}

void OAuth2Dialog::keyPressEvent(QKeyEvent* _event)
{
    if (_event->key() == Qt::Key_Q && _event->modifiers() == Qt::KeyboardModifier::ControlModifier)
    {
        Q_EMIT quitApp();
        d->view_->triggerPageAction(QWebEnginePage::Stop);
        close();
    }

    QDialog::keyPressEvent(_event);
}

} // end namespace Ui
