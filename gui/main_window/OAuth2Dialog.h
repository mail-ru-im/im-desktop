#pragma once

namespace Ui
{

class OAuth2Dialog : public QDialog
{
    Q_OBJECT
public:
    OAuth2Dialog(QWidget* _parent, Qt::WindowFlags _flags = Qt::Dialog);
    ~OAuth2Dialog();

    void setClientId(const QString& _clientId);
    const QString& clientId() const;

    void setAuthUrl(const QString& _url);
    const QString& authUrl() const;

    void start();

private Q_SLOTS:
    void onUrlChanged(const QUrl& _url);
    void onLoadFinished(bool _success);

Q_SIGNALS:
    void authCodeReceived(const QString& _token, QPrivateSignal);
    void errorOccured(int _error, QPrivateSignal);
    void quitApp();

protected:
    void closeEvent(QCloseEvent* _event) override;
    void keyPressEvent(QKeyEvent* _event) override;

private:
    void emitError();

private:
    friend class OAuth2DialogPrivate;
    std::unique_ptr<class OAuth2DialogPrivate> d;
};

} // end namespace Ui
