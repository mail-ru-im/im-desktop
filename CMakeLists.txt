cmake_minimum_required(VERSION 3.14)

include(CheckFunctionExists)

project(icq)

if(NOT PRODUCT)
    set(PRODUCT "icq")
    message("Default product is ${PRODUCT}")
endif()

message(STATUS "PRODUCT is ${PRODUCT}")

add_definitions(-DSUPPORT_EXTERNAL_CONFIG)
message(STATUS "add_definitions(-DSUPPORT_EXTERNAL_CONFIG)")

if(NOT WIN32)
    CHECK_FUNCTION_EXISTS(secure_getenv HAVE_SECURE_GETENV)
    if(HAVE_SECURE_GETENV)
        add_definitions(-DHAVE_SECURE_GETENV=1)
        message(STATUS "add_definitions(-DHAVE_SECURE_GETENV=1)")
    endif()
endif()

if(NOT APP_ENVIRONMENT)
    message(STATUS "... cmake: APP_ENVIRONMENT is empty")
else()
    message(STATUS "... cmake: APP_ENVIRONMENT is ${APP_ENVIRONMENT}")
    add_definitions(-DAPP_ENVIRONMENT="${APP_ENVIRONMENT}")
endif()

if(BUILD_FOR_STORE)
    message(STATUS "... cmake: This build for Store -> BUILD_FOR_STORE")
    add_definitions(-DBUILD_FOR_STORE=1)
    message(STATUS "add_definitions(-DBUILD_FOR_STORE=1)")
endif()

if(NOT ICQ_QT_DYNAMIC)
    add_definitions(-DICQ_QT_STATIC)
    message(STATUS "Qt linking: static")
    message(STATUS "add_definitions(-DICQ_QT_STATIC)")
else()
    add_definitions(-DICQ_QT_DYNAMIC)
    message(STATUS "Qt linking: dynamic")
    message(STATUS "add_definitions(-DICQ_QT_DYNAMIC)")
endif()


message(STATUS "DIR CMAKE_SOURCE_DIR = ${CMAKE_SOURCE_DIR}")
find_package(Git)
if (GIT_FOUND)
    message(STATUS "git found: ${GIT_EXECUTABLE} in version ${GIT_VERSION_STRING}")
    if(GIT_VERSION_STRING VERSION_LESS 2.23.0)
        execute_process(
            COMMAND ${GIT_EXECUTABLE} rev-parse --abbrev-ref HEAD
            WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
            OUTPUT_VARIABLE GIT_BRANCH_NAME
            OUTPUT_STRIP_TRAILING_WHITESPACE
        )
    else()
        execute_process(
            COMMAND ${GIT_EXECUTABLE} branch --show-current
            WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
            OUTPUT_VARIABLE GIT_BRANCH_NAME
            OUTPUT_STRIP_TRAILING_WHITESPACE
        )
    endif()
    message(STATUS "Current GIT BRANCH NAME = ${GIT_BRANCH_NAME}")
    add_definitions("-DGIT_BRANCH_NAME=\"${GIT_BRANCH_NAME}\"")
    message(STATUS "add_definitions(-DGIT_BRANCH_NAME=${GIT_BRANCH_NAME})")
endif (GIT_FOUND)

get_filename_component(ICQ_ROOT "${CMAKE_SOURCE_DIR}" ABSOLUTE)
set(PRODUCT_SPEC_DIR "${ICQ_ROOT}/products/${PRODUCT}")
if(EXISTS "${PRODUCT_SPEC_DIR}")
    message(STATUS "PRODUCT_SPEC_DIR = ${PRODUCT_SPEC_DIR}")
else()
    message(FATAL_ERROR "Folder ${PRODUCT_SPEC_DIR} is not exists!)")
endif()
include(${PRODUCT_SPEC_DIR}/project_settings.cmake)

if(${CMAKE_BUILD_TYPE} STREQUAL "Debug")
    set(ICQ_DEBUG ON)
    set(ICQ_BIN_DIR "${ICQ_ROOT}/bin/Debug")
    set(ICQ_LIB_DIR "${ICQ_ROOT}/bin/Debug")
    set(CMAKE_CONFIGURATION_TYPES "Debug" CACHE STRING "" FORCE)
    if(APPLE)
        add_definitions(-DDEBUG=1)
        message(STATUS "add_definitions(-DDEBUG=1)")
    endif()
elseif(${CMAKE_BUILD_TYPE} STREQUAL "Release")
    set(ICQ_RELEASE ON)
    set(ICQ_BIN_DIR "${ICQ_ROOT}/bin/Release")
    set(ICQ_LIB_DIR "${ICQ_ROOT}/bin/Release")
    set(CMAKE_CONFIGURATION_TYPES "Release" CACHE STRING "" FORCE)
    add_definitions(-DNDEBUG)
else()
    message(FATAL_ERROR "Need to specify CMAKE_BUILD_TYPE (-DCMAKE_BUILDTYPE=Debug|Release)")
endif()

file(MAKE_DIRECTORY ${ICQ_BIN_DIR})

message(STATUS "binaries output: " ${ICQ_BIN_DIR})
message(STATUS "libraries output: " ${ICQ_LIB_DIR})

set(ICQ_CORELIB_STATIC_LINKING ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(ENABLE_ASAN)
  set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fsanitize=address")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fsanitize=address")
  set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -fsanitize=address")
  list(APPEND _build_flags "-fsanitize=address")
endif()
if(ENABLE_TSAN)
  set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fsanitize=thread")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fsanitize=thread")
  set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -fsanitize=thread")
  list(APPEND _build_flags "-fsanitize=thread")
endif()
if(ENABLE_UBSAN)
  set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fsanitize=undefined")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fsanitize=undefined")
  set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -fsanitize=undefined")
  list(APPEND _build_flags "-fsanitize=undefined")
endif()

if(VOIP_AS_MODULE)
  # When building voip library from source (to do this, just add voip3 symlink to voip3 repo), include
  # voip sources before mangling flags, CMake options, etc. to avoid interference with voip build scripts
  add_subdirectory(voip3)
  set(VOIP_LIBRARIES libvoip::sdk_cpp_merged)
endif()


if(MSVC)
    set(ICQ_PLATFORM windows)
    set(ICQ_ARCH x86)

    if(ICQ_DEBUG)
        add_definitions(/MTd)
    else()
        add_definitions(/MT)
        # disable LTO because of https://bugreports.qt.io/browse/QTBUG-41301
        #add_definitions(/GL)
        #set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} /LTCG")
    endif()
    add_definitions(/Zi)
    add_definitions(/MP)
    set(CMAKE_HAVE_PTHREAD_H OFF)
    add_definitions(-D_UNICODE)
    add_definitions(-DUNICODE)
    #add_definitions(-D_VARIADIC_MAX=6)
    add_definitions(-D_SILENCE_ALL_CXX17_DEPRECATION_WARNINGS)
    add_definitions(/Zm500)
    add_definitions(/W4)
    add_definitions(/wd4100)
    add_definitions(/we5038) # Wreorder
    add_definitions(/we4018) # '<': signed/unsigned mismatch
    add_definitions(/we4067)
    add_definitions(/we4389) # '==': signed/unsigned mismatch
    add_definitions(/we4189) # local variable is initialized but not referenced
    add_definitions(/we4172) # returning address of local variable or temporary
    add_definitions(/we4245)
    add_definitions(/we4263)
    add_definitions(/we4264)
    add_definitions(/we4553)
    add_definitions(/we4573)
    add_definitions(/we4834) # 'nodiscard' attribute
    add_definitions(/we4715) # not all control paths return a value
    add_definitions(/we4130) #'==': logical operation on address of string constant
    add_definitions(/we4099) # type name first seen using 'struct' now seen using 'class'
    add_definitions(/we4146)
    add_definitions(/we4114)
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} /STACK:3145728") # for emoji map
    #add_definitions(/we4266) #update boost and qt or disable this warning for these headers

    set(WIN32_COMMON_OPTS "/Ob2 /Oi /Ot /GS- /GF /Gy /Gw /fp:except- /arch:SSE2 /Zc:inline")
    set(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE} ${WIN32_COMMON_OPTS}")
    set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} ${WIN32_COMMON_OPTS}")

    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} /DEBUG /OPT:REF /OPT:ICF")
    set(WIN_SDK_DIR "$ENV{ProgramFiles\(x86\)}/Windows Kits/10")
    set(WIN_SDK_VER "10.0.18362.0")
    include_directories("${WIN_SDK_DIR}/Include/${WIN_SDK_VER}/shared")
    include_directories("${WIN_SDK_DIR}/Include/${WIN_SDK_VER}/winrt")
    include_directories("${WIN_SDK_DIR}/Include/${WIN_SDK_VER}/um")
    link_directories("${WIN_SDK_DIR}/Lib/${WIN_SDK_VER}/um/x86")
