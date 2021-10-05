cmake_minimum_required(VERSION 3.17)

include(CheckFunctionExists)

project(icq)

message(STATUS "")
message(STATUS "[CMAKE]")
message(STATUS "[CMAKE] Using <CMakeLists.txt>")
message(STATUS "[CMAKE]")
message(STATUS "[CMAKE] CMake version = ${CMAKE_VERSION}")

if(NOT PRODUCT)
    set(PRODUCT "icq")
    message(STATUS "")
    message(STATUS "[INFO] Default product is ${PRODUCT}")
endif()

message(STATUS "[INFO] PRODUCT is ${PRODUCT}")

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
    message(STATUS "APP_ENVIRONMENT is empty")
else()
    message(STATUS "APP_ENVIRONMENT is ${APP_ENVIRONMENT}")
    add_definitions(-DAPP_ENVIRONMENT="${APP_ENVIRONMENT}")
endif()

if(BUILD_FOR_STORE)
    message(STATUS "[cmake] This build for Store -> BUILD_FOR_STORE")
    add_definitions(-DBUILD_FOR_STORE=1)
    message(STATUS "add_definitions(-DBUILD_FOR_STORE=1)")
endif()

if(BUILD_PKG_MSI)
    message(STATUS "[cmake] This build makes MSI package -> BUILD_PKG_MSI")
    message(STATUS "add_definitions(-DBUILD_PKG_MSI=1)")
    add_definitions(-DBUILD_PKG_MSI=1)
endif()

option(IM_QT_DYNAMIC "Qt dynamic linking" ON)

if(NOT IM_QT_DYNAMIC)
    add_definitions(-DIM_QT_STATIC)
    message(STATUS "[INFO] Qt linking: static")
    message(STATUS "[INFO] add_definitions(-DIM_QT_STATIC)")
else()
    add_definitions(-DIM_QT_DYNAMIC)
    message(STATUS "[INFO] Qt linking: dynamic")
    message(STATUS "[INFO] add_definitions(-DIM_QT_DYNAMIC)")
endif()

if (STRIP_VOIP)
    add_definitions(-DSTRIP_VOIP)
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
    execute_process(
        COMMAND ${GIT_EXECUTABLE} rev-parse --short HEAD
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
        OUTPUT_VARIABLE GIT_COMMIT_HASH
        OUTPUT_STRIP_TRAILING_WHITESPACE
    )
    message(STATUS "")
    message(STATUS "[BRANCH] Current GIT BRANCH NAME = ${GIT_BRANCH_NAME}")
    message(STATUS "[BRANCH] Current GIT COMMIT HASH = ${GIT_COMMIT_HASH}")
    message(STATUS "")
    
    if (ENABLE_BUILD_INFO)
        add_definitions("-DGIT_BRANCH_NAME=\"${GIT_BRANCH_NAME}\"")
        message(STATUS "add_definitions(-DGIT_BRANCH_NAME=${GIT_BRANCH_NAME})")

        add_definitions("-DGIT_COMMIT_HASH=\"${GIT_COMMIT_HASH}\"")
        message(STATUS "add_definitions(-DGIT_COMMIT_HASH=${GIT_COMMIT_HASH})")
    endif()
endif (GIT_FOUND)

if (ENABLE_BUILD_INFO)
    string(TIMESTAMP BUILD_TIME "%d/%m/%y %H:%M")
    string(TIMESTAMP BUILD_TIME_SHORT "%m%Y")
    add_definitions("-DBUILD_TIME=\"${BUILD_TIME}\"")
    add_definitions("-DBUILD_TIME_SHORT=\"${BUILD_TIME_SHORT}\"")
    message(STATUS "add_definitions(-DBUILD_TIME=${BUILD_TIME})")
    message(STATUS "Current BILD TIME = ${BUILD_TIME}")
endif()

get_filename_component(ICQ_ROOT "${CMAKE_SOURCE_DIR}" ABSOLUTE)
set(PRODUCT_SPEC_DIR "${ICQ_ROOT}/products/${PRODUCT}")
if(EXISTS "${PRODUCT_SPEC_DIR}")
    message(STATUS "PRODUCT_SPEC_DIR = ${PRODUCT_SPEC_DIR}")
else()
    message(FATAL_ERROR "Folder ${PRODUCT_SPEC_DIR} is not exists!)")
endif()
include(${PRODUCT_SPEC_DIR}/project_settings.cmake)

if(IM_QT_DYNAMIC)
    if (HAS_WEB_ENGINE)
        message(STATUS "[INFO] Qt WebEngine: ON")
        add_definitions(-DHAS_WEB_ENGINE)
    endif()
    if (HAS_OAUTH2)
    message(STATUS "[INFO] OAuth2: ON")
        add_definitions(-DHAS_OAUTH2)
    endif()
endif()

if("${CMAKE_BUILD_TYPE}" STREQUAL "Debug")
    set(ICQ_DEBUG ON)
    set(ICQ_BIN_DIR "${ICQ_ROOT}/bin/Debug")
    set(ICQ_LIB_DIR "${ICQ_ROOT}/bin/Debug")
    set(CMAKE_CONFIGURATION_TYPES "Debug" CACHE STRING "" FORCE)
    if(APPLE)
        add_definitions(-DDEBUG=1)
        message(STATUS "add_definitions(-DDEBUG=1)")
    endif()
elseif("${CMAKE_BUILD_TYPE}" STREQUAL "Release")
    set(ICQ_RELEASE ON)
    set(ICQ_BIN_DIR "${ICQ_ROOT}/bin/Release")
    set(ICQ_LIB_DIR "${ICQ_ROOT}/bin/Release")
    set(CMAKE_CONFIGURATION_TYPES "Release" CACHE STRING "" FORCE)
    add_definitions(-DNDEBUG)
else()
    message(FATAL_ERROR "Need to specify CMAKE_BUILD_TYPE (-DCMAKE_BUILDTYPE=Debug|Release)")
endif()
message(STATUS "CMAKE_BUILD_TYPE is ${CMAKE_BUILD_TYPE}")
message(STATUS "CMAKE_CONFIGURATION_TYPES is ${CMAKE_CONFIGURATION_TYPES}")

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


if (NOT STRIP_VOIP)
    if(VOIP_AS_MODULE)
      # When building voip library from source (to do this, just add voip3 symlink to voip3 repo), include
      # voip sources before mangling flags, CMake options, etc. to avoid interference with voip build scripts
      add_subdirectory(voip3)
      set(VOIP_LIBRARIES libvoip::sdk_cpp_merged)
    endif()
endif()


if(MSVC)
    set(ICQ_PLATFORM windows)
    set(ICQ_ARCH x86)
    if(NOT IM_QT_DYNAMIC)
        if(ICQ_DEBUG)
            add_definitions(/MTd)
        else()
            add_definitions(/MT)
        endif()
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
    add_definitions(/we4150) # deletion of pointer to incomplete type 'type'; no destructor called
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
    add_definitions(/we4551)
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} /STACK:3145728") # for emoji map
    #add_definitions(/we4266) #update boost and qt or disable this warning for these headers

    set(WIN32_COMMON_OPTS "/Ob2 /Oi /Ot /GS- /GF /Gy /Gw /fp:except- /arch:SSE2 /Zc:inline")
    set(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE} ${WIN32_COMMON_OPTS}")
    set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} ${WIN32_COMMON_OPTS}")

    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} /DEBUG /OPT:NOREF /OPT:NOICF")
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
    set(CMAKE_OSX_DEPLOYMENT_TARGET 10.13)

    set(MACOS_COMMON_FLAGS "-fvisibility-inlines-hidden -fvisibility=hidden -fasm-blocks -stdlib=libc++")
    set(MACOS_COMMON_FLAGS "${MACOS_COMMON_FLAGS} -mmacosx-version-min=${CMAKE_OSX_DEPLOYMENT_TARGET} -Werror=most -Werror=switch")
    list(APPEND _build_flags "-fvisibility-inlines-hidden")
    list(APPEND _build_flags "-fvisibility=hidden")
    list(APPEND _build_flags "-fasm-blocks")
    list(APPEND _build_flags "-stdlib=libc++")
    if("${CMAKE_BUILD_TYPE}" STREQUAL "Release")
        set(MACOS_COMMON_FLAGS "${MACOS_COMMON_FLAGS} -ffunction-sections -fdata-sections -fno-stack-protector")
        list(APPEND _build_flags "-ffunction-sections")
        list(APPEND _build_flags "-fdata-sections")
        list(APPEND _build_flags "-fno-stack-protector")
    endif()

    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${MACOS_COMMON_FLAGS}")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${MACOS_COMMON_FLAGS} -Werror=inconsistent-missing-override -Werror=effc++ -Werror=empty-body")
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -stdlib=libc++")
    add_definitions(-Werror=return-stack-address)
elseif(UNIX)
    set(LINUX ON)
    message(STATUS "Linux x64")
    set(LINUX64 ON)
    set(ICQ_ARCH x64)
    set(ARCH_FLAGS "-m64")
    set(ICQ_PLATFORM linux)
    set(ICQ_BIN_DIR "${ICQ_ROOT}/bin/${CMAKE_CONFIGURATION_TYPES}64")
    set(ICQ_LIB_DIR "${ICQ_ROOT}/bin/${CMAKE_CONFIGURATION_TYPES}64")
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
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} ${ARCH_FLAGS} -no-pie -Wl,--version-script=${ICQ_ROOT}/icq.exports -Wl,--gc-sections -Wl,--no-as-needed")
    #set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -Wl,-Map=${ICQ_BIN_DIR}/icq.map -Wl,--cref")
    if(NOT DISABLE_STATIC_CXX)
        set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -static-libgcc -static-libstdc++")
    endif()
    message(STATUS "[flags] CMAKE_C_FLAGS          = ${CMAKE_C_FLAGS}")
    message(STATUS "[flags] CMAKE_CXX_FLAGS        = ${CMAKE_CXX_FLAGS}")
    message(STATUS "[flags] CMAKE_CXX_FLAGS_DEBUG  = ${CMAKE_CXX_FLAGS_DEBUG}")
    message(STATUS "[flags] CMAKE_EXE_LINKER_FLAGS = ${CMAKE_EXE_LINKER_FLAGS}")
else()
    message(FATAL_ERROR "Unknown platform")
endif()

if(LINUX)
    if(NOT USE_TOOLCHAIN)
        add_compile_definitions(_GLIBCXX_USE_CXX11_ABI=1)
        message(STATUS "")
        message(STATUS "[flags] set _GLIBCXX_USE_CXX11_ABI=1")
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -D_GLIBCXX_USE_CXX11_ABI=1")
        message(STATUS "[flags] CMAKE_CXX_FLAGS        = ${CMAKE_CXX_FLAGS}")
        message(STATUS "")
    endif()
endif()

