#include "stdafx.h"
#include "Spellchecker.h"

#include "Spellchecker_p.h"
#include "SpellcheckUtils.h"

#include "../utils/utils.h"
#include "../utils/features.h"
#include "../utils/async/AsyncTask.h"

namespace
{
    size_t maxSuggestsCount()
    {
        return Features::spellCheckMaxSuggestCount();
    }
}

namespace spellcheck
{
    Spellchecker::Spellchecker()
    {
        workerThread_ = new QThread;
        impl_ = new SpellCheckerImpl;
        impl_->moveToThread(workerThread_);

        QObject::connect(workerThread_, &QThread::started, impl_, [impl_ = impl_]()
        {
            impl_->init();
        });

        workerThread_->start();

        QObject::connect(this, &Spellchecker::runInWorkerThread, impl_, [](std::function<void()> f)
        {
            Utils::ensureNonMainThread();
            if (f)
                f();
        });
    }

    Spellchecker::~Spellchecker()
    {
        workerThread_->quit();
        impl_->deleteLater();
        workerThread_->deleteLater();
    }

    Spellchecker& Spellchecker::instance()
    {
        static Spellchecker s;
        return s;
    }

    bool Spellchecker::isAvailable()
    {
        return platform::isAvailable();
    }

    bool Spellchecker::isWordSkippable(QStringView word)
    {
        return utils::isWordSkippable(word);
    }

    void Spellchecker::languages(std::function<void(std::vector<QString>)> f) const
    {
        Q_EMIT runInWorkerThread([f = std::move(f), weakThis = QPointer(this), weakImpl = QPointer(impl_)]() mutable
        {
            if (!weakThis || !weakImpl)
                return;
            Async::runInMain([f = std::move(f), res = weakImpl->languages()]() mutable
            {
                if (f)
                    f(std::move(res));
            });

        }, QPrivateSignal());
    }

    void Spellchecker::getSuggests(const QString& word, std::function<void(std::optional<Suggests>)> f) const
    {
        Q_EMIT runInWorkerThread([word, f = std::move(f), weakThis = QPointer(this), weakImpl = QPointer(impl_)]() mutable
        {
            if (!weakThis || !weakImpl)
                return;
            Async::runInMain([f = std::move(f), res = weakImpl->getSuggests(word, maxSuggestsCount())]() mutable
            {
                if (f)
                    f(std::move(res));
            });

        }, QPrivateSignal());
    }

    void Spellchecker::getSuggestsIfNeeded(const QString& word, std::function<void(std::optional<Suggests>, WordStatus)> f) const
    {
        Q_EMIT runInWorkerThread([word, f = std::move(f), weakThis = QPointer(this), weakImpl = QPointer(impl_)]() mutable
        {
            if (!weakThis || !weakImpl)
                return;

            auto checkResult = weakImpl->hasMisspelling(word);
            WordStatus status = {};
            status.isCorrect = !checkResult;
            status.isAdded = status.isCorrect && weakImpl->containsWord(word);

            Async::runInMain([f = std::move(f), res = checkResult ? weakImpl->getSuggests(word, maxSuggestsCount()) : std::nullopt, status]() mutable
            {
                if (f)
                    f(std::move(res), status);
            });

        }, QPrivateSignal());
    }

    void Spellchecker::addWord(const QString& word, std::function<void()> f)
    {
        Q_EMIT runInWorkerThread([word, f = std::move(f), weakThis = QPointer(this), weakImpl = QPointer(impl_)]() mutable
        {
            if (!weakThis || !weakImpl)
                return;
            weakImpl->addWord(word);
            Q_EMIT weakThis->dictionaryChanged(QPrivateSignal());
            if (f)
            {
                Async::runInMain([f = std::move(f)]() mutable
                {
                    f();
                });
            }

        }, QPrivateSignal());
    }

    void Spellchecker::removeWord(const QString& word, std::function<void()> f)
    {
        Q_EMIT runInWorkerThread([word, f = std::move(f), weakThis = QPointer(this), weakImpl = QPointer(impl_)]() mutable
        {
            if (!weakThis || !weakImpl)
                return;
            weakImpl->removeWord(word);
            Q_EMIT weakThis->dictionaryChanged(QPrivateSignal());
            if (f)
            {
                Async::runInMain([f = std::move(f)]() mutable
                {
                    f();
                });
            }

        }, QPrivateSignal());
    }

    void Spellchecker::ignoreWord(const QString& word, std::function<void()> f)
    {
        Q_EMIT runInWorkerThread([word, f = std::move(f), weakThis = QPointer(this), weakImpl = QPointer(impl_)]() mutable
        {
            if (!weakThis || !weakImpl)
                return;
            weakImpl->ignoreWord(word);
            Q_EMIT weakThis->dictionaryChanged(QPrivateSignal());
            if (f)
            {
                Async::runInMain([f = std::move(f)]() mutable
                {
                    f();
                });
            }

        }, QPrivateSignal());
    }

    void Spellchecker::containsWord(const QString& word, std::function<void(bool)> f) const
    {
        Q_EMIT runInWorkerThread([word, f = std::move(f), weakThis = QPointer(this), weakImpl = QPointer(impl_)]() mutable
        {
            if (!weakThis || !weakImpl)
                return;

            Async::runInMain([f = std::move(f), res = weakImpl->containsWord(word)]() mutable
            {
                if (f)
                    f(std::move(res));
            });

        }, QPrivateSignal());
    }

    void Spellchecker::hasMisspelling(const QString& word, std::function<void(bool)> f) const
    {
        Q_EMIT runInWorkerThread([word, f = std::move(f), weakThis = QPointer(this), weakImpl = QPointer(impl_)]() mutable
        {
            if (!weakThis || !weakImpl)
                return;

            Async::runInMain([f = std::move(f), res = weakImpl->hasMisspelling(word)]() mutable
            {
                if (f)
                    f(std::move(res));
            });

        }, QPrivateSignal());
    }

    void Spellchecker::checkText(const QString& text, std::function<void(std::optional<Misspellings>)> f) const
    {
        Q_EMIT runInWorkerThread([text, f = std::move(f), weakThis = QPointer(this), weakImpl = QPointer(impl_)]() mutable
        {
            if (!weakThis || !weakImpl)
                return;

            Async::runInMain([f = std::move(f), res = weakImpl->checkText(text)]() mutable
            {
                if (f)
                    f(std::move(res));
            });

        }, QPrivateSignal());
    }
}