elseif(APPLE)
    set(ICQ_PLATFORM macos)
    set(ICQ_ARCH x64)
    set(CMAKE_XCODE_ATTRIBUTE_DEBUG_INFORMATION_FORMAT "dwarf-with-dsym")
    set(CMAKE_XCODE_ATTRIBUTE_GCC_GENERATE_DEBUGGING_SYMBOLS "YES")
    set(CMAKE_XCODE_ATTRIBUTE_CODE_SIGN_IDENTITY "")
    set(CMAKE_OSX_DEPLOYMENT_TARGET 10.12)

    set(MACOS_COMMON_FLAGS "-ffunction-sections -fdata-sections -fno-stack-protector -fvisibility-inlines-hidden -fvisibility=hidden -fasm-blocks -stdlib=libc++")
    set(MACOS_COMMON_FLAGS "${MACOS_COMMON_FLAGS} -mmacosx-version-min=${CMAKE_OSX_DEPLOYMENT_TARGET} -Werror=most -Werror=switch")
    list(APPEND _build_flags "-ffunction-sections")
    list(APPEND _build_flags "-fdata-sections")
    list(APPEND _build_flags "-fno-stack-protector")
    list(APPEND _build_flags "-fvisibility-inlines-hidden")
    list(APPEND _build_flags "-fvisibility=hidden")
    list(APPEND _build_flags "-fasm-blocks")
    list(APPEND _build_flags "-stdlib=libc++")

    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${MACOS_COMMON_FLAGS}")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${MACOS_COMMON_FLAGS} -Werror=inconsistent-missing-override -Werror=effc++")
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -stdlib=libc++")
    add_definitions(-Werror=return-stack-address)
elseif(UNIX)
    set(LINUX ON)
    if(LINUX_ARCH EQUAL 32)
        set(LINUX32 ON)
        set(ICQ_ARCH x86)
        set(ARCH_FLAGS "-m32")
        set(ICQ_PLATFORM linux)
        set(ICQ_BIN_DIR "${ICQ_ROOT}/bin/${CMAKE_CONFIGURATION_TYPES}32")
        set(ICQ_LIB_DIR "${ICQ_ROOT}/bin/${CMAKE_CONFIGURATION_TYPES}32")
        message(STATUS "Linux x32")
    else()
        set(LINUX64 ON)
        set(ICQ_ARCH x64)
        set(ARCH_FLAGS "-m64")
        set(ICQ_PLATFORM linux)
        set(ICQ_BIN_DIR "${ICQ_ROOT}/bin/${CMAKE_CONFIGURATION_TYPES}64")
        set(ICQ_LIB_DIR "${ICQ_ROOT}/bin/${CMAKE_CONFIGURATION_TYPES}64")
        message(STATUS "Linux x64")
    endif()
    add_definitions(-fPIC)
    add_definitions(-Werror=return-local-addr)

    if (RELATIVE_DEBUG_PATHS)
        add_definitions(-fdebug-prefix-map=${ICQ_ROOT}=.)
    endif()

    set(LINUX_COMMON_FLAGS "-ffunction-sections -g -fdata-sections -fno-stack-protector -U_FORTIFY_SOURCE -DBOOST_NO_AUTO_PTR -Wno-deprecated-declarations")
    if(ENABLE_LTO)
      list(APPEND _build_flags "-flto")
      set(LINUX_COMMON_FLAGS "${LINUX_COMMON_FLAGS} -flto")
      set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -flto")
    endif()
    list(APPEND _build_flags "-ffunction-sections")
    list(APPEND _build_flags "-fdata-sections")
    list(APPEND _build_flags "-g")
    list(APPEND _build_flags "-fno-stack-protector")
    list(APPEND _build_flags "-U_FORTIFY_SOURCE")
    list(APPEND _build_flags "-DBOOST_NO_AUTO_PTR")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${ARCH_FLAGS} ${LINUX_COMMON_FLAGS}")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${ARCH_FLAGS} ${LINUX_COMMON_FLAGS}")
    set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -DDEBUG")
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} ${ARCH_FLAGS} -no-pie -Wl,--version-script=${ICQ_ROOT}/icq.exports -Wl,--gc-sections")
    #set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -Wl,-Map=${ICQ_BIN_DIR}/icq.map -Wl,--cref")
    if(NOT DISABLE_STATIC_CXX)
      set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -static-libgcc -static-libstdc++")
    endif()
else()
    message(FATAL_ERROR "Unknown platform")
endif()

#shared definitions for linux and mac
if (NOT MSVC)
    add_definitions(-Werror=reorder)
    add_definitions(-Wno-error=unused) #fixme
    # add_definitions(-Werror=unused-result)
    add_definitions(-Wno-error=missing-braces) #fixme
endif()

if (APPLE)
    add_definitions(-Werror=unused-comparison)
    add_definitions(-Werror=unused-lambda-capture)
endif()

message(STATUS "building on: ${ICQ_PLATFORM}")
if(ICQ_CORELIB_STATIC_LINKING)
    message(STATUS "corelib linking: static")
    add_definitions(-DICQ_CORELIB_STATIC_LINKING)
else()
    message(STATUS "corelib linking: dynamic")
endif()

option(ABORT_ON_ASSERT "Abort on asserts" OFF)

if (ABORT_ON_ASSERT)
    add_definitions(-DABORT_OB_ASSERTS)
endif()

# ---------------------------  paths  ----------------------------
set(CMAKE_EXECUTABLE_OUTPUT_DIRECTORY_DEBUG ${ICQ_BIN_DIR})
set(CMAKE_EXECUTABLE_OUTPUT_DIRECTORY_RELEASE ${ICQ_BIN_DIR})
set(CMAKE_EXECUTABLE_OUTPUT_PATH ${ICQ_BIN_DIR})
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY_DEBUG ${ICQ_LIB_DIR})
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY_RELEASE ${ICQ_LIB_DIR})
set(CMAKE_LIBRARY_OUTPUT_PATH ${ICQ_LIB_DIR})
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_DEBUG ${ICQ_BIN_DIR})
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELEASE ${ICQ_BIN_DIR})
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${ICQ_BIN_DIR})

# --------------------------  external  --------------------------
set(DOWNLOAD_URL "https://hb.bizmrg.com/icq-www/external")

set(EXT_LIBS_VERSION v9)

set(EXT_LIBS_FNAME "external_${EXT_LIBS_VERSION}.zip")
set(EXT_LIBS_MD5 "external_${EXT_LIBS_VERSION}.md5")

set(EXTERNAL_URL "${DOWNLOAD_URL}/${EXT_LIBS_VERSION}/${EXT_LIBS_FNAME}")

set(ICQ_EXTERNAL "${ICQ_ROOT}/external_${EXT_LIBS_VERSION}")
include_directories(${ICQ_EXTERNAL})

set(EXTERNAL_DOWNLOADS "${ICQ_ROOT}/downloads")

message(WARNING "")
message(WARNING "ICQ_EXTERNAL = ${ICQ_EXTERNAL}")
message(WARNING "")

message(STATUS "")
message(STATUS "[+] -> FetchContent [external]")

include(FetchContent)

set(EXTERNAL_FILE_WITH_MD5)
set(EXTERNAL_FILE_ZIPFILE_MD5)

set(FETCHCONTENT_QUIET OFF)


if(EXISTS "${EXTERNAL_DOWNLOADS}/tmp")
    message(STATUS "file(REMOVE_RECURSE ${EXTERNAL_DOWNLOADS}/tmp)")
    file(REMOVE_RECURSE "${EXTERNAL_DOWNLOADS}/tmp")
endif()
if(EXISTS "${EXTERNAL_DOWNLOADS}/src/${EXT_LIBS_VERSION}-populate-stamp")
    message(STATUS "file(REMOVE_RECURSE ${EXTERNAL_DOWNLOADS}/src/${EXT_LIBS_VERSION}-populate-stamp)")
    file(REMOVE_RECURSE "${EXTERNAL_DOWNLOADS}/src/${EXT_LIBS_VERSION}-populate-stamp")
endif()

if(NOT EXISTS "${EXTERNAL_DOWNLOADS}/src/${EXT_LIBS_MD5}")
    message(STATUS "No hash, continue")
    FetchContent_Declare(
        ${EXT_LIBS_VERSION}
        PREFIX "${EXTERNAL_DOWNLOADS}"
        URL ${EXTERNAL_URL}
        SOURCE_DIR "${ICQ_EXTERNAL}"
    )
