cmake_minimum_required(VERSION 3.4)

project(updater)


if(NOT UNIX OR APPLE)
    message(FATAL_ERROR "Only Linux is supported")
endif()

if(NOT APP_TYPE)
    message(FATAL_ERROR "You must specify the application type (-DAPP_TYPE=icq|agent)")
endif()

add_definitions(-DAPP_TYPE="${APP_TYPE}")
add_definitions(-fPIC)
get_filename_component(ICQ_ROOT "${CMAKE_SOURCE_DIR}/.." ABSOLUTE)
set(ICQ_EXTERNAL "${ICQ_ROOT}/external")
set(ICQ_QT_ROOT "${ICQ_EXTERNAL}/qt/linux")

if(LINUX_ARCH EQUAL 32)
    set(LINUX32 ON)
    set(ARCH_FLAGS "-m32")
    set(ICQ_ARCH x86)
    message(STATUS "Linux x32")
else()
    set(LINUX64 ON)
    set(ARCH_FLAGS "-m64")
    set(ICQ_ARCH x64)
    message(STATUS "Linux x64")
endif()

set(ICQ_QT_LIB_DIR "${ICQ_QT_ROOT}/lib/${ICQ_ARCH}")
if(LINUX32)
    set(UPDATER_QRC_SOURCE "${CMAKE_SOURCE_DIR}/resource32.qrc")
    set(UPDATER_BIN_DIR "${CMAKE_SOURCE_DIR}/bin/Release32")
else()
    set(UPDATER_QRC_SOURCE "${CMAKE_SOURCE_DIR}/resource64.qrc")
    set(UPDATER_BIN_DIR "${CMAKE_SOURCE_DIR}/bin/Release64")
endif()

set(UPDATER_QRC "${CMAKE_SOURCE_DIR}/resource.qrc")

set(ICQ_QT_INCLUDE_DIR "${ICQ_QT_ROOT}/include")

set(CMAKE_CXX_STANDARD 14)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${ARCH_FLAGS}")
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} ${ARCH_FLAGS} -static-libgcc -static-libstdc++")

set(QT_LIBRARIES ${ICQ_QT_LIB_DIR}/libQt5Core.a ${ICQ_QT_LIB_DIR}/libqtpcre.a)

set(SYSTEM_LIBRARIES -Wl,-Bdynamic -ldl -lpthread)

include_directories("${ICQ_QT_INCLUDE_DIR}")
include_directories("${ICQ_QT_INCLUDE_DIR}/QtCore")

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${UPDATER_BIN_DIR})

get_filename_component(ICQ_RCC_EXE "${ICQ_QT_ROOT}/bin/rcc" ABSOLUTE)

file(REMOVE ${UPDATER_QRC})

add_custom_command(OUTPUT ${UPDATER_QRC}
                   COMMAND sed -e s/%APP_TYPE%/${APP_TYPE}/ ${UPDATER_QRC_SOURCE} > ${UPDATER_QRC})

add_custom_command(OUTPUT qresource.cpp
                   COMMAND ${ICQ_RCC_EXE} ${UPDATER_QRC} -pass 1 -o qresource.cpp
                   DEPENDS ${UPDATER_QRC})

add_custom_command(OUTPUT qresource.tmp.o
                   COMMAND ${CMAKE_CXX_COMPILER} ${ARCH_FLAGS} -c qresource.cpp -o qresource.tmp.o
                   DEPENDS qresource.cpp)

add_custom_command(OUTPUT qresource_final.o
                   COMMAND ${ICQ_RCC_EXE} ${UPDATER_QRC} -pass 2 -temp qresource.tmp.o -o qresource_final.o
                   DEPENDS qresource.tmp.o)

add_executable(updater main.cpp qresource_final.o)

target_link_libraries(updater ${QT_LIBRARIES} ${SYSTEM_LIBRARIES})
