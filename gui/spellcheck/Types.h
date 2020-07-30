#pragma once

namespace spellcheck
{
    using Suggests = std::vector<QString>;
    struct WordStatus
    {
        bool isCorrect;
        bool isAdded;
    };
    using MisspellingRange = std::pair<int, int>;
    using Misspellings = std::vector<MisspellingRange>;
}