else()
    message(STATUS "Reading MD5 HASH from md5 file")
    file(READ "${EXTERNAL_DOWNLOADS}/src/${EXT_LIBS_MD5}" EXTERNAL_FILE_WITH_MD5)
    string(STRIP "${EXTERNAL_FILE_WITH_MD5}" EXTERNAL_FILE_WITH_MD5)
    message(STATUS "EXTERNAL_FILE_WITH_MD5 = ${EXTERNAL_FILE_WITH_MD5}")

    message(STATUS "[md5] Checksum MD5 HASH")
    file(MD5 "${EXTERNAL_DOWNLOADS}/src/${EXT_LIBS_FNAME}" EXTERNAL_FILE_ZIPFILE_MD5)
    message(STATUS "EXTERNAL_FILE_ZIPFILE_MD5 = ${EXTERNAL_FILE_ZIPFILE_MD5}")
    IF(NOT ${EXTERNAL_FILE_WITH_MD5} MATCHES "${EXTERNAL_FILE_ZIPFILE_MD5}")
        message(STATUS "[md5] not equals!!!")
        message(STATUS "File will be re-downloaded")
        FetchContent_Declare(
            ${EXT_LIBS_VERSION}
            PREFIX "${EXTERNAL_DOWNLOADS}"
            URL ${EXTERNAL_URL}
            SOURCE_DIR "${ICQ_EXTERNAL}"
        )
    else()
        FetchContent_Declare(
            ${EXT_LIBS_VERSION}
            PREFIX "${EXTERNAL_DOWNLOADS}"
            URL ${EXTERNAL_URL}
            URL_HASH MD5=${EXTERNAL_FILE_WITH_MD5}
            SOURCE_DIR "${ICQ_EXTERNAL}"
        )
    endif()
endif()

message(STATUS "FetchContent_MakeAvailable(${EXT_LIBS_VERSION})")
FetchContent_MakeAvailable(${EXT_LIBS_VERSION})

FetchContent_GetProperties(${EXT_LIBS_VERSION})
if (NOT ${EXT_LIBS_VERSION}_POPULATED)
    message(STATUS "FetchContent_Populate(${EXT_LIBS_VERSION})")
    FetchContent_Populate(${EXT_LIBS_VERSION})
endif()


if(EXISTS "${EXTERNAL_DOWNLOADS}/src/${EXT_LIBS_FNAME}")
    message(STATUS "[md5] Checksum MD5 HASH")
    file(MD5 "${EXTERNAL_DOWNLOADS}/src/${EXT_LIBS_FNAME}" EXTERNAL_FILE_WITH_MD5)
    string(STRIP "${EXTERNAL_FILE_WITH_MD5}" EXTERNAL_FILE_WITH_MD5)
    message(STATUS "[md5] EXTERNAL_FILE_WITH_MD5 = ${EXTERNAL_FILE_WITH_MD5}")
    message(STATUS "[write][force] Saving MD5 to file -> ${EXTERNAL_DOWNLOADS}/src/${EXT_LIBS_MD5}")
    FILE(WRITE "${EXTERNAL_DOWNLOADS}/src/${EXT_LIBS_MD5}" "${EXTERNAL_FILE_WITH_MD5}")
else()
    message(STATUS "[404] File external_${EXT_LIBS_VERSION}.zip not found!")
endif()

if(NOT EXISTS "${EXTERNAL_DOWNLOADS}/src/${EXT_LIBS_MD5}")
    message(STATUS "[404] File ${EXT_LIBS_MD5} was not created")
endif()

message(STATUS "[+] <- FetchContent [external]")
message(STATUS "")


set(ICQ_EXTERNAL_PLATFORM "${ICQ_EXTERNAL}/${ICQ_PLATFORM}")
# ------------------------  set_lib_var  -------------------------

macro(set_lib_var varName libName)
    SET(${varName} "${ICQ_EXTERNAL}/${libName}/lib/${ICQ_PLATFORM}/${ICQ_ARCH}")
endmacro()

# -------------------  platform specific path --------------------
set(ICQ_PLATFORM_SPECIFIC "${ICQ_EXTERNAL}/platform_specific/${ICQ_PLATFORM}/${ICQ_ARCH}")

# --------------------------    boost   --------------------------
if(LINUX)
    add_definitions(-DBOOST_UUID_RANDOM_PROVIDER_FORCE_POSIX) # fix missed SYS_getrandom in older kernels
endif()

if(WIN32)
    set(BOOST_INCLUDEDIR "${ICQ_EXTERNAL}/boost_1.72.0/include")
    set_lib_var(BOOST_LIBRARYDIR "boost_1.72.0")
else()
    set(BOOST_INCLUDEDIR "${ICQ_EXTERNAL}/boost/include")
    set_lib_var(BOOST_LIBRARYDIR boost)
endif()

set(Boost_USE_STATIC_LIBS ON)
set(Boost_USE_STATIC_RUNTIME ON)
set(Boost_NO_SYSTEM_PATHS ON)
if(MSVC)
    add_definitions(-D_SCL_SECURE_NO_WARNINGS)
    find_package(Boost REQUIRED chrono date_time filesystem locale system thread regex)
elseif(APPLE)
    find_package(Boost REQUIRED)
    set(Boost_LIBRARIES ${Boost_LIBRARIES} "${BOOST_LIBRARYDIR}/libboost_chrono.a")
    set(Boost_LIBRARIES ${Boost_LIBRARIES} "${BOOST_LIBRARYDIR}/libboost_locale.a")
    set(Boost_LIBRARIES ${Boost_LIBRARIES} "${BOOST_LIBRARYDIR}/libboost_system.a")
    set(Boost_LIBRARIES ${Boost_LIBRARIES} "${BOOST_LIBRARYDIR}/libboost_filesystem.a")
    set(Boost_LIBRARIES ${Boost_LIBRARIES} "${BOOST_LIBRARYDIR}/libboost_unit_test_framework.a")
    set(Boost_LIBRARIES ${Boost_LIBRARIES} "${BOOST_LIBRARYDIR}/libboost_regex.a")
    set(Boost_LIBRARIES ${Boost_LIBRARIES} "${BOOST_LIBRARYDIR}/libboost_thread.a")
elseif(LINUX)
    set(Boost_LIBRARIES ${Boost_LIBRARIES} "${BOOST_LIBRARYDIR}/libboost_filesystem.a")
    set(Boost_LIBRARIES ${Boost_LIBRARIES} "${BOOST_LIBRARYDIR}/libboost_locale.a")
    set(Boost_LIBRARIES ${Boost_LIBRARIES} "${BOOST_LIBRARYDIR}/libboost_system.a")
    set(Boost_LIBRARIES ${Boost_LIBRARIES} "${BOOST_LIBRARYDIR}/libboost_thread.a")
    set(Boost_LIBRARIES ${Boost_LIBRARIES} "${BOOST_LIBRARYDIR}/libboost_regex.a")
endif()
include_directories(${BOOST_INCLUDEDIR})


# -------------------------- rapid-json --------------------------
add_definitions(-DRAPIDJSON_HAS_STDSTRING=1 -DUSE_RAPIDJSON)
set(ICQ_RAPID_JSON "${ICQ_EXTERNAL}/rapidjson")
include_directories(${ICQ_RAPID_JSON}/include)

# --------------------------    nghttp2    --------------------------
add_definitions(-DNGHTTP2_STATICLIB)
set(ICQ_NGHTTP2 "${ICQ_EXTERNAL}/nghttp2")
set(NGHTTP2_INCLUDE_DIR "${ICQ_NGHTTP2}/include")
include_directories(${NGHTTP2_INCLUDE_DIR})
set_lib_var(NGHTTP2_LIBRARY_DIR nghttp2)
if(MSVC)
    set(NGHTTP2_LIBRARIES "${NGHTTP2_LIBRARY_DIR}/nghttp2.lib")
    message(${NGHTTP2_LIBRARIES})
else()
    set(NGHTTP2_LIBRARIES "${NGHTTP2_LIBRARY_DIR}/libnghttp2.a")
endif()

# --------------------------    curl    --------------------------
add_definitions(-DCURL_STATICLIB)
set(ICQ_CURL "${ICQ_EXTERNAL}/curl")
set(CURL_INCLUDE_DIR "${ICQ_CURL}/include")
include_directories(${CURL_INCLUDE_DIR})
set_lib_var(CURL_LIBRARY_DIR curl)
if(MSVC)
    set(CURL_LIBRARIES
        optimized "${CURL_LIBRARY_DIR}/libcurl.lib"
        debug "${CURL_LIBRARY_DIR}/libcurld.lib")
else()
    set(CURL_LIBRARIES "${CURL_LIBRARY_DIR}/libcurl.a")
endif()

# --------------------------   openssl   -------------------------
set(ICQ_OPENSSL "${ICQ_EXTERNAL}/openssl")
set(OPENSSL_LIBRARY_DIR "${ICQ_OPENSSL}/lib/${ICQ_PLATFORM}")
set_lib_var(OPENSSL_LIBRARY_DIR openssl)
include_directories(${ICQ_OPENSSL}/include)
if(MSVC)
    set(OPENSSL_LIBRARIES
        optimized "${OPENSSL_LIBRARY_DIR}/libeay32.lib" optimized "${OPENSSL_LIBRARY_DIR}/ssleay32.lib"
        debug "${OPENSSL_LIBRARY_DIR}/debug/libeay32.lib" debug "${OPENSSL_LIBRARY_DIR}/debug/ssleay32.lib")
