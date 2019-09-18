#pragma once

#include <QKeyEvent>

namespace Utils
{
    enum class KeyCombination
    {
        Ctrl_or_Cmd_AltShift
    };

    bool keyEventCorrespondsToKeyComb(QKeyEvent *keyEvent, KeyCombination combination);
    bool keyboardModifiersCorrespondToKeyComb(const Qt::KeyboardModifiers& modifiers, KeyCombination combination);
}