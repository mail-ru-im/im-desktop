#pragma once

namespace Translit
{
    std::vector<QVector<QString>> getPossibleStrings(const QString& text);
    constexpr int getMaxSearchTextLength() { return 50; }
}