elseif(LINUX)
    set(OPENSSL_LIBRARIES "${OPENSSL_LIBRARY_DIR}/libssl.a" "${OPENSSL_LIBRARY_DIR}/libcrypto.a")
elseif(APPLE)
    set(OPENSSL_LIBRARIES "${OPENSSL_LIBRARY_DIR}/libssl.a")
endif()

# --------------------------    zlib    --------------------------
set(ICQ_ZLIB "${ICQ_EXTERNAL}/zlib")
include_directories(${ICQ_ZLIB}/include)
set_lib_var(ZLIB_LIBRARY_DIR zlib)
if(MSVC)
    set(ZLIB_LIBRARIES
        optimized "${ZLIB_LIBRARY_DIR}/zlib.lib"
        debug "${ZLIB_LIBRARY_DIR}/zlibd.lib")
endif()

# ------------------------    libcrypto    -----------------------
if(APPLE)
    set(LIBCRYPTO_LIBRARIES "${OPENSSL_LIBRARY_DIR}/libcrypto.a")
endif()

# --------------------------   OpenAl   --------------------------
if(APPLE)
    set(DEFAULT_USE_SYSTEM_OPENAL ON)
else()
    set(DEFAULT_USE_SYSTEM_OPENAL OFF)
endif()

option(USE_SYSTEM_OPENAL "Use system openal." ${DEFAULT_USE_SYSTEM_OPENAL})

if (USE_SYSTEM_OPENAL)
    message(STATUS "openal: system")
    add_definitions(-DUSE_SYSTEM_OPENAL)
else()
    message(STATUS "openal: external")
    set(ICQ_OPENAL "${ICQ_EXTERNAL}/openal")
    set_lib_var(OPENAL_LIBRARY_DIR openal)
    include_directories(${ICQ_OPENAL}/include)
    add_definitions(-DAL_LIBTYPE_STATIC)
    if(MSVC)
        set(OPEN_AL_LIBRARIES
            optimized "${OPENAL_LIBRARY_DIR}/OpenAL32.lib"
            debug "${OPENAL_LIBRARY_DIR}/OpenAL32d.lib")
    else()
        set(OPEN_AL_LIBRARIES "${OPENAL_LIBRARY_DIR}/libopenal.a")
    endif()
endif()

# --------------------------   ffmpeg   --------------------------
set(ICQ_FFMPEG "${ICQ_EXTERNAL}/ffmpeg")
if(MSVC)
    include_directories(${ICQ_FFMPEG}/include_old)                                      # use old ffmped for windows until xp is being supported
    SET(FFMPEG_LIBRARY_DIR "${ICQ_EXTERNAL}/ffmpeg/lib_old/${ICQ_PLATFORM}/${ICQ_ARCH}")
    set(FFMPEG_LIBRARIES ${FFMPEG_LIBRARIES} "${FFMPEG_LIBRARY_DIR}/libavcodec.lib")
    set(FFMPEG_LIBRARIES ${FFMPEG_LIBRARIES} "${FFMPEG_LIBRARY_DIR}/libavfilter.lib")
    set(FFMPEG_LIBRARIES ${FFMPEG_LIBRARIES} "${FFMPEG_LIBRARY_DIR}/libavformat.lib")
    set(FFMPEG_LIBRARIES ${FFMPEG_LIBRARIES} "${FFMPEG_LIBRARY_DIR}/libavutil.lib")
    set(FFMPEG_LIBRARIES ${FFMPEG_LIBRARIES} "${FFMPEG_LIBRARY_DIR}/libswresample.lib")
    set(FFMPEG_LIBRARIES ${FFMPEG_LIBRARIES} "${FFMPEG_LIBRARY_DIR}/libswscale.lib")
else()
    include_directories(${ICQ_FFMPEG}/include)
    set_lib_var(FFMPEG_LIBRARY_DIR ffmpeg)
    set(FFMPEG_LIBRARIES ${FFMPEG_LIBRARIES} "${FFMPEG_LIBRARY_DIR}/libavfilter.a")
    set(FFMPEG_LIBRARIES ${FFMPEG_LIBRARIES} "${FFMPEG_LIBRARY_DIR}/libavformat.a") # libavformat.a depends on libavcodec.a
    set(FFMPEG_LIBRARIES ${FFMPEG_LIBRARIES} "${FFMPEG_LIBRARY_DIR}/libavcodec.a")
    set(FFMPEG_LIBRARIES ${FFMPEG_LIBRARIES} "${FFMPEG_LIBRARY_DIR}/libavutil.a")
    set(FFMPEG_LIBRARIES ${FFMPEG_LIBRARIES} "${FFMPEG_LIBRARY_DIR}/libswresample.a")
    set(FFMPEG_LIBRARIES ${FFMPEG_LIBRARIES} "${FFMPEG_LIBRARY_DIR}/libswscale.a")
endif()

# --------------------------   minizip   -------------------------
set(ICQ_MINIZIP "${ICQ_EXTERNAL}/minizip")
include_directories(${ICQ_MINIZIP}/include)

# --------------------------   libvoip   -------------------------
set(VOIP_LIBS_VERSION 3.3.0.1664)

set(VOIP_LIBS_FNAME "voip3_${VOIP_LIBS_VERSION}_desktop.zip")
set(VOIP_LIBS_MD5 "voip3_${VOIP_LIBS_VERSION}_desktop.md5")

set(VOIPLIB_URL "${DOWNLOAD_URL}/Voiplib/${VOIP_LIBS_VERSION}/${VOIP_LIBS_FNAME}")

set(VOIPLIB_DOWNLOADS "${ICQ_ROOT}/downloads")

set(VOIPLIB_ROOT "${ICQ_ROOT}/core/Voip/libvoip_${VOIP_LIBS_VERSION}")
include_directories(${VOIPLIB_ROOT})

message(STATUS "")
message(STATUS "[+] -> FetchContent [VOIP]")

# include(FetchContent)

set(VOIPLIB_FILE_WITH_MD5)
set(VOIPLIB_FILE_ZIPFILE_MD5)

set(FETCHCONTENT_QUIET OFF)

if(EXISTS "${VOIPLIB_DOWNLOADS}/src/${VOIP_LIBS_VERSION}-populate-stamp")
    message(STATUS "file(REMOVE_RECURSE ${VOIPLIB_DOWNLOADS}/src/${VOIP_LIBS_VERSION}-populate-stamp)")
    file(REMOVE_RECURSE "${VOIPLIB_DOWNLOADS}/src/${VOIP_LIBS_VERSION}-populate-stamp")
endif()

if(NOT EXISTS "${VOIPLIB_DOWNLOADS}/src/${VOIP_LIBS_MD5}")
    message(STATUS "No hash, continue")
    FetchContent_Declare(
        ${VOIP_LIBS_VERSION}
        PREFIX "${VOIPLIB_DOWNLOADS}"
        URL ${VOIPLIB_URL}
        SOURCE_DIR "${VOIPLIB_ROOT}"
    )
else()
    message(STATUS "Reading MD5 HASH from md5 file")
    file(READ "${VOIPLIB_DOWNLOADS}/src/${VOIP_LIBS_MD5}" VOIPLIB_FILE_WITH_MD5)
    string(STRIP "${VOIPLIB_FILE_WITH_MD5}" VOIPLIB_FILE_WITH_MD5)
    message(STATUS "VOIPLIB_FILE_WITH_MD5 = ${VOIPLIB_FILE_WITH_MD5}")

    message(STATUS "[md5] Checksum MD5 HASH")
    file(MD5 "${VOIPLIB_DOWNLOADS}/src/${VOIP_LIBS_FNAME}" VOIPLIB_FILE_ZIPFILE_MD5)
    message(STATUS "VOIPLIB_FILE_ZIPFILE_MD5 = ${VOIPLIB_FILE_ZIPFILE_MD5}")
    IF(NOT ${VOIPLIB_FILE_WITH_MD5} MATCHES "${VOIPLIB_FILE_ZIPFILE_MD5}")
        message(STATUS "[md5] not equals!!!")
        message(STATUS "File will be re-downloaded")
        FetchContent_Declare(
            ${VOIP_LIBS_VERSION}
            PREFIX "${VOIPLIB_DOWNLOADS}"
            URL ${VOIPLIB_URL}
            SOURCE_DIR "${VOIPLIB_ROOT}"
        )
    else()
        FetchContent_Declare(
            ${VOIP_LIBS_VERSION}
            PREFIX "${VOIPLIB_DOWNLOADS}"
            URL ${VOIPLIB_URL}
            URL_HASH MD5=${VOIPLIB_FILE_WITH_MD5}
            SOURCE_DIR "${VOIPLIB_ROOT}"
        )
    endif()
