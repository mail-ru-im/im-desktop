cmake_minimum_required(VERSION 3.18)

message(STATUS "")
message(STATUS "[CMAKE]")
message(STATUS "[CMAKE] including <requirements/common.cmake>")
message(STATUS "[CMAKE]")

set(DOWNLOADS_PATH "${ICQ_ROOT}/downloads")
set(ICQ_EXTERNAL "${ICQ_ROOT}/external_deps")

set(EXT_LIBS_VERSION "v42")
set(deps_ver_qt_utils "v13")

# Inform QA team if the variable is deleted, moved or the name will be changed
if(IM_QT_DYNAMIC)
    set(VOIP_LIBS_VERSION "3.6.5.2097") #dynamic
else()
    set(VOIP_LIBS_VERSION "3.6.5.2099") #static
endif()

set(DOWNLOADS_URL "https://hb.bizmrg.com/icq-www")

string(TOLOWER "${CMAKE_BUILD_TYPE}" build_type_lower)
if(build_type_lower STREQUAL "debug")
    set(PACKAGE_ENV "debug")
else()
    set(PACKAGE_ENV "release")
endif()


string(TOLOWER "${CUSTOM_CMAKE_SYSTEM}" CUSTOM_CMAKE_SYSTEM_LOWER )
if(CMAKE_SYSTEM_NAME STREQUAL Windows OR CUSTOM_CMAKE_SYSTEM_LOWER STREQUAL windows)
    set(PACKAGE_ARCH "x32")
elseif(CMAKE_SYSTEM_NAME STREQUAL Linux OR CUSTOM_CMAKE_SYSTEM_LOWER STREQUAL linux)
    set(PACKAGE_ARCH "x64")
elseif(CMAKE_SYSTEM_NAME STREQUAL Darwin OR CUSTOM_CMAKE_SYSTEM_LOWER STREQUAL macos)
    set(PACKAGE_ARCH "x64")
endif()


message(STATUS "[requirements] DOWNLOADS_URL         = ${DOWNLOADS_URL}")
message(STATUS "[requirements] DOWNLOADS_PATH        = ${DOWNLOADS_PATH}")
message(STATUS "[requirements] ICQ_EXTERNAL          = ${ICQ_EXTERNAL}")
message(STATUS "[requirements] EXT_LIBS_VERSION      = ${EXT_LIBS_VERSION}")
message(STATUS "[requirements] deps_ver_qt_utils     = ${deps_ver_qt_utils}")
message(STATUS "[requirements] VOIP_LIBS_VERSION     = ${VOIP_LIBS_VERSION}")
message(STATUS "[requirements] PACKAGE_ENV           = ${PACKAGE_ENV}")
message(STATUS "[requirements] PACKAGE_ARCH          = ${PACKAGE_ARCH}")


function(common_download_file URL FILENAME MD5_FILENAME)
    # message(STATUS "[downloading]")
    # message(STATUS "[downloading] common_download_file()")
    message(STATUS "[downloading] -> ${URL}")
    # message(STATUS "[downloading] -> ${FILENAME}")
    if (EXISTS "${MD5_FILEPATH}")
        file(REMOVE_RECURSE "${MD5_FILEPATH}")
    endif()

    if (EXISTS "${FILENAME}")
        file(REMOVE_RECURSE "${FILENAME}")
    endif()

    message(STATUS "[downloading] Downloading the file")
    file(DOWNLOAD "${URL}" "${FILENAME}"
         SHOW_PROGRESS
         TIMEOUT 900
         INACTIVITY_TIMEOUT 900
         TLS_VERIFY ON
         STATUS DOWNLOAD_STATE)

    list(GET DOWNLOAD_STATE 0 DOWNLOAD_STATE_CODE)
    list(GET DOWNLOAD_STATE 1 DOWNLOAD_STATE_TEXT)

    if (NOT DOWNLOAD_STATE_CODE EQUAL 0)
        message(WARNING "[downloading] DOWNLOAD_STATE = ${DOWNLOAD_STATE}")
        message(WARNING "[downloading] DOWNLOAD_STATE_CODE = ${DOWNLOAD_STATE_CODE}")
        message(WARNING "[downloading] DOWNLOAD_STATE_TEXT = ${DOWNLOAD_STATE_TEXT}")
        message(WARNING "[downloading] Failed to download ${FILENAME}")
        message(FATAL_ERROR "Failed to download the file, return value ${DOWNLOAD_STATE}")
    else()
        message(STATUS "[downloading] -> Completed!")
    endif()

    message(STATUS "[downloading]")
