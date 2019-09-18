cmake_minimum_required(VERSION 3.8)

project(omicron-desktop-tests CXX)

enable_testing()

include_directories(${TOP_DIR}/include)

set(OMICRON_TESTS
    tests.cpp)

add_executable(${PROJECT_NAME} ${OMICRON_TESTS})

if(MSVC)
    set(CMAKE_CXX_FLAGS "/EHsc")
    set(SYSTEM_LIBRARIES Ws2_32 Wldap32 psapi.lib)
elseif(APPLE)
    find_library(MAC_SECURITY Security)
    find_library(MAC_FOUNDATION Foundation)
    mark_as_advanced(MAC_SECURITY MAC_FOUNDATION)
    set(SYSTEM_LIBRARIES
        ${MAC_SECURITY} ${MAC_FOUNDATION}
        ldap z)
elseif(UNIX)
    set(SYSTEM_LIBRARIES)
endif()

if(OMICRON_USE_LIBCURL)
    if(MSVC)
        message(STATUS "Windows")
        set(CURL_LIBRARY_DIR "${CURL_DIR}/lib")
        set(ZLIB_LIBRARY_DIR "${TOP_DIR}/../external/zlib/lib")
        set(CURL_LIBRARIES
            optimized "${CURL_LIBRARY_DIR}/libcurl.lib" "${ZLIB_LIBRARY_DIR}/zlib.lib"
            debug "${CURL_LIBRARY_DIR}/libcurld.lib" "${ZLIB_LIBRARY_DIR}/zlibd.lib")
    elseif(APPLE)
        message(STATUS "Apple")
        set(CURL_LIBRARIES "${TOP_DIR}/../external/mac/libcurl.a")
    elseif(UNIX)
        message(STATUS "Linux")
        set(CURL_LIBRARIES curl pthread)
    endif()

    set(TEST_LIBRARIES ${CURL_LIBRARIES})

endif(OMICRON_USE_LIBCURL)

target_link_libraries(${PROJECT_NAME}
    ${SYSTEM_LIBRARIES}
    libomicron
    ${TEST_LIBRARIES})
