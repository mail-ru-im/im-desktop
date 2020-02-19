#pragma once

#include <stddef.h> /* size_t */

typedef enum {
    ZSTDW_OK = 0,
    ZSTDW_OUTPUT_BUFFER_TOO_SMALL = 1,
    ZSTDW_DICT_FILE_READ_ERROR = 2,
    ZSTDW_PARSE_JSON_ERROR = 3,          // needPreprocessJSON was specified, but data_in cannot be parsed as Json
    ZSTDW_CONTENT_SIZE_ERROR = 4,        // unknown content size
    ZSTDW_ZSTD_ERROR_RANGE_START = 100,  // shift for other errors from ZSTD (ZSTDW_ZSTD_ERROR_RANGE_START+ZSTD_ErrorCode)
    // ... here go zstd error codes (ZSTDW_ZSTD_ERROR_RANGE_START+ZSTD_ErrorCode)
} ZSTDW_STATUS;


// data_in - input byte buffer
// data_in_size - size of data_in in bytes
// data_out - allocated buffer for output
// data_out_size - size of data_out in bytes
// dict_file_path - path to zstd dictionary
// compressionLevel - compression level 0-100 (zstd use 3 by default)
// needPreprocessJSON - flag for preprocess byte array as json for sort keys and remove redundant spaces
//
// return values: ZSTDW_STATUS
// data_out_written - ZSTDW_OK: how many bytes has been written in output buffer,
//                    ZSTDW_OUTPUT_BUFFER_TOO_SMALL: maximum compressed size in worst case
ZSTDW_STATUS ZSTDW_Encode(const char *data_in,
    size_t data_in_size,
    char  *data_out,
    size_t data_out_size,
    size_t *data_out_written,
    const char *dict_file_path,
    int compressionLevel,
    bool needPreprocessJSON);

// Version of ZSTDW_Encode with dictionary in buffer dictBuffer with size dictSize
// dict_file_path - only used for check dictionary changes, not for load (if changed - load dictionary from buffer)
ZSTDW_STATUS ZSTDW_Encode_buf(const char *data_in,
    size_t data_in_size,
    char  *data_out,
    size_t data_out_size,
    size_t *data_out_written,
    const char *dict_file_path,
    const char* dictBuffer,
    int dictSize,
    int compressionLevel,
    bool needPreprocessJSON);


// data_in - input byte buffer
// data_in_size - size of data_in in bytes
// data_out - allocated buffer for output
// data_out_size - size of data_out in bytes
// dict_file_path - path to zstd dictionary
//
// return values: ZSTDW_STATUS
// data_out_written - ZSTDW_OK: how many bytes has been written in output buffer,
//                    ZSTDW_OUTPUT_BUFFER_TOO_SMALL: required size for output buffer (0 if cannot be calculated)
ZSTDW_STATUS ZSTDW_Decode(const char *data_in,
    size_t data_in_size,
    char  *data_out,
    size_t data_out_size,
    size_t *data_out_written,
    const char *dict_file_path);

// Version of ZSTDW_Decode with dictionary in buffer dictBuffer with size dictSize
// dict_file_path - only used for check dictionary changes, not for load (if changed - load dictionary from buffer)
ZSTDW_STATUS ZSTDW_Decode_buf(const char *data_in,
    size_t data_in_size,
    char  *data_out,
    size_t data_out_size,
    size_t *data_out_written,
    const char *dict_file_path,
    const char* dictBuffer,
    int dictSize);