endfunction(common_download_file)


function(common_file_verify_md5 ARCHIVE_PATH MD5_FILE_DESTINATION)
    message(STATUS "[verifying] common_file_verify_md5()")
    if(EXISTS "${ARCHIVE_PATH}")
        # message(STATUS "[md5] Checksum MD5 hash")
        file(MD5 "${ARCHIVE_PATH}" FILE_MD5_SUMM_DATA)
        string(STRIP "${FILE_MD5_SUMM_DATA}" FILE_MD5_SUMM_DATA)
        # message(STATUS "[md5] FILE_MD5_SUMM_DATA = ${FILE_MD5_SUMM_DATA}")

        # message(STATUS "[md5] [write] [force] Saving MD5 to file > ${MD5_FILE_DESTINATION}")
        FILE(WRITE "${MD5_FILE_DESTINATION}" "${FILE_MD5_SUMM_DATA}")
    else()
        message(WARNING "[404] File ${ARCHIVE_PATH} not found!")
    endif()

    if(NOT EXISTS "${MD5_FILE_DESTINATION}")
        message(WARNING "[404] File ${MD5_FILE_DESTINATION} was not created")
    endif()
    # message(STATUS "[verifying]")
endfunction(common_file_verify_md5)


function(common_file_extract ARCHIVE_PATH DESTINATION_PATH FORCE_EXTRACT MD5_FILEPATH)
    # message(STATUS "[extracting] common_file_extract()")
    if(NOT EXISTS "${DESTINATION_PATH}" OR FORCE_EXTRACT)
        if (EXISTS "${ARCHIVE_PATH}_unpacked")
            file(REMOVE_RECURSE "${ARCHIVE_PATH}_unpacked")
        endif()
    endif()

    if(NOT EXISTS "${DESTINATION_PATH}"
       OR "${FORCE_EXTRACT}"
       OR "${ARCHIVE_PATH}" IS_NEWER_THAN "${DESTINATION_PATH}"
       OR NOT EXISTS "${ARCHIVE_PATH}_unpacked")

        file(REMOVE_RECURSE "${DESTINATION_PATH}")
        file(MAKE_DIRECTORY "${DESTINATION_PATH}")

        file(ARCHIVE_EXTRACT INPUT "${ARCHIVE_PATH}"
             DESTINATION "${DESTINATION_PATH}"
        )

        if(EXISTS "${DESTINATION_PATH}")
            FILE(WRITE "${ARCHIVE_PATH}_unpacked" "OK")
        else()
            message(FATAL_ERROR "Failed to unarchive zip")
        endif()

    endif()
endfunction(common_file_extract)


