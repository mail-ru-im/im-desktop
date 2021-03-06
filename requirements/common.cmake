
set(DOWNLOADS_PATH "${ICQ_ROOT}/downloads")

# Inform QA team if the variable is deleted, moved or the name will be changed
set(VOIP_LIBS_VERSION "3.6.5.2072")

set(EXT_LIBS_VERSION "v32")

set(DOWNLOADS_URL "https://hb.bizmrg.com/icq-www/external")

# Platform specific
if(CMAKE_SYSTEM_NAME STREQUAL Linux)
    # Linux
    set(ICQ_EXTERNAL "${ICQ_ROOT}/external_deps")
    set(deps_ver_qt_utils "v13")

elseif(CMAKE_SYSTEM_NAME STREQUAL Darwin)
    # macOS
    set(ICQ_EXTERNAL "${ICQ_ROOT}/external")
    set(deps_ver_qt_utils "v13")

else()
    # Windows
    set(ICQ_EXTERNAL "${ICQ_ROOT}/external")
    set(deps_ver_qt_utils "v13")

endif()


# Define dependencies versions
set(deps_lib_version_boost "1.68.0")
set(deps_lib_version_breakpad "master")
set(deps_lib_version_breakpad_src "78180df6")
set(deps_lib_version_curl "7.62.0")
set(deps_lib_version_ffmpeg "4.3.1")
set(deps_lib_version_icu "67.1")
set(deps_lib_version_libdbus "1.12.20")
set(deps_lib_version_libphonenumber "8.12.11")
set(deps_lib_version_minizip "1.0.1e")
set(deps_lib_version_nghttp2 "1.41.0")
set(deps_lib_version_openal "1.20.1")
set(deps_lib_version_openssl "1.1.1")
set(deps_lib_version_protobuf "3.3.2")
set(deps_lib_version_qt "5.14.2")
set(deps_lib_version_rapidjson "master")
set(deps_lib_version_rapidjson_src "0ccdbf36")
set(deps_lib_version_re2 "2020-10-01")
set(deps_lib_version_zlib "1.2.11")
set(deps_lib_version_zstd "1.4.5")
set(deps_lib_version_rlottie "master-fix-crash")
set(deps_lib_version_rlottie_src "3cd0015")

