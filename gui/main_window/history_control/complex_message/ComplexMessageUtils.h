#pragma once

#include "../../../namespaces.h"

UI_COMPLEX_MESSAGE_NS_BEGIN

enum class ReplaceReason
{
    NoMeta,
    OtherReason,
};

class IItemBlock;
class GenericBlock;
class ComplexMessageItem;

QSize limitSize(const QSize &size, const QSize &maxSize, const QSize& _minSize = QSize());

constexpr auto mediaVisiblePlayPercent = 0.5;

bool mergeToPreviousOrReplaceByText(std::vector<IItemBlock*>& _blocks, IItemBlock* _base, ComplexMessageItem* _parent, ReplaceReason _reason = ReplaceReason::OtherReason);
bool mergeToPreviousOrReplaceByText(std::vector<GenericBlock*>& _blocks, IItemBlock* _base, ComplexMessageItem* _parent, ReplaceReason _reason = ReplaceReason::OtherReason);

UI_COMPLEX_MESSAGE_NS_END