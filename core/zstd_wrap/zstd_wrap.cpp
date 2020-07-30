#include "stdafx.h"
#include "zstd_wrap.h"

#define ZSTD_STATIC_LINKING_ONLY  // to use advanced zstd functions
#include "zstd.h"

#include "zstd_errors.h"
#include "zstd_context.h"
#include <fstream>
#include <iostream>
#include <vector>
#include <condition_variable>
#ifdef USE_RAPIDJSON
#include "tools/json_helper.h"
#else
#include <nlohmann/json.hpp>

// for convenience
using json = nlohmann::json;
#endif

using namespace std;

static Context context;
static std::mutex g_lock_encode, g_lock_decode;


//preprocess json string
//sort keys and remove extra spaces (nlohmann do it by default)
//if error in parse occurred then get source data and work with it (do encoding but return special error key)
int preprocessJSON(const char *data_in, size_t data_in_size, string &s_out)
{
    int res = 0;
#ifndef USE_RAPIDJSON
    //https://nlohmann.github.io/json/md_binary_formats.html
    try
    {
        string s(data_in, data_in_size);
        json j = json::parse(s);
        s_out = j.dump();
    }
    catch (const std::exception&)
    {
        s_out = string(data_in, data_in_size);
        res = -1;
    }
#else
    rapidjson::Document doc;
    if (doc.Parse(data_in, data_in_size).HasParseError())
    {
        s_out = string(data_in, data_in_size);
        res = -1;
    }
    else
    {
        core::tools::sort_json_keys_by_name(doc);

        rapidjson::StringBuffer buffer;
        rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
        doc.Accept(writer);

        s_out = core::rapidjson_get_string(buffer);
    }
#endif
    return res;
}

//encode data
ZSTDW_STATUS ZSTDW_Encode(const char *data_in,
    size_t data_in_size,
    char  *data_out,
    size_t data_out_size,
    size_t *data_out_written,
    const char *dict_file_path,
    int compressionLevel,
    bool needPreprocessJSON)
{
    return ZSTDW_Encode_buf(data_in, data_in_size, data_out, data_out_size, data_out_written, dict_file_path, nullptr, 0, compressionLevel, needPreprocessJSON);
}

//encode data
//universal function
//if dictSize is 0 then load dictionary from file else from buffer
ZSTDW_STATUS ZSTDW_Encode_buf(const char *data_in,
    size_t data_in_size,
    char  *data_out,
    size_t data_out_size,
    size_t *data_out_written,
    const char *dict_file_path,
    const char* dictBuffer,
    int dictSize,
    int compressionLevel,
    bool needPreprocessJSON)
{
    *data_out_written = 0;

    //preprocess json string if needed
    //if error in parse occurred then get source data and work with it (do encoding but return special error key)
    string s_data_in_preproc;
    int res_parse = 0;
    if (needPreprocessJSON) {
        res_parse = preprocessJSON(data_in, data_in_size, s_data_in_preproc);
    }

    ZSTDW_STATUS res;
    {
        std::lock_guard<std::mutex> lk(g_lock_encode);
        //check dict
        if (context.checkCtxEncode(dict_file_path, dictBuffer, dictSize, compressionLevel) == 0) {
            //encode
            size_t res_zstd;
            if (needPreprocessJSON)
                res_zstd = ZSTD_compress_usingCDict(context.cctx, data_out, data_out_size, s_data_in_preproc.c_str(), s_data_in_preproc.size(), context.c_dict);
            else
                res_zstd = ZSTD_compress_usingCDict(context.cctx, data_out, data_out_size, data_in, data_in_size, context.c_dict);
            ZSTD_ErrorCode er_code = ZSTD_getErrorCode(res_zstd);

            if (er_code == ZSTD_ErrorCode::ZSTD_error_no_error) {
                *data_out_written = res_zstd;
                res = ZSTDW_STATUS::ZSTDW_OK;
            }
            else {
                *data_out_written = ZSTD_compressBound(data_in_size);
                if (er_code == ZSTD_ErrorCode::ZSTD_error_dstSize_tooSmall)
                    res = ZSTDW_STATUS::ZSTDW_OUTPUT_BUFFER_TOO_SMALL;
                else
                    res = (ZSTDW_STATUS)(ZSTDW_STATUS::ZSTDW_ZSTD_ERROR_RANGE_START + er_code);
            }
        }
        else
            res= ZSTDW_STATUS::ZSTDW_DICT_FILE_READ_ERROR;
    }
    //if only error in parse occurred then return special error key
    if (res== ZSTDW_STATUS::ZSTDW_OK && res_parse!=0)
        res= ZSTDW_STATUS::ZSTDW_PARSE_JSON_ERROR;
    return res;
}

//decode data
ZSTDW_STATUS ZSTDW_Decode(const char *data_in,
    size_t data_in_size,
    char  *data_out,
    size_t data_out_size,
    size_t *data_out_written,
    const char *dict_file_path)
{
    return ZSTDW_Decode_buf(data_in, data_in_size, data_out, data_out_size, data_out_written, dict_file_path, nullptr, 0);
}

//decode data
//universal function
//if dictSize is 0 then load dictionary from file else from buffer
ZSTDW_STATUS ZSTDW_Decode_buf(const char *data_in,
    size_t data_in_size,
    char  *data_out,
    size_t data_out_size,
    size_t *data_out_written,
    const char *dict_file_path,
    const char* dictBuffer,
    int dictSize)
{
    *data_out_written = 0;

    ZSTDW_STATUS res;
    {
        std::lock_guard<std::mutex> lk(g_lock_decode);
        //check dict
        if (context.checkCtxDecode(dict_file_path, dictBuffer, dictSize) == 0) {
            //decode
            size_t res_zstd = ZSTD_decompress_usingDDict(context.dctx, data_out, data_out_size, data_in, data_in_size, context.d_dict);
            ZSTD_ErrorCode er_code = ZSTD_getErrorCode(res_zstd);

            if (er_code == ZSTD_ErrorCode::ZSTD_error_no_error) {
                *data_out_written = res_zstd;
                res = ZSTDW_STATUS::ZSTDW_OK;
            }
            else {
                const auto req_size = ZSTD_decompressBound(data_in, data_in_size);
                if (req_size == ZSTD_CONTENTSIZE_ERROR) {
                    res = ZSTDW_STATUS::ZSTDW_CONTENT_SIZE_ERROR;
                }
                else {
                    *data_out_written = req_size;
                    if (er_code == ZSTD_ErrorCode::ZSTD_error_dstSize_tooSmall)
                        res = ZSTDW_STATUS::ZSTDW_OUTPUT_BUFFER_TOO_SMALL;
                    else
                        res = (ZSTDW_STATUS)(ZSTDW_STATUS::ZSTDW_ZSTD_ERROR_RANGE_START + er_code);
                }
            }
        }
        else {
            res = ZSTDW_STATUS::ZSTDW_DICT_FILE_READ_ERROR;
        }
    }
    return res;
}