endif()

message(STATUS "FetchContent_MakeAvailable(${VOIP_LIBS_VERSION})")
FetchContent_MakeAvailable(${VOIP_LIBS_VERSION})

FetchContent_GetProperties(${VOIP_LIBS_VERSION})
if (NOT ${VOIP_LIBS_VERSION}_POPULATED)
    message(STATUS "FetchContent_Populate(${VOIP_LIBS_VERSION})")
    FetchContent_Populate(${VOIP_LIBS_VERSION})
endif()


if(EXISTS "${VOIPLIB_DOWNLOADS}/src/${VOIP_LIBS_FNAME}")
    message(STATUS "[md5] Checksum MD5 HASH")
    file(MD5 "${VOIPLIB_DOWNLOADS}/src/${VOIP_LIBS_FNAME}" VOIPLIB_FILE_WITH_MD5)
    string(STRIP "${VOIPLIB_FILE_WITH_MD5}" VOIPLIB_FILE_WITH_MD5)
    message(STATUS "[md5] VOIPLIB_FILE_WITH_MD5 = ${VOIPLIB_FILE_WITH_MD5}")
    message(STATUS "[write][force] Saving MD5 to file -> ${VOIPLIB_DOWNLOADS}/src/${VOIP_LIBS_MD5}")
    FILE(WRITE "${VOIPLIB_DOWNLOADS}/src/${VOIP_LIBS_MD5}" "${VOIPLIB_FILE_WITH_MD5}")
else()
    message(STATUS "[404] File ${VOIP_LIBS_FNAME} not found!")
endif()

if(NOT EXISTS "${VOIPLIB_DOWNLOADS}/src/${VOIP_LIBS_MD5}")
    message(STATUS "[404] File ${VOIP_LIBS_MD5} was not created")
endif()

message(STATUS "[+] <- FetchContent [VOIP]")
message(STATUS "")


if(MSVC)
    set(VOIP_LIBRARIES "${VOIP_LIBRARIES}" msdmo.lib dmoguids.lib wmcodecdspuuid.lib amstrmid.lib winmm.lib secur32.lib ws2_32.lib comsuppw.lib)
    set(VOIP_LIBRARIES "${VOIP_LIBRARIES}" optimized comsuppw.lib debug comsuppwd.lib)
    set(VOIP_DX_COMPILER_LIBRARY "${ICQ_PLATFORM_SPECIFIC}/d3dcompiler_43.dll")
endif()

if(NOT VOIP_AS_MODULE)
    include_directories("${VOIPLIB_ROOT}/include")
    if(MSVC)
        set(VOIP_LIBRARIES "${VOIP_LIBRARIES}" optimized "${VOIPLIB_ROOT}/lib/windows.Release.x86/libvoip_sdk_merged_x86.lib" debug "${VOIPLIB_ROOT}/lib/windows.Debug.x86/libvoip_sdk_merged_x86d.lib")

    elseif(APPLE)
        set(VOIP_LIBRARIES "${VOIPLIB_ROOT}/lib/macos.Release.x64/libvoip_sdk_merged_x64.a")

    elseif(LINUX)
        if(LINUX32)
            set(VOIP_LIBRARIES "${VOIPLIB_ROOT}/lib/linux.Release.x86/libvoip_sdk_merged_x86.a")
        else()
            set(VOIP_LIBRARIES "${VOIPLIB_ROOT}/lib/linux.Release.x64/libvoip_sdk_merged_x64.a")
        endif()
    endif()
endif()

if (STRIP_VOIP)
    add_definitions(-DSTRIP_VOIP)
endif()



# --------------------------     qt     --------------------------
set(ICQ_QT_ROOT "${ICQ_EXTERNAL}/qt/${ICQ_PLATFORM}")

if(LINUX)
    set(ICQ_QT_INCLUDE_DIR "${ICQ_QT_ROOT}/include")
    set(ICQ_QT_LIB_DIR "${ICQ_QT_ROOT}/lib/${ICQ_ARCH}")
    set(ICQ_QT_BIN_DIR "${ICQ_QT_ROOT}/bin")
    set(ICQ_QT_PLUGINS_DIR "${ICQ_QT_ROOT}/plugins/${ICQ_ARCH}")
    EXEC_PROGRAM("chmod +x ${ICQ_QT_BIN_DIR}/lrelease")
    EXEC_PROGRAM("chmod +x ${ICQ_QT_BIN_DIR}/rcc")
    EXEC_PROGRAM("chmod +x ${ICQ_QT_BIN_DIR}/uic")
    EXEC_PROGRAM("chmod +x ${ICQ_QT_BIN_DIR}/moc")
else()
    set(ICQ_QT_INCLUDE_DIR "${ICQ_QT_ROOT}/include")
    set(ICQ_QT_LIB_DIR "${ICQ_QT_ROOT}/lib")
    set(ICQ_QT_BIN_DIR "${ICQ_QT_ROOT}/bin")
    set(ICQ_QT_PLUGINS_DIR "${ICQ_QT_ROOT}/plugins")
endif()

if(APPLE)
    set(ICQ_QT_VERSION "5.12.8")
else()
    set(ICQ_QT_VERSION "5.14.2")
endif()
include_directories("${ICQ_QT_INCLUDE_DIR}")
include_directories("${ICQ_QT_INCLUDE_DIR}/QtCore")
include_directories("${ICQ_QT_INCLUDE_DIR}/QtCore/${ICQ_QT_VERSION}/QtCore")
include_directories("${ICQ_QT_INCLUDE_DIR}/QtCore/${ICQ_QT_VERSION}")
include_directories("${ICQ_QT_INCLUDE_DIR}/QtGui")
include_directories("${ICQ_QT_INCLUDE_DIR}/QtGui/${ICQ_QT_VERSION}/QtGui")
include_directories("${ICQ_QT_INCLUDE_DIR}/QtGui/${ICQ_QT_VERSION}")
include_directories("${ICQ_QT_INCLUDE_DIR}/QtWidgets")
include_directories("${ICQ_QT_INCLUDE_DIR}/QtWidgets/${ICQ_QT_VERSION}/QtWidgets")
include_directories("${ICQ_QT_INCLUDE_DIR}/QtWidgets/${ICQ_QT_VERSION}")
include_directories("${ICQ_QT_INCLUDE_DIR}/QtOpenGL")
include_directories("${ICQ_QT_INCLUDE_DIR}/QtNetwork")
include_directories("${ICQ_QT_INCLUDE_DIR}/QtSvg")
include_directories("${ICQ_QT_INCLUDE_DIR}/QtMacExtras")
if(IM_AUTO_TESTING)
    message(STATUS "... include: QtTest")
    include_directories("${ICQ_QT_INCLUDE_DIR}/QtTest")
endif()