# ---------------------------  linux toolchain --------------------------------
# i.e. -DUSE_TOOLCHAIN=/storage/toolchains
if(USE_TOOLCHAIN)
    if(LINUX)
        message(STATUS "")
        message(STATUS "[linux toolchain] --- --- --- -> ")

        set(CMAKE_SYSTEM_NAME Linux)
        set(CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY)

        message(STATUS "[toolchain x86_64]")
        set(TOOLCHAIN_NAME "x86_64-centos7-linux-gnu")
        set(TOOLCHAIN_ARCH "x86_64-pc-linux-gnu")

        set(TOOLCHAIN_PATH "${USE_TOOLCHAIN}/${TOOLCHAIN_NAME}")
        if (NOT EXISTS "${TOOLCHAIN_PATH}")
            message(FATAL_ERROR "Toolchain ${TOOLCHAIN_PATH} not found!)")
        endif()

        set(CMAKE_C_COMPILER "${TOOLCHAIN_PATH}/bin/${TOOLCHAIN_NAME}-gcc" CACHE FILEPATH "" FORCE)
        set(CMAKE_CXX_COMPILER "${TOOLCHAIN_PATH}/bin/${TOOLCHAIN_NAME}-g++" CACHE FILEPATH "" FORCE)
        set(CMAKE_LINKER "${TOOLCHAIN_PATH}/bin/${TOOLCHAIN_NAME}-ld" CACHE FILEPATH "" FORCE)
        set(CMAKE_C_COMPILER_AR "${TOOLCHAIN_PATH}/bin/${TOOLCHAIN_NAME}-ar" CACHE FILEPATH "" FORCE)
        set(CMAKE_CXX_COMPILER_AR "${TOOLCHAIN_PATH}/bin/${TOOLCHAIN_NAME}-ar" CACHE FILEPATH "" FORCE)
        set(CMAKE_C_COMPILER_RANLIB "${TOOLCHAIN_PATH}/bin/${TOOLCHAIN_NAME}-ranlib" CACHE FILEPATH "" FORCE)
        set(CMAKE_CXX_COMPILER_RANLIB "${TOOLCHAIN_PATH}/bin/${TOOLCHAIN_NAME}-ranlib" CACHE FILEPATH "" FORCE)
        set(CMAKE_OBJCOPY "${TOOLCHAIN_PATH}/bin/${TOOLCHAIN_NAME}-objcopy" CACHE FILEPATH "" FORCE)
        set(CMAKE_OBJDUMP "${TOOLCHAIN_PATH}/bin/${TOOLCHAIN_NAME}-objdump" CACHE FILEPATH "" FORCE)
        set(CMAKE_NM "${TOOLCHAIN_PATH}/bin/${TOOLCHAIN_NAME}-nm" CACHE FILEPATH "" FORCE)
        set(CMAKE_AR "${TOOLCHAIN_PATH}/bin/${TOOLCHAIN_NAME}-ar" CACHE FILEPATH "" FORCE)
        set(CMAKE_ADDR2LINE "${TOOLCHAIN_PATH}/bin/${TOOLCHAIN_NAME}-addr2line" CACHE FILEPATH "" FORCE)
        set(CMAKE_RANLIB "${TOOLCHAIN_PATH}/bin/${TOOLCHAIN_NAME}-ranlib" CACHE FILEPATH "" FORCE)
        set(CMAKE_READELF "${TOOLCHAIN_PATH}/bin/${TOOLCHAIN_NAME}-readelf" CACHE FILEPATH "" FORCE)
        set(CMAKE_STRIP "${TOOLCHAIN_PATH}/bin/${TOOLCHAIN_NAME}-strip" CACHE FILEPATH "" FORCE)

        set(CMAKE_SYSROOT "${TOOLCHAIN_PATH}/${TOOLCHAIN_NAME}/sysroot")
        set(CMAKE_TC_ROOT_PATH "${TOOLCHAIN_PATH}/${TOOLCHAIN_NAME}")
        set(CMAKE_FIND_ROOT_PATH "${TOOLCHAIN_PATH}/${TOOLCHAIN_NAME}/sysroot")

        message(STATUS "TOOLCHAIN_NAME -> ${TOOLCHAIN_NAME}")
        message(STATUS "TOOLCHAIN_ARCH -> ${TOOLCHAIN_ARCH}")

        set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
        set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
        set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
        set(CMAKE_FIND_ROOT_PATH_MODE_PACKAGE ONLY)

        message(STATUS "")
        message(STATUS "[toolchain] CMAKE_TRY_COMPILE_TARGET_TYPE = ${CMAKE_TRY_COMPILE_TARGET_TYPE}")
        message(STATUS "[toolchain] CMAKE_C_COMPILER              = ${CMAKE_C_COMPILER}")
        message(STATUS "[toolchain] CMAKE_CXX_COMPILER            = ${CMAKE_CXX_COMPILER}")
        message(STATUS "[toolchain] CMAKE_LINKER                  = ${CMAKE_LINKER}")
        message(STATUS "")
        message(STATUS "[toolchain] CMAKE_SYSROOT                 = ${CMAKE_SYSROOT}")
        message(STATUS "[toolchain] CMAKE_FIND_ROOT_PATH          = ${CMAKE_FIND_ROOT_PATH}")
        message(STATUS "[toolchain] CMAKE_SYSTEM_INCLUDE_PATH     = ${CMAKE_SYSTEM_INCLUDE_PATH}")
        message(STATUS "[toolchain] CMAKE_SYSTEM_FRAMEWORK_PATH   = ${CMAKE_SYSTEM_FRAMEWORK_PATH}")
        message(STATUS "")

        file(GLOB gcc_include_version RELATIVE "${CMAKE_TC_ROOT_PATH}/include/c++" "${CMAKE_TC_ROOT_PATH}/include/c++/*")
        include_directories(${CMAKE_TC_ROOT_PATH}/include/c++/${gcc_include_version})
        include_directories(${CMAKE_TC_ROOT_PATH}/include/c++/${gcc_include_version}/bits)
        include_directories(${CMAKE_TC_ROOT_PATH}/include)
        include_directories(${CMAKE_FIND_ROOT_PATH}/usr/include/X11)
        include_directories(${CMAKE_FIND_ROOT_PATH}/usr/include/GL)
        include_directories(${CMAKE_FIND_ROOT_PATH}/usr/include)

        link_directories("${CMAKE_FIND_ROOT_PATH}/lib")
        link_directories("${CMAKE_FIND_ROOT_PATH}/usr/lib")

        message(STATUS "[linux toolchain] <- --- --- ---")
        message(STATUS "")
    endif()
endif()
# -----------------------------------------------------------------------------


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


# ---------------------------  paths  -----------------------------------------
set(CMAKE_EXECUTABLE_OUTPUT_DIRECTORY_DEBUG ${ICQ_BIN_DIR})
set(CMAKE_EXECUTABLE_OUTPUT_DIRECTORY_RELEASE ${ICQ_BIN_DIR})
set(CMAKE_EXECUTABLE_OUTPUT_PATH ${ICQ_BIN_DIR})
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY_DEBUG ${ICQ_LIB_DIR})
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY_RELEASE ${ICQ_LIB_DIR})
set(CMAKE_LIBRARY_OUTPUT_PATH ${ICQ_LIB_DIR})
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_DEBUG ${ICQ_BIN_DIR})
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELEASE ${ICQ_BIN_DIR})
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${ICQ_BIN_DIR})


# --------------------------  validator  --------------------------------------
if(RUN_VALIDATOR)
    message(STATUS "[STAGE] VALIDATORS")
    find_package(PythonInterp 3 REQUIRED)
    message(STATUS "[INFO] PYTHON_EXECUTABLE = ${PYTHON_EXECUTABLE}")
    execute_process(COMMAND
                    "${PYTHON_EXECUTABLE}" buildsystem/products_json_validator.py --products "${ICQ_ROOT}/products"
                    WORKING_DIRECTORY ${ICQ_ROOT}
                    RESULT_VARIABLE RETURN_VALUE)
    if (NOT RETURN_VALUE EQUAL 0)
        message(STATUS "Fix JSON or update validator rules in buildsystem/products_json_validator.py")
        message(FATAL_ERROR "Products JSON check failed, return value ${RETURN_VALUE}")
    else()
        message(STATUS "Products JSON ${ICQ_ROOT}/products are valid")
    endif()
endif()
# -----------------------------------------------------------------------------


message(STATUS "")
message(STATUS "[requirements] Reading common variables from requirements/CMakeLists.txt")

include(requirements/common.cmake)

# --------------------------  external  ---------------------------------------
# message(STATUS "")
# message(STATUS "[EXTERNAL DEPENDENCIES]")
if(LINUX)
    include(requirements/linux.cmake)
elseif(APPLE)
    include(requirements/macos.cmake)
else()
    include(requirements/windows.cmake)
endif()
common_print_dependencies_info()

message(STATUS "[requirements] ICQ_EXTERNAL               = ${ICQ_EXTERNAL}")
include_directories(${ICQ_EXTERNAL})
# -----------------------------------------------------------------------------

set(STATIC_RUNTIME_PREFIX "")
if(IM_QT_DYNAMIC)
    if(MSVC)
        set(QT_LIB_EXT "lib")
        set(QT_DLL_EXT "dll")
        set(QT_SUP_EXT "lib")
        set(QT_PLUGIN_EXT "dll")
    elseif(APPLE)
        set(QT_LIB_EXT "dylib")
        set(QT_SUP_EXT "a")
        set(QT_PLUGIN_EXT "dylib")
    else()
        set(QT_LIB_EXT "so")
        set(QT_SUP_EXT "a")
        set(QT_PLUGIN_EXT "so")
    endif()
else()
    if(MSVC)
        set(QT_LIB_EXT "lib")
        set(QT_SUP_EXT "lib")
        set(QT_PLUGIN_EXT "lib")
        set(STATIC_RUNTIME_PREFIX "/static_runtime")
    else()
        set(QT_LIB_EXT "a")
        set(QT_SUP_EXT "a")
        set(QT_PLUGIN_EXT "a")
    endif()
endif()


# ------------------------  set_lib_var  --------------------------------------
macro(set_lib_var varName libName)
    SET(${varName} "${libName}/lib")
    message(STATUS "[lib] ${varName} -> ${${varName}}")
endmacro(set_lib_var)

# ------------------------  set_include_var  ----------------------------------
macro(set_include_var varName includePath)
    SET(${varName} "${includePath}")
    include_directories("${includePath}")
    message(STATUS "[include] ${varName} -> ${${varName}}")
    # message(STATUS "[include] include_directories(${includePath})")
endmacro(set_include_var)

# -------------------  platform specific path ---------------------------------
set(ICQ_PLATFORM_SPECIFIC "${ICQ_EXTERNAL}/additionals")


# --------------------------    boost   ---------------------------------------
if(LINUX)
    add_definitions(-DBOOST_UUID_RANDOM_PROVIDER_FORCE_POSIX) # fix missed SYS_getrandom in older kernels
endif()

set_lib_var(BOOST_LIBRARY_DIR "${pkg_boost_location}")
set_include_var(BOOST_INCLUDEDIR "${pkg_boost_location}/include")

set(Boost_USE_STATIC_LIBS ON)
if(NOT IM_QT_DYNAMIC)
    set(Boost_USE_STATIC_RUNTIME ON)
endif()
set(Boost_NO_SYSTEM_PATHS ON)
if(MSVC)
    add_definitions(-D_SCL_SECURE_NO_WARNINGS)
    find_package(Boost REQUIRED chrono date_time filesystem locale system thread regex)
elseif(APPLE)
    find_package(Boost REQUIRED)
    set(Boost_LIBRARIES ${Boost_LIBRARIES} "${BOOST_LIBRARY_DIR}/libboost_chrono.a")
    set(Boost_LIBRARIES ${Boost_LIBRARIES} "${BOOST_LIBRARY_DIR}/libboost_locale.a")
    set(Boost_LIBRARIES ${Boost_LIBRARIES} "${BOOST_LIBRARY_DIR}/libboost_system.a")
    set(Boost_LIBRARIES ${Boost_LIBRARIES} "${BOOST_LIBRARY_DIR}/libboost_filesystem.a")
    set(Boost_LIBRARIES ${Boost_LIBRARIES} "${BOOST_LIBRARY_DIR}/libboost_unit_test_framework.a")
    set(Boost_LIBRARIES ${Boost_LIBRARIES} "${BOOST_LIBRARY_DIR}/libboost_regex.a")
    set(Boost_LIBRARIES ${Boost_LIBRARIES} "${BOOST_LIBRARY_DIR}/libboost_thread.a")
elseif(LINUX)
    set(Boost_LIBRARIES ${Boost_LIBRARIES} "${BOOST_LIBRARY_DIR}/libboost_filesystem.a")
    set(Boost_LIBRARIES ${Boost_LIBRARIES} "${BOOST_LIBRARY_DIR}/libboost_locale.a")
    set(Boost_LIBRARIES ${Boost_LIBRARIES} "${BOOST_LIBRARY_DIR}/libboost_system.a")
    set(Boost_LIBRARIES ${Boost_LIBRARIES} "${BOOST_LIBRARY_DIR}/libboost_thread.a")
    set(Boost_LIBRARIES ${Boost_LIBRARIES} "${BOOST_LIBRARY_DIR}/libboost_regex.a")
endif()


# -------------------------- rapidjson ----------------------------------------
add_definitions(-DRAPIDJSON_HAS_STDSTRING=1 -DUSE_RAPIDJSON)
set_include_var(RAPID_JSON_HEADERS "${pkg_src_rapidjson_location}/include")

# --------------------------    nghttp2    ------------------------------------
add_definitions(-DNGHTTP2_STATICLIB)
set_lib_var(NGHTTP2_LIBRARY_DIR "${pkg_nghttp2_location}")
set_include_var(NGHTTP2_INCLUDE_DIR "${pkg_nghttp2_location}/include")
if(MSVC)
    set(NGHTTP2_LIBRARIES "${NGHTTP2_LIBRARY_DIR}/nghttp2.lib")
    message(STATUS "[INFO] NGHTTP2_LIBRARIES = ${NGHTTP2_LIBRARIES}")
else()
    set(NGHTTP2_LIBRARIES "${NGHTTP2_LIBRARY_DIR}/libnghttp2.a")
endif()


# --------------------------    curl    ---------------------------------------
add_definitions(-DCURL_STATICLIB)
set_lib_var(CURL_LIBRARY_DIR "${pkg_curl_location}")
set_include_var(CURL_INCLUDE_DIR "${pkg_curl_location}/include/curl")
if(MSVC)
    set(CURL_LIBRARIES
        optimized "${CURL_LIBRARY_DIR}/libcurl.lib"
        debug "${CURL_LIBRARY_DIR}/libcurld.lib")
