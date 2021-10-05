#include "stdafx.h"
#include "../Spellchecker_p.h"

#include <wrl/client.h>
#define MIN_SPELLING_NTDDI 0
#include <spellcheck.h>

using namespace Microsoft::WRL;

namespace spellcheck
{
    static LPCWSTR Q2WString(const QString& s)
    {
        return LPCWSTR(s.utf16());
    }

    namespace platform
    {
        bool isAvailable()
        {
            const static bool isAvailable = IsWindows8OrGreater();
            return isAvailable;
        }

        static bool isFarsi(QStringView lang) // MsSpellCheckingFacility.dll crashes with 'Access violation' with ComprehensiveCheck
        {
            return lang.startsWith(u"fa");
        }

        class Details
        {
        private:
            static std::vector<QString> uiLanguages();
            bool isLangSupport(const QString& lang) const;

            void cleanup()
            {
                spellcheckers = {};
                factory = nullptr;
                CoUninitialize();
            }

            static constexpr bool isSupportedAction(CORRECTIVE_ACTION action) noexcept
            {
                return action == CORRECTIVE_ACTION_GET_SUGGESTIONS
                    || action == CORRECTIVE_ACTION_REPLACE;
            };

            struct SpellingError
            {
                ULONG start = 0;
                ULONG length = 0;
                CORRECTIVE_ACTION action = CORRECTIVE_ACTION_NONE;
            };

            static std::optional<SpellingError> getError(const ComPtr<ISpellingError>& error)
            {
                SpellingError result;
                if (SUCCEEDED(error->get_StartIndex(&result.start)) && SUCCEEDED(error->get_Length(&result.length)) && SUCCEEDED(error->get_CorrectiveAction(&result.action)) && isSupportedAction(result.action))
                    return std::make_optional(std::move(result));
                return std::nullopt;
            }


        private:
            ComPtr<ISpellCheckerFactory> factory;
            std::vector<std::pair<QString, ComPtr<ISpellChecker>>> spellcheckers;

        public:
            Details()
            {
                if (CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED) == S_OK)
                {
                    if (CoCreateInstance(__uuidof(SpellCheckerFactory), nullptr, CLSCTX_INPROC_SERVER | CLSCTX_LOCAL_SERVER, IID_PPV_ARGS(&factory)) == S_OK)
                    {
                        for (auto&& lang : uiLanguages())
                        {
                            if (!isLangSupport(lang))
                                continue;
                            if (ComPtr<ISpellChecker> spellchecker; SUCCEEDED(factory->CreateSpellChecker(Q2WString(lang), &spellchecker)))
                                spellcheckers.push_back({ std::move(lang), std::move(spellchecker) });
                        }
                    }
                }
                if (spellcheckers.empty())
                {
                    cleanup();
                    throw "init error";
                }

            }
            ~Details()
            {
                cleanup();
            }

            [[nodiscard]] Suggests getSuggests(const QString& word, size_t maxCount) const;
            [[nodiscard]] std::vector<QString> languages() const;
            [[nodiscard]] Misspellings checkText(const QString& text) const;
            [[nodiscard]] bool hasMisspelling(const QString& word) const;
            void addWord(const QString& word);
            void removeWord(const QString& word);
            [[nodiscard]] bool containsWord(const QString& word) const;
            void ignoreWord(const QString& word);
        };

        std::vector<QString> Details::uiLanguages()
        {
            std::vector<QString> res;
            if (const auto dir = QDir(qEnvironmentVariable("appdata") % u"\\Microsoft\\Spelling"); dir.exists())
            {
                const auto list = dir.entryList(QDir::Dirs | QDir::NoDotAndDotDot);
                for (const auto& x : list)
                    res.push_back(x);
            }
            const auto list = QLocale::system().uiLanguages();
            for (const auto& lang : list)
                if (std::none_of(res.begin(), res.end(), [&lang](const auto &x) { return lang.compare(x, Qt::CaseInsensitive) == 0; }))
                    res.push_back(lang);
            return res;
        }

        bool Details::isLangSupport(const QString& lang) const
        {
            auto isSupported = BOOL(false);
            return SUCCEEDED(factory->IsSupported(Q2WString(lang), &isSupported)) && isSupported;
        }

