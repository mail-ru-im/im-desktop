cmake_minimum_required(VERSION 3.17)

project(${APP_PROJECT_NAME})

message(STATUS "APP_PROJECT_NAME is ${APP_PROJECT_NAME}")

macro(QT5_MAKE_OUTPUT_FILE infile prefix ext outfile )
    string(LENGTH ${CMAKE_CURRENT_BINARY_DIR} _binlength)
    string(LENGTH ${infile} _infileLength)
    set(_checkinfile ${CMAKE_CURRENT_SOURCE_DIR})
    if(_infileLength GREATER _binlength)
        string(SUBSTRING "${infile}" 0 ${_binlength} _checkinfile)
        if(_checkinfile STREQUAL "${CMAKE_CURRENT_BINARY_DIR}")
            file(RELATIVE_PATH rel ${CMAKE_CURRENT_BINARY_DIR} ${infile})
        else()
            file(RELATIVE_PATH rel ${CMAKE_CURRENT_SOURCE_DIR} ${infile})
        endif()
    else()
        file(RELATIVE_PATH rel ${CMAKE_CURRENT_SOURCE_DIR} ${infile})
    endif()
    if(WIN32 AND rel MATCHES "^([a-zA-Z]):(.*)$") # absolute path
        set(rel "${CMAKE_MATCH_1}_${CMAKE_MATCH_2}")
    endif()
    set(_outfile "${CMAKE_CURRENT_BINARY_DIR}/${rel}")
    string(REPLACE ".." "__" _outfile ${_outfile})
    get_filename_component(outpath ${_outfile} PATH)
    get_filename_component(_outfile ${_outfile} NAME_WE)
    file(MAKE_DIRECTORY ${outpath})
    set(${outfile} ${outpath}/${prefix}${_outfile}.${ext})
endmacro()

function(_QT5_PARSE_QRC_FILE infile _out_depends _rc_depends)
    get_filename_component(rc_path ${infile} PATH)

    if(EXISTS "${infile}")
        #  parse file for dependencies
        #  all files are absolute paths or relative to the location of the qrc file
        file(READ "${infile}" RC_FILE_CONTENTS)
        string(REGEX MATCHALL "<file[^<]+" RC_FILES "${RC_FILE_CONTENTS}")
        foreach(RC_FILE ${RC_FILES})
            string(REGEX REPLACE "^<file[^>]*>" "" RC_FILE "${RC_FILE}")
            if(NOT IS_ABSOLUTE "${RC_FILE}")
                set(RC_FILE "${rc_path}/${RC_FILE}")
            endif()
            set(RC_DEPENDS ${RC_DEPENDS} "${RC_FILE}")
        endforeach()
        # Since this cmake macro is doing the dependency scanning for these files,
        # let's make a configured file and add it as a dependency so cmake is run
        # again when dependencies need to be recomputed.
        qt5_make_output_file("${infile}" "" "qrc.depends" out_depends)
        configure_file("${infile}" "${out_depends}" COPYONLY)
    else()
        # The .qrc file does not exist (yet). Let's add a dependency and hope
        # that it will be generated later
        set(out_depends)
    endif()

    set(${_out_depends} ${out_depends} PARENT_SCOPE)
    set(${_rc_depends} ${RC_DEPENDS} PARENT_SCOPE)
endfunction()

#-----------------------------------------------------------------
# use qt5_add_big_resources since qt5.12