else()
    set(CURL_LIBRARIES "${CURL_LIBRARY_DIR}/libcurl.a")
endif()


# --------------------------   openssl   --------------------------------------
set_lib_var(OPENSSL_LIBRARY_DIR "${pkg_openssl_location}")
set_include_var(OPENSSL_INCLUDE_DIR "${pkg_openssl_location}/include")
if(MSVC)
    set(OPENSSL_LIBRARIES
        optimized "${OPENSSL_LIBRARY_DIR}/libcrypto.lib" optimized "${OPENSSL_LIBRARY_DIR}/libssl.lib"
        debug "${OPENSSL_LIBRARY_DIR}/libcrypto.lib" debug "${OPENSSL_LIBRARY_DIR}/libssl.lib")
elseif(LINUX)
    set(OPENSSL_LIBRARIES "${OPENSSL_LIBRARY_DIR}/libssl.a" "${OPENSSL_LIBRARY_DIR}/libcrypto.a")
elseif(APPLE)
    set(OPENSSL_LIBRARIES "${OPENSSL_LIBRARY_DIR}/libssl.a" "${OPENSSL_LIBRARY_DIR}/libcrypto.a")
endif()


# --------------------------    zlib    ---------------------------------------
set_include_var(ZLIB_INCLUDE_DIR "${pkg_zlib_location}/include")
if(MSVC)
	# just precompiled binary
	message(INFO "[lib] ZLIB_LIBRARIES = ${pkg_bin_zlib.lib_location}")
    set(ZLIB_LIBRARIES "${pkg_bin_zlib.lib_location}")
else()
	set_lib_var(ZLIB_LIBRARY_DIR "${pkg_zlib_location}")
endif()


# --------------------------   OpenAl   --------------------------
if (NOT STRIP_AV_MEDIA)
    if(APPLE)
        set(DEFAULT_USE_SYSTEM_OPENAL ON)
    else()
        set(DEFAULT_USE_SYSTEM_OPENAL OFF)
    endif()

    option(USE_SYSTEM_OPENAL "Use system openal." ${DEFAULT_USE_SYSTEM_OPENAL})

    if (USE_SYSTEM_OPENAL)
        message(STATUS "[INFO] OpenAL: System")
        add_definitions(-DUSE_SYSTEM_OPENAL)
    else()
        message(STATUS "[INFO] OpenAL: External")
        set_lib_var(OPENAL_LIBRARY_DIR "${pkg_openal_location}")
        set_include_var(OPENAL_INCLUDE_DIR "${pkg_openal_location}/include")
        add_definitions(-DAL_LIBTYPE_STATIC)
        if(MSVC)
            set(OPEN_AL_LIBRARIES
                optimized "${OPENAL_LIBRARY_DIR}/OpenAL32.lib"
                debug "${OPENAL_LIBRARY_DIR}/OpenAL32.lib")
        else()
            set(OPEN_AL_LIBRARIES "${OPENAL_LIBRARY_DIR}/libopenal.a")
        endif()
    endif()
endif()

# --------------------------   ffmpeg   --------------------------
if (STRIP_AV_MEDIA)
    add_definitions(-DSTRIP_AV_MEDIA)
else()

    set_lib_var(FFMPEG_LIBRARY_DIR "${pkg_ffmpeg_location}")
    set_include_var(FFMPEG_INCLUDE_DIR "${pkg_ffmpeg_location}/include")
    if (MSVC)
        set(LIBRARY_EXTENTION "lib")
    else()
        set(LIBRARY_EXTENTION "a")
    endif()

    set(FFMPEG_LIBRARIES ${FFMPEG_LIBRARIES} "${FFMPEG_LIBRARY_DIR}/libavfilter.${LIBRARY_EXTENTION}")
    set(FFMPEG_LIBRARIES ${FFMPEG_LIBRARIES} "${FFMPEG_LIBRARY_DIR}/libavformat.${LIBRARY_EXTENTION}") # libavformat.a depends on libavcodec.a
    set(FFMPEG_LIBRARIES ${FFMPEG_LIBRARIES} "${FFMPEG_LIBRARY_DIR}/libavcodec.${LIBRARY_EXTENTION}")
    set(FFMPEG_LIBRARIES ${FFMPEG_LIBRARIES} "${FFMPEG_LIBRARY_DIR}/libavutil.${LIBRARY_EXTENTION}")
    set(FFMPEG_LIBRARIES ${FFMPEG_LIBRARIES} "${FFMPEG_LIBRARY_DIR}/libswresample.${LIBRARY_EXTENTION}")
    set(FFMPEG_LIBRARIES ${FFMPEG_LIBRARIES} "${FFMPEG_LIBRARY_DIR}/libswscale.${LIBRARY_EXTENTION}")

endif()


# --------------------------   rlottie   --------------------------------------
if (NOT STRIP_AV_MEDIA)
    set_lib_var(RLOTTIE_LIBRARY_DIR "${pkg_rlottie_location}")
    set_include_var(RLOTTIE_INCLUDE_DIR "${pkg_rlottie_location}/include")
    if(MSVC)
        set(RLOTTIE_LIBRARIES
            optimized "${RLOTTIE_LIBRARY_DIR}/rlottie.lib"
            debug "${RLOTTIE_LIBRARY_DIR}/rlottie.lib")
    else()
        set(RLOTTIE_LIBRARIES "${RLOTTIE_LIBRARY_DIR}/librlottie.a")
    endif()
endif()


# --------------------------   minizip   --------------------------------------
set(IM_MINIZIP "${pkg_src_minizip_location}/include")
include_directories(${IM_MINIZIP})
message(STATUS "[include] include_directories(${IM_MINIZIP})")


# --------------------------   d3dcompiler  -----------------------------------
set(D3D_COMPILER_LIBRARY "${pkg_bin_d3dcompiler_47.dll_location}")

# --------------------------   GPU blacklist  -----------------------------------
set(GPU_BLACKLIST "${ICQ_ROOT}/config/qt/${ICQ_PLATFORM}/gpu_blacklist.json")

# --------------------------   mesa3d   ---------------------------------------
set(MESA_LIBRARY "${pkg_mesa3d_location}/lib/opengl32sw.dll")


# -------------------   windows runtime libraries  ----------------------------
if (IM_QT_DYNAMIC)
    if (MSVC)
        set(WINSDK_RUNTIME "${pkg_runtime_location}")
        if("${CMAKE_BUILD_TYPE}" STREQUAL "Debug")
            set(RUNTIME_LIBRARIES_TO_COPY
                ${WINSDK_RUNTIME}/concrt140d.dll
                ${WINSDK_RUNTIME}/msvcp140d.dll
                ${WINSDK_RUNTIME}/vccorlib140d.dll
                ${WINSDK_RUNTIME}/vcruntime140d.dll
                ${WINSDK_RUNTIME}/msvcp140d_codecvt_ids.dll
                ${WINSDK_RUNTIME}/ucrtbased.dll
                )
        else()
            set(RUNTIME_LIBRARIES_TO_COPY
                ${WINSDK_RUNTIME}/concrt140.dll
                ${WINSDK_RUNTIME}/msvcp140.dll
                ${WINSDK_RUNTIME}/vccorlib140.dll
                ${WINSDK_RUNTIME}/vcruntime140.dll
                ${WINSDK_RUNTIME}/msvcp140_codecvt_ids.dll
                ${WINSDK_RUNTIME}/ucrtbase.dll
                ${WINSDK_RUNTIME}/api-ms-win-core-console-l1-1-0.dll
                ${WINSDK_RUNTIME}/api-ms-win-core-console-l1-2-0.dll
                ${WINSDK_RUNTIME}/api-ms-win-core-datetime-l1-1-0.dll
                ${WINSDK_RUNTIME}/api-ms-win-core-debug-l1-1-0.dll
                ${WINSDK_RUNTIME}/api-ms-win-core-errorhandling-l1-1-0.dll
                ${WINSDK_RUNTIME}/api-ms-win-core-file-l1-1-0.dll
                ${WINSDK_RUNTIME}/api-ms-win-core-file-l1-2-0.dll
                ${WINSDK_RUNTIME}/api-ms-win-core-file-l2-1-0.dll
                ${WINSDK_RUNTIME}/api-ms-win-core-handle-l1-1-0.dll
                ${WINSDK_RUNTIME}/api-ms-win-core-heap-l1-1-0.dll
                ${WINSDK_RUNTIME}/api-ms-win-core-interlocked-l1-1-0.dll
                ${WINSDK_RUNTIME}/api-ms-win-core-libraryloader-l1-1-0.dll
                ${WINSDK_RUNTIME}/api-ms-win-core-localization-l1-2-0.dll
                ${WINSDK_RUNTIME}/api-ms-win-core-memory-l1-1-0.dll
                ${WINSDK_RUNTIME}/api-ms-win-core-namedpipe-l1-1-0.dll
                ${WINSDK_RUNTIME}/api-ms-win-core-processenvironment-l1-1-0.dll
                ${WINSDK_RUNTIME}/api-ms-win-core-processthreads-l1-1-0.dll
                ${WINSDK_RUNTIME}/api-ms-win-core-processthreads-l1-1-1.dll
                ${WINSDK_RUNTIME}/api-ms-win-core-profile-l1-1-0.dll
                ${WINSDK_RUNTIME}/api-ms-win-core-rtlsupport-l1-1-0.dll
                ${WINSDK_RUNTIME}/api-ms-win-core-string-l1-1-0.dll
                ${WINSDK_RUNTIME}/api-ms-win-core-synch-l1-1-0.dll
                ${WINSDK_RUNTIME}/api-ms-win-core-synch-l1-2-0.dll
                ${WINSDK_RUNTIME}/api-ms-win-core-sysinfo-l1-1-0.dll
                ${WINSDK_RUNTIME}/api-ms-win-core-timezone-l1-1-0.dll
                ${WINSDK_RUNTIME}/api-ms-win-core-util-l1-1-0.dll
                ${WINSDK_RUNTIME}/API-MS-Win-core-xstate-l2-1-0.dll
                ${WINSDK_RUNTIME}/api-ms-win-crt-conio-l1-1-0.dll
                ${WINSDK_RUNTIME}/api-ms-win-crt-convert-l1-1-0.dll
                ${WINSDK_RUNTIME}/api-ms-win-crt-environment-l1-1-0.dll
                ${WINSDK_RUNTIME}/api-ms-win-crt-filesystem-l1-1-0.dll
                ${WINSDK_RUNTIME}/api-ms-win-crt-heap-l1-1-0.dll
                ${WINSDK_RUNTIME}/api-ms-win-crt-locale-l1-1-0.dll
                ${WINSDK_RUNTIME}/api-ms-win-crt-math-l1-1-0.dll
                ${WINSDK_RUNTIME}/api-ms-win-crt-multibyte-l1-1-0.dll
                ${WINSDK_RUNTIME}/api-ms-win-crt-private-l1-1-0.dll
                ${WINSDK_RUNTIME}/api-ms-win-crt-process-l1-1-0.dll
                ${WINSDK_RUNTIME}/api-ms-win-crt-runtime-l1-1-0.dll
                ${WINSDK_RUNTIME}/api-ms-win-crt-stdio-l1-1-0.dll
                ${WINSDK_RUNTIME}/api-ms-win-crt-string-l1-1-0.dll
                ${WINSDK_RUNTIME}/api-ms-win-crt-time-l1-1-0.dll
                ${WINSDK_RUNTIME}/api-ms-win-crt-utility-l1-1-0.dll
                )
        endif()
    endif()
endif()