function(download_file URL FILENAME MD5_FILENAME)
    message(STATUS "[downloading]")
    message(STATUS "[downloading] download_file()")
    message(STATUS "[downloading] -> ${URL}")
    message(STATUS "[downloading] -> ${FILENAME}")
    if (EXISTS "${MD5_FILEPATH}")
        file(REMOVE_RECURSE "${MD5_FILEPATH}")
    endif()

    if (EXISTS "${FILENAME}")
        file(REMOVE_RECURSE "${FILENAME}")
    endif()

    message(STATUS "[downloading] Downloading the file")
    file(DOWNLOAD "${URL}" "${FILENAME}"
         SHOW_PROGRESS
         TIMEOUT 600
         INACTIVITY_TIMEOUT 600
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
endfunction(download_file)


function(file_verify_md5 ARCHIVE_PATH MD5_FILE_DESTINATION)
    message(STATUS "[verifying] file_verify_md5()")
    if(EXISTS "${ARCHIVE_PATH}")
        message(STATUS "[md5] Checksum MD5 hash")
        file(MD5 "${ARCHIVE_PATH}" FILE_MD5_SUMM_DATA)
        string(STRIP "${FILE_MD5_SUMM_DATA}" FILE_MD5_SUMM_DATA)
        message(STATUS "[md5] FILE_MD5_SUMM_DATA = ${FILE_MD5_SUMM_DATA}")

        message(STATUS "[md5] [write] [force] Saving MD5 to file > ${MD5_FILE_DESTINATION}")
        FILE(WRITE "${MD5_FILE_DESTINATION}" "${FILE_MD5_SUMM_DATA}")
    else()
        message(WARNING "[404] File ${ARCHIVE_PATH} not found!")
    endif()

    if(NOT EXISTS "${MD5_FILE_DESTINATION}")
        message(WARNING "[404] File ${MD5_FILE_DESTINATION} was not created")
    endif()
    message(STATUS "[verifying]")
endfunction(file_verify_md5)


function(file_extract ARCHIVE_PATH DESTINATION_PATH FORCE_EXTRACT MD5_FILEPATH)
    message(STATUS "[extracting] file_extract()")
    if(NOT EXISTS "${DESTINATION_PATH}" OR FORCE_EXTRACT)
        if (EXISTS "${ARCHIVE_PATH}_unpacked")
            file(REMOVE_RECURSE "${ARCHIVE_PATH}_unpacked")
        endif()
    endif()

    if(NOT EXISTS "${DESTINATION_PATH}"
       OR "${FORCE_EXTRACT}"
       OR "${ARCHIVE_PATH}" IS_NEWER_THAN "${DESTINATION_PATH}"
       OR NOT EXISTS "${ARCHIVE_PATH}_unpacked")

        message(STATUS "[extracting] Extracting compressed file")
        message(STATUS "[extracting] -> ${ARCHIVE_PATH}")
        message(STATUS "[extracting] -> ${DESTINATION_PATH}")

        file(REMOVE_RECURSE "${DESTINATION_PATH}")
        file(MAKE_DIRECTORY "${DESTINATION_PATH}")

        execute_process(COMMAND ${CMAKE_COMMAND} -E tar xzf "${ARCHIVE_PATH}"
                        WORKING_DIRECTORY "${DESTINATION_PATH}"
                        RESULT_VARIABLE EXTRACT_ARCHIVE_RETURN_VALUE)

        if (NOT EXTRACT_ARCHIVE_RETURN_VALUE EQUAL 0)
            message(STATUS "[extracting] Failed to Unpack ${ARCHIVE_PATH}")
            message(STATUS "[extracting] Zip file is not valid or complete")
            if (EXISTS "${MD5_FILEPATH}")
                message(STATUS "[extracting] MD5 file was removed, please retry one more time")
                file(REMOVE_RECURSE "${MD5_FILEPATH}")
            endif()
            message(FATAL_ERROR "Failed to unarchive zip, return value ${EXTRACT_ARCHIVE_RETURN_VALUE}")
        else()
            message(STATUS "[extracting] -> Success")
        endif()

        if(EXISTS "${DESTINATION_PATH}")
            FILE(WRITE "${ARCHIVE_PATH}_unpacked" "OK")
        else()
            message(FATAL_ERROR "Failed to unarchive zip")
        endif()

    else()
        message(STATUS "[extracting] Skip extracting, folder exists")
        if (EXISTS "${ARCHIVE_PATH}_unpacked")
            message(STATUS "[extracting] -> Status file: ${ARCHIVE_PATH}_unpacked")
            message(STATUS "[extracting] -> Status: OK")
        endif()
        message(STATUS "[extracting] -> ${DESTINATION_PATH}")
    endif()
    message(STATUS "")
endfunction(file_extract)


function(common_prepare_libs action pkg_md5 pkg_link pkg_destination pkg_unpacked)

    if(NOT EXISTS "${pkg_md5}")
        message(STATUS "[md5] No md5 hash, continue")
        download_file("${pkg_link}" "${pkg_destination}" "${pkg_md5}")
        file_verify_md5("${pkg_destination}" "${pkg_md5}")

        # Don't extract
        if(${action} STREQUAL all)
            file_extract("${pkg_destination}" "${pkg_unpacked}" 1 "${pkg_md5}")
        endif()
    else()
        message(STATUS "[md5] Reading MD5 HASH from md5 file")
        file(READ "${pkg_md5}" EXTERNAL_FILE_WITH_MD5)
        string(STRIP "${EXTERNAL_FILE_WITH_MD5}" EXTERNAL_FILE_WITH_MD5)
        message(STATUS "[md5] EXTERNAL_FILE_WITH_MD5 = ${EXTERNAL_FILE_WITH_MD5}")

        message(STATUS "[md5] Checksum MD5 HASH")
        file(MD5 "${pkg_destination}" EXTERNAL_FILE_ZIPFILE_MD5)
        message(STATUS "[md5] ${pkg_destination} = ${EXTERNAL_FILE_ZIPFILE_MD5}")

        IF(NOT "${EXTERNAL_FILE_WITH_MD5}" MATCHES "${EXTERNAL_FILE_ZIPFILE_MD5}")
            message(STATUS "[md5] MD5 mismatch")
            message(STATUS "[md5] The file will be re-downloaded")
            download_file("${pkg_link}" "${pkg_destination}" "${pkg_md5}")
            file_verify_md5("${pkg_destination}" "${pkg_md5}")

            # Don't extract
            if(${action} STREQUAL all)
                file_extract("${pkg_destination}" "${pkg_unpacked}" 1 "${pkg_md5}")
            endif()
        else()
            # Don't download if exists & don't extract if exists
            message(STATUS "[md5] file: OK, verified")
            message(STATUS "")

            # Don't extract
            if(${action} STREQUAL all)
                file_extract("${pkg_destination}" "${pkg_unpacked}" 0 "${pkg_md5}")
            endif()
        endif()
    endif()

endfunction(common_prepare_libs)

