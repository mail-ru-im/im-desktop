// stdafx.h : include file for standard system include files,
// or project specific include files that are used frequently, but
// are changed infrequently
//

#pragma once

#ifdef _WIN32

#define _WIN32_LEAN_AND_MEAN             // Exclude rarely-used stuff from Windows headers
// Windows Header Files:
#include <WinSDKVer.h>
#define WINVER 0x0500
#define _WIN32_WINDOWS 0x0500
#define _WIN32_WINNT 0x0600
#define _ATL_XP_TARGETING

#define _CRT_SECURE_NO_WARNINGS

#include "targetver.h"
#include <windows.h>
#endif //WIN32

#include <memory>
#include <functional>
#include <list>
#include <atomic>
#include <map>
#include <string>
#include <vector>
#include <sstream>
#include <cassert>
#include <unordered_map>
#include <set>
#include <mutex>
#include <thread>
#include <string.h>
#include <iostream>

#include "../common.shared/common.h"

#if !defined(_WIN32) && !defined(ABORT_ON_ASSERT) && defined(DEBUG)
#define assert(condition) \
do { if(!(condition)){ std::cerr << "ASSERT FAILED: " << #condition << " " << __FILE__ << ":" << __LINE__ << std::endl; } } while (0)
#endif

// TODO: reference additional headers your program requires here
