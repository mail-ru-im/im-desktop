cmake_minimum_required(VERSION 3.8)
### integrate to icq
#set(CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/CMake;${CMAKE_MODULE_PATH}")
#include(Utilities)
#
#get_filename_component(TOP_DIR "${CMAKE_SOURCE_DIR}" ABSOLUTE)
#
#set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/bin)
#set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/bin)
#set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/bin)

# ---------------------------  paths  ----------------------------
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY_DEBUG ${ICQ_LIB_DIR})
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY_RELEASE ${ICQ_LIB_DIR})
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${ICQ_LIB_DIR})
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY_DEBUG ${ICQ_LIB_DIR})
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY_RELEASE ${ICQ_LIB_DIR})
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${ICQ_LIB_DIR})
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_DEBUG ${ICQ_BIN_DIR})
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELEASE ${ICQ_BIN_DIR})
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${ICQ_BIN_DIR})

set(LIB_MAJOR_VERSION "0")
set(LIB_MINOR_VERSION "1")
set(LIB_PATCH_VERSION "0")
set(LIB_VERSION_STRING "${LIB_MAJOR_VERSION}.${LIB_MINOR_VERSION}.${LIB_PATCH_VERSION}")

project(libomicron
    VERSION ${LIB_VERSION_STRING}
    LANGUAGES CXX)

option(OMICRON_USE_LIBCURL "Use libcurl for downloading." ON)
option(OMICRON_BUILD_TESTS "Build libomicron tests." OFF)

# --------------------------   libomicron   -------------------------
set(SUBPROJECT_ROOT "${ICQ_ROOT}/libomicron")

find_sources(SUBPROJECT_SOURCES "${SUBPROJECT_ROOT}/src" "cpp")
if(APPLE)
    find_sources(SUBPROJECT_MM_SOURCES "${SUBPROJECT_ROOT}/src" "mm")
endif()
find_sources(SUBPROJECT_HEADERS_MAIN "${SUBPROJECT_ROOT}/include" "h")
find_sources(SUBPROJECT_HEADERS_ADD "${SUBPROJECT_ROOT}/src" "h")
set(SUBPROJECT_HEADERS ${SUBPROJECT_HEADERS_MAIN} ${SUBPROJECT_HEADERS_ADD})

set_source_group("sources" "${SUBPROJECT_ROOT}" ${SUBPROJECT_SOURCES} ${SUBPROJECT_MM_SOURCES} ${SUBPROJECT_HEADERS})

### integrate to icq
#set(CMAKE_CXX_STANDARD 11)
#set(CMAKE_CXX_STANDARD_REQUIRED ON)
#
#file(GLOB SOURCES ${TOP_DIR}/src/*.cpp)
#file(GLOB HEADERS ${TOP_DIR}/include/omicron/*.h)

include_directories(${SUBPROJECT_ROOT}/include)
include_directories(${SUBPROJECT_ROOT}/src)

add_definitions(-DRAPIDJSON_HAS_STDSTRING=1)
set(RAPID_JSON_DIR "${SUBPROJECT_ROOT}/external/rapidjson-master")
include_directories(${RAPID_JSON_DIR}/include)

if(OMICRON_USE_LIBCURL)
    add_definitions(-DOMICRON_USE_LIBCURL)
### integrate to icq
#    add_definitions(-DCURL_STATICLIB)
#    set(CURL_DIR "${TOP_DIR}/../external/curl")
#    include_directories(${CURL_DIR}/include)
endif()

if(MSVC)
    use_precompiled_header_msvc("stdafx.h" "${SUBPROJECT_ROOT}/src/stdafx.cpp" ${SOURCES})
#    add_definitions(/FI"stdafx.h")
elseif(APPLE)
    use_precompiled_header_mac(PCH_BIN "${SUBPROJECT_ROOT}/src/stdafx.h" ${SOURCES})
#    add_definitions(-stdlib=libc++)
#    add_definitions(-std=gnu++11)
#    add_definitions(-mmacosx-version-min=10.9)
#    add_definitions(-include "stdafx.h")
elseif(UNIX)
    use_precompiled_header_linux(PCH_BIN "${SUBPROJECT_ROOT}/src/stdafx.h" ${SOURCES})
#    add_definitions(-std=gnu++11)
#    add_definitions(-include "stdafx.h")
endif()

add_library(${PROJECT_NAME} STATIC
    ${SUBPROJECT_SOURCES}
    ${SUBPROJECT_MM_SOURCES}
    ${SUBPROJECT_HEADERS}
    ${PCH_BIN})

if(OMICRON_BUILD_TESTS)
    add_subdirectory(test)
endif()