function(ICQ_ADD_BIG_RESOURCES outfiles )
    set(options)
    set(oneValueArgs)
    set(multiValueArgs OPTIONS)

    cmake_parse_arguments(_RCC "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

    set(rcc_files ${_RCC_UNPARSED_ARGUMENTS})
    set(rcc_options ${_RCC_OPTIONS})

    if("${rcc_options}" MATCHES "-binary")
        message(WARNING "Use icq_add_binary_resources for binary option")
    endif()

    foreach(it ${rcc_files})
        get_filename_component(outfilename ${it} NAME_WE)
        get_filename_component(infile ${it} ABSOLUTE)
        set(tmpoutfile ${CMAKE_CURRENT_BINARY_DIR}/qrc_${outfilename}tmp.cpp)
        set(outfile ${CMAKE_CURRENT_BINARY_DIR}/qrc_${outfilename}.o)

        _QT5_PARSE_QRC_FILE(${infile} _out_depends _rc_depends)
        set_source_files_properties(${infile} PROPERTIES SKIP_AUTORCC ON)
        add_custom_command(OUTPUT ${tmpoutfile}
                           COMMAND ${ICQ_RCC_EXE} ${rcc_options} --name ${outfilename} --pass 1 --output ${tmpoutfile} ${infile}
                           DEPENDS ${infile} ${_rc_depends} "${out_depends}" VERBATIM)
        set_source_files_properties(${tmpoutfile} PROPERTIES SKIP_AUTOMOC ON)
        set_source_files_properties(${tmpoutfile} PROPERTIES SKIP_AUTOUIC ON)
        add_custom_target(big_resources_${outfilename} ALL DEPENDS ${tmpoutfile})
        add_library(rcc_object_${outfilename} OBJECT ${tmpoutfile})
        add_dependencies(rcc_object_${outfilename} big_resources_${outfilename})
        add_custom_command(OUTPUT ${outfile}
                           COMMAND ${ICQ_RCC_EXE}
                           ARGS ${rcc_options} --name ${outfilename} --pass 2 --temp $<TARGET_OBJECTS:rcc_object_${outfilename}> --output ${outfile} ${infile}
                           DEPENDS rcc_object_${outfilename}
                           VERBATIM)
       list(APPEND ${outfiles} ${outfile})
    endforeach()
    set(${outfiles} ${${outfiles}} PARENT_SCOPE)
endfunction()


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

# -------------------------- definitions -------------------------
if(${CMAKE_BUILD_TYPE} STREQUAL "Release")
    add_definitions(-DQT_NO_DEBUG_OUTPUT)
endif()

add_definitions(-DQT_NO_CAST_FROM_ASCII)
add_definitions(-DQT_NO_CAST_TO_ASCII)
add_definitions(-DQT_NO_CAST_FROM_BYTEARRAY)
add_definitions(-DQT_STRICT_ITERATORS)
add_definitions(-DQT_NO_KEYWORDS)

add_definitions(-DRLOTTIE_BUILD)

option(LOTTIE_FRAME_CACHE "Enable lottie frame cache in player" ON)
if (LOTTIE_FRAME_CACHE)
    add_definitions(-DLOTTIE_FRAME_CACHE)
endif()

if(IM_AUTO_TESTING)
    message(STATUS "... CMAKE_INSTALL_RPATH_USE_LINK_PATH TRUE")
    SET(CMAKE_INSTALL_RPATH_USE_LINK_PATH TRUE)
endif()

message(STATUS "ICQ_ROOT is ${ICQ_ROOT}")
if(MSVC)
    add_definitions(/FI"stdafx.h")
    set(SYSTEM_LIBRARIES d3d9 dxguid opengl32 Ws2_32 Imm32 Winmm runtimeobject strmiids secur32 glu32 psapi userenv version dwmapi shell32 rpcrt4 wtsapi32)
elseif(APPLE)
    set(ICQ_MAC_ROOT "${ICQ_ROOT}/mac/ICQ")
    message(STATUS "ICQ_MAC_ROOT is ${ICQ_MAC_ROOT}")
    add_definitions(-include "stdafx.h")
    include_directories(${ICQ_MAC_ROOT})
    find_library(MAC_OPENGL OpenGL)
    find_library(MAC_APP_KIT AppKit)
    find_library(MAC_FOUNDATION Foundation)
    find_library(MAC_AVFOUNDATION AVFoundation)
    find_library(MAC_IOKIT IOKit)
    find_library(MAC_SYSTEM_CONFIGURATION SystemConfiguration)
    find_library(MAC_SECURITY Security)
    find_library(MAC_CORE_MEDIA CoreMedia)
    find_library(MAC_CORE_AUDIO CoreAudio)
    find_library(MAC_AUDIO_TOOLBOX AudioToolbox)
    if (USE_SYSTEM_OPENAL)
        find_library(MAC_OPEN_AL OpenAL)
    endif()
    find_library(MAC_AUDIO_UNIT AudioUnit)
    find_library(MAC_CARBON Carbon)
    find_library(MAC_QUARTZ Quartz)
    find_library(MAC_METAL Metal)
    find_library(MAC_IOSURFACE IOSurface)
    find_library(MAC_GSS GSS)
    find_library(MAC_METAL_KIT MetalKit)
    find_library(MAC_XCTEST XCTest PATHS ${ICQ_PLATFORM_SPECIFIC})
    find_library(MAC_SCRIPTING_BRIDGE ScriptingBridge)
    find_library(MAC_VIDEO_TOOLBOX VideoToolbox)
    find_library(MAC_VIDEO_DECODE_ACCELERATION VideoDecodeAcceleration)
    find_library(MAC_SPARKLE Sparkle PATHS ${ICQ_PLATFORM_SPECIFIC})

    include_directories(${MAC_SPARKLE})

    mark_as_advanced(
        MAC_OPENGL
        MAC_APP_KIT
        MAC_FOUNDATION
        MAC_AVFOUNDATION
        MAC_IOKIT
        MAC_SYSTEM_CONFIGURATION
        MAC_SECURITY
        MAC_CORE_MEDIA
        MAC_CORE_AUDIO
        MAC_AUDIO_TOOLBOX
        MAC_OPEN_AL
        MAC_AUDIO_UNIT
        MAC_CARBON
        MAC_QUARTZ
        MAC_SCRIPTING_BRIDGE
        MAC_VIDEO_TOOLBOX
        MAC_VIDEO_DECODE_ACCELERATION
        MAC_METAL_KIT
        MAC_METAL
        MAC_IOSURFACE
        MAC_GSS
        MAC_SPARKLE
        MAC_XCTEST)

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

    if(ICQ_UPDATE)
        message(STATUS "Updates: ON [ICQ_UPDATE]")
        message(STATUS "add_definitions(-DUPDATES)")
        add_definitions(-DUPDATES)
        set(SYSTEM_LIBRARIES ${SYSTEM_LIBRARIES} ${MAC_SPARKLE})
    else()
        message(STATUS "Updates: OFF")
    endif()

    if(IM_AUTO_TESTING)
        message(STATUS "... add XCTest, libQt5Test.a")
        message(STATUS "... 1 linking -weak_framework XCTest")
        set(SYSTEM_LIBRARIES ${SYSTEM_LIBRARIES} ${MAC_XCTEST})
        set(SYSTEM_LIBRARIES ${SYSTEM_LIBRARIES} ${ICQ_QT_LIB_DIR}/libQt5Test.a)
        set(CMAKE_STATIC_LINKER_FLAGS "${CMAKE_STATIC_LINKER_FLAGS} -F${ICQ_FRAMEWORKS_BUNDLE_DIR} -weak_framework XCTest")
    endif()

    message(STATUS "PRODUCT_SPEC_DIR = ${PRODUCT_SPEC_DIR}")
    set(PRODUCT_MAC "${PRODUCT_SPEC_DIR}/mac")
    message(STATUS "PRODUCT_MAC = ${PRODUCT_MAC}")
    set(MACOSX_BUNDLE_ICON_FILE ${PRODUCT_MAC}/icons-bundle.icns)
    message(STATUS "MACOSX_BUNDLE_ICON_FILE = ${MACOSX_BUNDLE_ICON_FILE}")

    set_source_files_properties(${MACOSX_BUNDLE_ICON_FILE} PROPERTIES MACOSX_PACKAGE_LOCATION Resources)
    set_source_files_properties(${MACOSX_BUNDLE_ICON_FILE} PROPERTIES XCODE_EXPLICIT_FILE_TYPE "compiled")

elseif(LINUX)
    # add_definitions(-include "stdafx.h")
    message("Linux: Set SYSTEM_LIBRARIES")

    set_lib_var(DBUS_LIBRARYDIR libdbus_${deps_lib_version_libdbus})

    set(SYSTEM_LIBRARIES
        corelib
        core
        ${CURL_LIBRARIES}
        ${OPENSSL_LIBRARIES}
        ${ICQ_PLATFORM_SPECIFIC}/libXi.a
        ${ICQ_PLATFORM_SPECIFIC}/libxkbcommon-x11.a
        ${ICQ_PLATFORM_SPECIFIC}/libxkbcommon.a
        ${DBUS_LIBRARYDIR}/libdbus-1.a
        ${ICQ_PLATFORM_SPECIFIC}/libxcb-util.a
        ${ICQ_PLATFORM_SPECIFIC}/libXext.a
        ${ICQ_PLATFORM_SPECIFIC}/libXau.a
        ${ICQ_PLATFORM_SPECIFIC}/libXdmcp.a
        ${VOIP_LIBRARIES}
        -Wl,-Bdynamic -ldl -lstdc++fs -lgpg-error -lfontconfig -lfreetype -luuid -lrt -lexpat -lz -lpthread -lX11 -lX11-xcb -lxcb -lXrandr -lXinerama -lXcursor -lXext -lXdamage -lXfixes -lXcomposite)

    if(ICQ_DEBUG)
        message(STATUS "ICQ_DEBUG = ${ICQ_DEBUG}")
        set(SYSTEM_LIBRARIES ${SYSTEM_LIBRARIES} -ludev)
    endif()

    if(IM_AUTO_TESTING)
        message(STATUS "[testing] set ${ICQ_QT_LIB_DIR}/libQt5Test.a to QT_LIBRARIES")
        add_definitions(-DQT_CORE_LIB=1)
        add_definitions(-DQT_GUI_LIB=1)
        add_definitions(-DQT_WIDGETS_LIB=1)
        add_definitions(-DQT_TESTLIB_LIB=1)
        set(CMAKE_STATIC_LINKER_FLAGS "${CMAKE_STATIC_LINKER_FLAGS} -Wl -fsanitize=address")
        set(QT_LIBRARIES ${QT_LIBRARIES} ${ICQ_QT_LIB_DIR}/libQt5Test.a)
    endif()

endif()

# ------------------------- release notes  -----------------------
find_package(PythonInterp 3 REQUIRED)
message(STATUS "PYTHON_EXECUTABLE = ${PYTHON_EXECUTABLE}")
execute_process(COMMAND
                    ${PYTHON_EXECUTABLE} buildsystem/update_release_notes.py
                WORKING_DIRECTORY
                    ${ICQ_ROOT})

# --------------------------    gui      -------------------------
set(SUBPROJECT_ROOT "${ICQ_ROOT}/gui")

message(STATUS "SUBPROJECT_ROOT = ${SUBPROJECT_ROOT}")
find_sources(SUBPROJECT_SOURCES "${SUBPROJECT_ROOT}" "cpp")
if(APPLE)
    find_sources(SUBPROJECT_MM_SOURCES "${SUBPROJECT_ROOT}" "mm")
endif()
find_sources(SUBPROJECT_HEADERS "${SUBPROJECT_ROOT}" "h")

if(IM_AUTO_TESTING)
    message(STATUS "Webconsole Testing: on")
    add_definitions(-DIM_AUTO_TESTING=1)
    if(LINUX)
        message(STATUS "Qt5Test LINUX READY")
        set(CMAKE_CXX_STANDARD_REQUIRED ON)
        set(CMAKE_CXX_EXTENSIONS OFF)
        find_package(Qt5Gui REQUIRED HINTS ${Qt5Tests_PREFIX_PATH})
        find_package(Qt5Widgets REQUIRED)
        find_package(Qt5Test REQUIRED HINTS ${Qt5Tests_PREFIX_PATH})
        # find_package(Qt5Test ${REQUIRED_QT_VERSION} CONFIG QUIET)
        # find_package(Qt5Test REQUIRED)
        #    ${Qt5Test_DEFINITIONS}
        #    ${Qt5Test_INCLUDE_DIRS}
        #    ${Qt5Test_LIBRARIES}
        # include_directories(
        #     ${Qt5Gui_INCLUDE_DIRS}
        #     ${Qt5Test_INCLUDE_DIRS}
        # )

        # add_definitions(
        #     ${Qt5Gui_DEFINITIONS}
        #     ${Qt5Test_DEFINITIONS}
        # )

        if(NOT Qt5Test_FOUND)
            message(STATUS "Qt5Test not found, autotests will not be built.")
            return()
        endif()
    endif()
else()
    message(STATUS "Webconsole Testing: off")
    message(STATUS "skipped: " "${ICQ_ROOT}/gui/webserver/webserver.h")
    list(REMOVE_ITEM SUBPROJECT_HEADERS "${ICQ_ROOT}/gui/webserver/webserver.h")
    message(STATUS "skipped: " "${ICQ_ROOT}/gui/webserver/webserver.cpp")
    list(REMOVE_ITEM SUBPROJECT_SOURCES "${ICQ_ROOT}/gui/webserver/webserver.cpp")
endif()

if (STRIP_VOIP)
    list(FILTER SUBPROJECT_HEADERS EXCLUDE REGEX  "${ICQ_ROOT}/gui/voip/*")
    list(FILTER SUBPROJECT_MM_SOURCES EXCLUDE REGEX  "${ICQ_ROOT}/gui/voip/*")
    list(FILTER SUBPROJECT_SOURCES EXCLUDE REGEX  "${ICQ_ROOT}/gui/voip/*")
    list(REMOVE_ITEM SUBPROJECT_HEADERS "${ICQ_ROOT}/gui/main_window/settings/SettingsVoip.h")
    list(REMOVE_ITEM SUBPROJECT_SOURCES "${ICQ_ROOT}/gui/main_window/settings/SettingsVoip.cpp")
endif()

if (STRIP_AV_MEDIA)
    list(FILTER SUBPROJECT_HEADERS EXCLUDE REGEX  "${ICQ_ROOT}/gui/main_window/mplayer/*")
    list(FILTER SUBPROJECT_MM_SOURCES EXCLUDE REGEX  "${ICQ_ROOT}/gui/main_window/mplayer/*")
    list(FILTER SUBPROJECT_SOURCES EXCLUDE REGEX  "${ICQ_ROOT}/gui/main_window/mplayer/*")

    list(REMOVE_ITEM SUBPROJECT_HEADERS "${ICQ_ROOT}/gui/memory_stats/FFmpegPlayerMemMonitor.h")
    list(REMOVE_ITEM SUBPROJECT_SOURCES "${ICQ_ROOT}/gui/memory_stats/FFmpegPlayerMemMonitor.cpp")

    list(REMOVE_ITEM SUBPROJECT_HEADERS "${ICQ_ROOT}/gui/previewer/FfmpegViewerImpl.h")
    list(REMOVE_ITEM SUBPROJECT_SOURCES "${ICQ_ROOT}/gui/previewer/FfmpegViewerImpl.cpp")

    list(FILTER SUBPROJECT_HEADERS EXCLUDE REGEX  "${ICQ_ROOT}/gui/media/ptt/*")
    list(FILTER SUBPROJECT_MM_SOURCES EXCLUDE REGEX  "${ICQ_ROOT}/gui/media/ptt/*")
    list(FILTER SUBPROJECT_SOURCES EXCLUDE REGEX  "${ICQ_ROOT}/gui/media/ptt/*")

    list(FILTER SUBPROJECT_HEADERS EXCLUDE REGEX  "${ICQ_ROOT}/gui/main_window/input_widget/panels/PanelPtt*")
    list(FILTER SUBPROJECT_MM_SOURCES EXCLUDE REGEX  "${ICQ_ROOT}/gui/main_window/input_widget/panels/PanelPtt*")
    list(FILTER SUBPROJECT_SOURCES EXCLUDE REGEX  "${ICQ_ROOT}/gui/main_window/input_widget/panels/PanelPtt*")

    list(FILTER SUBPROJECT_HEADERS EXCLUDE REGEX  "${ICQ_ROOT}/gui/main_window/sounds/*")
    list(FILTER SUBPROJECT_MM_SOURCES EXCLUDE REGEX  "${ICQ_ROOT}/gui/main_window/sounds/*")
    list(FILTER SUBPROJECT_SOURCES EXCLUDE REGEX  "${ICQ_ROOT}/gui/main_window/sounds/*")
endif()

set_source_group("sources" "${SUBPROJECT_ROOT}" ${SUBPROJECT_SOURCES} ${SUBPROJECT_MM_SOURCES} ${SUBPROJECT_HEADERS})


# --------------------------   common    -------------------------
if (NOT LINUX)
    set(COMMON_ROOT "${ICQ_ROOT}/common.shared")
else()
    set(COMMON_ROOT "${ICQ_ROOT}/common.shared/message_processing")
endif()

find_sources(COMMON_SOURCES "${COMMON_ROOT}" "cpp")
find_sources(COMMON_HEADERS "${COMMON_ROOT}" "h")

if (STRIP_CRASH_HANDLER)
    list(FILTER COMMON_HEADERS EXCLUDE REGEX  "${ICQ_ROOT}/common.shared/crash_report/*")
    list(FILTER COMMON_SOURCES EXCLUDE REGEX  "${ICQ_ROOT}/common.shared/crash_report/*")
endif()

set_source_group("sources" "${COMMON_ROOT}" ${COMMON_SOURCES} ${COMMON_HEADERS})


# -------------------------   gui.shared   -----------------------
set(GUI_SHARED_ROOT "${ICQ_ROOT}/gui.shared")

find_sources(GUI_SHARED_SOURCES "${GUI_SHARED_ROOT}" "cpp")
find_sources(GUI_SHARED_HEADERS "${GUI_SHARED_ROOT}" "h")

set_source_group("sources" "${GUI_SHARED_ROOT}" ${GUI_SHARED_SOURCES} ${GUI_SHARED_HEADERS})


# ----------------------------   mac   ---------------------------
if(APPLE)
    set(ICQ_PROTOLIB_ROOT "${ICQ_PLATFORM_SPECIFIC}/protolib")

    include_directories(${ICQ_PROTOLIB_ROOT})

    find_sources(ICQ_PROTOLIB_SOURCES "${ICQ_PROTOLIB_ROOT}" "cpp")
    find_sources(ICQ_PROTOLIB_MM_SOURCES "${ICQ_PROTOLIB_ROOT}" "mm")
    find_sources(ICQ_PROTOLIB_HEADERS "${ICQ_PROTOLIB_ROOT}" "h")

    set_source_group("protolib" "${ICQ_PROTOLIB_ROOT}"
        ${ICQ_PROTOLIB_SOURCES} ${ICQ_PROTOLIB_MM_SOURCES} ${ICQ_PROTOLIB_HEADERS})

    set(ICQ_SSKEYCHAIN_ROOT "${ICQ_PLATFORM_SPECIFIC}/SSKeychain")

    include_directories(${ICQ_SSKEYCHAIN_ROOT})

    find_sources(ICQ_SSKEYCHAIN_MM_SOURCES "${ICQ_SSKEYCHAIN_ROOT}" "mm")
    find_sources(ICQ_SSKEYCHAIN_HEADERS "${ICQ_SSKEYCHAIN_ROOT}" "h")

    set_source_group("SSKeychain" "${ICQ_SSKEYCHAIN_ROOT}"
        ${ICQ_SSKEYCHAIN_MM_SOURCES} ${ICQ_SSKEYCHAIN_HEADERS})

endif()

# --------------------------    mocs    --------------------------
generate_mocs(ICQ_MOC_FILES ${SUBPROJECT_HEADERS} ${COMMON_HEADERS} ${GUI_SHARED_HEADERS})


# -------------------------  translations  -----------------------
set(ICQ_LANGUAGES ar cs de en fr pt ru uk zh tr vi es)
set(ICQ_TRANSLATIONS_DIR "${SUBPROJECT_ROOT}/translations")
set(ICQ_LANGUAGE_FILES)
set(ICQ_QM_FILES)

message(STATUS "ICQ_LANGUAGES = ${ICQ_LANGUAGES}")
message(STATUS "ICQ_TRANSLATIONS_DIR = ${ICQ_TRANSLATIONS_DIR}")
message(STATUS "ICQ_LANGUAGE_FILES = ${ICQ_LANGUAGE_FILES}")
message(STATUS "ICQ_QM_FILES = ${ICQ_QM_FILES}")

get_filename_component(ICQ_LRELEASE_EXE "${ICQ_QT_BIN_DIR}/lrelease" ABSOLUTE)
foreach(lang IN ITEMS ${ICQ_LANGUAGES})
    set(lang_file "${ICQ_TRANSLATIONS_DIR}/${lang}.ts")
    set(qm_file "${ICQ_TRANSLATIONS_DIR}/${lang}.qm")
    add_custom_command(OUTPUT "${qm_file}"
        COMMAND ${ICQ_LRELEASE_EXE} ${lang_file} -qm ${qm_file}
        DEPENDS ${lang_file})
    list(APPEND ICQ_LANGUAGE_FILES "${lang_file}")
    list(APPEND ICQ_QM_FILES "${qm_file}")
endforeach()
source_group("translations" FILES ${ICQ_LANGUAGE_FILES})
source_group("translations" FILES ${ICQ_QM_FILES})

# ----------------------------------------------------------------

set(ICQ_RESOURCES_DIR "${SUBPROJECT_ROOT}/resources")
message(STATUS "ICQ_RESOURCES_DIR = ${ICQ_RESOURCES_DIR}")

# ----------------------  check themes json  ---------------------
set(ICQ_THEMES_META_PATH "${PRODUCT_SPEC_DIR}/app/resources/themes/meta.json")
set(ICQ_THEMES_CHECKER "${ICQ_ROOT}/buildsystem/check_json.py")
set(ICQ_THEMES_CHECKER_STAMP "${PROJECT_BINARY_DIR}/json_check.stamp")

find_package(PythonInterp)
# configure time
message(STATUS "Configure time checking themes JSON ${ICQ_THEMES_META_PATH}")
message(STATUS "Executing ${PYTHON_EXECUTABLE} ${ICQ_THEMES_CHECKER} ${ICQ_THEMES_META_PATH}")
execute_process(
        WORKING_DIRECTORY ${ICQ_ROOT}
        COMMAND ${PYTHON_EXECUTABLE} ${ICQ_THEMES_CHECKER} ${ICQ_THEMES_META_PATH}
        RESULT_VARIABLE RETURN_VALUE
        )
if (NOT RETURN_VALUE EQUAL 0)
    message(FATAL_ERROR "Themes JSON check failed, return value ${RETURN_VALUE}")
else()
    message(STATUS "Themes JSON ${ICQ_THEMES_META_PATH} is valid")
endif()

# build time
add_custom_command(
        DEPENDS ${ICQ_THEMES_META_PATH}
        WORKING_DIRECTORY ${ICQ_ROOT}
        COMMAND ${PYTHON_EXECUTABLE} ${ICQ_THEMES_CHECKER} ${ICQ_THEMES_META_PATH}
        COMMAND ${CMAKE_COMMAND} -E touch ${ICQ_THEMES_CHECKER_STAMP}
        OUTPUT ${ICQ_THEMES_CHECKER_STAMP}
        COMMENT "Build time checking themes JSON ${ICQ_THEMES_META_PATH}"
        )

add_custom_target(check_themes_meta DEPENDS ${ICQ_THEMES_CHECKER_STAMP})

# --------------------------  resources  -------------------------

file(GLOB_RECURSE ICQ_GIF_FILES "${SUBPROJECT_ROOT}/*.gif")
file(GLOB_RECURSE ICQ_ICO_FILES "${SUBPROJECT_ROOT}/*.ico")
file(GLOB_RECURSE ICQ_JPG_FILES "${SUBPROJECT_ROOT}/*.jpg")
file(GLOB_RECURSE ICQ_PNG_FILES "${SUBPROJECT_ROOT}/*.png")
file(GLOB_RECURSE ICQ_TTF_FILES "${SUBPROJECT_ROOT}/*.ttf")
file(GLOB_RECURSE ICQ_WAV_FILES "${SUBPROJECT_ROOT}/*.wav")

set_source_group("resources" ${ICQ_RESOURCES_DIR} ${ICQ_GIF_FILES} ${ICQ_ICO_FILES}
    ${ICQ_JPG_FILES} ${ICQ_PNG_FILES} ${ICQ_TTF_FILES} ${ICQ_WAV_FILES})

file(GLOB_RECURSE ICQ_QSS_FILES "${SUBPROJECT_ROOT}/*.qss")
set_source_group("qss" ${SUBPROJECT_ROOT} ${ICQ_QSS_FILES})

set(ICQ_RESOURCE_FILES ${ICQ_QSS_FILES} ${ICQ_GIF_FILES} ${ICQ_ICO_FILES}
    ${ICQ_JPG_FILES} ${ICQ_PNG_FILES} ${ICQ_TTF_FILES} ${ICQ_WAV_FILES})

if(APPLE)
    set_source_files_properties(${ICQ_GIF_FILES} PROPERTIES XCODE_EXPLICIT_FILE_TYPE "compiled" )
    set_source_files_properties(${ICQ_ICO_FILES} PROPERTIES XCODE_EXPLICIT_FILE_TYPE "compiled" )
    set_source_files_properties(${ICQ_JPG_FILES} PROPERTIES XCODE_EXPLICIT_FILE_TYPE "compiled" )
    set_source_files_properties(${ICQ_PNG_FILES} PROPERTIES XCODE_EXPLICIT_FILE_TYPE "compiled" )
    set_source_files_properties(${ICQ_TTF_FILES} PROPERTIES XCODE_EXPLICIT_FILE_TYPE "compiled" )
    set_source_files_properties(${ICQ_WAV_FILES} PROPERTIES XCODE_EXPLICIT_FILE_TYPE "compiled" )
    set_source_files_properties(${ICQ_QSS_FILES} PROPERTIES XCODE_EXPLICIT_FILE_TYPE "compiled" )
endif()

set(ICQ_RESOURCES "${SUBPROJECT_ROOT}/resource.qrc")

set(PRODUCT_RESOURCES "${PRODUCT_SPEC_DIR}/app/resources/product.qrc")
message(STATUS "ICQ_RESOURCES = ${ICQ_RESOURCES}")

message(STATUS "PRODUCT_RESOURCES = ${PRODUCT_RESOURCES}")

get_filename_component(ICQ_RCC_EXE "${ICQ_QT_BIN_DIR}/rcc" ABSOLUTE)
if(MSVC)
    message(STATUS "Making BIG resources from ${ICQ_RESOURCES} and ${PRODUCT_RESOURCES}")
    icq_add_big_resources(RCC_FILES ${ICQ_RESOURCES} ${PRODUCT_RESOURCES})

    set(RC_FILE "${SUBPROJECT_ROOT}/Resource.rc")
    set(RC_SOURCE "${PRODUCT_SPEC_DIR}/app/resources/Resource.rc")
    add_custom_command(OUTPUT ${RC_FILE}
        DEPENDS ${RC_SOURCE}
        COMMAND ${CMAKE_COMMAND} -E copy ${RC_SOURCE} ${RC_FILE})
    add_custom_target(generate_rc DEPENDS ${RC_FILE})
    source_group("sources" FILES ${RC_FILE})
    file(COPY "${PRODUCT_SPEC_DIR}/app/resources/main_window/appicon.ico" DESTINATION ${SUBPROJECT_ROOT})
    if(NOT VOIP_AS_MODULE)
        # do not works when open project in msvc
        set(ADDITIONAL_FILES ${ADDITIONAL_FILES} ${RC_FILE})
    endif()
else()
    set(ICQ_RESOURCES_OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/qresource.cpp")
    add_custom_command(OUTPUT ${ICQ_RESOURCES_OUTPUT}
        COMMAND ${ICQ_RCC_EXE} ${ICQ_RESOURCES} ${PRODUCT_RESOURCES} -o ${ICQ_RESOURCES_OUTPUT}
        DEPENDS ${ICQ_QM_FILES} ${ICQ_RESOURCES} ${PRODUCT_RESOURCES} ${ICQ_RESOURCE_FILES})
endif()
source_group("resources" FILES ${ICQ_RESOURCES})
source_group("resources" FILES ${ICQ_RESOURCES_OUTPUT})

# ----------------------------------------------------------------
include_directories(${SUBPROJECT_ROOT})

if(MSVC)
    use_precompiled_header_msvc("stdafx.h" "${SUBPROJECT_ROOT}/stdafx.cpp"
        ${SUBPROJECT_SOURCES} ${COMMON_SOURCES} ${GUI_SHARED_SOURCES} ${ICQ_MOC_FILES})
elseif(APPLE)
    use_precompiled_header_mac(PCH_BIN "${SUBPROJECT_ROOT}/stdafx.h"
        ${SUBPROJECT_SOURCES} ${COMMON_SOURCES} ${GUI_SHARED_SOURCES} ${ICQ_MOC_FILES})
elseif(LINUX)
    use_precompiled_header_linux(PCH_BIN "${SUBPROJECT_ROOT}/stdafx.h"
        ${SUBPROJECT_SOURCES} ${COMMON_SOURCES} ${GUI_SHARED_SOURCES} ${ICQ_MOC_FILES})
endif()

add_executable(${PROJECT_NAME} MACOSX_BUNDLE WIN32
    ${SUBPROJECT_SOURCES} ${SUBPROJECT_MM_SOURCES} ${SUBPROJECT_HEADERS}
    ${COMMON_SOURCES} ${COMMON_HEADERS}
    ${GUI_SHARED_SOURCES} ${GUI_SHARED_HEADERS}
    ${BREAKPAD_SOURCE}
    ${ICQ_PROTOLIB_SOURCES} ${ICQ_PROTOLIB_MM_SOURCES} ${ICQ_PROTOLIB_HEADERS}
    ${ICQ_SSKEYCHAIN_MM_SOURCES} ${ICQ_SSKEYCHAIN_HEADERS}
    ${ICQ_MOC_FILES}
    ${ICQ_LANGUAGE_FILES}
    ${ICQ_QM_FILES}
    ${ICQ_RESOURCES_OUTPUT}
    ${ADDITIONAL_FILES}
    ${PCH_BIN}
    ${ICQ_RESOURCE_FILES}
    ${RCC_FILES}
    ${MACOSX_BUNDLE_ICON_FILE}
    ${MACOSX_BUNDLE_INFO_PLIST})

add_dependencies(${PROJECT_NAME} check_themes_meta)

# if(IM_AUTO_TESTING)
# message(STATUS "... add_executable ${MAC_XCTEST}")
# add_executable(${MAC_XCTEST})
# endif()

set_property(DIRECTORY ${SUBPROJECT_ROOT} PROPERTY VS_STARTUP_PROJECT ${PROJECT_NAME})

if(MSVC)
    #add_dependencies(${PROJECT_NAME} GENERATE_RC)
endif()

if(ICQ_CORELIB_STATIC_LINKING)
    if(MSVC)
        target_link_libraries(${PROJECT_NAME}
            ${OPEN_AL_LIBRARIES}
            ${FFMPEG_LIBRARIES}
            ${RLOTTIE_LIBRARIES}
            ${QT_LIBRARIES}
            ${SYSTEM_LIBRARIES}
            ${VOIP_LIBRARIES}
            ${ZLIB_LIBRARIES}
            ${PHONENUMBER_LIBRARIES}
            ${PROTOBUF_LIBRARIES}
            ${BREAKPAD_LIBRARIES}
            ${CRASHPAD_LIBRARIES}
            ${RE2_LIBRARIES}
            Iphlpapi.lib
            corelib)
    else()
        message(STATUS "-> target_link_libraries [static linking]")
        if(IM_AUTO_TESTING)
            message(STATUS "... LINUX_TESTING: target_link_libraries with Qt5::Test STATIC_LINKING")
        endif()
        message(STATUS "ICU_LIBRARIES -> ${ICU_LIBRARIES}")
        target_link_libraries(${PROJECT_NAME}
        ${OPEN_AL_LIBRARIES}
        ${FFMPEG_LIBRARIES}
        ${RLOTTIE_LIBRARIES}
        ${QT_LIBRARIES}
        ${PHONENUMBER_LIBRARIES}
        ${ICU_LIBRARIES}
        ${SYSTEM_LIBRARIES}
        ${BREAKPAD_LIBRARIES}
        ${CRASHPAD_LIBRARIES}
        ${RE2_LIBRARIES}
        corelib)
    endif()
else()
    if(MSVC)
        target_link_libraries(${PROJECT_NAME}
            ${OPEN_AL_LIBRARIES}
            ${FFMPEG_LIBRARIES}
            ${RLOTTIE_LIBRARIES}
            ${QT_LIBRARIES}
            ${ZLIB_LIBRARIES}
            Iphlpapi.lib
            ${SYSTEM_LIBRARIES}
            ${BREAKPAD_LIBRARIES}
            ${CRASHPAD_LIBRARIES})
    else()
        message(STATUS "-> target_link_libraries [dynamic linking]")
        if(IM_AUTO_TESTING)
            message(STATUS "... LINUX_TESTING: target_link_libraries with Qt5::Test DYNAMIC_LINKING")
            target_link_libraries(${PROJECT_NAME}
                ${OPEN_AL_LIBRARIES}
                ${FFMPEG_LIBRARIES}
                ${RLOTTIE_LIBRARIES}
                ${QT_LIBRARIES}
                ${ZLIB_LIBRARIES}
                ${SYSTEM_LIBRARIES}
                ${BREAKPAD_LIBRARIES}
                ${CRASHPAD_LIBRARIES}
                Qt5::Test)
        else()
            target_link_libraries(${PROJECT_NAME}
                ${OPEN_AL_LIBRARIES}
                ${FFMPEG_LIBRARIES}
                ${RLOTTIE_LIBRARIES}
                ${QT_LIBRARIES}
                ${ZLIB_LIBRARIES}
                ${SYSTEM_LIBRARIES}
                ${BREAKPAD_LIBRARIES}
                ${CRASHPAD_LIBRARIES})
        endif()
    endif()
    add_dependencies(${PROJECT_NAME} corelib)
endif()


if(MSVC)
    if(ICQ_DEBUG)
        set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} /NODEFAULTLIB:\"libcmt.lib\"")
    endif()
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} /SUBSYSTEM:WINDOWS")


    # Rename icq.exe to product.exe
    if(NOT APP_OUT_EXE_NAME)
        # icq.exe, magent.exe, myteam.exe etc
        set(APP_OUT_EXE_NAME "icq")
    endif()
    set_target_properties(${PROJECT_NAME} PROPERTIES OUTPUT_NAME "${APP_OUT_EXE_NAME}")

