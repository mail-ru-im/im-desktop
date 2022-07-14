#pragma once

class KeychainManager final : public QObject
{
    Q_OBJECT
    Q_DISABLE_COPY_MOVE(KeychainManager)

private:
    KeychainManager();
    ~KeychainManager();

public:
    static KeychainManager& instance();

    static QString serviceName();

public Q_SLOTS:
    void save(const QString& _key, const QByteArray& _bytes);
    void save(const QString& _key, const QString& _text);

    void load(const QString& _key);
    void remove(const QString& _key);

Q_SIGNALS:
    void loaded(const QString& _key, const QByteArray& _bytes);
    void saved(const QString& _key);
    void removed(const QString& _key);
    void error(const QString& _key, int _errorCode, const QString& _errorText);

};
