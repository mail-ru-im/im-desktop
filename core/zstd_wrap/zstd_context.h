#pragma once

#include "zstd.h"
#include <string>


class Context
{
public:
    Context();
    ~Context();

    //Check by dict_file_path if dict changes or if context not initialize and create context for encoder.
    //If dictSize is 0 then load dictionary from file else from buffer.
    //Return -1 if file dict read error or 0 if success.
    int checkCtxEncode(const char *dict_file_path, const char* dictBuffer, int dictSize, int compressionLevel);

    //Check by dict_file_path if dict changes or if context not initialize and create context for decoder.
    //If dictSize is 0 then load dictionary from file else from buffer.
    //Return -1 if file dict read error or 0 if success.
    int checkCtxDecode(const char *dict_file_path, const char* dictBuffer, int dictSize);

    ZSTD_CCtx* cctx = nullptr;
    ZSTD_DCtx* dctx = nullptr;
    ZSTD_CDict* c_dict = nullptr;
    ZSTD_DDict* d_dict = nullptr;

private:
    int cur_compressionLevel = 3;
    bool initEncode = false;
    bool initDecode = false;
    std::string encode_dict_file_path;
    std::string decode_dict_file_path;

    //Init context for encode.
    //If dictSize is 0 then load dictionary from file else from buffer.
    int initCtxEncode(std::string dict_file_path, const char* dictBuffer, int dictSize, int cur_compressionLevel);
    //Init context for decode.
    //If dictSize is 0 then load dictionary from file else from buffer.
    int initCtxDecode(std::string dict_file_path, const char* dictBuffer, int dictSize);
};
