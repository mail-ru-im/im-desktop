cmake_minimum_required(VERSION 3.19)


project(core)

message(STATUS "")
message(STATUS "[CMAKE]")
message(STATUS "[CMAKE] including <core/CMakeLists.txt>")
message(STATUS "[CMAKE]")

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


# --------------------------    core     -------------------------
set(SUBPROJECT_ROOT "${ICQ_ROOT}/core")

find_sources(SUBPROJECT_SOURCES "${SUBPROJECT_ROOT}" "cpp")
find_sources(SUBPROJECT_HEADERS "${SUBPROJECT_ROOT}" "h")

if(APPLE)
    find_sources(SUBPROJECT_MM_SOURCES "${SUBPROJECT_ROOT}" "mm")
endif()

set_source_group("sources" "${SUBPROJECT_ROOT}" ${SUBPROJECT_SOURCES} ${SUBPROJECT_MM_SOURCES} ${SUBPROJECT_HEADERS})

if(MSVC)
    find_sources(SUBPROJECT_HEADERS "${SUBPROJECT_ROOT}" "natvis")
endif()

if (STRIP_VOIP)
    list(FILTER SUBPROJECT_HEADERS EXCLUDE REGEX  "${SUBPROJECT_ROOT}/Voip/*")
    list(FILTER SUBPROJECT_MM_SOURCES EXCLUDE REGEX  "${SUBPROJECT_ROOT}/Voip/*")
    list(FILTER SUBPROJECT_SOURCES EXCLUDE REGEX  "${SUBPROJECT_ROOT}/Voip/*")
endif()

if (STRIP_NET_CHANGE_NOTIFY)
    add_definitions(-DSTRIP_NET_CHANGE_NOTIFY)
    list(FILTER SUBPROJECT_HEADERS EXCLUDE REGEX  "${SUBPROJECT_ROOT}/network/network*")
    list(FILTER SUBPROJECT_MM_SOURCES EXCLUDE REGEX  "${SUBPROJECT_ROOT}/network/network*")
    list(FILTER SUBPROJECT_SOURCES EXCLUDE REGEX  "${SUBPROJECT_ROOT}/network/network*")

    list(FILTER SUBPROJECT_HEADERS EXCLUDE REGEX  "${SUBPROJECT_ROOT}/network/internal/*")
    list(FILTER SUBPROJECT_MM_SOURCES EXCLUDE REGEX  "${SUBPROJECT_ROOT}/network/internal*")
    list(FILTER SUBPROJECT_SOURCES EXCLUDE REGEX  "${SUBPROJECT_ROOT}/network/internal*")

    list(FILTER SUBPROJECT_HEADERS EXCLUDE REGEX  "${SUBPROJECT_ROOT}/network/linux/*")
    list(FILTER SUBPROJECT_MM_SOURCES EXCLUDE REGEX  "${SUBPROJECT_ROOT}/network/linux/*")
    list(FILTER SUBPROJECT_SOURCES EXCLUDE REGEX  "${SUBPROJECT_ROOT}/network/linux/*")

    list(FILTER SUBPROJECT_HEADERS EXCLUDE REGEX  "${SUBPROJECT_ROOT}/network/macos/*")
    list(FILTER SUBPROJECT_MM_SOURCES EXCLUDE REGEX  "${SUBPROJECT_ROOT}/network/macos/*")
    list(FILTER SUBPROJECT_SOURCES EXCLUDE REGEX  "${SUBPROJECT_ROOT}/network/macos/*")

    list(FILTER SUBPROJECT_HEADERS EXCLUDE REGEX  "${SUBPROJECT_ROOT}/network/win32/*")
    list(FILTER SUBPROJECT_MM_SOURCES EXCLUDE REGEX  "${SUBPROJECT_ROOT}/network/win32/*")
    list(FILTER SUBPROJECT_SOURCES EXCLUDE REGEX  "${SUBPROJECT_ROOT}/network/win32/*")
endif()

# --------------------------   common    -------------------------
set(COMMON_ROOT "${ICQ_ROOT}/common.shared")

find_sources(COMMON_SOURCES "${COMMON_ROOT}" "cpp")
find_sources(COMMON_HEADERS "${COMMON_ROOT}" "h")

if (STRIP_CRASH_HANDLER)
    list(FILTER COMMON_SOURCES EXCLUDE REGEX  "${COMMON_ROOT}/crash_report/*")
    list(FILTER COMMON_HEADERS EXCLUDE REGEX  "${COMMON_ROOT}/crash_report/*")