# --------------------------  libvoip  ----------------------------------------
if (NOT STRIP_VOIP)

    set(VOIP_LIBS_NAME "voip3_${VOIP_LIBS_VERSION}_desktop")
    set(VOIP_LIBS_FILE_DESTINATION "${DOWNLOADS_PATH}/${VOIP_LIBS_NAME}.zip")
    set(VOIP_LIBS_FILE_MD5 "${VOIP_LIBS_FILE_DESTINATION}.md5")

    set(VOIP_LIBS_URL "${DOWNLOADS_URL}/Voiplib/${VOIP_LIBS_VERSION}/${VOIP_LIBS_NAME}.zip")

    set(VOIPLIB_ROOT "${ICQ_ROOT}/core/Voip/libvoip_${VOIP_LIBS_VERSION}")
    include_directories(${VOIPLIB_ROOT})

    message(STATUS "")
    message(STATUS "[VOIP]")
    message(STATUS "[VOIP] DOWNLOADS_URL              = ${DOWNLOADS_URL}")
    message(STATUS "[VOIP] DOWNLOADS_PATH             = ${DOWNLOADS_PATH}")

    message(STATUS "[VOIP] VOIP_LIBS_VERSION          = ${VOIP_LIBS_VERSION}")
    message(STATUS "[VOIP] VOIP_LIBS_NAME             = ${VOIP_LIBS_NAME}")
    message(STATUS "[VOIP] VOIP_LIBS_FILE_DESTINATION = ${VOIP_LIBS_FILE_DESTINATION}")
    message(STATUS "[VOIP] VOIP_LIBS_FILE_MD5         = ${VOIP_LIBS_FILE_MD5}")
    message(STATUS "[VOIP] VOIP_LIBS_URL              = ${VOIP_LIBS_URL}")
    message(STATUS "[VOIP] VOIPLIB_ROOT               = ${VOIPLIB_ROOT}")

    if(NOT EXISTS "${VOIP_LIBS_FILE_MD5}")
        message(STATUS "[md5] No md5 hash, continue")
        common_download_file("${VOIP_LIBS_URL}" "${VOIP_LIBS_FILE_DESTINATION}" "${VOIP_LIBS_FILE_MD5}")
        common_file_verify_md5("${VOIP_LIBS_FILE_DESTINATION}" "${VOIP_LIBS_FILE_MD5}")
        common_file_extract("${VOIP_LIBS_FILE_DESTINATION}" "${VOIPLIB_ROOT}" 1 "${VOIP_LIBS_FILE_MD5}")
    else()
        message(STATUS "[md5] Reading MD5 HASH from md5 file")
        file(READ "${VOIP_LIBS_FILE_MD5}" VOIP_LIBS_FILE_WITH_MD5)
        string(STRIP "${VOIP_LIBS_FILE_WITH_MD5}" VOIP_LIBS_FILE_WITH_MD5)
        message(STATUS "[md5] VOIP_LIBS_FILE_WITH_MD5 = ${VOIP_LIBS_FILE_WITH_MD5}")

        message(STATUS "[md5] Checksum MD5 HASH")
        file(MD5 "${VOIP_LIBS_FILE_DESTINATION}" VOIP_LIBS_FILE_ZIPFILE_MD5)
        message(STATUS "[md5] ${VOIP_LIBS_FILE_DESTINATION} = ${VOIP_LIBS_FILE_ZIPFILE_MD5}")

        IF(NOT "${VOIP_LIBS_FILE_WITH_MD5}" MATCHES "${VOIP_LIBS_FILE_ZIPFILE_MD5}")
            message(STATUS "[md5] MD5 mismatch")
            message(STATUS "[md5] The file will be re-downloaded")
            common_download_file("${VOIP_LIBS_URL}" "${VOIP_LIBS_FILE_DESTINATION}" "${VOIP_LIBS_FILE_MD5}")
            common_file_verify_md5("${VOIP_LIBS_FILE_DESTINATION}" "${VOIP_LIBS_FILE_MD5}")
            common_file_extract("${VOIP_LIBS_FILE_DESTINATION}" "${VOIPLIB_ROOT}" 1 "${VOIP_LIBS_FILE_MD5}")
        else()
            # Don't download if exists & don't extract if exists
            message(STATUS "[md5] file: OK, verified")
            message(STATUS "")
            common_file_extract("${VOIP_LIBS_FILE_DESTINATION}" "${VOIPLIB_ROOT}" 0 "${VOIP_LIBS_FILE_MD5}")
        endif()
    endif()

    if(MSVC)
        set(VOIP_LIBRARIES "${VOIP_LIBRARIES}" msdmo.lib dmoguids.lib wmcodecdspuuid.lib amstrmid.lib winmm.lib secur32.lib ws2_32.lib comsuppw.lib)
        set(VOIP_LIBRARIES "${VOIP_LIBRARIES}" optimized comsuppw.lib debug comsuppwd.lib)
    endif()

    if(NOT VOIP_AS_MODULE)
        include_directories("${VOIPLIB_ROOT}/include")
        if(MSVC)
            set(VOIP_LIBRARIES "${VOIP_LIBRARIES}" optimized "${VOIPLIB_ROOT}/lib/windows.Release.x86/libvoip_sdk_merged_x86.lib" debug "${VOIPLIB_ROOT}/lib/windows.Debug.x86/libvoip_sdk_merged_x86d.lib")

        elseif(APPLE)
            set(VOIP_LIBRARIES "${VOIPLIB_ROOT}/lib/macos.Release.x64/libvoip_sdk_merged_x64.a")

        elseif(LINUX)
            set(VOIP_LIBRARIES "${VOIPLIB_ROOT}/lib/linux.Release.x64/libvoip_sdk_merged_x64.a")
        endif()
    endif()

endif()
# -----------------------------------------------------------------------------

# ------------------------ additional libs & xkb ------------------------------
if(LINUX)

    set_lib_var(LIBXAU_LIBRARY_DIR "${pkg_libxau_location}")
    set_include_var(LIBXAU_INCLUDEDIR "${pkg_libxau_location}/include")

    set_lib_var(LIBXDMCP_LIBRARY_DIR "${pkg_libxdmcp_location}")
    set_include_var(LIBXDMCP_INCLUDE_DIR "${pkg_libxdmcp_location}/include")

    # Manual set, because custom *.a paths (not in lib subdir)
    SET(XKBCOMMON_LIBRARY_DIR "${pkg_xkbcommon_location}")
    SET(XKBCOMMON_X11_LIBRARY_DIR "${pkg_xkbcommon-x11_location}")
    message(STATUS "[lib] XKBCOMMON_LIBRARY_DIR -> ${XKBCOMMON_LIBRARY_DIR}")
    message(STATUS "[lib] XKBCOMMON_X11_LIBRARY_DIR -> ${XKBCOMMON_X11_LIBRARY_DIR}")

    set_lib_var(DBUS_LIBRARY_DIR ${pkg_libdbus_location})

    set_lib_var(XCBUTIL_LIBRARY_DIR "${pkg_xcb-util_location}")
    set_include_var(XCBUTIL_INCLUDE_DIR "${pkg_xcb-util_location}/include")

    set(ADDITIONAL_LIBRARIES
        "${pkg_bin_libXi.a_location}"
        "${LIBXAU_LIBRARY_DIR}/libXau.a"
        "${LIBXDMCP_LIBRARY_DIR}/libXdmcp.a"
        "${XKBCOMMON_LIBRARY_DIR}/libxkbcommon.a"
        "${XKBCOMMON_X11_LIBRARY_DIR}/libxkbcommon-x11.a"
        "${DBUS_LIBRARY_DIR}/libdbus-1.a"
        "${XCBUTIL_LIBRARY_DIR}/libxcb-util.a"
        "${pkg_bin_libXext.a_location}"
    )

    message(STATUS "[libs] ADDITIONAL_LIBRARIES -> ${ADDITIONAL_LIBRARIES}")

    set_lib_var(XCB_LIBRARY_DIR "${pkg_xcb_location}")
    set_include_var(XCB_INCLUDE_DIR "${pkg_xcb_location}/include")

    set_lib_var(XPROTO_LIBRARY_DIR "${pkg_xproto_location}")
    set_include_var(XPROTO_INCLUDE_DIR "${pkg_xproto_location}/include")

    set_lib_var(XCB-UTIL-IMAGE_LIBRARY_DIR "${pkg_xcb-util-image_location}")
    set_include_var(XCB-UTIL-IMAGE_INCLUDE_DIR "${pkg_xcb-util-image_location}/include")

    set_lib_var(XCB-UTIL-WM_LIBRARY_DIR "${pkg_xcb-util-wm_location}")
    set_include_var(XCB-UTIL-WM_INCLUDE_DIR "${pkg_xcb-util-wm_location}/include")

    set_lib_var(XCB-UTIL-KEYSYMS_LIBRARY_DIR "${pkg_xcb-util-keysyms_location}")
    set_include_var(XCB-UTIL-KEYSYMS_INCLUDE_DIR "${pkg_xcb-util-keysyms_location}/include")

    set_lib_var(XCB-UTIL-RENDERUTIL_LIBRARY_DIR "${pkg_xcb-util-renderutil_location}")
    set_include_var(XCB-UTIL-RENDERUTIL_INCLUDE_DIR "${pkg_xcb-util-renderutil_location}/include")

    set(XCB_LIBRARIES
        "${XCB_LIBRARY_DIR}/libxcb.a"
        "${XCB-UTIL-KEYSYMS_LIBRARY_DIR}/libxcb-keysyms.a"
        "${XCB-UTIL-IMAGE_LIBRARY_DIR}/libxcb-image.a"
        "${XCB_LIBRARY_DIR}/libxcb-shm.a"
        "${XCB-UTIL-WM_LIBRARY_DIR}/libxcb-icccm.a"
        "${XCB_LIBRARY_DIR}/libxcb-sync.a"
        "${XCB_LIBRARY_DIR}/libxcb-xfixes.a"
        "${XCB_LIBRARY_DIR}/libxcb-shape.a"
        "${XCB_LIBRARY_DIR}/libxcb-randr.a"
        "${XCB_LIBRARY_DIR}/libxcb-render.a"
        "${XCB-UTIL-RENDERUTIL_LIBRARY_DIR}/libxcb-render-util.a"
        "${XCB_LIBRARY_DIR}/libxcb-xkb.a"

        "${XCB_LIBRARY_DIR}/libxcb-xinput.a"

        "${XCB_LIBRARY_DIR}/libxcb-composite.a"
        "${XCB_LIBRARY_DIR}/libxcb-damage.a"
        "${XCB_LIBRARY_DIR}/libxcb-dpms.a"
        "${XCB_LIBRARY_DIR}/libxcb-dri2.a"
        "${XCB_LIBRARY_DIR}/libxcb-dri3.a"
        "${XCB_LIBRARY_DIR}/libxcb-glx.a"
        "${XCB_LIBRARY_DIR}/libxcb-present.a"
        "${XCB_LIBRARY_DIR}/libxcb-record.a"
        "${XCB_LIBRARY_DIR}/libxcb-res.a"
        "${XCB_LIBRARY_DIR}/libxcb-screensaver.a"
        "${XCB_LIBRARY_DIR}/libxcb-shm.a"
        "${XCB_LIBRARY_DIR}/libxcb-xf86dri.a"
        "${XCB_LIBRARY_DIR}/libxcb-xtest.a"
        "${XCB_LIBRARY_DIR}/libxcb-xv.a"
        "${XCB_LIBRARY_DIR}/libxcb-xvmc.a"

        )
    message(STATUS "[libs] XCB_LIBRARIES -> ${XCB_LIBRARIES}")

endif()

# --------------------------     qt     ---------------------------------------
message(STATUS "")
set(ICQ_QT_ROOT "${pkg_qt_location}")
message(STATUS "[INFO] ICQ_QT_ROOT = ${ICQ_QT_ROOT}")

set(ICQ_QT_BIN_DIR "${ICQ_QT_ROOT}/bin")
set(ICQ_QT_LIB_DIR "${ICQ_QT_ROOT}/lib")
set(ICQ_QT_INCLUDE_DIR "${ICQ_QT_ROOT}/include")
set(ICQ_QT_LIBEXEC_DIR "${ICQ_QT_ROOT}/libexec")
set(ICQ_QT_PLUGINS_DIR "${ICQ_QT_ROOT}/plugins")
set(ICQ_QT_RESOURCES_DIR "${ICQ_QT_ROOT}/resources")

if(LINUX)
    EXEC_PROGRAM("chmod +x ${pkg_bin_lrelease_location}")
    EXEC_PROGRAM("chmod +x ${pkg_bin_moc_location}")
    EXEC_PROGRAM("chmod +x ${pkg_bin_rcc_location}")
    EXEC_PROGRAM("chmod +x ${pkg_bin_uic_location}")
    set(IM_QT_BIN_LRELEASE "${pkg_bin_lrelease_location}")
    set(IM_QT_BIN_MOC "${pkg_bin_moc_location}")
    set(IM_QT_BIN_RCC "${pkg_bin_rcc_location}")
    set(IM_QT_BIN_UIC "${pkg_bin_uic_location}")

elseif(MSVC)
	set(IM_QT_BIN_LRELEASE "${pkg_bin_lrelease.exe_location}")
    get_filename_component(IM_QT_BIN_MOC "${ICQ_QT_BIN_DIR}/moc.exe" ABSOLUTE)
    get_filename_component(IM_QT_BIN_RCC "${ICQ_QT_BIN_DIR}/rcc.exe" ABSOLUTE)
    get_filename_component(IM_QT_BIN_UIC "${ICQ_QT_BIN_DIR}/uic.exe" ABSOLUTE)	

