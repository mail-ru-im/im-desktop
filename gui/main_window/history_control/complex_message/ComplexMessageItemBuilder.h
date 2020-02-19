#pragma once

#include "../../../namespaces.h"

UI_COMPLEX_MESSAGE_NS_BEGIN

class ComplexMessageItem;

namespace ComplexMessageItemBuilder
{
    enum class ForcePreview
    {
        No,
        Yes
    };

    std::unique_ptr<ComplexMessageItem> makeComplexItem(QWidget *_parent, const Data::MessageBuddy& _msg, ForcePreview _forcePreview);

}

UI_COMPLEX_MESSAGE_NS_END
