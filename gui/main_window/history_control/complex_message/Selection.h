#pragma once

#include "../../../namespaces.h"

UI_COMPLEX_MESSAGE_NS_BEGIN

enum class BlockSelectionType
{
    Invalid,

    Min,

    None,
    PartialInternal,
    FromBeginning,
    TillEnd,
    Full,

    Max,
};

inline QTextStream& operator<<(QTextStream &oss, const BlockSelectionType type)
{
    assert(type > BlockSelectionType::Min);
    assert(type < BlockSelectionType::Max);

    switch(type)
    {
        case BlockSelectionType::FromBeginning: oss << ql1s("FromBeginning"); break;
        case BlockSelectionType::Full: oss << ql1s("Full"); break;
        case BlockSelectionType::None: oss << ql1s("None"); break;
        case BlockSelectionType::PartialInternal: oss << ql1s("ParialInternal"); break;
        case BlockSelectionType::TillEnd: oss << ql1s("TillEnd"); break;

        default:
            assert(!"unexpected BlockSelectionType value");
            break;
    }

    return oss;
}

UI_COMPLEX_MESSAGE_NS_END