else()
    get_filename_component(IM_QT_BIN_LRELEASE "${ICQ_QT_BIN_DIR}/lrelease" ABSOLUTE)
    get_filename_component(IM_QT_BIN_MOC "${ICQ_QT_BIN_DIR}/moc" ABSOLUTE)
    get_filename_component(IM_QT_BIN_RCC "${ICQ_QT_BIN_DIR}/rcc" ABSOLUTE)
    get_filename_component(IM_QT_BIN_UIC "${ICQ_QT_BIN_DIR}/uic" ABSOLUTE)
endif()

message(STATUS "[Qt Utils] IM_QT_BIN_LRELEASE = ${IM_QT_BIN_LRELEASE}")
message(STATUS "[Qt Utils] IM_QT_BIN_MOC      = ${IM_QT_BIN_MOC}")
message(STATUS "[Qt Utils] IM_QT_BIN_RCC      = ${IM_QT_BIN_RCC}")
message(STATUS "[Qt Utils] IM_QT_BIN_UIC      = ${IM_QT_BIN_UIC}")

set(ICQ_QT_VERSION "${pkg_qt_headers}")
set(QT_MAJOR_VERSION "5")

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
    message(STATUS "[INFO] include: QtTest")
    include_directories("${ICQ_QT_INCLUDE_DIR}/QtTest")
endif()

if(NOT IM_QT_DYNAMIC)
    add_definitions(-DQT_STATIC)
endif()

if(MSVC)
    set(QT_LIBRARIES
        optimized ${ICQ_QT_LIB_DIR}/Qt5Gui.${QT_LIB_EXT} debug ${ICQ_QT_LIB_DIR}/Qt5Guid.${QT_LIB_EXT}
        optimized ${ICQ_QT_LIB_DIR}/Qt5Core.${QT_LIB_EXT} debug ${ICQ_QT_LIB_DIR}/Qt5Cored.${QT_LIB_EXT}
        optimized ${ICQ_QT_LIB_DIR}/Qt5Concurrent.${QT_LIB_EXT} debug ${ICQ_QT_LIB_DIR}/Qt5Concurrentd.${QT_LIB_EXT}
        optimized ${ICQ_QT_LIB_DIR}/Qt5EventDispatcherSupport.${QT_LIB_EXT} debug ${ICQ_QT_LIB_DIR}/Qt5EventDispatcherSupportd.${QT_LIB_EXT}
        optimized ${ICQ_QT_LIB_DIR}/Qt5FontDatabaseSupport.${QT_LIB_EXT} debug ${ICQ_QT_LIB_DIR}/Qt5FontDatabaseSupportd.${QT_LIB_EXT}
        optimized ${ICQ_QT_LIB_DIR}/Qt5Network.${QT_LIB_EXT} debug ${ICQ_QT_LIB_DIR}/Qt5Networkd.${QT_LIB_EXT}
        optimized ${ICQ_QT_LIB_DIR}/Qt5Multimedia.${QT_LIB_EXT} debug ${ICQ_QT_LIB_DIR}/Qt5Multimediad.${QT_LIB_EXT}
        optimized ${ICQ_QT_LIB_DIR}/Qt5MultimediaWidgets.${QT_LIB_EXT} debug ${ICQ_QT_LIB_DIR}/Qt5MultimediaWidgetsd.${QT_LIB_EXT}
        optimized ${ICQ_QT_LIB_DIR}/Qt5Multimedia.${QT_LIB_EXT} debug ${ICQ_QT_LIB_DIR}/Qt5Multimediad.${QT_LIB_EXT}
        optimized ${ICQ_QT_LIB_DIR}/Qt5Test.${QT_LIB_EXT} debug ${ICQ_QT_LIB_DIR}/Qt5Testd.${QT_LIB_EXT}
        optimized ${ICQ_QT_LIB_DIR}/Qt5ThemeSupport.${QT_LIB_EXT} debug ${ICQ_QT_LIB_DIR}/Qt5ThemeSupportd.${QT_LIB_EXT}
        optimized ${ICQ_QT_LIB_DIR}/Qt5OpenGL.${QT_LIB_EXT} debug ${ICQ_QT_LIB_DIR}/Qt5OpenGLd.${QT_LIB_EXT}
        optimized ${ICQ_QT_LIB_DIR}/Qt5Svg.${QT_LIB_EXT} debug ${ICQ_QT_LIB_DIR}/Qt5Svgd.${QT_LIB_EXT}
        optimized ${ICQ_QT_LIB_DIR}/Qt5PlatformCompositorSupport.${QT_LIB_EXT} debug ${ICQ_QT_LIB_DIR}/Qt5PlatformCompositorSupportd.${QT_LIB_EXT}
        optimized ${ICQ_QT_LIB_DIR}/Qt5Widgets.${QT_LIB_EXT} debug ${ICQ_QT_LIB_DIR}/Qt5Widgetsd.${QT_LIB_EXT}
        optimized ${ICQ_QT_LIB_DIR}/Qt5WinExtras.${QT_LIB_EXT} debug ${ICQ_QT_LIB_DIR}/Qt5WinExtrasd.${QT_LIB_EXT}
        optimized ${ICQ_QT_LIB_DIR}/Qt5WindowsUIAutomationSupport.${QT_LIB_EXT} debug ${ICQ_QT_LIB_DIR}/Qt5WindowsUIAutomationSupportd.${QT_LIB_EXT}
        optimized ${ICQ_QT_LIB_DIR}/qtpcre2.${QT_LIB_EXT} debug ${ICQ_QT_LIB_DIR}/qtpcre2d.${QT_LIB_EXT}
        optimized ${ICQ_QT_LIB_DIR}/qtmain.${QT_LIB_EXT} debug ${ICQ_QT_LIB_DIR}/qtmaind.${QT_LIB_EXT}
        optimized ${ICQ_QT_LIB_DIR}/qtharfbuzz.${QT_LIB_EXT} debug ${ICQ_QT_LIB_DIR}/qtharfbuzzd.${QT_LIB_EXT}
        optimized ${ICQ_QT_LIB_DIR}/qtfreetype.${QT_LIB_EXT} debug ${ICQ_QT_LIB_DIR}/qtfreetyped.${QT_LIB_EXT}
        optimized ${ICQ_QT_LIB_DIR}/qtlibpng.${QT_LIB_EXT} debug ${ICQ_QT_LIB_DIR}/qtlibpngd.${QT_LIB_EXT}
    )

    set(QT_PLUGINS
        optimized ${ICQ_QT_PLUGINS_DIR}/imageformats/qgif.${QT_PLUGIN_EXT} debug ${ICQ_QT_PLUGINS_DIR}/imageformats/qgifd.${QT_PLUGIN_EXT}
        optimized ${ICQ_QT_PLUGINS_DIR}/imageformats/qicns.${QT_PLUGIN_EXT} debug ${ICQ_QT_PLUGINS_DIR}/imageformats/qicnsd.${QT_PLUGIN_EXT}
        optimized ${ICQ_QT_PLUGINS_DIR}/imageformats/qico.${QT_PLUGIN_EXT} debug ${ICQ_QT_PLUGINS_DIR}/imageformats/qicod.${QT_PLUGIN_EXT}
        optimized ${ICQ_QT_PLUGINS_DIR}/imageformats/qjpeg.${QT_PLUGIN_EXT} debug ${ICQ_QT_PLUGINS_DIR}/imageformats/qjpegd.${QT_PLUGIN_EXT}
        optimized ${ICQ_QT_PLUGINS_DIR}/imageformats/qsvg.${QT_PLUGIN_EXT} debug ${ICQ_QT_PLUGINS_DIR}/imageformats/qsvgd.${QT_PLUGIN_EXT}
        optimized ${ICQ_QT_PLUGINS_DIR}/imageformats/qtga.${QT_PLUGIN_EXT} debug ${ICQ_QT_PLUGINS_DIR}/imageformats/qtgad.${QT_PLUGIN_EXT}
        optimized ${ICQ_QT_PLUGINS_DIR}/imageformats/qtiff.${QT_PLUGIN_EXT} debug ${ICQ_QT_PLUGINS_DIR}/imageformats/qtiffd.${QT_PLUGIN_EXT}
        optimized ${ICQ_QT_PLUGINS_DIR}/imageformats/qwbmp.${QT_PLUGIN_EXT} debug ${ICQ_QT_PLUGINS_DIR}/imageformats/qwbmpd.${QT_PLUGIN_EXT}
        optimized ${ICQ_QT_PLUGINS_DIR}/imageformats/qwebp.${QT_PLUGIN_EXT} debug ${ICQ_QT_PLUGINS_DIR}/imageformats/qwebpd.${QT_PLUGIN_EXT}
        optimized ${ICQ_QT_PLUGINS_DIR}/audio/qtaudio_windows.${QT_PLUGIN_EXT} debug ${ICQ_QT_PLUGINS_DIR}/audio/qtaudio_windowsd.${QT_PLUGIN_EXT}
        optimized ${ICQ_QT_PLUGINS_DIR}/platforms/qwindows.${QT_PLUGIN_EXT} debug ${ICQ_QT_PLUGINS_DIR}/platforms/qwindowsd.${QT_PLUGIN_EXT}
    )

    if(IM_QT_DYNAMIC)
        set_lib_var(ANGLE_LIB_DIR "${pkg_angle_location}")
        set_include_var(ANGLE_INCLUDE_DIR "${pkg_angle_location}/include")
        set(ANGLE_ROOT "${pkg_angle_location}")
        message(STATUS "[lib] ANGLE_ROOT -> ${ANGLE_ROOT}")

        if("${CMAKE_BUILD_TYPE}" STREQUAL "Debug")
            set(QT_LIBRARIES_TO_COPY
                ${ICQ_QT_BIN_DIR}/Qt5Guid.${QT_DLL_EXT}
                ${ICQ_QT_BIN_DIR}/Qt5Cored.${QT_DLL_EXT}
                ${ICQ_QT_BIN_DIR}/Qt5Concurrentd.${QT_DLL_EXT}
                ${ICQ_QT_BIN_DIR}/Qt5Networkd.${QT_DLL_EXT}
                ${ICQ_QT_BIN_DIR}/Qt5Multimediad.${QT_DLL_EXT}
                ${ICQ_QT_BIN_DIR}/Qt5MultimediaWidgetsd.${QT_DLL_EXT}
                ${ICQ_QT_BIN_DIR}/Qt5Multimediad.${QT_DLL_EXT}
                ${ICQ_QT_BIN_DIR}/Qt5Testd.${QT_DLL_EXT}
                ${ICQ_QT_BIN_DIR}/Qt5OpenGLd.${QT_DLL_EXT}
                ${ICQ_QT_BIN_DIR}/Qt5Svgd.${QT_DLL_EXT}
                ${ICQ_QT_BIN_DIR}/Qt5Widgetsd.${QT_DLL_EXT}
                ${ICQ_QT_BIN_DIR}/Qt5WinExtrasd.${QT_DLL_EXT}
                )

            set(QT_IMAGE_PLUGINS_TO_COPY
                ${ICQ_QT_PLUGINS_DIR}/imageformats/qgifd.${QT_DLL_EXT}
                ${ICQ_QT_PLUGINS_DIR}/imageformats/qicnsd.${QT_DLL_EXT}
                ${ICQ_QT_PLUGINS_DIR}/imageformats/qicod.${QT_DLL_EXT}
                ${ICQ_QT_PLUGINS_DIR}/imageformats/qjpegd.${QT_DLL_EXT}
                ${ICQ_QT_PLUGINS_DIR}/imageformats/qsvgd.${QT_DLL_EXT}
                ${ICQ_QT_PLUGINS_DIR}/imageformats/qtgad.${QT_DLL_EXT}
                ${ICQ_QT_PLUGINS_DIR}/imageformats/qtiffd.${QT_DLL_EXT}
                ${ICQ_QT_PLUGINS_DIR}/imageformats/qwbmpd.${QT_DLL_EXT}
                ${ICQ_QT_PLUGINS_DIR}/imageformats/qwebpd.${QT_DLL_EXT}
                )

            set(QT_ICONENGINES_PLUGINS_TO_COPY
                ${ICQ_QT_PLUGINS_DIR}/iconengines/qsvgicond.${QT_DLL_EXT}
                )

            set(QT_PLATFORMS_PLUGINS_TO_COPY
                ${ICQ_QT_PLUGINS_DIR}/platforms/qwindowsd.${QT_DLL_EXT}
                )

            set(QT_AUDIO_PLUGINS_TO_COPY
                ${ICQ_QT_PLUGINS_DIR}/audio/qtaudio_windowsd.${QT_DLL_EXT}
                )

            set(OPENGL_LIBRARIES_TO_COPY
                ${ANGLE_LIB_DIR}/libEGLd.dll
                ${ANGLE_LIB_DIR}/libGLESv2d.dll
                )


            if(HAS_WEB_ENGINE)
                set(QT_LIBRARIES "${QT_LIBRARIES}"
                    ${ICQ_QT_LIB_DIR}/Qt5WebEngined.${QT_LIB_EXT}
                    ${ICQ_QT_LIB_DIR}/Qt5WebEngineCored.${QT_LIB_EXT}
                    ${ICQ_QT_LIB_DIR}/Qt5WebEngineWidgetsd.${QT_LIB_EXT}
                    ${ICQ_QT_LIB_DIR}/Qt5Quickd.${QT_LIB_EXT}
                    ${ICQ_QT_LIB_DIR}/Qt5QmlModelsd.${QT_LIB_EXT}
                    ${ICQ_QT_LIB_DIR}/Qt5Qmld.${QT_LIB_EXT}
                    ${ICQ_QT_LIB_DIR}/Qt5Positioningd.${QT_LIB_EXT}
                    ${ICQ_QT_LIB_DIR}/Qt5QuickWidgetsd.${QT_LIB_EXT}
					${ICQ_QT_LIB_DIR}/Qt5WebChanneld.${QT_LIB_EXT}
                    )

                set(QT_LIBRARIES_TO_COPY "${QT_LIBRARIES_TO_COPY}"
                    ${ICQ_QT_BIN_DIR}/Qt5WebEngined.${QT_DLL_EXT}
                    ${ICQ_QT_BIN_DIR}/Qt5WebEngineCored.${QT_DLL_EXT}
                    ${ICQ_QT_BIN_DIR}/Qt5WebEngineWidgetsd.${QT_DLL_EXT}
                    ${ICQ_QT_BIN_DIR}/Qt5Quickd.${QT_DLL_EXT}
                    ${ICQ_QT_BIN_DIR}/Qt5QmlModelsd.${QT_DLL_EXT}
                    ${ICQ_QT_BIN_DIR}/Qt5Qmld.${QT_DLL_EXT}
                    ${ICQ_QT_BIN_DIR}/Qt5Positioningd.${QT_DLL_EXT}
                    ${ICQ_QT_BIN_DIR}/Qt5QuickWidgetsd.${QT_DLL_EXT}
                    ${ICQ_QT_BIN_DIR}/Qt5WebChanneld.${QT_DLL_EXT}
                    ${ICQ_QT_BIN_DIR}/Qt5PrintSupportd.${QT_DLL_EXT}
                    )

                set(QT_LIBEXEC_TO_COPY
                    ${ICQ_QT_BIN_DIR}/QtWebEngineProcessd.exe
                )
            endif()
        else()
            set(QT_LIBRARIES_TO_COPY
                ${ICQ_QT_BIN_DIR}/Qt5Gui.${QT_DLL_EXT}
                ${ICQ_QT_BIN_DIR}/Qt5Core.${QT_DLL_EXT}
                ${ICQ_QT_BIN_DIR}/Qt5Concurrent.${QT_DLL_EXT}
                ${ICQ_QT_BIN_DIR}/Qt5Network.${QT_DLL_EXT}
                ${ICQ_QT_BIN_DIR}/Qt5Multimedia.${QT_DLL_EXT}
                ${ICQ_QT_BIN_DIR}/Qt5MultimediaWidgets.${QT_DLL_EXT}
                ${ICQ_QT_BIN_DIR}/Qt5Multimedia.${QT_DLL_EXT}
                ${ICQ_QT_BIN_DIR}/Qt5Test.${QT_DLL_EXT}
                ${ICQ_QT_BIN_DIR}/Qt5OpenGL.${QT_DLL_EXT}
                ${ICQ_QT_BIN_DIR}/Qt5Svg.${QT_DLL_EXT}
                ${ICQ_QT_BIN_DIR}/Qt5Widgets.${QT_DLL_EXT}
                ${ICQ_QT_BIN_DIR}/Qt5WinExtras.${QT_DLL_EXT}
                )

            set(QT_IMAGE_PLUGINS_TO_COPY
                ${ICQ_QT_PLUGINS_DIR}/imageformats/qgif.${QT_DLL_EXT}
                ${ICQ_QT_PLUGINS_DIR}/imageformats/qicns.${QT_DLL_EXT}
                ${ICQ_QT_PLUGINS_DIR}/imageformats/qico.${QT_DLL_EXT}
                ${ICQ_QT_PLUGINS_DIR}/imageformats/qjpeg.${QT_DLL_EXT}
                ${ICQ_QT_PLUGINS_DIR}/imageformats/qsvg.${QT_DLL_EXT}
                ${ICQ_QT_PLUGINS_DIR}/imageformats/qtga.${QT_DLL_EXT}
                ${ICQ_QT_PLUGINS_DIR}/imageformats/qtiff.${QT_DLL_EXT}
                ${ICQ_QT_PLUGINS_DIR}/imageformats/qwbmp.${QT_DLL_EXT}
                ${ICQ_QT_PLUGINS_DIR}/imageformats/qwebp.${QT_DLL_EXT}
                )

            set(QT_ICONENGINES_PLUGINS_TO_COPY
                ${ICQ_QT_PLUGINS_DIR}/iconengines/qsvgicon.${QT_DLL_EXT}
                )

            set(QT_PLATFORMS_PLUGINS_TO_COPY
                ${ICQ_QT_PLUGINS_DIR}/platforms/qwindows.${QT_DLL_EXT}
                )

            set(QT_AUDIO_PLUGINS_TO_COPY
                ${ICQ_QT_PLUGINS_DIR}/audio/qtaudio_windows.${QT_DLL_EXT}
                )

            set(OPENGL_LIBRARIES_TO_COPY
                ${ANGLE_LIB_DIR}/libEGL.dll
                ${ANGLE_LIB_DIR}/libGLESv2.dll
                )

            if(HAS_WEB_ENGINE)
                set(QT_LIBRARIES "${QT_LIBRARIES}"
                    ${ICQ_QT_LIB_DIR}/Qt5WebEngine.${QT_LIB_EXT}
                    ${ICQ_QT_LIB_DIR}/Qt5WebEngineCore.${QT_LIB_EXT}
                    ${ICQ_QT_LIB_DIR}/Qt5WebEngineWidgets.${QT_LIB_EXT}
                    ${ICQ_QT_LIB_DIR}/Qt5Quick.${QT_LIB_EXT}
                    ${ICQ_QT_LIB_DIR}/Qt5QmlModels.${QT_LIB_EXT}
                    ${ICQ_QT_LIB_DIR}/Qt5Qml.${QT_LIB_EXT}
                    ${ICQ_QT_LIB_DIR}/Qt5Positioning.${QT_LIB_EXT}
                    ${ICQ_QT_LIB_DIR}/Qt5QuickWidgets.${QT_LIB_EXT}
					${ICQ_QT_LIB_DIR}/Qt5WebChannel.${QT_LIB_EXT}
                    )

                set(QT_LIBRARIES_TO_COPY "${QT_LIBRARIES_TO_COPY}"
                    ${ICQ_QT_BIN_DIR}/Qt5WebEngine.${QT_DLL_EXT}
                    ${ICQ_QT_BIN_DIR}/Qt5WebEngineCore.${QT_DLL_EXT}
                    ${ICQ_QT_BIN_DIR}/Qt5WebEngineWidgets.${QT_DLL_EXT}
                    ${ICQ_QT_BIN_DIR}/Qt5Quick.${QT_DLL_EXT}
                    ${ICQ_QT_BIN_DIR}/Qt5QmlModels.${QT_DLL_EXT}
                    ${ICQ_QT_BIN_DIR}/Qt5Qml.${QT_DLL_EXT}
                    ${ICQ_QT_BIN_DIR}/Qt5Positioning.${QT_DLL_EXT}
                    ${ICQ_QT_BIN_DIR}/Qt5QuickWidgets.${QT_DLL_EXT}
                    ${ICQ_QT_BIN_DIR}/Qt5WebChannel.${QT_DLL_EXT}
                    ${ICQ_QT_BIN_DIR}/Qt5PrintSupport.${QT_DLL_EXT}
                    )

                set(QT_LIBEXEC_TO_COPY
                    ${ICQ_QT_BIN_DIR}/QtWebEngineProcess.exe
                )
            endif()
        endif()
        set(QT_RESOURCES_TO_COPY
            ${ICQ_QT_RESOURCES_DIR}/icudtl.dat
            ${ICQ_QT_RESOURCES_DIR}/qtwebengine_devtools_resources.pak
            ${ICQ_QT_RESOURCES_DIR}/qtwebengine_resources.pak
            ${ICQ_QT_RESOURCES_DIR}/qtwebengine_resources_100p.pak
            ${ICQ_QT_RESOURCES_DIR}/qtwebengine_resources_200p.pak
            )

        if(HAS_WEB_ENGINE)
            set(QT_TRANSLATIONS_DIR "${ICQ_QT_ROOT}/translations")
        endif()
    else()
        set(QT_LIBRARIES "${QT_LIBRARIES}" "${QT_PLUGINS}")
    endif()