endif()

set_source_group("sources" "${COMMON_ROOT}" ${COMMON_SOURCES} ${COMMON_HEADERS})


# --------------------------  minizip    -------------------------
if(NOT WIN32)
    set(MINIZIP_ROOT "${pkg_src_minizip_location}/include")
    message(STATUS "[core][minizip] MINIZIP_ROOT = ${MINIZIP_ROOT}")

    find_sources(MINIZIP_SOURCES "${MINIZIP_ROOT}" "c")
    find_sources(MINIZIP_HEADERS "${MINIZIP_ROOT}" "h")

    list(REMOVE_ITEM MINIZIP_SOURCES "${MINIZIP_ROOT}/iowin32.c")
    list(REMOVE_ITEM MINIZIP_HEADERS "${MINIZIP_ROOT}/iowin32.h")
    list(REMOVE_ITEM MINIZIP_SOURCES "${MINIZIP_ROOT}/miniunz.c")
    list(REMOVE_ITEM MINIZIP_SOURCES "${MINIZIP_ROOT}/minizip.c")

    message(STATUS "[core][minizip] set_source_group(minizip ${MINIZIP_ROOT} ${MINIZIP_SOURCES} ${MINIZIP_HEADERS})")
    set_source_group("minizip" "${MINIZIP_ROOT}" ${MINIZIP_SOURCES} ${MINIZIP_HEADERS})

endif()


# ---------------------  zstd dictionaries sources ----------------------
if (STRIP_ZSTD)
    list(FILTER SUBPROJECT_HEADERS EXCLUDE REGEX  "${SUBPROJECT_ROOT}/zstd*")
    list(FILTER SUBPROJECT_MM_SOURCES EXCLUDE REGEX  "${SUBPROJECT_ROOT}/zstd*")
    list(FILTER SUBPROJECT_SOURCES EXCLUDE REGEX  "${SUBPROJECT_ROOT}/zstd*")
else()
    set(ZSTD_DICTIONARIES_ROOT ${SUBPROJECT_ROOT}/zstd_wrap/zstd_dicts)
    set(ZSTD_DICTIONARIES_MD5SUM ${ZSTD_DICTIONARIES_ROOT}/.md5sums)
    set(ZSTD_DICTIONARIES_HEADERS ${ZSTD_DICTIONARIES_ROOT}/internal_request_dict.h ${ZSTD_DICTIONARIES_ROOT}/internal_response_dict.h)
    set(ZSTD_DICTIONARIES_SOURCES ${ZSTD_DICTIONARIES_ROOT}/internal_request_dict.cpp ${ZSTD_DICTIONARIES_ROOT}/internal_response_dict.cpp)
    set_source_group("zstd_dicts" "${ZSTD_DICTIONARIES_ROOT}" ${ZSTD_DICTIONARIES_HEADERS} ${ZSTD_DICTIONARIES_SOURCES})
endif()


# ----------------------------------------------------------------
include_directories(${SUBPROJECT_ROOT})

if(MSVC)
    use_precompiled_header_msvc("stdafx.h" "${SUBPROJECT_ROOT}/stdafx.cpp" ${SUBPROJECT_SOURCES} ${COMMON_SOURCES})
elseif(APPLE)
    use_precompiled_header_mac(PCH_BIN "${SUBPROJECT_ROOT}/stdafx.h" ${SUBPROJECT_SOURCES} ${COMMON_SOURCES})
elseif(LINUX)
    use_precompiled_header_linux(PCH_BIN "${SUBPROJECT_ROOT}/stdafx.h" ${SUBPROJECT_SOURCES} ${COMMON_SOURCES})
endif()

add_library(${PROJECT_NAME} STATIC
    ${SUBPROJECT_SOURCES} ${SUBPROJECT_MM_SOURCES} ${SUBPROJECT_HEADERS}
    ${COMMON_SOURCES} ${COMMON_HEADERS}
    ${MINIZIP_SOURCES} ${MINIZIP_HEADERS}
    ${ZSTD_DICTIONARIES_HEADERS} ${ZSTD_DICTIONARIES_SOURCES}
    ${PCH_BIN})

if(MSVC)
    set (CMAKE_CXX_FLAGS "/EHsc /bigobj")
