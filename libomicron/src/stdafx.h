#pragma once

#ifdef _WIN32
#define WINVER 0x0501
#define _WIN32_WINDOWS 0x0501
#define _WIN32_WINNT 0x0600

#include <windows.h>
#include <io.h>
#else
#include <unistd.h>
#include <sys/stat.h>
#endif

#include <cctype>
#include <cstdio>
#include <cstdint>
#include <ctime>
#include <iostream>
#include <sstream>
#include <fstream>
#include <string>
#include <codecvt>
#include <locale>
#include <map>
#include <array>
#include <memory>
#include <chrono>
#include <thread>
#include <atomic>
#include <mutex>
#include <condition_variable>

#include <rapidjson/document.h>
#include "rapidjson/writer.h"
#include "rapidjson/stringbuffer.h"

#include <omicron/omicron_conf.h>
#include <omicron/omicron.h>

#include <tools.h>

// workaround for std::make_unique (absent in C++11)
#ifdef _WIN32
using std::make_unique;
#else
#if __cplusplus > 201103L
using std::make_unique;
#else
namespace omicronlib {
    template <typename T, typename... Args>
    std::unique_ptr<T> make_unique(Args&&... args)
    {
        return std::unique_ptr<T>(new T(std::forward<Args>(args)...));
    }
}
#endif
#endif