function(common_prepare_libs action pkg_md5 pkg_link pkg_destination pkg_unpacked)

    if(NOT EXISTS "${pkg_md5}")
        # message(STATUS "[md5] No md5 hash, continue")
        common_download_file("${pkg_link}" "${pkg_destination}" "${pkg_md5}")
        common_file_verify_md5("${pkg_destination}" "${pkg_md5}")

        # Don't extract
        if(${action} STREQUAL all)
            common_file_extract("${pkg_destination}" "${pkg_unpacked}" 1 "${pkg_md5}")
        endif()
    else()
        # message(STATUS "[md5] Reading MD5 HASH from md5 file")
        file(READ "${pkg_md5}" EXTERNAL_FILE_WITH_MD5)
        string(STRIP "${EXTERNAL_FILE_WITH_MD5}" EXTERNAL_FILE_WITH_MD5)
        # message(STATUS "[md5] EXTERNAL_FILE_WITH_MD5 = ${EXTERNAL_FILE_WITH_MD5}")

        # message(STATUS "[md5] Checksum MD5 HASH")
        file(MD5 "${pkg_destination}" EXTERNAL_FILE_ZIPFILE_MD5)
        # message(STATUS "[md5] ${pkg_destination} = ${EXTERNAL_FILE_ZIPFILE_MD5}")

        IF(NOT "${EXTERNAL_FILE_WITH_MD5}" MATCHES "${EXTERNAL_FILE_ZIPFILE_MD5}")
            message(STATUS "[md5] MD5 mismatch")
            message(STATUS "[md5] The file will be re-downloaded")
            common_download_file("${pkg_link}" "${pkg_destination}" "${pkg_md5}")
            common_file_verify_md5("${pkg_destination}" "${pkg_md5}")

            # Don't extract
            if(${action} STREQUAL all)
                common_file_extract("${pkg_destination}" "${pkg_unpacked}" 1 "${pkg_md5}")
            endif()
        else()
            # Don't download if exists & don't extract if exists
            # message(STATUS "[md5] file: OK, verified")
            # message(STATUS "")

            # Don't extract
            if(${action} STREQUAL all)
                common_file_extract("${pkg_destination}" "${pkg_unpacked}" 0 "${pkg_md5}")
            endif()
        endif()
    endif()

endfunction(common_prepare_libs)


macro(common_add_ext_package depName depVersion depLinking depArch depHeaders depCommit depEnvironment depUrl)
    string(TOLOWER "${CUSTOM_CMAKE_SYSTEM}" CUSTOM_CMAKE_SYSTEM_LOWER )
    if(CMAKE_SYSTEM_NAME STREQUAL Windows OR CUSTOM_CMAKE_SYSTEM_LOWER STREQUAL windows)
        set(PKG_PLATFORM "windows")
    elseif(CMAKE_SYSTEM_NAME STREQUAL Linux OR CUSTOM_CMAKE_SYSTEM_LOWER STREQUAL linux)
        set(PKG_PLATFORM "linux")
    elseif(CMAKE_SYSTEM_NAME STREQUAL Darwin OR CUSTOM_CMAKE_SYSTEM_LOWER STREQUAL macos)
        set(PKG_PLATFORM "macos")
    endif()

    add_custom_target(deps_pkg_${depName}_target)

    if(${depEnvironment} STREQUAL "release")
        # Release
        set(pkg_fname "${depName}_${depVersion}_${PKG_PLATFORM}_${depArch}_${depLinking}.tar.gz")
        set(pkg_link "${DOWNLOADS_URL}/external/${depName}/lib/${depVersion}/${pkg_fname}")
        set(pkg_destination "${DOWNLOADS_PATH}/${pkg_fname}")
    else()
        # Debug
        set(pkg_fname "${depName}_${depVersion}_${PKG_PLATFORM}_${depArch}_${depLinking}_debug.tar.gz")
        set(pkg_link "${DOWNLOADS_URL}/external/${depName}/lib/${depVersion}/${pkg_fname}")
        set(pkg_destination "${DOWNLOADS_PATH}/${pkg_fname}")
    endif()
    set(pkg_md5 "${pkg_destination}.md5")
    set(pkg_unpacked "${ICQ_EXTERNAL}/${depName}_${depVersion}_${depEnvironment}")

    set_target_properties(deps_pkg_${depName}_target PROPERTIES
        pkg_name "${depName}"
        pkg_filename "${pkg_fname}"
        version "${depVersion}"
        linking "${depLinking}"
        arch "${depArch}"
        headers "${depHeaders}"
        commit "${depCommit}"
        url_file "${depUrl}"
        environment "${depEnvironment}"
        file_hash "${fileHash}"
        pkg_location "${ICQ_EXTERNAL}/${depName}_${depVersion}_${depEnvironment}"
        pkg_destination "${pkg_destination}"
    )
    get_target_property(pkg_${depName}_name deps_pkg_${depName}_target pkg_name)
    get_target_property(pkg_${depName}_filename deps_pkg_${depName}_target pkg_filename)
    get_target_property(pkg_${depName}_version deps_pkg_${depName}_target version)
    get_target_property(pkg_${depName}_linking deps_pkg_${depName}_target linking)
    get_target_property(pkg_${depName}_arch deps_pkg_${depName}_target arch)
    get_target_property(pkg_${depName}_headers deps_pkg_${depName}_target headers)
    get_target_property(pkg_${depName}_commit deps_pkg_${depName}_target commit)
    get_target_property(pkg_${depName}_url_file deps_pkg_${depName}_target url_file)
    get_target_property(pkg_${depName}_environment deps_pkg_${depName}_target environment)
    get_target_property(pkg_${depName}_file_hash deps_pkg_${depName}_target file_hash)
    get_target_property(pkg_${depName}_location deps_pkg_${depName}_target pkg_location)
    get_target_property(pkg_${depName}_pkg_destination deps_pkg_${depName}_target pkg_destination)

    message(STATUS "|-> ${depName} [${depVersion}] - ${pkg_link} [${depEnvironment}]")
    common_prepare_libs("all" "${pkg_md5}" "${pkg_link}" "${pkg_destination}" "${pkg_unpacked}")
