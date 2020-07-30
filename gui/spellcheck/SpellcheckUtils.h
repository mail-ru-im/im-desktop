#pragma once

#include "Types.h"

namespace spellcheck
{
namespace utils
{
    QChar::Script wordScript(QStringView word) noexcept;
    QChar::Script localeToScriptCode(const QStringRef& locale) noexcept;

    bool isWordSkippable(QStringView word);

    std::vector<QString> systemLanguages();

    std::vector<QChar::Script> supportedScripts();

    using CheckFunction = std::function<bool(QStringView)>;
    Misspellings checkText(QStringView, CheckFunction f);
}
}
