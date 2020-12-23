#pragma once

#include "../../../namespaces.h"

UI_COMPLEX_MESSAGE_NS_BEGIN

QSize limitSize(const QSize &size, const QSize &maxSize, const QSize& _minSize = QSize());

constexpr auto mediaVisiblePlayPercent = 0.5;

UI_COMPLEX_MESSAGE_NS_END