add_definitions(-DQT_STATIC)
if(MSVC)
    set(QT_LIBRARIES
        optimized ${ICQ_QT_LIB_DIR}/Qt5Gui.lib debug ${ICQ_QT_LIB_DIR}/Qt5Guid.lib
        optimized ${ICQ_QT_LIB_DIR}/Qt5Core.lib debug ${ICQ_QT_LIB_DIR}/Qt5Cored.lib
        optimized ${ICQ_QT_LIB_DIR}/Qt5EventDispatcherSupport.lib debug ${ICQ_QT_LIB_DIR}/Qt5EventDispatcherSupportd.lib
        optimized ${ICQ_QT_LIB_DIR}/Qt5FontDatabaseSupport.lib debug ${ICQ_QT_LIB_DIR}/Qt5FontDatabaseSupportd.lib
        optimized ${ICQ_QT_LIB_DIR}/Qt5Network.lib debug ${ICQ_QT_LIB_DIR}/Qt5Networkd.lib        
        optimized ${ICQ_QT_LIB_DIR}/Qt5Multimedia.lib debug ${ICQ_QT_LIB_DIR}/Qt5Multimediad.lib
        optimized ${ICQ_QT_LIB_DIR}/Qt5MultimediaWidgets.lib debug ${ICQ_QT_LIB_DIR}/Qt5MultimediaWidgetsd.lib
        optimized ${ICQ_QT_LIB_DIR}/Qt5Multimedia.lib debug ${ICQ_QT_LIB_DIR}/Qt5Multimediad.lib
        optimized ${ICQ_QT_LIB_DIR}/Qt5Test.lib debug ${ICQ_QT_LIB_DIR}/Qt5Testd.lib
        optimized ${ICQ_QT_LIB_DIR}/Qt5ThemeSupport.lib debug ${ICQ_QT_LIB_DIR}/Qt5ThemeSupportd.lib
        optimized ${ICQ_QT_LIB_DIR}/Qt5OpenGL.lib debug ${ICQ_QT_LIB_DIR}/Qt5OpenGLd.lib
        optimized ${ICQ_QT_LIB_DIR}/Qt5Svg.lib debug ${ICQ_QT_LIB_DIR}/Qt5Svgd.lib
        optimized ${ICQ_QT_LIB_DIR}/Qt5PlatformCompositorSupport.lib debug ${ICQ_QT_LIB_DIR}/Qt5PlatformCompositorSupportd.lib
        optimized ${ICQ_QT_LIB_DIR}/Qt5Widgets.lib debug ${ICQ_QT_LIB_DIR}/Qt5Widgetsd.lib
        optimized ${ICQ_QT_LIB_DIR}/Qt5WinExtras.lib debug ${ICQ_QT_LIB_DIR}/Qt5WinExtrasd.lib
        optimized ${ICQ_QT_LIB_DIR}/Qt5WindowsUIAutomationSupport.lib debug ${ICQ_QT_LIB_DIR}/Qt5WindowsUIAutomationSupportd.lib
        optimized ${ICQ_QT_LIB_DIR}/qtpcre2.lib debug ${ICQ_QT_LIB_DIR}/qtpcre2d.lib
        optimized ${ICQ_QT_LIB_DIR}/qtmain.lib debug ${ICQ_QT_LIB_DIR}/qtmaind.lib
        optimized ${ICQ_QT_LIB_DIR}/qtharfbuzz.lib debug ${ICQ_QT_LIB_DIR}/qtharfbuzzd.lib
        optimized ${ICQ_QT_LIB_DIR}/qtfreetype.lib debug ${ICQ_QT_LIB_DIR}/qtfreetyped.lib
        optimized ${ICQ_QT_LIB_DIR}/qtlibpng.lib debug ${ICQ_QT_LIB_DIR}/qtlibpngd.lib
        optimized ${ICQ_QT_PLUGINS_DIR}/imageformats/qgif.lib debug ${ICQ_QT_PLUGINS_DIR}/imageformats/qgifd.lib
        optimized ${ICQ_QT_PLUGINS_DIR}/imageformats/qicns.lib debug ${ICQ_QT_PLUGINS_DIR}/imageformats/qicnsd.lib
        optimized ${ICQ_QT_PLUGINS_DIR}/imageformats/qico.lib debug ${ICQ_QT_PLUGINS_DIR}/imageformats/qicod.lib
        optimized ${ICQ_QT_PLUGINS_DIR}/imageformats/qjpeg.lib debug ${ICQ_QT_PLUGINS_DIR}/imageformats/qjpegd.lib
        optimized ${ICQ_QT_PLUGINS_DIR}/imageformats/qsvg.lib debug ${ICQ_QT_PLUGINS_DIR}/imageformats/qsvgd.lib
        optimized ${ICQ_QT_PLUGINS_DIR}/imageformats/qtga.lib debug ${ICQ_QT_PLUGINS_DIR}/imageformats/qtgad.lib
        optimized ${ICQ_QT_PLUGINS_DIR}/imageformats/qtiff.lib debug ${ICQ_QT_PLUGINS_DIR}/imageformats/qtiffd.lib
        optimized ${ICQ_QT_PLUGINS_DIR}/imageformats/qwbmp.lib debug ${ICQ_QT_PLUGINS_DIR}/imageformats/qwbmpd.lib
        optimized ${ICQ_QT_PLUGINS_DIR}/imageformats/qwebp.lib debug ${ICQ_QT_PLUGINS_DIR}/imageformats/qwebpd.lib
        optimized ${ICQ_QT_PLUGINS_DIR}/audio/qtaudio_windows.lib debug ${ICQ_QT_PLUGINS_DIR}/audio/qtaudio_windowsd.lib
        optimized ${ICQ_QT_PLUGINS_DIR}/platforms/qwindows.lib debug ${ICQ_QT_PLUGINS_DIR}/platforms/qwindowsd.lib
    )
elseif(APPLE)
    add_definitions(-DQT_GUI_LIB=1)
    set(QT_LIBRARIES
        ${ICQ_QT_LIB_DIR}/libQt5Concurrent.a
        ${ICQ_QT_LIB_DIR}/libQt5Core.a
        ${ICQ_QT_LIB_DIR}/libQt5DBus.a
        ${ICQ_QT_LIB_DIR}/libQt5Gui.a
        ${ICQ_QT_LIB_DIR}/libQt5Multimedia.a
        ${ICQ_QT_LIB_DIR}/libQt5Network.a
        ${ICQ_QT_LIB_DIR}/libQt5PrintSupport.a
        ${ICQ_QT_LIB_DIR}/libQt5Widgets.a
        ${ICQ_QT_LIB_DIR}/libQt5MacExtras.a
        ${ICQ_QT_LIB_DIR}/libQt5Svg.a
        ${ICQ_QT_LIB_DIR}/libQt5ClipboardSupport.a
        ${ICQ_QT_LIB_DIR}/libQt5FontDatabaseSupport.a
        ${ICQ_QT_LIB_DIR}/libQt5AccessibilitySupport.a
        ${ICQ_QT_LIB_DIR}/libQt5GraphicsSupport.a
        ${ICQ_QT_LIB_DIR}/libQt5ThemeSupport.a
        ${ICQ_QT_LIB_DIR}/libqtfreetype.a
        ${ICQ_QT_LIB_DIR}/libqtharfbuzz.a
        ${ICQ_QT_LIB_DIR}/libqtpcre2.a
        ${ICQ_QT_LIB_DIR}/libqtlibpng.a
        ${ICQ_QT_LIB_DIR}/libQt5MacExtras.a
        ${ICQ_QT_PLUGINS_DIR}/imageformats/libqgif.a
        ${ICQ_QT_PLUGINS_DIR}/imageformats/libqicns.a
        ${ICQ_QT_PLUGINS_DIR}/imageformats/libqico.a
        ${ICQ_QT_PLUGINS_DIR}/imageformats/libqjpeg.a
        ${ICQ_QT_PLUGINS_DIR}/imageformats/libqmacheif.a
        ${ICQ_QT_PLUGINS_DIR}/imageformats/libqmacjp2.a
        ${ICQ_QT_PLUGINS_DIR}/imageformats/libqsvg.a
        ${ICQ_QT_PLUGINS_DIR}/imageformats/libqtga.a
        ${ICQ_QT_PLUGINS_DIR}/imageformats/libqtiff.a
        ${ICQ_QT_PLUGINS_DIR}/imageformats/libqwbmp.a
        ${ICQ_QT_PLUGINS_DIR}/imageformats/libqwebp.a
        ${ICQ_QT_PLUGINS_DIR}/platforms/libqcocoa.a
        )

    if(IM_AUTO_TESTING)
        message(STATUS "... add definitions -DQT_WIDGETS_LIB -DQT_CORE_LIB")
        add_definitions(-DQT_WIDGETS_LIB)
        add_definitions(-DQT_CORE_LIB)
    endif()

elseif(LINUX)
    set(QT_LIBRARIES
        ${ICQ_QT_PLUGINS_DIR}/platforms/libqxcb.a
        ${ICQ_QT_LIB_DIR}/libQt5XcbQpa.a
        ${ICQ_QT_LIB_DIR}/libQt5ServiceSupport.a
        ${ICQ_QT_LIB_DIR}/libQt5ThemeSupport.a
        ${ICQ_QT_LIB_DIR}/libQt5FontDatabaseSupport.a
        ${ICQ_QT_LIB_DIR}/libQt5XkbCommonSupport.a
        ${ICQ_QT_LIB_DIR}/libQt5LinuxAccessibilitySupport.a
        ${ICQ_QT_LIB_DIR}/libQt5AccessibilitySupport.a
        ${ICQ_QT_LIB_DIR}/libQt5EdidSupport.a
        ${ICQ_QT_LIB_DIR}/libQt5DBus.a
        ${ICQ_QT_LIB_DIR}/libQt5Widgets.a
        ${ICQ_QT_LIB_DIR}/libQt5Gui.a
        ${ICQ_QT_LIB_DIR}/libQt5Network.a
        ${ICQ_QT_LIB_DIR}/libQt5Multimedia.a
        ${ICQ_QT_LIB_DIR}/libQt5Concurrent.a
        ${ICQ_QT_LIB_DIR}/libxcb-static.a
        ${ICQ_QT_LIB_DIR}/libQt5Core.a
        ${ICQ_QT_LIB_DIR}/libQt5Svg.a
        ${ICQ_QT_LIB_DIR}/libqtpcre2.a
        ${ICQ_QT_LIB_DIR}/libqtlibpng.a
        ${ICQ_QT_PLUGINS_DIR}/bearer/libqconnmanbearer.a
        ${ICQ_QT_PLUGINS_DIR}/bearer/libqnmbearer.a
        ${ICQ_QT_PLUGINS_DIR}/bearer/libqgenericbearer.a
        ${ICQ_QT_PLUGINS_DIR}/imageformats/libqgif.a
        ${ICQ_QT_PLUGINS_DIR}/imageformats/libqicns.a
        ${ICQ_QT_PLUGINS_DIR}/imageformats/libqico.a
        ${ICQ_QT_PLUGINS_DIR}/imageformats/libqjpeg.a
        ${ICQ_QT_PLUGINS_DIR}/imageformats/libqsvg.a
        ${ICQ_QT_PLUGINS_DIR}/imageformats/libqtga.a
        ${ICQ_QT_PLUGINS_DIR}/imageformats/libqtiff.a
        ${ICQ_QT_PLUGINS_DIR}/imageformats/libqwbmp.a
        ${ICQ_QT_PLUGINS_DIR}/imageformats/libqwebp.a
        )
