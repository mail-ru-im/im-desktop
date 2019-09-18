#include "stdafx.h"
#include "local_peer.h"
#include "../qt_overload.h"

#include <QtNetwork/QLocalSocket>
#include <QtNetwork/QLocalServer>

using msglen_t = quint32;

namespace
{
    constexpr std::chrono::milliseconds answerTimeout = std::chrono::seconds(2);
    const QString executeCmd = crossprocess_message_execute_url_command % ql1c(':');
    const QByteArray shutdownResponse = QByteArrayLiteral("icq");

    QString get_crossprocess_pipe_name()
    {
        if (build::is_biz())
            return crossprocess_pipe_name_biz;

        if (build::is_dit())
            return crossprocess_pipe_name_dit;

        if (build::is_agent())
            return crossprocess_pipe_name_agent;

        return crossprocess_pipe_name_icq;
    }
}

Q_LOGGING_CATEGORY(localPeer, "localPeer")

namespace Utils
{
    //----------------------------------------------------------------------
    // LocalConnection
    //----------------------------------------------------------------------

    LocalConnection::LocalConnection(QLocalSocket* _socket, const ConnPolicy _writePolicy)
        : QObject(nullptr)
        , socket_(nullptr)
        , bytesWritten_(0)
        , bytesToReceive_(-1)
        , bytesToSend_(0)
        , exitOnDisconnect_(false)
        , timer_(new QTimer(this))
    {
        assert(_socket);

        socket_ = _socket;
        socket_->setParent(this);

        connect(socket_, &QLocalSocket::disconnected, this, &LocalConnection::onDisconnected);
        connect(socket_, &QLocalSocket::readyRead, this, &LocalConnection::onReadyRead);
        connect(socket_, QOverload<QLocalSocket::LocalSocketError>::of(&QLocalSocket::error), this, [this](QLocalSocket::LocalSocketError _err)
        {
            qCDebug(localPeer) << this << "error" << _err << '-' << socket_->errorString();
            onError();
        });

        if (_writePolicy == ConnPolicy::DisconnectAfterWrite)
            connect(socket_, &QLocalSocket::bytesWritten, this, &LocalConnection::onBytesWritten);

        timer_->setInterval(answerTimeout.count());
        timer_->setSingleShot(true);
        connect(timer_, &QTimer::timeout, this, [this]()
        {
            qCDebug(localPeer) << this << "timed out";
            onError();
        });

        qCDebug(localPeer) << this << "created";
    }

    LocalConnection::~LocalConnection()
    {
        if (exitOnDisconnect_)
            QApplication::exit(0);

        qCDebug(localPeer) << this << "deleted";
    }

    void LocalConnection::connectToServer()
    {
        qCDebug(localPeer) << this << "going to connect";
        socket_->connectToServer(get_crossprocess_pipe_name());
    }

    void LocalConnection::disconnectFromServer()
    {
        qCDebug(localPeer) << this << "going to disconnect";
        socket_->disconnectFromServer();
    }

    void LocalConnection::connectAndSendCommand(const QString& _command)
    {
        connect(socket_, &QLocalSocket::connected, this, [_command, this]()
        {
            qCDebug(localPeer) << this << "connected";

            writeCommand(_command);
        });

        connectToServer();
        timer_->start();
    }

    void LocalConnection::onBytesWritten(const qint64 _bytes)
    {
        assert(_bytes > 0);

        bytesWritten_ += _bytes;

        if (bytesWritten_ >= bytesToSend_)
        {
            qCDebug(localPeer) << this << "all needed" << bytesWritten_ << "bytes written";
            disconnectFromServer();
        }
    };

    void LocalConnection::onReadyRead()
    {
        const auto avBytes = socket_->bytesAvailable();
        const auto needBytes = bytesToReceive_ == -1 ? sizeof(msglen_t) : bytesToReceive_;

        if (avBytes >= needBytes)
        {
            if (bytesToReceive_ == -1)
            {
                QDataStream ds(socket_);
                ds >> bytesToReceive_;

                if (avBytes == sizeof(msglen_t))
                    return;
            }

            qCDebug(localPeer) << this << "all needed" << needBytes << "bytes read";

            timer_->stop();
            emit dataReady(socket_->readAll());
        }
    }

    void LocalConnection::onDisconnected()
    {
        qCDebug(localPeer) << this << "disconnected";

        if (bytesWritten_ < bytesToSend_)
        {
            qCDebug(localPeer) << this << "error: disconnected before all needed" << bytesToSend_ << "bytes were written";
            emit error();
        }

        if (exitOnDisconnect_)
            QApplication::exit(0);

        emit disconnected();
        deleteLater();
    }

    void LocalConnection::onError()
    {
        emit error();
        disconnectFromServer();
    }

    void LocalConnection::writeRaw(const char* _data, const qint64 _len)
    {
        assert(_data && _len > 0);

        bytesToSend_ = _len;

        QDataStream ds(socket_);
        ds.writeRawData(_data, _len);
    }