endif()

set(CONFIG_JSON_PATH "${ICQ_ROOT}/products/${PRODUCT}/config.json")
file(READ ${CONFIG_JSON_PATH} config_data)

file(REMOVE "${ICQ_ROOT}/common.shared/config/config_data.h")
file(WRITE "${ICQ_ROOT}/common.shared/config/config_data.h" "//Autogenerated by CMake from ${CONFIG_JSON_PATH}\n#pragma once\ninline constexpr std::string_view config_json() noexcept\n{\nreturn R\"#(${config_data})#\";\n}")


# --------------------------  copy voip  -------------------------
if (NOT STRIP_VOIP)
    if(VOIP_AS_MODULE)
        target_link_libraries(${PROJECT_NAME}
            PRIVATE
            ${VOIP_LIBRARIES})
    endif()
endif()


# --------------------------  copy d3dcompiler  -------------------------
if(MSVC)
    add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy ${D3D_COMPILER_LIBRARY} ${ICQ_BIN_DIR})
endif()

# --------------------------  copy gpu blacklist  -------------------------
if(MSVC)
    add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy ${GPU_BLACKLIST} ${ICQ_BIN_DIR})
endif()

# --------------------------  copy mesa library  -------------------------
if(MSVC)
    add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy ${MESA_LIBRARY} ${ICQ_BIN_DIR})
    add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy ${MESA_LIBRARY2} ${ICQ_BIN_DIR})
endif()

# -----------------------------  copy Qt  -------------------------------
if(MSVC)
    set(ICQ_PLUGINS_PLATFORMS_DIR "${ICQ_BIN_DIR}/plugins/platforms")
    add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E make_directory ${ICQ_PLUGINS_PLATFORMS_DIR})

    set(ICQ_PLUGINS_IMAGE_DIR "${ICQ_BIN_DIR}/plugins/imageformats")
    add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E make_directory ${ICQ_PLUGINS_IMAGE_DIR})

    set(ICQ_PLUGINS_ICONENGINES_DIR "${ICQ_BIN_DIR}/plugins/iconengines")
    add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E make_directory ${ICQ_PLUGINS_ICONENGINES_DIR})

    set(ICQ_PLUGINS_AUDIO_DIR "${ICQ_BIN_DIR}/plugins/audio")
    add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E make_directory ${ICQ_PLUGINS_AUDIO_DIR})

    set(ICQ_PLUGINS_PRINT_DIR "${ICQ_BIN_DIR}/plugins/printsupport")
    add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E make_directory ${ICQ_PLUGINS_PRINT_DIR})

    if (HAS_WEB_ENGINE)
        set(ICQ_RESOURCES_DIR "${ICQ_BIN_DIR}/resources")
        add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E make_directory ${ICQ_RESOURCES_DIR})
    endif()

    FOREACH(PLUGIN ${QT_PLATFORMS_PLUGINS_TO_COPY})
        add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy ${PLUGIN} ${ICQ_PLUGINS_PLATFORMS_DIR})
    ENDFOREACH()

    FOREACH(PLUGIN ${QT_PRINTER_PLUGINS_TO_COPY})
        add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy ${PLUGIN} ${ICQ_PLUGINS_PRINT_DIR})
    ENDFOREACH()

    FOREACH(PLUGIN ${QT_IMAGE_PLUGINS_TO_COPY})
        add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy ${PLUGIN} ${ICQ_PLUGINS_IMAGE_DIR})
    ENDFOREACH()

    FOREACH(PLUGIN ${QT_ICONENGINES_PLUGINS_TO_COPY})
        add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy ${PLUGIN} ${ICQ_PLUGINS_ICONENGINES_DIR})
    ENDFOREACH()

    FOREACH(PLUGIN ${QT_AUDIO_PLUGINS_TO_COPY})
        add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy ${PLUGIN} ${ICQ_PLUGINS_AUDIO_DIR})
    ENDFOREACH()

    FOREACH(LIBEXEC ${QT_LIBEXEC_TO_COPY})
        add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy ${LIBEXEC} ${ICQ_BIN_DIR})
    ENDFOREACH()

    FOREACH(LIBRARY ${QT_LIBRARIES_TO_COPY})
        add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy ${LIBRARY} ${ICQ_BIN_DIR})
    ENDFOREACH()

    FOREACH(OPENGL_LIBRARY ${OPENGL_LIBRARIES_TO_COPY})
        add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy ${OPENGL_LIBRARY} ${ICQ_BIN_DIR})
    ENDFOREACH()

    if (HAS_WEB_ENGINE)
        FOREACH(RESOURCE ${QT_RESOURCES_TO_COPY})
            add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E copy ${RESOURCE} ${ICQ_RESOURCES_DIR})
        ENDFOREACH()
    endif()

    if (IM_QT_DYNAMIC AND HAS_WEB_ENGINE)
        set(ICQ_TRANSLATIONS_DIR "${ICQ_BIN_DIR}/translations")
        add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E make_directory ${ICQ_TRANSLATIONS_DIR})

        add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_directory ${QT_TRANSLATIONS_DIR} ${ICQ_TRANSLATIONS_DIR})
    endif()

    im_copy_qt_quick_libraries(LIBS ${QT_QUICK_CONTROLS2_LIBRARIES_TO_COPY} FOLDER ${ICQ_BIN_DIR}/QtQuick/Controls.2)
    im_copy_qt_quick_libraries(LIBS ${QT_QUICK_TEMPLATES2_LIBRARIES_TO_COPY} FOLDER ${ICQ_BIN_DIR}/QtQuick/Templates.2)
    im_copy_qt_quick_libraries(LIBS ${QT_QUICK_DIALOGS_LIBRARIES_TO_COPY} FOLDER ${ICQ_BIN_DIR}/QtQuick/Dialogs)
    im_copy_qt_quick_libraries(LIBS ${QT_QUICK_EXTRAS_LIBRARIES_TO_COPY} FOLDER ${ICQ_BIN_DIR}/QtQuick/Extras)
    im_copy_qt_quick_libraries(LIBS ${QT_QUICK_LAYOUTS_LIBRARIES_TO_COPY} FOLDER ${ICQ_BIN_DIR}/QtQuick/Layouts)
    im_copy_qt_quick_libraries(LIBS ${QT_QUICK2_LIBRARIES_TO_COPY} FOLDER ${ICQ_BIN_DIR}/QtQuick.2)

