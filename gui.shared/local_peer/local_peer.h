#pragma once

class QLocalServer;
class QLocalSocket;

Q_DECLARE_LOGGING_CATEGORY(localPeer)

namespace Utils
{
    class LocalConnection : public QObject
    {
        Q_OBJECT

    Q_SIGNALS:
        void dataReady(const QByteArray& _data);
        void disconnected();
        void error();

    public Q_SLOTS:
        void onBytesWritten(const qint64 _bytes);

    private Q_SLOTS:
        void onReadyRead();
        void onDisconnected();
        void onError();

    public:
        enum class ConnPolicy
        {
            DisconnectAfterWrite,
            KeepAlive
        };

        LocalConnection(QLocalSocket* _socket, const ConnPolicy _writePolicy = ConnPolicy::DisconnectAfterWrite);
        ~LocalConnection();

        void connectToServer();
        void disconnectFromServer();

        void connectAndSendCommand(const QString& _command);

        void writeRaw(const char* _data, const qint64 _len);
        void writeCommand(const QString& _command);

        void setBytesToReceive(const int _bytes);
        void exitOnDisconnect();

    private:
        QLocalSocket* socket_;
        qint64 bytesWritten_;

        int bytesToReceive_;
        int bytesToSend_;

        bool exitOnDisconnect_;
        QTimer* timer_;
    };

    class LocalPeer : public QObject
    {
        Q_OBJECT

    Q_SIGNALS:
        void commandSent();
        void hwndReceived(unsigned int _hwnd);
        void processIdReceived(unsigned int _processId);
        void schemeUrl(const QString& _url);
        void needActivate();
        void error();

    private Q_SLOTS:
        void receiveConnection();

    public:
        LocalPeer(QObject* _parent = nullptr, bool _isServer = false);

        void setMainWindow(QMainWindow* _wnd);

        void getProcessId();
        void getHwndAndActivate();
        void sendShutdown();
        void sendUrlCommand(const QString& _command);

    private:
        void processMessage(LocalConnection* _conn, const QString& _message);

    private:
        QMainWindow* mainWindow_;

        QLocalServer* server_;
    };
}