endmacro(common_add_ext_package)


macro(common_add_ext_srcpkg depName depVersion depCommit depUrl depHash)
    add_custom_target(deps_src_${depName}_target)

    if(NOT ${depCommit} STREQUAL "OFF")
        set(pkg_fname "${depName}_${depVersion}_${depCommit}.tar.gz")
        set(pkg_link "${DOWNLOADS_URL}/external/${depName}/src/${depVersion}/${pkg_fname}")
        set(pkg_destination "${DOWNLOADS_PATH}/${pkg_fname}")
        set(pkg_md5 "${pkg_destination}.md5")
    else()
        set(pkg_fname "${depName}_${depVersion}.tar.gz")
        set(pkg_link "${DOWNLOADS_URL}/external/${depName}/src/${depVersion}/${pkg_fname}")
        set(pkg_destination "${DOWNLOADS_PATH}/${pkg_fname}")
        set(pkg_md5 "${pkg_destination}.md5")
    endif()
    set(pkg_unpacked "${ICQ_EXTERNAL}/${depName}_src_${depVersion}")

    set_target_properties(deps_src_${depName}_target PROPERTIES
        pkg_name "${depName}"
        pkg_filename "${pkg_fname}"
        version "${depVersion}"
        commit "${depCommit}"
        url_file "${depUrl}"
        file_hash "${depHash}"
        pkg_location "${ICQ_EXTERNAL}/${depName}_src_${depVersion}"
        pkg_destination "${pkg_destination}"
    )
    get_target_property(pkg_src_${depName}_name deps_src_${depName}_target pkg_name)
    get_target_property(pkg_src_${depName}_filename deps_src_${depName}_target pkg_filename)
    get_target_property(pkg_src_${depName}_version deps_src_${depName}_target version)
    get_target_property(pkg_src_${depName}_commit deps_src_${depName}_target commit)
    get_target_property(pkg_src_${depName}_url_file deps_src_${depName}_target url_file)
    get_target_property(pkg_src_${depName}_file_hash deps_src_${depName}_target file_hash)
    get_target_property(pkg_src_${depName}_location deps_src_${depName}_target pkg_location)
    get_target_property(pkg_src_${depName}_pkg_destination deps_src_${depName}_target pkg_destination)

    message(STATUS "|-> ${depName} [${depVersion}] - ${pkg_link}")
    common_prepare_libs("all" "${pkg_md5}" "${pkg_link}" "${pkg_destination}" "${pkg_unpacked}")
