#include "stdafx.h"

#include "../Spellchecker_p.h"


namespace spellcheck
{
    namespace platform
    {
        bool isAvailable()
        {
            return false;
        }

        class Details{};
    }

    SpellCheckerImpl::SpellCheckerImpl() = default;
    SpellCheckerImpl::~SpellCheckerImpl() = default;

    void SpellCheckerImpl::init()
    {
    }

    std::vector<QString> SpellCheckerImpl::languages() const
    {
        return {};
    }

    std::optional<Suggests> SpellCheckerImpl::getSuggests(const QString&, size_t) const
    {
        return {};
    }

    void SpellCheckerImpl::addWord(const QString&)
    {
    }

    void SpellCheckerImpl::removeWord(const QString&)
    {
    }

    void SpellCheckerImpl::ignoreWord(const QString&)
    {
    }

    bool SpellCheckerImpl::containsWord(const QString&) const
    {
        return false;
    }

    bool SpellCheckerImpl::hasMisspelling(const QString&) const
    {
        return false;
    }

    std::optional<Misspellings> SpellCheckerImpl::checkText(const QString&) const
    {
        return {};
    }

    void SpellCheckerImpl::updateKeyboardLanguages()
    {
    }
}
