#pragma once

#ifndef CORE_NS
    #define CORE_NS core
    #define CORE_NS_BEGIN namespace CORE_NS {
    #define CORE_NS_END }
#endif

#define THEMES_NS_BEGIN namespace Themes {
#define THEMES_NS_END }

#define UTILS_NS_BEGIN namespace Utils {
#define UTILS_NS_END }

#define UTILS_EXIF_NS_BEGIN UTILS_NS_BEGIN namespace Exif {
#define UTILS_EXIF_NS_END } UTILS_NS_END

#define UI_NS_BEGIN namespace Ui {
#define UI_NS_END }

#define UI_COMPLEX_MESSAGE_NS_BEGIN UI_NS_BEGIN namespace ComplexMessage {
#define UI_COMPLEX_MESSAGE_NS_END } UI_NS_END

#define DATA_NS_BEGIN namespace Data {
#define DATA_NS_END }

#define FONTS_NS_BEGIN namespace Fonts {
#define FONTS_NS_END }

#define PLATFORM_NS platform
#define PLATFORM_NS_BEGIN namespace PLATFORM_NS {
#define PLATFORM_NS_END }

#define MEMSTATS_NS Memory_Stats
#define MEMSTATS_NS_BEGIN namespace MEMSTATS_NS {
#define MEMSTATS_NS_END }