elseif(APPLE)
    add_definitions(-DQT_GUI_LIB=1)
    set(QT_LIBRARIES
        ${ICQ_QT_LIB_DIR}/libQt5Concurrent.${QT_LIB_EXT}
        ${ICQ_QT_LIB_DIR}/libQt5Core.${QT_LIB_EXT}
        ${ICQ_QT_LIB_DIR}/libQt5DBus.${QT_LIB_EXT}
        ${ICQ_QT_LIB_DIR}/libQt5Gui.${QT_LIB_EXT}
        ${ICQ_QT_LIB_DIR}/libQt5Multimedia.${QT_LIB_EXT}
        ${ICQ_QT_LIB_DIR}/libQt5Network.${QT_LIB_EXT}
        ${ICQ_QT_LIB_DIR}/libQt5PrintSupport.${QT_LIB_EXT}
        ${ICQ_QT_LIB_DIR}/libQt5Widgets.${QT_LIB_EXT}
        ${ICQ_QT_LIB_DIR}/libQt5MacExtras.${QT_LIB_EXT}
        ${ICQ_QT_LIB_DIR}/libQt5Svg.${QT_LIB_EXT}
        ${ICQ_QT_LIB_DIR}/libQt5ClipboardSupport.${QT_SUP_EXT}
        ${ICQ_QT_LIB_DIR}/libQt5FontDatabaseSupport.${QT_SUP_EXT}
        ${ICQ_QT_LIB_DIR}/libQt5AccessibilitySupport.${QT_SUP_EXT}
        ${ICQ_QT_LIB_DIR}/libQt5GraphicsSupport.${QT_SUP_EXT}
        ${ICQ_QT_LIB_DIR}/libQt5ThemeSupport.${QT_SUP_EXT}
        ${ICQ_QT_LIB_DIR}/libqtfreetype.${QT_SUP_EXT}
        ${ICQ_QT_LIB_DIR}/libqtharfbuzz.${QT_SUP_EXT}
        ${ICQ_QT_LIB_DIR}/libqtpcre2.${QT_SUP_EXT}
        ${ICQ_QT_LIB_DIR}/libqtlibpng.${QT_SUP_EXT}
        ${ICQ_QT_LIB_DIR}/libQt5MacExtras.${QT_LIB_EXT}
        ${ICQ_QT_PLUGINS_DIR}/imageformats/libqgif.${QT_PLUGIN_EXT}
        ${ICQ_QT_PLUGINS_DIR}/imageformats/libqicns.${QT_PLUGIN_EXT}
        ${ICQ_QT_PLUGINS_DIR}/imageformats/libqico.${QT_PLUGIN_EXT}
        ${ICQ_QT_PLUGINS_DIR}/imageformats/libqjpeg.${QT_PLUGIN_EXT}
        ${ICQ_QT_PLUGINS_DIR}/imageformats/libqmacheif.${QT_PLUGIN_EXT}
        ${ICQ_QT_PLUGINS_DIR}/imageformats/libqmacjp2.${QT_PLUGIN_EXT}
        ${ICQ_QT_PLUGINS_DIR}/imageformats/libqsvg.${QT_PLUGIN_EXT}
        ${ICQ_QT_PLUGINS_DIR}/imageformats/libqtga.${QT_PLUGIN_EXT}
        ${ICQ_QT_PLUGINS_DIR}/imageformats/libqtiff.${QT_PLUGIN_EXT}
        ${ICQ_QT_PLUGINS_DIR}/imageformats/libqwbmp.${QT_PLUGIN_EXT}
        ${ICQ_QT_PLUGINS_DIR}/imageformats/libqwebp.${QT_PLUGIN_EXT}
        ${ICQ_QT_PLUGINS_DIR}/platforms/libqcocoa.${QT_PLUGIN_EXT}
        )

    if(IM_QT_DYNAMIC)
        set(QT_LIBRARIES_TO_COPY
            ${ICQ_QT_LIB_DIR}/libQt5Concurrent.${QT_LIB_EXT}
            ${ICQ_QT_LIB_DIR}/libQt5Core.${QT_LIB_EXT}
            ${ICQ_QT_LIB_DIR}/libQt5DBus.${QT_LIB_EXT}
            ${ICQ_QT_LIB_DIR}/libQt5Gui.${QT_LIB_EXT}
            ${ICQ_QT_LIB_DIR}/libQt5Multimedia.${QT_LIB_EXT}
            ${ICQ_QT_LIB_DIR}/libQt5Network.${QT_LIB_EXT}
            ${ICQ_QT_LIB_DIR}/libQt5PrintSupport.${QT_LIB_EXT}
            ${ICQ_QT_LIB_DIR}/libQt5Widgets.${QT_LIB_EXT}
            ${ICQ_QT_LIB_DIR}/libQt5MacExtras.${QT_LIB_EXT}
            ${ICQ_QT_LIB_DIR}/libQt5Svg.${QT_LIB_EXT}
            ${ICQ_QT_LIB_DIR}/libQt5MacExtras.${QT_LIB_EXT}
            )

        set(QT_IMAGE_PLUGINS_TO_COPY
            ${ICQ_QT_PLUGINS_DIR}/imageformats/libqgif.${QT_LIB_EXT}
            ${ICQ_QT_PLUGINS_DIR}/imageformats/libqicns.${QT_LIB_EXT}
            ${ICQ_QT_PLUGINS_DIR}/imageformats/libqico.${QT_LIB_EXT}
            ${ICQ_QT_PLUGINS_DIR}/imageformats/libqjpeg.${QT_LIB_EXT}
            ${ICQ_QT_PLUGINS_DIR}/imageformats/libqmacheif.${QT_LIB_EXT}
            ${ICQ_QT_PLUGINS_DIR}/imageformats/libqmacjp2.${QT_LIB_EXT}
            ${ICQ_QT_PLUGINS_DIR}/imageformats/libqsvg.${QT_LIB_EXT}
            ${ICQ_QT_PLUGINS_DIR}/imageformats/libqtga.${QT_LIB_EXT}
            ${ICQ_QT_PLUGINS_DIR}/imageformats/libqtiff.${QT_LIB_EXT}
            ${ICQ_QT_PLUGINS_DIR}/imageformats/libqwbmp.${QT_LIB_EXT}
            ${ICQ_QT_PLUGINS_DIR}/imageformats/libqwebp.${QT_LIB_EXT}
            )

        set(QT_ICONENGINES_PLUGINS_TO_COPY
            ${ICQ_QT_PLUGINS_DIR}/iconengines/libqsvgicon.${QT_LIB_EXT}
            )

        set(QT_PLATFORMS_PLUGINS_TO_COPY
            ${ICQ_QT_PLUGINS_DIR}/platforms/libqcocoa.${QT_LIB_EXT}
            )

        if(HAS_WEB_ENGINE)
            set(QT_TRANSLATIONS_DIR "${ICQ_QT_ROOT}/translations")

            set(QT_LIBRARIES "${QT_LIBRARIES}"
                ${ICQ_QT_LIB_DIR}/libQt5WebEngine.${QT_LIB_EXT}
                ${ICQ_QT_LIB_DIR}/libQt5WebEngineCore.${QT_LIB_EXT}
                ${ICQ_QT_LIB_DIR}/libQt5WebEngineWidgets.${QT_LIB_EXT}
                ${ICQ_QT_LIB_DIR}/libQt5Quick.${QT_LIB_EXT}
                ${ICQ_QT_LIB_DIR}/libQt5QmlModels.${QT_LIB_EXT}
                ${ICQ_QT_LIB_DIR}/libQt5WebChannel.${QT_LIB_EXT}
                ${ICQ_QT_LIB_DIR}/libQt5Qml.${QT_LIB_EXT}
                ${ICQ_QT_LIB_DIR}/libQt5Positioning.${QT_LIB_EXT}
                ${ICQ_QT_LIB_DIR}/libQt5QuickWidgets.${QT_LIB_EXT}
                )

            set(QT_LIBRARIES_TO_COPY "${QT_LIBRARIES_TO_COPY}"
                ${ICQ_QT_LIB_DIR}/libQt5WebEngine.${QT_LIB_EXT}
                ${ICQ_QT_LIB_DIR}/libQt5WebEngineCore.${QT_LIB_EXT}
                ${ICQ_QT_LIB_DIR}/libQt5WebEngineWidgets.${QT_LIB_EXT}
                ${ICQ_QT_LIB_DIR}/libQt5Quick.${QT_LIB_EXT}
                ${ICQ_QT_LIB_DIR}/libQt5QmlModels.${QT_LIB_EXT}
                ${ICQ_QT_LIB_DIR}/libQt5WebChannel.${QT_LIB_EXT}
                ${ICQ_QT_LIB_DIR}/libQt5Qml.${QT_LIB_EXT}
                ${ICQ_QT_LIB_DIR}/libQt5Positioning.${QT_LIB_EXT}
                ${ICQ_QT_LIB_DIR}/libQt5QuickWidgets.${QT_LIB_EXT}
                )

            set(QT_LIBEXEC_TO_COPY
                ${ICQ_QT_LIBEXEC_DIR}/${QT_WEBENGINE_PROCESS_NAME}
                )

            set(QT_RESOURCES_TO_COPY
                ${ICQ_QT_RESOURCES_DIR}/icudtl.dat
                ${ICQ_QT_RESOURCES_DIR}/qtwebengine_devtools_resources.pak
                ${ICQ_QT_RESOURCES_DIR}/qtwebengine_resources.pak
                ${ICQ_QT_RESOURCES_DIR}/qtwebengine_resources_100p.pak
                ${ICQ_QT_RESOURCES_DIR}/qtwebengine_resources_200p.pak
                ${ICQ_ROOT}/config/qt/${ICQ_PLATFORM}/qt.conf
                )
        endif()
    endif()

    if(IM_AUTO_TESTING)
        message(STATUS "[INFO] add definitions -DQT_WIDGETS_LIB -DQT_CORE_LIB")
        add_definitions(-DQT_WIDGETS_LIB)
        add_definitions(-DQT_CORE_LIB)
    endif()

