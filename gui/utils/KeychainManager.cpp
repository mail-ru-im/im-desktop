#include "stdafx.h"
#include "keychainmanager.h"
#include <keychain.h>

namespace
{

template<class _Job>
_Job* createJob()
{
    _Job* job = new _Job(KeychainManager::serviceName(), &KeychainManager::instance());
    job->setAutoDelete(true);
    job->setInsecureFallback(false);
    return job;
}

}

KeychainManager::KeychainManager() : QObject(nullptr)
{
}

KeychainManager::~KeychainManager()
{
}

KeychainManager& KeychainManager::instance()
{
    static KeychainManager globalInstance;
    return globalInstance;
}

QString KeychainManager::serviceName()
{
    return qsl("im-desktop");
}

void KeychainManager::save(const QString& _key, const QByteArray& _bytes)
{
    using QKeychain::WritePasswordJob;
    using QKeychain::Job;
    auto job = createJob<WritePasswordJob>();
    connect(job, &Job::finished, this, [this](Job* _job)
    {
        if (_job->error())
            Q_EMIT error(_job->key(), _job->error(), _job->errorString());
        else
            Q_EMIT saved(_job->key());
    });

    job->setKey(_key);
    job->setBinaryData(_bytes);
    job->start();
}

void KeychainManager::save(const QString& _key, const QString& _text)
{
    using QKeychain::WritePasswordJob;
    using QKeychain::Job;
    auto job = createJob<WritePasswordJob>();
    connect(job, &Job::finished, this, [this](Job* _job)
    {
        if (_job->error())
            Q_EMIT error(_job->key(), _job->error(), _job->errorString());
        else
            Q_EMIT saved(_job->key());
    });

    job->setKey(_key);
    job->setTextData(_text);
    job->start();
}

void KeychainManager::load(const QString& _key)
{
    using QKeychain::ReadPasswordJob;
    using QKeychain::Job;
    auto job = createJob<ReadPasswordJob>();
    connect(job, &Job::finished, this, [this](Job* _job)
    {
        if (_job->error())
            Q_EMIT error(_job->key(), _job->error(), _job->errorString());
        else
            Q_EMIT loaded(_job->key(), static_cast<ReadPasswordJob*>(_job)->binaryData());
    });

    job->setKey(_key);
    job->start();
}

void KeychainManager::remove(const QString &_key)
{
    using QKeychain::DeletePasswordJob;
    using QKeychain::Job;
    auto job = createJob<DeletePasswordJob>();
    connect(job, &Job::finished, this, [this](Job* _job)
    {
        if (_job->error())
            Q_EMIT error(_job->key(), _job->error(), _job->errorString());
        else
            Q_EMIT removed(_job->key());
    });

    job->setKey(_key);
    job->start();
}