endmacro(common_add_ext_srcpkg)


macro(common_add_additionals depName depUrl depHash)
    add_custom_target(deps_additional_${depName}_target)

    set(pkg_fname "${depName}.tar.gz")
    set(pkg_destination "${DOWNLOADS_PATH}/additionals/${pkg_fname}")
    set(pkg_link "${DOWNLOADS_URL}/external/platform_specific/lib/${pkg_fname}")
    set(pkg_md5 "${pkg_destination}.md5")
    set(pkg_unpacked "${ICQ_EXTERNAL}/additionals/${depName}")

    set_target_properties(deps_additional_${depName}_target PROPERTIES
        pkg_name "${depName}"
        pkg_filename "${pkg_fname}"
        url_file "${depUrl}"
        file_hash "${depHash}"
        pkg_version "Unspecified"
        pkg_location "${ICQ_EXTERNAL}/additionals/${depName}"
        pkg_destination "${pkg_destination}"
    )
    get_target_property(pkg_additional_${depName}_name deps_additional_${depName}_target pkg_name)
    get_target_property(pkg_additional_${depName}_filename deps_additional_${depName}_target pkg_filename)
    get_target_property(pkg_additional_${depName}_url_file deps_additional_${depName}_target url_file)
    get_target_property(pkg_additional_${depName}_file_hash deps_additional_${depName}_target file_hash)
    get_target_property(pkg_additional_${depName}_version deps_additional_${depName}_target pkg_version)
    get_target_property(pkg_additional_${depName}_location deps_additional_${depName}_target pkg_location)
    get_target_property(pkg_additional_${depName}_pkg_destination deps_additional_${depName}_target pkg_destination)

    message(STATUS "|-> ${depName} - ${pkg_link}")
    common_prepare_libs("all" "${pkg_md5}" "${pkg_link}" "${pkg_destination}" "${pkg_unpacked}")
endmacro(common_add_additionals)


macro(common_add_ext_binaries depName depVersion depUrl depHash)
    add_custom_target(deps_bins_${depName}_target)

    set(pkg_fname "${depName}")
    set(pkg_link "${DOWNLOADS_URL}/external/platform_specific/bin/${depVersion}/${pkg_fname}")
    set(pkg_destination "${DOWNLOADS_PATH}/bins/${pkg_fname}")
    set(pkg_md5 "${pkg_destination}.md5")

    set_target_properties(deps_bins_${depName}_target PROPERTIES
        pkg_name "${depName}"
        pkg_filename "${pkg_fname}"
        pkg_version "${depVersion}"
        url_file "${depUrl}"
        file_hash "${depHash}"
        pkg_location "${DOWNLOADS_PATH}/bins/${depName}"
        pkg_destination "${pkg_destination}"
    )
    get_target_property(pkg_bin_${depName}_name deps_bins_${depName}_target pkg_name)
    get_target_property(pkg_bin_${depName}_filename deps_bins_${depName}_target pkg_filename)
    get_target_property(pkg_bin_${depName}_url_file deps_bins_${depName}_target url_file)
    get_target_property(pkg_bin_${depName}_file_hash deps_bins_${depName}_target file_hash)
    get_target_property(pkg_bin_${depName}_version deps_bins_${depName}_target pkg_version)
    get_target_property(pkg_bin_${depName}_location deps_bins_${depName}_target pkg_location)
    get_target_property(pkg_bin_${depName}_pkg_destination deps_bins_${depName}_target pkg_destination)

    message(STATUS "|-> ${depName} - ${pkg_link}")
    common_prepare_libs("download" "${pkg_md5}" "${pkg_link}" "${pkg_destination}" "SKIPPED")
endmacro(common_add_ext_binaries)