elseif(LINUX)
    if(IM_QT_DYNAMIC)
        set(QT_EXT ".${ICQ_QT_VERSION}")
    endif()
    set(QT_LIBRARIES
        ${ICQ_QT_LIB_DIR}/libQt5XcbQpa.${QT_LIB_EXT}${QT_EXT}
        ${ICQ_QT_LIB_DIR}/libQt5ServiceSupport.${QT_SUP_EXT}
        ${ICQ_QT_LIB_DIR}/libQt5ThemeSupport.${QT_SUP_EXT}
        ${ICQ_QT_LIB_DIR}/libQt5FontDatabaseSupport.${QT_SUP_EXT}
        ${ICQ_QT_LIB_DIR}/libQt5XkbCommonSupport.${QT_SUP_EXT}
        ${ICQ_QT_LIB_DIR}/libQt5LinuxAccessibilitySupport.${QT_SUP_EXT}
        ${ICQ_QT_LIB_DIR}/libQt5AccessibilitySupport.${QT_SUP_EXT}
        ${ICQ_QT_LIB_DIR}/libQt5EdidSupport.${QT_SUP_EXT}
        ${ICQ_QT_LIB_DIR}/libQt5DBus.${QT_LIB_EXT}${QT_EXT}
        ${ICQ_QT_LIB_DIR}/libQt5Widgets.${QT_LIB_EXT}${QT_EXT}
        ${ICQ_QT_LIB_DIR}/libQt5Gui.${QT_LIB_EXT}${QT_EXT}
        ${ICQ_QT_LIB_DIR}/libQt5Network.${QT_LIB_EXT}${QT_EXT}
        ${ICQ_QT_LIB_DIR}/libQt5Multimedia.${QT_LIB_EXT}${QT_EXT}
        ${ICQ_QT_LIB_DIR}/libQt5Concurrent.${QT_LIB_EXT}${QT_EXT}
        ${ICQ_QT_LIB_DIR}/libQt5Core.${QT_LIB_EXT}${QT_EXT}
        ${ICQ_QT_LIB_DIR}/libQt5Svg.${QT_LIB_EXT}${QT_EXT}
        ${XCB_LIBRARIES}
        ${XCB_LIBRARY_DIR}/libxcb.${QT_SUP_EXT}
        )

    set(QT_LIBRARIES_TO_COPY
        ${ICQ_QT_LIB_DIR}/libQt5XcbQpa.${QT_LIB_EXT}${QT_EXT}
        ${ICQ_QT_LIB_DIR}/libQt5DBus.${QT_LIB_EXT}${QT_EXT}
        ${ICQ_QT_LIB_DIR}/libQt5Widgets.${QT_LIB_EXT}${QT_EXT}
        ${ICQ_QT_LIB_DIR}/libQt5Gui.${QT_LIB_EXT}${QT_EXT}
        ${ICQ_QT_LIB_DIR}/libQt5Network.${QT_LIB_EXT}${QT_EXT}
        ${ICQ_QT_LIB_DIR}/libQt5Multimedia.${QT_LIB_EXT}${QT_EXT}
        ${ICQ_QT_LIB_DIR}/libQt5Concurrent.${QT_LIB_EXT}${QT_EXT}
        ${ICQ_QT_LIB_DIR}/libQt5Core.${QT_LIB_EXT}${QT_EXT}
        ${ICQ_QT_LIB_DIR}/libQt5Svg.${QT_LIB_EXT}${QT_EXT}
        ${ICQ_QT_LIB_DIR}/libQt5OpenGL.${QT_LIB_EXT}${QT_EXT}
        ${ICQ_QT_LIB_DIR}/libQt5PrintSupport.${QT_LIB_EXT}${QT_EXT}
        )

    set(QT_PLATFORMS_PLUGINS_TO_COPY
        ${ICQ_QT_PLUGINS_DIR}/platforms/libqxcb.${QT_LIB_EXT}
        )

    set(QT_XCBGLINTERGRATIONS_PLUGINS_TO_COPY
        ${ICQ_QT_PLUGINS_DIR}/xcbglintegrations/libqxcb-glx-integration.${QT_LIB_EXT}
        )

    set(QT_BEARER_PLUGINS_TO_COPY
        ${ICQ_QT_PLUGINS_DIR}/bearer/libqconnmanbearer.${QT_LIB_EXT}
        ${ICQ_QT_PLUGINS_DIR}/bearer/libqnmbearer.${QT_LIB_EXT}
        ${ICQ_QT_PLUGINS_DIR}/bearer/libqgenericbearer.${QT_LIB_EXT}
        )
    
    set(QT_IMAGE_PLUGINS_TO_COPY
        ${ICQ_QT_PLUGINS_DIR}/imageformats/libqgif.${QT_LIB_EXT}
        ${ICQ_QT_PLUGINS_DIR}/imageformats/libqicns.${QT_LIB_EXT}
        ${ICQ_QT_PLUGINS_DIR}/imageformats/libqico.${QT_LIB_EXT}
        ${ICQ_QT_PLUGINS_DIR}/imageformats/libqjpeg.${QT_LIB_EXT}
        ${ICQ_QT_PLUGINS_DIR}/imageformats/libqsvg.${QT_LIB_EXT}
        ${ICQ_QT_PLUGINS_DIR}/imageformats/libqtga.${QT_LIB_EXT}
        ${ICQ_QT_PLUGINS_DIR}/imageformats/libqtiff.${QT_LIB_EXT}
        ${ICQ_QT_PLUGINS_DIR}/imageformats/libqwbmp.${QT_LIB_EXT}
        ${ICQ_QT_PLUGINS_DIR}/imageformats/libqwebp.${QT_LIB_EXT}
        )
    
    set(QT_CONF_FILE ${ICQ_ROOT}/config/qt/${ICQ_PLATFORM}/qt.conf)

    if (NOT IM_QT_DYNAMIC)
        set(QT_LIBRARIES 
            "${QT_PLATFORMS_PLUGINS_TO_COPY}"
            "${QT_LIBRARIES}"
            ${ICQ_QT_LIB_DIR}/libqtpcre2.${QT_LIB_EXT}
            ${ICQ_QT_LIB_DIR}/libqtlibpng.${QT_LIB_EXT}
            "${QT_IMAGE_PLUGINS_TO_COPY}"
            "${QT_BEARER_PLUGINS_TO_COPY}"
            ${ICQ_QT_LIB_DIR}/libxcb-static.${QT_LIB_EXT}
        )
    endif()

    if(HAS_WEB_ENGINE)
        set(QT_LIBRARIES "${QT_LIBRARIES}"
            ${ICQ_QT_LIB_DIR}/libQt5WebEngine.${QT_LIB_EXT}${QT_EXT}
            ${ICQ_QT_LIB_DIR}/libQt5WebEngineCore.${QT_LIB_EXT}${QT_EXT}
            ${ICQ_QT_LIB_DIR}/libQt5WebEngineWidgets.${QT_LIB_EXT}${QT_EXT}
            ${ICQ_QT_LIB_DIR}/libQt5Quick.${QT_LIB_EXT}${QT_EXT}
            ${ICQ_QT_LIB_DIR}/libQt5QmlModels.${QT_LIB_EXT}${QT_EXT}
            ${ICQ_QT_LIB_DIR}/libQt5Qml.${QT_LIB_EXT}${QT_EXT}
            ${ICQ_QT_LIB_DIR}/libQt5Positioning.${QT_LIB_EXT}${QT_EXT}
            ${ICQ_QT_LIB_DIR}/libQt5QuickWidgets.${QT_LIB_EXT}${QT_EXT}
			${ICQ_QT_LIB_DIR}/libQt5WebChannel.${QT_LIB_EXT}${QT_EXT}
            )

        set(QT_LIBRARIES_TO_COPY "${QT_LIBRARIES_TO_COPY}"
            ${ICQ_QT_LIB_DIR}/libQt5WebEngine.${QT_LIB_EXT}${QT_EXT}
            ${ICQ_QT_LIB_DIR}/libQt5WebEngineCore.${QT_LIB_EXT}${QT_EXT}
            ${ICQ_QT_LIB_DIR}/libQt5WebEngineWidgets.${QT_LIB_EXT}${QT_EXT}
            ${ICQ_QT_LIB_DIR}/libQt5Quick.${QT_LIB_EXT}${QT_EXT}
            ${ICQ_QT_LIB_DIR}/libQt5QmlModels.${QT_LIB_EXT}${QT_EXT}
            ${ICQ_QT_LIB_DIR}/libQt5Qml.${QT_LIB_EXT}${QT_EXT}
            ${ICQ_QT_LIB_DIR}/libQt5Positioning.${QT_LIB_EXT}${QT_EXT}
            ${ICQ_QT_LIB_DIR}/libQt5QuickWidgets.${QT_LIB_EXT}${QT_EXT}
			${ICQ_QT_LIB_DIR}/libQt5WebChannel.${QT_LIB_EXT}${QT_EXT}
            )

        set(QT_LIBEXEC_TO_COPY
            ${ICQ_QT_LIBEXEC_DIR}/QtWebEngineProcess
            )
        
        set(QT_RESOURCES_TO_COPY
            ${ICQ_QT_RESOURCES_DIR}/icudtl.dat
            ${ICQ_QT_RESOURCES_DIR}/qtwebengine_devtools_resources.pak
            ${ICQ_QT_RESOURCES_DIR}/qtwebengine_resources.pak
            ${ICQ_QT_RESOURCES_DIR}/qtwebengine_resources_100p.pak
            ${ICQ_QT_RESOURCES_DIR}/qtwebengine_resources_200p.pak
            )
    endif()