        Suggests Details::getSuggests(const QString& word, size_t maxCount) const
        {
            Suggests result;
            size_t count = 0;
            const auto w = Q2WString(word);
            for (const auto& [lang, checker] : spellcheckers)
            {
                if (isFarsi(lang))
                    continue;
                if (ComPtr<IEnumString> suggestions; checker->Suggest(w, &suggestions) == S_OK)
                {
                    for (;;)
                    {
                        wchar_t* suggestion = nullptr;
                        if (suggestions->Next(1, &suggestion, nullptr) != S_OK)
                            break;
                        auto s = QString::fromWCharArray(suggestion, wcslen(suggestion));
                        CoTaskMemFree(suggestion);
                        if (!s.isEmpty())
                        {
                            result.push_back(std::move(s));
                            if (++count >= maxCount)
                                return result;
                        }
                    }
                }
            }
            return result;
        }

        std::vector<QString> Details::languages() const
        {
            std::vector<QString> res;
            res.reserve(spellcheckers.size());
            for (const auto& lang : boost::adaptors::keys(spellcheckers))
                res.push_back(lang);
            return res;
        }

        Misspellings Details::checkText(const QString& text) const
        {
            Misspellings res;
            const auto t = Q2WString(text);
            for (const auto& [lang, checker] : spellcheckers)
            {
                ComPtr<IEnumSpellingError> errors;

                HRESULT hr = isFarsi(lang)
                    ? checker->Check(t, &errors)
                    : checker->ComprehensiveCheck(t, &errors);
                if (!(SUCCEEDED(hr) && errors))
                    continue;

                Misspellings current;
                for (;;)
                {
                    ComPtr<ISpellingError> error;
                    if (errors->Next(&error) != S_OK)
                        break;
                    if (error)
                    {
                        if (auto e = getError(error))
                        {
                            if (auto r = MisspellingRange{ int(e->start), int(e->length) }; res.empty() || std::any_of(res.cbegin(), res.cend(), [r](auto x) { return x == r; }))
                                current.push_back(r);
                        }
                    }
                }
                if (current.empty())
                {
                    res = {};
                    break;
                }
                res = std::move(current);
            }

            return res;
        }

        bool Details::hasMisspelling(const QString& word) const
        {
            if (spellcheckers.empty())
                return false;
            const auto w = Q2WString(word);
            for (const auto& [lang, checker] : spellcheckers)
            {
                ComPtr<IEnumSpellingError> errors;

                HRESULT hr = checker->Check(w, &errors);
                if (!(SUCCEEDED(hr) && errors))
                    continue;

                ComPtr<ISpellingError> error;
                if (errors->Next(&error) != S_OK)
                    return false;
                if (error)
                {
                    if (auto e = getError(error); !e)
                        return false;
                }
            }
            return true;
        }

        void Details::addWord(const QString& word)
        {
            const auto w = Q2WString(word);
            for (const auto& checker : boost::adaptors::values(spellcheckers))
                checker->Add(w);
        }

        void Details::removeWord(const QString& word)
        {
            const auto w = Q2WString(word);
            for (const auto& checker : boost::adaptors::values(spellcheckers))
            {
                ComPtr<ISpellChecker2> spellchecker2;
                checker->QueryInterface(IID_PPV_ARGS(&spellchecker2));
                if (spellchecker2)
                    spellchecker2->Remove(w);
            }
        }

        bool Details::containsWord(const QString& word) const
        {
            Q_UNUSED(word);
            return false;
        }

        void Details::ignoreWord(const QString& word)
        {
            const auto w = Q2WString(word);
            for (const auto& checker : boost::adaptors::values(spellcheckers))
                checker->Ignore(w);
        }
    }

    SpellCheckerImpl::SpellCheckerImpl() = default;
    SpellCheckerImpl::~SpellCheckerImpl() = default;

    void SpellCheckerImpl::init()
    {
        if (platform::isAvailable())
        {
            try
            {
                platform_ = std::make_unique<platform::Details>();
            }
            catch (...)
            {
            }
        }
    }

    std::vector<QString> SpellCheckerImpl::languages() const
    {
        if (platform_)
            return platform_->languages();
        return {};
    }

    std::optional<Suggests> SpellCheckerImpl::getSuggests(const QString& word, size_t maxCount) const
    {
        if (platform_)
            return platform_->getSuggests(word, maxCount);
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
    }
}