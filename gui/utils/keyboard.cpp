#include "keyboard.h"

namespace Utils
{
    bool keyEventCorrespondsToKeyComb(QKeyEvent *keyEvent, KeyCombination combination)
    {
        assert(keyEvent);
        if (!keyEvent)
            return false;

        auto modifiers = keyEvent->modifiers();

        switch (combination)
        {
        case KeyCombination::Ctrl_or_Cmd_AltShift:
            return modifiers.testFlag(Qt::ControlModifier) &&
                   modifiers.testFlag(Qt::AltModifier) &&
                   modifiers.testFlag(Qt::ShiftModifier);
            break;
        default:
            assert(!"unhandled key combination, fix");
        }

        return false;
    }

    bool keyboardModifiersCorrespondToKeyComb(const Qt::KeyboardModifiers& modifiers, KeyCombination combination)
    {
        switch (combination)
        {
        case KeyCombination::Ctrl_or_Cmd_AltShift:
            return modifiers.testFlag(Qt::ControlModifier) &&
                   modifiers.testFlag(Qt::AltModifier) &&
                   modifiers.testFlag(Qt::ShiftModifier);
            break;
        default:
            assert(!"unhandled key combination for modifiers, fix");
        }

        return false;
    }
}