endif()


# ------------------------- protobuf  -----------------------------------------
set_lib_var(PROTOBUF_LIBRARY_DIR "${pkg_protobuf_location}")
set_include_var(PROTOBUF_INCLUDE_DIR "${pkg_protobuf_location}/include")


# ------------------------ libphonenumber  ------------------------------------
if (NOT STRIP_VOIP)
    set_lib_var(PHONENUMBER_LIBRARY_DIR "${pkg_libphonenumber_location}")
    set_include_var(PHONENUMBER_INCLUDE_DIR "${pkg_libphonenumber_location}/include")
    add_definitions ("-DI18N_PHONENUMBERS_USE_BOOST")
    if(LINUX OR APPLE)
        set(PHONENUMBER_LIBRARIES ${PHONENUMBER_LIBRARY_DIR}/libphonenumber.a)
    elseif(MSVC)
        set(PHONENUMBER_LIBRARIES optimized ${PHONENUMBER_LIBRARY_DIR}/phonenumber.lib debug ${PHONENUMBER_LIBRARY_DIR}/phonenumber.lib)
    endif()
endif()


# --------------------------- breakpad  ---------------------------------------
if (STRIP_CRASH_HANDLER)
    add_definitions(-DSTRIP_CRASH_HANDLER)
else()
    if (LINUX)

        set_lib_var(BREAKPAD_LIBRARY_DIR "${pkg_breakpad_location}")
        set(IM_BREAKPAD "${pkg_src_breakpad_location}/src")

        FILE(GLOB breakpad_common_files
            ${IM_BREAKPAD}/common/*.cc
            ${IM_BREAKPAD}/common/*.h
        )

        FILE(GLOB breakpad_client_files
            ${IM_BREAKPAD}/client/*.cc
            ${IM_BREAKPAD}/client/*.h
        )

        include_directories(${IM_BREAKPAD})
        include_directories(${IM_BREAKPAD}/client)
        include_directories(${IM_BREAKPAD}/common)
        message(STATUS "[include] include_directories(${IM_BREAKPAD})")
        message(STATUS "[include] include_directories(${IM_BREAKPAD}/client)")
        message(STATUS "[include] include_directories(${IM_BREAKPAD}/common)")
        set(BREAKPAD_LIBRARIES ${BREAKPAD_LIBRARY_DIR}/libbreakpad_client.a)

        FILE(GLOB_RECURSE breakpad_common_platform_files
            ${IM_BREAKPAD}/common/linux/*.cc
            ${IM_BREAKPAD}/common/linux/*.h
        )

        FILE(GLOB_RECURSE breakpad_client_platform_files
            ${IM_BREAKPAD}/client/linux/*.cc
            ${IM_BREAKPAD}/client/linux/*.h
        )

        SOURCE_GROUP(breakpad FILES ${breakpad_common_files} ${breakpad_client_files}
        ${breakpad_common_platform_files} ${breakpad_client_platform_files})
    endif()


    if (APPLE)
        if (BUILD_FOR_STORE)
            message(STATUS "[INFO] > BREAKPAD")

            set_lib_var(BREAKPAD_LIBRARY_DIR "${pkg_breakpad_location}")
            set(IM_BREAKPAD "${pkg_src_breakpad_location}/src")

            include_directories(${IM_BREAKPAD})
            include_directories(${IM_BREAKPAD}/client)
            include_directories(${IM_BREAKPAD}/common)
            set(BREAKPAD_LIBRARIES ${BREAKPAD_LIBRARY_DIR}/libbreakpad.a)

            set(BREAKPAD_SOURCE
            ${IM_BREAKPAD}/common/string_conversion.cc
            ${IM_BREAKPAD}/common/convert_UTF.cc
            ${IM_BREAKPAD}/common/md5.cc
            ${IM_BREAKPAD}/client/minidump_file_writer.cc
            ${IM_BREAKPAD}/client/mac/crash_generation/crash_generation_client.cc
            ${IM_BREAKPAD}/client/mac/handler/exception_handler.cc
            ${IM_BREAKPAD}/client/mac/handler/minidump_generator.cc
            ${IM_BREAKPAD}/client/mac/handler/breakpad_nlist_64.cc
            ${IM_BREAKPAD}/client/mac/handler/dynamic_images.cc
            ${IM_BREAKPAD}/client/mac/handler/protected_memory_allocator.cc
            ${IM_BREAKPAD}/common/mac/bootstrap_compat.cc
            ${IM_BREAKPAD}/common/mac/file_id.cc
            ${IM_BREAKPAD}/common/mac/macho_id.cc
            ${IM_BREAKPAD}/common/mac/macho_reader.cc
            ${IM_BREAKPAD}/common/mac/macho_utilities.cc
            ${IM_BREAKPAD}/common/mac/macho_walker.cc
            ${IM_BREAKPAD}/common/mac/string_utilities.cc
            ${IM_BREAKPAD}/common/mac/MachIPC.mm)

        else()

            # --------------------------- crashpad  --------------------------
            message(STATUS "[INFO] > CRASHPAD")
            set(IM_CRASHPAD "${pkg_crashpad_location}")
            set_lib_var(CRASHPAD_LIBRARY_DIR "${pkg_crashpad_location}")

            include_directories(${IM_CRASHPAD}/include)
            include_directories(${IM_CRASHPAD}/include/mini_chromium)

            set(CRASHPAD_LIBRARIES
                ${CRASHPAD_LIBRARY_DIR}/libclient.a
                ${CRASHPAD_LIBRARY_DIR}/libbase.a
                ${CRASHPAD_LIBRARY_DIR}/libsnapshot.a
                ${CRASHPAD_LIBRARY_DIR}/libmig_output.a
                ${CRASHPAD_LIBRARY_DIR}/libutil.a)
        endif()

    endif()
endif()


# --------------------------- re2  --------------------------------------------
if (STRIP_RE2)
    add_definitions(-DSTRIP_RE2)
else()
    set_lib_var(RE2_LIBRARY_DIR "${pkg_re2_location}")
    set_include_var(PHONENUMBER_INCLUDE_DIR "${pkg_re2_location}/include")

    if(LINUX OR APPLE)
        set(RE2_LIBRARIES ${RE2_LIBRARY_DIR}/libre2.a)
    elseif (MSVC)
        set(RE2_LIBRARIES optimized ${RE2_LIBRARY_DIR}/re2.lib debug ${RE2_LIBRARY_DIR}/re2.lib)
    endif()
endif()


# --------------------------    zstd    --------------------------
if (STRIP_ZSTD)
    add_definitions(-DSTRIP_ZSTD)
else()
    set_lib_var(ZSTD_LIBRARY_DIR "${pkg_zstd_location}")
    set_include_var(ZSTD_INCLUDE_DIR "${pkg_zstd_location}/include")
    if(MSVC)
        set(ZSTD_LIBRARIES
            optimized "${ZSTD_LIBRARY_DIR}/zstd_static.lib"
            debug "${ZSTD_LIBRARY_DIR}/zstd_static.lib")
    else()
        set(ZSTD_LIBRARIES "${ZSTD_LIBRARY_DIR}/libzstd.a")
    endif()
endif()


# ------------------------ source groups  -------------------------------------
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


# -------------------------- precompiled --------------------------------------
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



# --------------------------    scan    ---------------------------------------
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


# --------------------------    mocs    ---------------------------------------
function(generate_mocs output ${ARGN})
    set(ICQ_MOC_DIR "${CMAKE_CURRENT_BINARY_DIR}/mocs")
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
                COMMAND ${IM_QT_BIN_MOC} -f ${header} -o ${moc_file} ${header} 2> /dev/null
                DEPENDS ${header} VERBATIM)
        elseif(LINUX)
            add_custom_command(OUTPUT ${moc_file}
                COMMAND ${IM_QT_BIN_MOC} -D__linux__ -f ${header} -o ${moc_file} ${header} 2> /dev/null
                DEPENDS ${header} VERBATIM)
        else()
            add_custom_command(OUTPUT ${moc_file}
                COMMAND ${IM_QT_BIN_MOC} -f ${header} -o ${moc_file} ${header}
                DEPENDS ${header} VERBATIM)
        endif()
    endforeach()
    set_source_group("mocs" "${ICQ_MOC_DIR}" ${_moc_files})
    set(${output} ${_moc_files} PARENT_SCOPE)
endfunction()


# # -------------------------  translations  ------------------------------------
# function(generate_translations language_files qm_files translations_dir ${ARGN})
#     set(_language_files)
#     set(_qm_files)

#     message(STATUS "[info] Define IM_QT_BIN_LRELEASE to ${IM_QT_BIN_LRELEASE}/lrelease")
#     get_filename_component(IM_QT_BIN_LRELEASE "${ICQ_QT_BIN_DIR}/lrelease" ABSOLUTE)
#     message(STATUS "[info] IM_QT_BIN_LRELEASE = ${IM_QT_BIN_LRELEASE}")
#     foreach(lang IN ITEMS ${ARGN})
#         set(lang_file "${translations_dir}/${lang}.ts")
#         set(qm_file "${translations_dir}/${lang}.qm")
#         add_custom_command(OUTPUT "${qm_file}"
#             COMMAND ${IM_QT_BIN_LRELEASE} ${lang_file} -qm ${qm_file}
#             DEPENDS ${lang_file})
#         list(APPEND _language_files "${lang_file}")
#         list(APPEND _qm_files "${qm_file}")
#     endforeach()
#     source_group("translations" FILES ${_language_files})
#     source_group("translations" FILES ${_qm_files})
#     set(${language_files} ${_language_files} PARENT_SCOPE)
#     set(${qm_files} ${_qm_files} PARENT_SCOPE)
# endfunction()


# --------------------------  subdirs   ---------------------------------------
add_subdirectory(core)
add_subdirectory(corelib)
add_subdirectory(gui)
add_subdirectory(libomicron)

# Set <PROJECT_NAME> as Startup Project for Visual Studio *.sln
# set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT ${PROJECT_NAME})
