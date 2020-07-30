#pragma once

#include "Types.h"

namespace spellcheck
{
    class ActualGuard
    {
        bool isActual_ = true;
        bool hasError_ = false;
        std::function<void(bool)> onResult_;
    public:
        ActualGuard(std::function<void(bool)> f) : onResult_(std::move(f)) {}
        ~ActualGuard() { if (onResult_) onResult_(hasError()); }
        void ignore() noexcept { if (isActual_) isActual_ = false; };
        bool isActual() const noexcept { return isActual_; }
        void setError() { if (!hasError_) hasError_ = true; }
        bool hasError() const noexcept { return hasError_; }
    };

    class SpellCheckerImpl;
    class Spellchecker : public QObject
    {
        Q_OBJECT

    Q_SIGNALS:
        void runInWorkerThread(std::function<void()>, QPrivateSignal) const;
        void dictionaryChanged(QPrivateSignal);
    public:
        ~Spellchecker();
        static Spellchecker& instance();
        static bool isAvailable();
        static bool isWordSkippable(QStringView word);

        void languages(std::function<void(std::vector<QString>)>) const;
        void getSuggests(const QString& word, std::function<void(std::optional<Suggests>)>) const;
        void getSuggestsIfNeeded(const QString& word, std::function<void(std::optional<Suggests>, WordStatus)>) const;
        void addWord(const QString& word, std::function<void()> = {});
        void removeWord(const QString& word, std::function<void()> = {});
        void ignoreWord(const QString& word, std::function<void()> = {});
        void containsWord(const QString& word, std::function<void(bool)>) const;
        void hasMisspelling(const QString& word, std::function<void(bool)>) const;
        void checkText(const QString& text, std::function<void(std::optional<Misspellings>)>) const;

    private:
        Spellchecker();

        SpellCheckerImpl* impl_ = nullptr;
        QThread* workerThread_ = nullptr;
    };
}
