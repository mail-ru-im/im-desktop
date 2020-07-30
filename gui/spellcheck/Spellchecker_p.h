#pragma once

#include "Types.h"

namespace spellcheck
{
    namespace platform
    {
        class Details;
        bool isAvailable();
    }

    class SpellCheckerImpl : public QObject
    {
        Q_OBJECT
    public:
        SpellCheckerImpl();
        ~SpellCheckerImpl();

        std::vector<QString> languages() const;
        std::optional<Suggests> getSuggests(const QString& word, size_t maxCount) const;
        void addWord(const QString& word);
        void removeWord(const QString& word);
        void ignoreWord(const QString& word);
        bool containsWord(const QString& word) const;
        bool hasMisspelling(const QString& word) const;
        std::optional<Misspellings> checkText(const QString& text) const;

        void init();

    private:
        std::unique_ptr<platform::Details> platform_;
    };
}