    void LocalConnection::writeCommand(const QString& _command)
    {
        assert(!_command.isEmpty());

        const QByteArray message = _command.toUtf8();
        bytesToSend_ = message.size();

        QDataStream ds(socket_);
        ds.writeBytes(message.constData(), message.size());
    }

    void LocalConnection::setBytesToReceive(const int _bytes)
    {
        bytesToReceive_ = _bytes;
    }

    void LocalConnection::exitOnDisconnect()
    {
        exitOnDisconnect_ = true;
    }

    //----------------------------------------------------------------------
    // LocalPeer
    //----------------------------------------------------------------------

    LocalPeer::LocalPeer(QObject* _parent, bool _isServer)
        : QObject(_parent)
        , mainWindow_(nullptr)
        , server_(nullptr)
    {
        if (_isServer)
        {
            server_ = new QLocalServer(this);
            server_->setSocketOptions(QLocalServer::UserAccessOption);

            connect(server_, &QLocalServer::newConnection, this, &LocalPeer::receiveConnection);

            server_->listen(get_crossprocess_pipe_name());
        }
    }

    void LocalPeer::setMainWindow(QMainWindow* _wnd)
    {
        mainWindow_ = _wnd;
    }

    void LocalPeer::getProcessId()
    {
        auto conn = new LocalConnection(new QLocalSocket(), LocalConnection::ConnPolicy::KeepAlive);

        connect(conn, &LocalConnection::error, this, &LocalPeer::error);
        connect(conn, &LocalConnection::dataReady, this, [conn, this](const QByteArray& _data)
        {
            QDataStream ds(_data);
            unsigned int procId = 0;
            ds.readRawData((char*)&procId, sizeof(procId));

            emit processIdReceived(procId);

            conn->disconnectFromServer();
        });

        conn->setBytesToReceive(sizeof(unsigned int));
        conn->connectAndSendCommand(crossprocess_message_get_process_id);
    }

    void LocalPeer::getHwndAndActivate()
    {
        auto conn = new LocalConnection(new QLocalSocket(), LocalConnection::ConnPolicy::KeepAlive);

        connect(conn, &LocalConnection::error, this, &LocalPeer::error);
        connect(conn, &LocalConnection::dataReady, this, [conn, this](const QByteArray& _data)
        {
            QDataStream ds(_data);
            unsigned int hwnd = 0;
            ds.readRawData((char*)&hwnd, sizeof(hwnd));

            emit hwndReceived(hwnd);

            conn->disconnectFromServer();
        });

        conn->setBytesToReceive(sizeof(unsigned int));
        conn->connectAndSendCommand(crossprocess_message_get_hwnd_activate);
    }

    void LocalPeer::sendShutdown()
    {
        auto conn = new LocalConnection(new QLocalSocket(), LocalConnection::ConnPolicy::KeepAlive);

        connect(conn, &LocalConnection::error, this, &LocalPeer::error);
        connect(conn, &LocalConnection::dataReady, this, [conn, this](const QByteArray& _data)
        {
            if (_data == shutdownResponse)
                emit commandSent();

            conn->disconnectFromServer();
        });

        conn->setBytesToReceive(shutdownResponse.size());
        conn->connectAndSendCommand(crossprocess_message_shutdown_process);
    }

    void LocalPeer::sendUrlCommand(const QString& _command)
    {
        auto conn = new LocalConnection(new QLocalSocket());

        connect(conn, &LocalConnection::error, this, &LocalPeer::error);
        connect(conn, &LocalConnection::disconnected, this, &LocalPeer::commandSent);

        conn->connectAndSendCommand(executeCmd % _command);
    }

    void LocalPeer::processMessage(LocalConnection* _conn, const QString& _message)
    {
        if (_message == crossprocess_message_get_process_id)
        {
#ifdef _WIN32
            unsigned int process_id = ::GetCurrentProcessId();
            _conn->writeRaw((const char*)&process_id, sizeof(process_id));
#endif //_WIN32
        }
        else if (_message == crossprocess_message_get_hwnd_activate)
        {
            unsigned int hwnd = 0;
            if (mainWindow_)
            {
                hwnd = (unsigned int)mainWindow_->winId();
                emit needActivate();
            }
            _conn->writeRaw((const char*)&hwnd, sizeof(hwnd));
        }
        else if (_message.startsWith(executeCmd) && _message.length() > executeCmd.length())
        {
            QString url = _message.mid(executeCmd.length());
            emit schemeUrl(url);
        }
        else
        {
            if (_message == crossprocess_message_shutdown_process)
                _conn->exitOnDisconnect();

            _conn->writeRaw(shutdownResponse.constData(), shutdownResponse.size());
        }
    }

    void LocalPeer::receiveConnection()
    {
        auto socket = server_->nextPendingConnection();
        if (!socket)
            return;

        auto conn = new LocalConnection(socket);

        connect(conn, &LocalConnection::dataReady, this, [conn, this](const QByteArray& _data)
        {
            processMessage(conn, QString::fromUtf8(_data));
        });
    }
}
