cmake_minimum_required(VERSION 3.19)

project(unittests)

common_add_ext_srcpkg("googletest" "master" "2d07f12b" OFF OFF)

execute_process(COMMAND "${CMAKE_COMMAND}" -G "${CMAKE_GENERATOR}" .
    WORKING_DIRECTORY "${pkg_src_googletest_location}")

execute_process(COMMAND "${CMAKE_COMMAND}" --build .
    WORKING_DIRECTORY "${pkg_src_googletest_location}")

# Prevent GoogleTest from overriding our compiler/linker options
# when building with Visual Studio
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)

# Add googletest directly to our build. This adds the following targets:
# gtest, gtest_main, gmock and gmock_main
add_subdirectory("${pkg_src_googletest_location}"
                 "${pkg_src_googletest_location}")

set(CMAKE_CXX_STANDARD 17)

message(STATUS "")
message(STATUS "[CMAKE]")
message(STATUS "[CMAKE] including <unittests/CMakeLists.txt>")
message(STATUS "[CMAKE]")

include_directories(../)

# ---------------------------  paths  ----------------------------
set(CMAKE_EXECUTABLE_OUTPUT_DIRECTORY_DEBUG ${ICQ_BIN_DIR})
set(CMAKE_EXECUTABLE_OUTPUT_DIRECTORY_RELEASE ${ICQ_BIN_DIR})
set(CMAKE_EXECUTABLE_OUTPUT_PATH ${ICQ_BIN_DIR})
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY_DEBUG ${ICQ_LIB_DIR})
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY_RELEASE ${ICQ_LIB_DIR})
set(CMAKE_LIBRARY_OUTPUT_PATH ${ICQ_LIB_DIR})
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY_DEBUG ${ICQ_LIB_DIR})
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY_RELEASE ${ICQ_LIB_DIR})
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${ICQ_LIB_DIR})
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_DEBUG ${ICQ_BIN_DIR})
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELEASE ${ICQ_BIN_DIR})
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${ICQ_BIN_DIR})

if(APPLE)
    set(SYSTEM_LIBRARIES
        ${MAC_OPENGL}
        ${MAC_APP_KIT}
        ${MAC_FOUNDATION}
        ${MAC_AVFOUNDATION}
        ${MAC_IOKIT}
        ${MAC_SYSTEM_CONFIGURATION}
        z
        ${MAC_SECURITY}
        ${MAC_CORE_MEDIA}
        ${MAC_CORE_AUDIO}
        ${MAC_AUDIO_TOOLBOX}
        ${MAC_OPEN_AL}
        ${MAC_AUDIO_UNIT}
        ${MAC_CARBON}
        ${MAC_QUARTZ}
        ${MAC_SCRIPTING_BRIDGE}
        ${MAC_VIDEO_TOOLBOX}
        ${MAC_VIDEO_DECODE_ACCELERATION}
        ${MAC_METAL_KIT}
        ${MAC_METAL}
        ${MAC_IOSURFACE}
        ${MAC_GSS}
        ${VOIP_LIBRARIES}
        libcups.2.dylib
        iconv
        bsm)
elseif(LINUX)
    set(SYSTEM_LIBRARIES
        corelib
        core
        ${CURL_LIBRARIES}
        ${OPENSSL_LIBRARIES}
        ${VOIP_LIBRARIES}
        -ldl -lstdc++fs -lgpg-error -lfontconfig -lfreetype -luuid
        -lpthread -lm -lrt -lexpat -lz -Wl,-Bstatic -Wl,-Bdynamic
        -lX11 -lX11-xcb -lxcb -lXrandr -lXcursor -lXext -lXdamage -lXfixes -lXcomposite)

    if(CMAKE_CXX_COMPILER_VERSION VERSION_LESS 10.2)
        message(STATUS "WARNING: GCC version is ${CMAKE_CXX_COMPILER_VERSION}")
        execute_process(COMMAND bash "-c" "g++ -print-file-name=libstdc++.a" OUTPUT_VARIABLE LIBSTD OUTPUT_STRIP_TRAILING_WHITESPACE)
        message(STATUS "linking against ${LIBSTD}")
        set(SYSTEM_LIBRARIES ${SYSTEM_LIBRARIES} ${LIBSTD})
    endif()

    if(ICQ_DEBUG)
        message(STATUS "[INFO] ICQ_DEBUG = ${ICQ_DEBUG}")
        set(SYSTEM_LIBRARIES ${SYSTEM_LIBRARIES} -ludev)
    endif()
endif()

# -------------------------  unittests   ------------------------
set(SUBPROJECT_ROOT "${ICQ_ROOT}/unittests")
message(STATUS "SUBPROJECT_ROOT = ${SUBPROJECT_ROOT}")

find_sources(SUBPROJECT_SOURCES "${SUBPROJECT_ROOT}/src" "cpp")
find_sources(SUBPROJECT_HEADERS "${SUBPROJECT_ROOT}/src" "h")
file(GLOB IM_SOURCES
    "${ICQ_ROOT}/gui/types/fstring.h"
    "${ICQ_ROOT}/gui/types/fstring.cpp")

set_source_group("sources" "${SUBPROJECT_ROOT}" ${SUBPROJECT_SOURCES} ${SUBPROJECT_HEADERS})

# ----------------------------------------------------------------
include_directories(${SUBPROJECT_ROOT} "${ICQ_ROOT}/gui")
if(APPLE)
    include_directories("${ICQ_ROOT}/mac/ICQ")
endif()

add_executable(${PROJECT_NAME}
    ${SUBPROJECT_SOURCES} ${SUBPROJECT_HEADERS} ${COMMON_SOURCES} ${IM_SOURCES})

if(MSVC)
    target_link_libraries(${PROJECT_NAME}
        core
        ${Boost_LIBRARIES}
        ${ADDITIONAL_LIBS}
        ${SYSTEM_LIBRARIES}
        ${QT_LIBRARIES}
        ${VOIP_LIBRARIES}
        ${ZLIB_LIBRARIES}
        ${PHONENUMBER_LIBRARIES}
        ${PROTOBUF_LIBRARIES}
        ${RE2_LIBRARIES}
        Iphlpapi.lib
        corelib
        gtest
        gtest_main)
elseif(APPLE)
    target_link_libraries(${PROJECT_NAME}
        core
        ${Boost_LIBRARIES}
        ${ADDITIONAL_LIBS}
        ${SYSTEM_LIBRARIES}
        ${ICQ_QT_LIB_DIR}/libQt5Core.${QT_LIB_EXT}
        ${VOIP_LIBRARIES}
        ${ZLIB_LIBRARIES}
        ${PHONENUMBER_LIBRARIES}
        ${PROTOBUF_LIBRARIES}
        ${RE2_LIBRARIES}
        corelib
        gtest
        gtest_main)
else()
    target_link_libraries(${PROJECT_NAME}
        core
        ${Boost_LIBRARIES}
        ${ADDITIONAL_LIBS}
        ${SYSTEM_LIBRARIES}
        ${ICQ_QT_LIB_DIR}/libQt5Core.${QT_LIB_EXT}
        ${VOIP_LIBRARIES}
        ${ZLIB_LIBRARIES}
        ${PHONENUMBER_LIBRARIES}
        ${PROTOBUF_LIBRARIES}
        ${RE2_LIBRARIES}
        corelib
        gtest
        gtest_main)
endif()
