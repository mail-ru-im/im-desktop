cmake_minimum_required(VERSION 3.4)

project(coretest)

# -------------------------- definitions -------------------------
if(MSVC)
    add_definitions(-D_UNICODE)
endif()


# --------------------------  coretest   -------------------------
set(SUBPROJECT_ROOT "${ICQ_ROOT}/coretest")

find_sources(SUBPROJECT_SOURCES "${SUBPROJECT_ROOT}" "cpp")
find_sources(SUBPROJECT_HEADERS "${SUBPROJECT_ROOT}" "h")

set_source_group("sources" "${SUBPROJECT_ROOT}" ${SUBPROJECT_SOURCES} ${SUBPROJECT_HEADERS})


# ----------------------------------------------------------------
include_directories(${SUBPROJECT_ROOT})

add_executable(${PROJECT_NAME} ${SUBPROJECT_SOURCES} ${SUBPROJECT_HEADERS})

use_precompiled_header_msvc("stdafx.h" "${SUBPROJECT_ROOT}/stdafx.cpp" ${SUBPROJECT_SOURCES})

target_link_libraries(${PROJECT_NAME}
    ${SYSTEM_LIBRARIES}
    ${VOIP_LIBRARIES}
    ${ZLIB_LIBRARIES}
    ${BREAKPAD_LIBRARIES}
    ${RE2_LIBRARIES}
    corelib)