endif()


# ---------------------------   bundle   -------------------------
if(APPLE)

    set(ICQ_BUNDLE "${ICQ_BIN_DIR}/${PROJECT_NAME}.app")

    set(ICQ_BUNDLE_DIR "${ICQ_BUNDLE}/Contents")

    set(ICQ_PLIST "${ICQ_BUNDLE_DIR}/Info.plist")
    set(PLIST_BUDDY "/usr/libexec/PlistBuddy")

    set(MACOSX_BUNDLE_INFO_PLIST "${PRODUCT_MAC}/Info.plist")
    SET_TARGET_PROPERTIES(${PROJECT_NAME} PROPERTIES MACOSX_BUNDLE_INFO_PLIST "${PRODUCT_MAC}/Info.plist")

    message(STATUS "DIR ICQ_BIN_DIR    = ${ICQ_BIN_DIR}")
    message(STATUS "DIR ICQ_BUNDLE     = ${ICQ_BUNDLE}")
    message(STATUS "DIR ICQ_BUNDLE_DIR = ${ICQ_BUNDLE_DIR}")
    message(STATUS "DIR ICQ_PLIST      = ${ICQ_PLIST}")

    message(STATUS "-> copy ${PRODUCT_MAC}/Info.plist to ${ICQ_PLIST}")
    add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
            COMMAND cp -f "${PRODUCT_MAC}/Info.plist" ${ICQ_PLIST})

    string(TIMESTAMP COPYRIGHT_YEAR "%Y")
    message(STATUS "-> COPYRIGHT_YEAR = ${COPYRIGHT_YEAR}")
    set(PLIST_COPYRIGHT_MESSAGE "Copyright © ${COPYRIGHT_YEAR} Mail.ru. All rights reserved.")

    message(STATUS "-> plist Set :NSHumanReadableCopyright ${PLIST_COPYRIGHT_MESSAGE}")
    add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
        COMMAND ${PLIST_BUDDY} -c "Set :NSHumanReadableCopyright ${PLIST_COPYRIGHT_MESSAGE}" ${ICQ_PLIST} VERBATIM)

    message(STATUS "-> plist Set :LSMinimumSystemVersion ${CMAKE_OSX_DEPLOYMENT_TARGET}")
    add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
        COMMAND ${PLIST_BUDDY} -c "Set :LSMinimumSystemVersion ${CMAKE_OSX_DEPLOYMENT_TARGET}" ${ICQ_PLIST} VERBATIM)

    message(STATUS "copy ${ICQ_MAC_ROOT}/PkgInfo")
    add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy "${ICQ_MAC_ROOT}/PkgInfo" ${ICQ_BUNDLE_DIR})

    set(ICQ_FRAMEWORKS_BUNDLE_DIR "${ICQ_BUNDLE_DIR}/Frameworks")
    message(STATUS "DIR ICQ_FRAMEWORKS_BUNDLE_DIR = ${ICQ_FRAMEWORKS_BUNDLE_DIR}")
    add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E make_directory ${ICQ_FRAMEWORKS_BUNDLE_DIR})

    set(ICQ_QT_BUNDLE_DIR "${ICQ_FRAMEWORKS_BUNDLE_DIR}/Qt")
    message(STATUS "DIR ICQ_QT_BUNDLE_DIR = ${ICQ_QT_BUNDLE_DIR}")
    add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E make_directory ${ICQ_QT_BUNDLE_DIR})

    message(STATUS "make_directory ${ICQ_PLUGINS_BUNDLE_DIR}")
    set(ICQ_PLUGINS_BUNDLE_DIR "${ICQ_BUNDLE_DIR}/PlugIns")
        add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E make_directory ${ICQ_PLUGINS_BUNDLE_DIR})

    if ((NOT BUILD_FOR_STORE) AND (NOT STRIP_CRASH_HANDLER))
        message(STATUS "make_directory ${ICQ_HELPERS_BUNDLE_DIR}")
        set(ICQ_HELPERS_BUNDLE_DIR "${ICQ_BUNDLE_DIR}/Helpers")
            add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E make_directory ${ICQ_HELPERS_BUNDLE_DIR})

        add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
            COMMAND cp -f ${ICQ_CRASHPAD}/bin/crashpad_handler ${ICQ_HELPERS_BUNDLE_DIR})
    endif()

    if(ICQ_UPDATE)
        message(STATUS "-> copy Autoupdate.app")
        add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E make_directory ${ICQ_PLATFORM_SPECIFIC}/Sparkle.framework/Versions/A/Resources/Autoupdate.app/Contents/PlugIns)
        message(STATUS "-> copy Sparkle.framework")
        add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
            COMMAND rsync -K -a --exclude=Headers --exclude=Versions/A/Headers "${ICQ_PLATFORM_SPECIFIC}/Sparkle.framework" "${ICQ_FRAMEWORKS_BUNDLE_DIR}")
        message(STATUS "-> install_name_tool Sparkle.framework")
        add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
            COMMAND install_name_tool -change "@rpath/Sparkle.framework/Versions/A/Sparkle" "@executable_path/../Frameworks/Sparkle.framework/Sparkle" ${ICQ_BUNDLE_DIR}/MacOS/${PROJECT_NAME})
    endif()

    if(IM_AUTO_TESTING)
        message(STATUS "... copying XCTest.framework")
        add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
            COMMAND rsync -K -a --exclude=Headers --exclude=Versions/A/Headers "${ICQ_PLATFORM_SPECIFIC}/XCTest.framework" "${ICQ_FRAMEWORKS_BUNDLE_DIR}")
    endif()

    set(ICQ_RESOURCES_BUNDLE_DIR "${ICQ_BUNDLE_DIR}/Resources")
    message(STATUS "DIR ICQ_RESOURCES_BUNDLE_DIR = ${ICQ_RESOURCES_BUNDLE_DIR}")
    add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E make_directory ${ICQ_RESOURCES_BUNDLE_DIR})

    if(IM_QT_DYNAMIC)
        if(APPLE)
            set(ICQ_PLUGINS_PLATFORMS_BUNDLE_DIR "${ICQ_PLUGINS_BUNDLE_DIR}/platforms")
            add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E make_directory ${ICQ_PLUGINS_PLATFORMS_BUNDLE_DIR})

            set(ICQ_PLUGINS_IMAGE_BUNDLE_DIR "${ICQ_PLUGINS_BUNDLE_DIR}/imageformats")
            add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E make_directory ${ICQ_PLUGINS_IMAGE_BUNDLE_DIR})

            FOREACH(PLUGIN ${QT_IMAGE_PLUGINS_TO_COPY})
                get_filename_component(FILE_NAME ${PLUGIN} NAME)
                add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
                    COMMAND cp -f ${PLUGIN} ${ICQ_PLUGINS_IMAGE_BUNDLE_DIR})
                add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
                    COMMAND install_name_tool -change ${FILE_NAME} @rpath/${FILE_NAME} ${ICQ_BUNDLE_DIR}/MacOS/${PROJECT_NAME})
            ENDFOREACH()

            FOREACH(PLUGIN ${QT_PLATFORMS_PLUGINS_TO_COPY})
                get_filename_component(FILE_NAME ${PLUGIN} NAME)
                add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
                    COMMAND cp -f ${PLUGIN} ${ICQ_PLUGINS_PLATFORMS_BUNDLE_DIR})
                add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
                    COMMAND install_name_tool -change ${FILE_NAME} @rpath/${FILE_NAME} ${ICQ_BUNDLE_DIR}/MacOS/${PROJECT_NAME})
            ENDFOREACH()

            FOREACH(LIBEXEC ${QT_LIBEXEC_TO_COPY})
                get_filename_component(LIBEXEC_NAME ${LIBEXEC} NAME)
                add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
                    COMMAND cp -f ${LIBEXEC} ${ICQ_BUNDLE_DIR}/MacOS)
                add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
                    COMMAND install_name_tool -add_rpath @loader_path/../Frameworks/Qt ${ICQ_BUNDLE_DIR}/MacOS/${LIBEXEC_NAME})
            ENDFOREACH()

            FOREACH(LIBRARY ${QT_LIBRARIES_TO_COPY})
                get_filename_component(FILE_NAME ${LIBRARY} NAME)
                set(ORIGINAL_FILE_NAME ${FILE_NAME})
                string(REPLACE "${QT_LIB_EXT}" "5.${QT_LIB_EXT}" FILE_NAME ${FILE_NAME})
                add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
                    COMMAND cp -f ${LIBRARY} ${ICQ_QT_BUNDLE_DIR})
                add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
                    COMMAND install_name_tool -change @rpath/${FILE_NAME} @rpath/${ORIGINAL_FILE_NAME} ${ICQ_BUNDLE_DIR}/MacOS/${PROJECT_NAME})
                FOREACH(LIBEXEC ${QT_LIBEXEC_TO_COPY})
                    get_filename_component(LIBEXEC_NAME ${LIBEXEC} NAME)
                    add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
                        COMMAND install_name_tool -change @rpath/${FILE_NAME} @rpath/${ORIGINAL_FILE_NAME} ${ICQ_BUNDLE_DIR}/MacOS/${LIBEXEC_NAME})
                ENDFOREACH()
            ENDFOREACH()

            FOREACH(RESOURCE ${QT_RESOURCES_TO_COPY})
                add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
                    COMMAND cp -f -R ${RESOURCE} ${ICQ_BUNDLE_DIR}/Resources)
            ENDFOREACH()

            add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
                COMMAND install_name_tool -add_rpath @loader_path/../Frameworks ${ICQ_BUNDLE_DIR}/MacOS/${PROJECT_NAME})
            add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
                COMMAND install_name_tool -add_rpath @loader_path/../Frameworks/Qt ${ICQ_BUNDLE_DIR}/MacOS/${PROJECT_NAME})
            add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
                COMMAND install_name_tool -add_rpath @loader_path/../PlugIns/imageformats ${ICQ_BUNDLE_DIR}/MacOS/${PROJECT_NAME})
            add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
                COMMAND install_name_tool -add_rpath @loader_path/../PlugIns/platforms ${ICQ_BUNDLE_DIR}/MacOS/${PROJECT_NAME})

            if(IM_AUTO_TESTING)
                add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
                    COMMAND install_name_tool -change "@rpath/XCTest.framework/Versions/A/XCTest" "@executable_path/../Frameworks/XCTest.framework/XCTest" ${ICQ_BUNDLE_DIR}/MacOS/${PROJECT_NAME})
            endif()
        endif()
    endif()

    # rename bundle
    if(NOT "${PROJECT_NAME}" STREQUAL "${APP_OUTPUT_NAME}")
        message(STATUS "[rename] Bundle will be renamed from [${PROJECT_NAME}] to [${APP_OUTPUT_NAME}.app]")

        add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E rename "${ICQ_BUNDLE}/Contents/MacOS/${PROJECT_NAME}" "${ICQ_BUNDLE}/Contents/MacOS/${APP_OUTPUT_NAME}"
            COMMAND ${CMAKE_COMMAND} -E rename "${ICQ_BUNDLE}" "${ICQ_BIN_DIR}/${APP_OUTPUT_NAME}.app"
        )

    endif()


endif()
