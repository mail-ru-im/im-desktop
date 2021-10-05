#include "stdafx.h"

#include "../Spellchecker_p.h"
#include "../SpellcheckUtils.h"
#include "../../utils/macos/mac_support.h"

#import <AppKit/AppKit.h>

namespace spellcheck
{
    namespace platform
    {
        bool isAvailable()
        {
            return true;
        }

        namespace
        {
            NSSpellChecker *SharedSpellChecker()
            {
                @try
                {
                    return [NSSpellChecker sharedSpellChecker];
                }
                @catch (id exception)
                {
                    return nil;
                }
            }
        }

        class Details
        {
            [[nodiscard]] static NSString *toNSString(QStringView s)
            {
                return [NSString stringWithCharacters: reinterpret_cast<const UniChar*>(s.data()) length: s.size()];
            }

        public:

            [[nodiscard]] const std::vector<QString>& languages(const std::vector<QString>& _keyboardLanguages) const
            {
                if (_keyboardLanguages.empty())
                {
                    const static auto languages = utils::systemLanguages();
                    return languages;
                }

                return _keyboardLanguages;
            }

            [[nodiscard]]  const std::vector<QChar::Script>& scripts() const
            {
                const static auto scripts = utils::supportedScripts();
                return scripts;
            }

            [[nodiscard]] bool isWordSkippable(QStringView word) const
            {
                if (const auto& s = scripts(); std::none_of(s.begin(), s.end(), [sc = utils::wordScript(word) ](auto x) { return x == sc; }))
                    return true;

                return utils::isWordSkippable(word);
            }

            void addWord(const QString& word)
            {
                [SharedSpellChecker() learnWord: word.toNSString()];
            }

            void removeWord(const QString& word)
            {
                [SharedSpellChecker() unlearnWord: word.toNSString()];
            }

            void ignoreWord(const QString& word)
            {
                [SharedSpellChecker() ignoreWord: word.toNSString() inSpellDocumentWithTag: 0];
            }

            [[nodiscard]] bool containsWord(const QString& word) const
            {
                return [SharedSpellChecker() hasLearnedWord: word.toNSString()];
            }

            [[nodiscard]] bool hasMisspelling(QStringView word) const
            {
                if (word.isEmpty())
                    return false;
                const auto size = word.size();
                NSArray<NSTextCheckingResult *> *spellRanges =
                    [SharedSpellChecker()
                        checkString: toNSString(word)
                        range: NSMakeRange(0, size)
                        types: NSTextCheckingTypeSpelling
                        options: nil
                        inSpellDocumentWithTag: 0
                        orthography: nil
                        wordCount: nil];
                return spellRanges.count != 0;
            }

            [[nodiscard]] Misspellings checkText(QStringView text) const
            {
                return utils::checkText(text, [this](QStringView word)
                {
                    return !isWordSkippable(word) && hasMisspelling(word);
                });
            }

            [[nodiscard]] Suggests getSuggests(QStringView word, size_t maxCount, const std::vector<QString>& _keyboardLanguages) const
            {
                Suggests res;

                if (word.isEmpty())
                    return res;

                auto *nsWord = toNSString(word);
                auto guesses = [&nsWord, wordRange = NSMakeRange(0, word.size())](const auto& lang) {
                    return [SharedSpellChecker() guessesForWordRange:wordRange
                        inString: nsWord
                        language: toNSString(lang)
                        inSpellDocumentWithTag: 0];
                };

                size_t wordCounter = 0;
                const auto wordScript = utils::wordScript(word);

                for (const auto &lang : languages(_keyboardLanguages))
                {
                    if (wordScript != utils::localeToScriptCode(QStringRef(&lang)))
                        continue;
                    for (NSString *guess in guesses(lang))
                    {
                        res.push_back(QString::fromNSString(guess));
                        if (++wordCounter >= maxCount)
                            return res;
                    }
                }
                return res;
            }
        };
    }


    SpellCheckerImpl::SpellCheckerImpl()
    {
        const auto checker = platform::SharedSpellChecker();
        Q_UNUSED(checker);
    }

    SpellCheckerImpl::~SpellCheckerImpl() = default;

    void SpellCheckerImpl::init()
    {
        platform_ = std::make_unique<platform::Details>();
    }

    std::vector<QString> SpellCheckerImpl::languages() const
    {
        return platform_ ? platform_->languages(keyboardLanguages_) : std::vector<QString>{};
    }

    std::optional<Suggests> SpellCheckerImpl::getSuggests(const QString& word, size_t maxCount) const
    {
        if (platform_)
            return platform_->getSuggests(word, maxCount, keyboardLanguages_);
        return {};
    }

    void SpellCheckerImpl::addWord(const QString& word)
    {
        if (platform_)
            platform_->addWord(word);
    }

    void SpellCheckerImpl::removeWord(const QString& word)
    {
        if (platform_)
            platform_->removeWord(word);
    }

    void SpellCheckerImpl::ignoreWord(const QString& word)
    {
        if (platform_)
            platform_->ignoreWord(word);
    }

    bool SpellCheckerImpl::containsWord(const QString& word) const
    {
        return platform_ && platform_->containsWord(word);
    }

    bool SpellCheckerImpl::hasMisspelling(const QString& word) const
    {
        return platform_ && platform_->hasMisspelling(word);
    }

    std::optional<Misspellings> SpellCheckerImpl::checkText(const QString& text) const
    {
        if (platform_)
            return platform_->checkText(text);
        return {};
    }

    void SpellCheckerImpl::updateKeyboardLanguages()
    {
        keyboardLanguages_ = MacSupport::getKeyboardLanguages();
    }
}
