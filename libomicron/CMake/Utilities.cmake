# File containing various utilities

# -------------------------- precompiled -------------------------
macro(use_precompiled_header_msvc pch_h pch_cpp ${ARGN})
    set(_pch_bin "${PROJECT_BINARY_DIR}/${CMAKE_BUILD_TYPE}.${PROJECT_NAME}.pch")
    foreach(_source IN ITEMS ${ARGN})
        set_source_files_properties(${_source} PROPERTIES
            COMPILE_FLAGS "/Yu\"${pch_h}\" /Fp\"${_pch_bin}\""
            OBJECT_DEPENDS "${_pch_bin}")
    endforeach()
    set_source_files_properties("${pch_cpp}" PROPERTIES
        COMPILE_FLAGS "/Yc\"${pch_h}\" /Fp\"${_pch_bin}\""
        OBJECT_DEPENDS "${_pch_bin}")
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
        COMMAND ${CMAKE_CXX_COMPILER} ${_build_flags} ${_paths} ${_definitions} -mmacosx-version-min=10.9 -stdlib=libc++ -std=gnu++11 -x c++-header ${pch_h} -o ${_pch_bin}
        DEPENDS ${pch_h} VERBATIM)
    foreach(_source IN ITEMS ${ARGN})
        set_source_files_properties(${_source} PROPERTIES
            COMPILE_FLAGS "-include-pch ${_pch_bin}"
            OBJECT_DEPENDS "${_pch_bin}")
    endforeach()
    set(${output} ${_pch_bin} PARENT_SCOPE)
endfunction()

function(use_precompiled_header_linux output pch_h ${ARGN})
    set(_pch_bin "${pch_h}.gch")
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
        COMMAND ${CMAKE_CXX_COMPILER} ${_build_flags} ${_paths} ${_definitions} -std=gnu++11 ${ARCH_FLAGS} -fPIC -x c++-header -o ${_pch_bin} ${pch_h}
        DEPENDS ${pch_h} VERBATIM)
    foreach(_source IN ITEMS ${ARGN})
        set_source_files_properties(${_source} PROPERTIES
            COMPILE_FLAGS "-std=gnu++11 ${ARCH_FLAGS} -Winvalid-pch"
            OBJECT_DEPENDS "${_pch_bin}")
    endforeach()
    set(${output} ${_pch_bin} PARENT_SCOPE)
endfunction()