function(common_get_all_targets uniqueTargetsName varDefined targetPattern)
    set(${uniqueTargetsName})
    common_get_all_targets_recursive(${uniqueTargetsName} ${CMAKE_CURRENT_SOURCE_DIR} ${targetPattern})
    set(${varDefined} ${${uniqueTargetsName}} PARENT_SCOPE)
endfunction(common_get_all_targets)


macro(common_get_all_targets_recursive uniqueTargetsName dirName targetPattern)
    get_property(subdirectories DIRECTORY ${dirName} PROPERTY SUBDIRECTORIES)
    foreach(subdir ${subdirectories})
        common_get_all_targets_recursive(${uniqueTargetsName} ${subdir} ${targetPattern})
    endforeach()
    get_property(current_targets_${uniqueTargetsName} DIRECTORY ${dirName} PROPERTY BUILDSYSTEM_TARGETS)
    foreach(targetItem ${current_targets_${uniqueTargetsName}})
        if(${targetItem} MATCHES "${targetPattern}")
            list(APPEND ${uniqueTargetsName} ${targetItem})
        endif()
    endforeach()
endmacro(common_get_all_targets_recursive)


function(common_print_dependencies_info)
    common_get_all_targets(depPackagesTargets all_dep_pkg_targets deps_pkg_)
    common_get_all_targets(depSrcTargets all_dep_src_targets deps_src_)
    common_get_all_targets(depAdditionalsTargets all_dep_additionals_targets deps_additional_)
    common_get_all_targets(depBinsTargets all_dep_bins_targets deps_bins_)
    message(STATUS "")
    message(STATUS "")
    message(STATUS "[IM EXTERNAL DEPENDECIES] Listing:")
    message(STATUS "|")
    foreach(itemTarget ${all_dep_pkg_targets} ${all_dep_src_targets} ${all_dep_additionals_targets} ${all_dep_bins_targets})
        string(REPLACE "_target" "" EXTRACTED_NAME ${itemTarget})
        if(${itemTarget} MATCHES "deps_pkg_")
            string(REPLACE "deps_pkg_" "" EXTRACTED_NAME ${EXTRACTED_NAME})
            message(STATUS "|-> ${pkg_${EXTRACTED_NAME}_name} = ${pkg_${EXTRACTED_NAME}_version} [${pkg_${EXTRACTED_NAME}_arch}][${pkg_${EXTRACTED_NAME}_linking}]")
        endif()
        if(${itemTarget} MATCHES "deps_src_")
            string(REPLACE "deps_src_" "" EXTRACTED_NAME ${EXTRACTED_NAME})
            message(STATUS "|-> ${pkg_src_${EXTRACTED_NAME}_name} = ${pkg_src_${EXTRACTED_NAME}_version} [${pkg_src_${EXTRACTED_NAME}_arch}][${pkg_src_${EXTRACTED_NAME}_linking}]")
        endif()
        if(${itemTarget} MATCHES "deps_additional_")
            string(REPLACE "deps_additional_" "" EXTRACTED_NAME ${EXTRACTED_NAME})
            message(STATUS "|-> ${pkg_additional_${EXTRACTED_NAME}_name} = ${pkg_additional_${EXTRACTED_NAME}_version} [${pkg_additional_${EXTRACTED_NAME}_arch}][${pkg_additional_${EXTRACTED_NAME}_linking}]")
        endif()
        if(${itemTarget} MATCHES "deps_bins_")
            string(REPLACE "deps_bins_" "" EXTRACTED_NAME ${EXTRACTED_NAME})
            message(STATUS "|-> ${pkg_bin_${EXTRACTED_NAME}_name} = ${pkg_bin_${EXTRACTED_NAME}_version} [${pkg_bin_${EXTRACTED_NAME}_arch}][${pkg_additional_${EXTRACTED_NAME}_linking}]")
        endif()

    endforeach()
    message(STATUS "|")
    message(STATUS "[IM EXTERNAL DEPENDECIES] Listing -> Completed")
    message(STATUS "")
    message(STATUS "")
endfunction(common_print_dependencies_info)