endif()

# ------------------------ phonenumber  -----------------------
set(ICQ_PHONENUMBER "${ICQ_EXTERNAL}/phonenumber")
set_lib_var(PHONENUMBER_LIBRARY_DIR phonenumber)
add_definitions ("-DI18N_PHONENUMBERS_USE_BOOST")
if(MSVC)
    include_directories(${ICQ_PHONENUMBER}/include_win)
else()
    include_directories(${ICQ_PHONENUMBER}/include)
endif()

if(LINUX OR APPLE)
    set(PHONENUMBER_LIBRARIES ${PHONENUMBER_LIBRARY_DIR}/libphonenumber.a)
elseif(MSVC)
    set(PHONENUMBER_LIBRARIES optimized ${PHONENUMBER_LIBRARY_DIR}/phonenumber.lib debug ${PHONENUMBER_LIBRARY_DIR}/phonenumberd.lib)
endif()

# ------------------------- protobuf  -------------------------
set(ICQ_PROTOBUF "${ICQ_EXTERNAL}/protobuf")
set_lib_var(PROTOBUF_LIBRARY_DIR protobuf)

include_directories(${ICQ_PROTOBUF}/include)

# --------------------------- breakpad  --------------------------

if (LINUX)
    message(STATUS "> BREAKPAD")
    set(ICQ_BREAKPAD "${ICQ_EXTERNAL}/breakpad/src")
    set_lib_var(BREAKPAD_LIBRARY_DIR breakpad)

    FILE(GLOB breadpad_common_files
        ${ICQ_BREAKPAD}/common/*.cc
        ${ICQ_BREAKPAD}/common/*.h
    )

    FILE(GLOB breadpad_client_files
        ${ICQ_BREAKPAD}/client/*.cc
        ${ICQ_BREAKPAD}/client/*.h
    )

    include_directories(${ICQ_BREAKPAD})
    include_directories(${ICQ_BREAKPAD}/client)
    include_directories(${ICQ_BREAKPAD}/common)
    set(BREAKPAD_LIBRARIES ${BREAKPAD_LIBRARY_DIR}/libbreakpad_client.a)

    FILE(GLOB_RECURSE breadpad_common_platform_files
        ${ICQ_BREAKPAD}/common/linux/*.cc
        ${ICQ_BREAKPAD}/common/linux/*.h
    )

    FILE(GLOB_RECURSE breadpad_client_platform_files
        ${ICQ_BREAKPAD}/client/linux/*.cc
        ${ICQ_BREAKPAD}/client/linux/*.h
    )

    SOURCE_GROUP(breakpad FILES ${breadpad_common_files} ${breadpad_client_files}
    ${breadpad_common_platform_files} ${breadpad_client_platform_files})
endif()


if (APPLE)
    if (BUILD_FOR_STORE)
        message(STATUS "> BREAKPAD")
        set(ICQ_BREAKPAD "${ICQ_EXTERNAL}/breakpad/src")
        set_lib_var(BREAKPAD_LIBRARY_DIR breakpad)

        include_directories(${ICQ_BREAKPAD})
        include_directories(${ICQ_BREAKPAD}/client)
        include_directories(${ICQ_BREAKPAD}/common)
        set(BREAKPAD_LIBRARIES ${BREAKPAD_LIBRARY_DIR}/libbreakpad.a)

        set(BREAKPAD_SOURCE
        ${ICQ_BREAKPAD}/common/string_conversion.cc
        ${ICQ_BREAKPAD}/common/convert_UTF.cc
        ${ICQ_BREAKPAD}/common/md5.cc
        ${ICQ_BREAKPAD}/client/minidump_file_writer.cc
        ${ICQ_BREAKPAD}/client/mac/crash_generation/crash_generation_client.cc
        ${ICQ_BREAKPAD}/client/mac/handler/exception_handler.cc
        ${ICQ_BREAKPAD}/client/mac/handler/minidump_generator.cc
        ${ICQ_BREAKPAD}/client/mac/handler/breakpad_nlist_64.cc
        ${ICQ_BREAKPAD}/client/mac/handler/dynamic_images.cc
        ${ICQ_BREAKPAD}/client/mac/handler/protected_memory_allocator.cc
        ${ICQ_BREAKPAD}/common/mac/bootstrap_compat.cc
        ${ICQ_BREAKPAD}/common/mac/file_id.cc
        ${ICQ_BREAKPAD}/common/mac/macho_id.cc
        ${ICQ_BREAKPAD}/common/mac/macho_reader.cc
        ${ICQ_BREAKPAD}/common/mac/macho_utilities.cc
        ${ICQ_BREAKPAD}/common/mac/macho_walker.cc
        ${ICQ_BREAKPAD}/common/mac/string_utilities.cc
        ${ICQ_BREAKPAD}/common/mac/MachIPC.mm)
    else()
# --------------------------- crashpad  --------------------------
        message(STATUS "> CRASHPAD")
        set(ICQ_CRASHPAD "${ICQ_EXTERNAL}/crashpad")
        set_lib_var(CRASHPAD_LIBRARY_DIR crashpad)

        include_directories(${ICQ_CRASHPAD}/include)
        include_directories(${ICQ_CRASHPAD}/include/mini_chromium)

        set(CRASHPAD_LIBRARIES
            ${CRASHPAD_LIBRARY_DIR}/libclient.a
            ${CRASHPAD_LIBRARY_DIR}/libutil.a
            ${CRASHPAD_LIBRARY_DIR}/libbase.a
            ${CRASHPAD_LIBRARY_DIR}/libsnapshot.a)
    endif()
endif()

# --------------------------- re2  ----------------------------
set(ICQ_RE2 "${ICQ_EXTERNAL}/re2")
set_lib_var(RE2_LIBRARY_DIR re2)

include_directories(${ICQ_RE2}/include)
if(LINUX OR APPLE)
    set(RE2_LIBRARIES ${RE2_LIBRARY_DIR}/libre2.a)
elseif (MSVC)
    set(RE2_LIBRARIES optimized ${RE2_LIBRARY_DIR}/libre2.lib debug ${RE2_LIBRARY_DIR}/libre2d.lib)
endif()

# --------------------------    zstd    --------------------------
set(ICQ_ZSTD "${ICQ_EXTERNAL}/zstd")
set(ZSTD_INCLUDE_DIR "${ICQ_ZSTD}/include")
include_directories(${ZSTD_INCLUDE_DIR})
set_lib_var(ZSTD_LIBRARY_DIR zstd)
if(MSVC)
    set(ZSTD_LIBRARIES
        optimized "${ZSTD_LIBRARY_DIR}/libzstd_static.lib"
        debug "${ZSTD_LIBRARY_DIR}/libzstd_staticd.lib")
else()
    set(ZSTD_LIBRARIES "${ZSTD_LIBRARY_DIR}/libzstd.a")
endif()

# ------------------------ source groups  ------------------------
macro(set_source_group group_name root_dir ${ARGN})
    get_filename_component(_path ${root_dir} ABSOLUTE)
    foreach(_source IN ITEMS ${ARGN})
        get_filename_component(_source_path "${_source}" PATH)
        string(REPLACE "${_path}" "" _source_trimmed "${_source_path}")
        set(_source_group "${group_name}${_source_trimmed}")
        string(REPLACE "/" "\\" _source_group "${_source_group}")
        source_group("${_source_group}" FILES "${_source}")
    endforeach()
endmacro()


# -------------------------- precompiled -------------------------
macro(use_precompiled_header_msvc pch_h pch_cpp ${ARGN})
    set(_pch_bin "${PROJECT_BINARY_DIR}/${CMAKE_BUILD_TYPE}.${PROJECT_NAME}.pch")
    foreach(_source IN ITEMS ${ARGN})
        if(NOT "${_source}" STREQUAL "${pch_cpp}")
            set_source_files_properties(${_source} PROPERTIES
                COMPILE_FLAGS "/Yu\"${pch_h}\" /Fp\"${_pch_bin}\""
                OBJECT_DEPENDS "${_pch_bin}")
        endif()
    endforeach()
    set_source_files_properties("${pch_cpp}" PROPERTIES
        COMPILE_FLAGS "/Yc\"${pch_h}\" /Fp\"${_pch_bin}\""
        OBJECT_OUTPUTS "${_pch_bin}")
endmacro()


function(use_precompiled_header_mac output pch_h ${ARGN})
    set(_pch_bin "${PROJECT_BINARY_DIR}/${CMAKE_BUILD_TYPE}-${PROJECT_NAME}.pch")
    if(CMAKE_OSX_SYSROOT)
        message(STATUS "isysroot " ${CMAKE_OSX_SYSROOT})
        list(APPEND _build_flags -isysroot "${CMAKE_OSX_SYSROOT}")
    endif()
    get_property(_raw_includes DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY INCLUDE_DIRECTORIES)
    foreach(_it IN ITEMS ${_raw_includes})
        list(APPEND _paths "-I${_it}")
    endforeach()
    get_property(_raw_definitions DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY COMPILE_DEFINITIONS)
    foreach(_it IN ITEMS ${_raw_definitions})
        list(APPEND _definitions "-D${_it}")
    endforeach()
    if(ICQ_RELEASE)
        string(REPLACE " " ";" _cmake_flags ${CMAKE_CXX_FLAGS_RELEASE})
        foreach(it IN ITEMS ${_cmake_flags})
            list(APPEND _build_flags "${it}")
        endforeach()
    endif()
    add_custom_command(OUTPUT ${_pch_bin}
        COMMAND ${CMAKE_CXX_COMPILER} ${_build_flags} ${_paths} ${_definitions} -mmacosx-version-min=${CMAKE_OSX_DEPLOYMENT_TARGET} -std=gnu++1z -x c++-header ${pch_h} -o ${_pch_bin}
        DEPENDS ${pch_h} VERBATIM)
    foreach(_source IN ITEMS ${ARGN})
        set_source_files_properties(${_source} PROPERTIES
            COMPILE_FLAGS "-include-pch ${_pch_bin}"
            OBJECT_DEPENDS "${_pch_bin}")
    endforeach()
    set(${output} ${_pch_bin} PARENT_SCOPE)
endfunction()


function(use_precompiled_header_linux output pch_h ${ARGN})
    get_filename_component(pch_h_name ${pch_h} NAME)
    set(_pch_bin "${PROJECT_BINARY_DIR}/${pch_h_name}.gch")
    get_property(_raw_includes DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY INCLUDE_DIRECTORIES)
    foreach(_it IN ITEMS ${_raw_includes})
        list(APPEND _paths "-I${_it}")
    endforeach()
    get_property(_raw_definitions DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY COMPILE_DEFINITIONS)
    foreach(_it IN ITEMS ${_raw_definitions})
        list(APPEND _definitions "-D${_it}")
    endforeach()
    if(ICQ_RELEASE)
        string(REPLACE " " ";" _cmake_flags ${CMAKE_CXX_FLAGS_RELEASE})
        foreach(it IN ITEMS ${_cmake_flags})
            list(APPEND _build_flags "${it}")
        endforeach()
    endif()
    if(ICQ_DEBUG)
        string(REPLACE " " ";" _cmake_flags ${CMAKE_CXX_FLAGS_DEBUG})
        foreach(it IN ITEMS ${_cmake_flags})
            list(APPEND _build_flags "${it}")
        endforeach()
    endif()

    add_custom_command(OUTPUT ${_pch_bin}
        COMMAND ${CMAKE_CXX_COMPILER} ${_build_flags} ${_paths} ${_definitions} -std=c++1z ${ARCH_FLAGS} -fPIC -x c++-header ${pch_h} -o ${_pch_bin}
        DEPENDS ${pch_h} VERBATIM)
    foreach(_source IN ITEMS ${ARGN})
        set_source_files_properties(${_source} PROPERTIES
            COMPILE_FLAGS "-include ${PROJECT_BINARY_DIR}/${pch_h_name} -std=c++1z ${ARCH_FLAGS} -Winvalid-pch"
            OBJECT_DEPENDS "${_pch_bin}")
    endforeach()
    set(${output} ${_pch_bin} PARENT_SCOPE)
endfunction()


# --------------------------    scan    --------------------------
function(find_sources output source_dir extension)
    file(GLOB_RECURSE _path_list "${source_dir}/*.${extension}")
    foreach(_path IN ITEMS ${_path_list})
        get_filename_component(_dir "${_path}" DIRECTORY)
        string(REGEX MATCH "win32$" _win32 ${_dir})
        string(REGEX MATCH "linux$" _linux ${_dir})
        string(REGEX MATCH "macos$" _macos ${_dir})
        if(WIN32)
            if(_linux OR _macos)
                message(STATUS "skipped: " ${_path})
                list(REMOVE_ITEM _path_list ${_path})
            endif()
        elseif(APPLE)
            if(_linux OR _win32)
                message(STATUS "skipped: " ${_path})
                list(REMOVE_ITEM _path_list ${_path})
            endif()
        else()
            if(_win32 OR _macos)
                message(STATUS "skipped: " ${_path})
                list(REMOVE_ITEM _path_list ${_path})
            endif()
        endif()
    endforeach()
    set(${output} ${_path_list} PARENT_SCOPE)
endfunction()


# --------------------------    mocs    --------------------------
function(generate_mocs output ${ARGN})
    set(ICQ_MOC_DIR "${CMAKE_CURRENT_BINARY_DIR}/mocs")
    get_filename_component(ICQ_MOC_EXE "${ICQ_QT_BIN_DIR}/moc" ABSOLUTE)
    foreach(header IN ITEMS ${ARGN})
        file(READ "${header}" file_content)
        string(FIND "${file_content}" "Q_OBJECT" pos1)
        string(FIND "${file_content}" "Q_SLOTS" pos2)
        string(FIND "${file_content}" "Q_SIGNALS" pos3)

        if ((pos1 EQUAL -1) AND (pos2 EQUAL -1) AND (pos3 EQUAL -1))
            continue()
        endif()

        get_filename_component(name "${header}" NAME_WE)
        file(RELATIVE_PATH relative_path ${ICQ_ROOT} ${header})
        get_filename_component(relative_dir "${relative_path}" DIRECTORY)
        set(moc_dir "${ICQ_MOC_DIR}/${relative_dir}")
        file(MAKE_DIRECTORY ${moc_dir})
        set(moc_file "${moc_dir}/moc_${name}.cpp")
        list(APPEND _moc_files ${moc_file})
        if(APPLE)
            add_custom_command(OUTPUT ${moc_file}
                COMMAND ${ICQ_MOC_EXE} -f ${header} -o ${moc_file} ${header} 2> /dev/null
                DEPENDS ${header} VERBATIM)
        elseif(LINUX)
            add_custom_command(OUTPUT ${moc_file}
                COMMAND ${ICQ_MOC_EXE} -D__linux__ -f ${header} -o ${moc_file} ${header} 2> /dev/null
                DEPENDS ${header} VERBATIM)
        else()
            add_custom_command(OUTPUT ${moc_file}
                COMMAND ${ICQ_MOC_EXE} -f ${header} -o ${moc_file} ${header}
                DEPENDS ${header} VERBATIM)
        endif()
    endforeach()
    set_source_group("mocs" "${ICQ_MOC_DIR}" ${_moc_files})
    set(${output} ${_moc_files} PARENT_SCOPE)
endfunction()


# -------------------------  translations  -----------------------
function(generate_translations language_files qm_files translations_dir ${ARGN})
    set(_language_files)
    set(_qm_files)
    get_filename_component(ICQ_LRELEASE_EXE "${ICQ_QT_BIN_DIR}/lrelease" ABSOLUTE)
    foreach(lang IN ITEMS ${ARGN})
        set(lang_file "${translations_dir}/${lang}.ts")
        set(qm_file "${translations_dir}/${lang}.qm")
        add_custom_command(OUTPUT "${qm_file}"
            COMMAND ${ICQ_LRELEASE_EXE} ${lang_file} -qm ${qm_file}
            DEPENDS ${lang_file})
        list(APPEND _language_files "${lang_file}")
        list(APPEND _qm_files "${qm_file}")
    endforeach()
    source_group("translations" FILES ${_language_files})
    source_group("translations" FILES ${_qm_files})
    set(${language_files} ${_language_files} PARENT_SCOPE)
    set(${qm_files} ${_qm_files} PARENT_SCOPE)
endfunction()


# --------------------------  subdirs   --------------------------
add_subdirectory(core)
add_subdirectory(corelib)
add_subdirectory(gui)
add_subdirectory(libomicron)
