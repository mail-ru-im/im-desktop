#include "stdafx.h"
#include "Utils.h"
#include "../core_dispatcher.h"

#ifdef _WIN32
#include "win32/win32spec.h"
#elif defined(__linux__)
#include "linux/linuxspec.h"
#elif defined(__APPLE__)
#include "macos/macosspec.h"
#include <Carbon/Carbon.h>
#endif

void platform_specific::showInCurrentWorkspace(QWidget* _w, QWidget* _initiator)
{
#ifdef __APPLE__
    platform_macos::showInWorkspace(_w);
#elif defined(_WIN32)
    platform_win32::showInWorkspace(_w, _initiator);
#elif defined(__linux__)
    // currently a stub - no implementation yet
    platform_linux::showInWorkspace(_w, _initiator);
#endif
}

bool platform_specific::windowIsOverlapped(QWidget* _window)
{
    if (!_window)
        return false;

#ifdef _WIN32
    return platform_win32::windowIsOverlapped(_window);
#elif defined (__APPLE__)
    return !platform_macos::windowIsOnActiveSpace(_window) || platform_macos::windowIsOverlapped(_window);
#elif defined (__linux__)
    return platform_linux::windowIsOverlapped(_window);
#endif
}

bool platform_specific::isSpacebarDown()
{
#ifdef _WIN32
    return platform_win32::isSpacebarDown();
#elif defined(__APPLE__)
    unsigned char keyMap[16];
    GetKeys((BigEndianUInt32*)&keyMap);
    return (0 != ((keyMap[kVK_Space >> 3] >> (kVK_Space & 7)) & 1));
#elif defined(__linux__)
    return platform_linux::isSpacebarDown();
#else // Unknown OS
    return false;
#endif
}

void ButtonStatistic::Data::reset()
{
    flags_ = ButtonFlags{};
    currentTime_ = 0;
}

void ButtonStatistic::Data::send()
{
    if (currentTime_ < kMinTimeForStatistic)
        return;

    core::stats::event_props_type props;
    auto boolalpha = [](bool value) -> std::string_view { return value ? "Yes" : "No"; };
    const QMetaEnum metaEnum = QMetaEnum::fromType<ButtonStatistic::ButtonFlags>();
    for (int i = 0; i < metaEnum.keyCount(); ++i)
        props.emplace_back(metaEnum.key(i), boolalpha(bool(flags_ & metaEnum.value(i))));
    Ui::GetDispatcher()->post_stats_to_core(core::stats::stats_event_names::voip_calls_interface_buttons, props);
}