endif()

# --------------------------  copy Runtime Libraries  -------------------------
if(MSVC)
    FOREACH(RUNTIME_LIBRARY ${RUNTIME_LIBRARIES_TO_COPY})
    add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy ${RUNTIME_LIBRARY} ${ICQ_BIN_DIR})
    ENDFOREACH()
endif()

# --------------------------  integrate zstd dictionaries  -------------------------
if (NOT STRIP_ZSTD)
    find_package(PythonInterp 3 REQUIRED)

    add_custom_command(OUTPUT ${ZSTD_DICTIONARIES_HEADERS} ${ZSTD_DICTIONARIES_SOURCES}
        DEPENDS ${ZSTD_DICTIONARIES_MD5SUM}
        WORKING_DIRECTORY ${ICQ_ROOT}
        COMMAND ${PYTHON_EXECUTABLE} buildsystem/prepare_zstd_dict.py ${ZSTD_DICTIONARIES_ROOT} im-zstd-dict-desktop-request request_dict
        COMMAND ${PYTHON_EXECUTABLE} buildsystem/prepare_zstd_dict.py ${ZSTD_DICTIONARIES_ROOT} im-zstd-dict-desktop-response response_dict
        COMMENT "Generating sources files for zstd dictionaries in ${ZSTD_DICTIONARIES_ROOT}")

    add_custom_target(check_zstd_dictionaries
        BYPRODUCTS ${ZSTD_DICTIONARIES_MD5SUM}
        WORKING_DIRECTORY ${ICQ_ROOT}
        COMMAND ${PYTHON_EXECUTABLE} buildsystem/get_files_md5sum.py ${ZSTD_DICTIONARIES_ROOT} ".*\.zdict" > ${ZSTD_DICTIONARIES_MD5SUM}.new
        COMMAND ${CMAKE_COMMAND} -E copy_if_different ${ZSTD_DICTIONARIES_MD5SUM}.new ${ZSTD_DICTIONARIES_MD5SUM}
        COMMAND ${CMAKE_COMMAND} -E remove -f ${ZSTD_DICTIONARIES_MD5SUM}.new
        COMMENT "Checking zstd dictionaries in ${ZSTD_DICTIONARIES_ROOT}")

    add_dependencies(${PROJECT_NAME} check_zstd_dictionaries)
endif()
