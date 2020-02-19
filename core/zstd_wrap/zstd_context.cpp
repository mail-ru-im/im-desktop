#include "stdafx.h"
#include "zstd_context.h"
#include <fstream>
#include <iostream>


using namespace std;


int ReadAllBytes(const char * filename, char* &read_buf, int *read_size)
{
    ifstream ifs(filename, ios::binary | ios::ate);
    if (!ifs.is_open())
        return -1;

    ifstream::pos_type pos = ifs.tellg();

    int length = pos;
    read_buf = new char[length];
    ifs.seekg(0, ios::beg);
    ifs.read(read_buf, length);

    *read_size = length;
    return 0;
}


Context::Context() {
    //init ctx (no parallel works allowed)
    cctx = ZSTD_createCCtx();
    //init ctx
    dctx = ZSTD_createDCtx();
};


Context::~Context() {
    if (cctx) {
        ZSTD_freeCCtx(cctx);
        cctx = nullptr;
    }
    if (c_dict) {
        ZSTD_freeCDict(c_dict);
        c_dict = nullptr;
    }

    if (dctx) {
        ZSTD_freeDCtx(dctx);
        dctx = nullptr;
    }
    if (d_dict) {
        ZSTD_freeDDict(d_dict);
        d_dict = nullptr;
    }
};


//Init context for encode.
//If dictSize is 0 then load dictionary from file else from buffer.
int Context::initCtxEncode(string dict_file_path, const char* dictBuffer, int dictSize, int new_compressionLevel) {
    initEncode = false;

    if (c_dict) {
        ZSTD_freeCDict(c_dict);
        c_dict = nullptr;
    }

    //init dict
    int res = 0;
    if (dictSize == 0) {
        char* fileDictBuffer = nullptr;
        res = ReadAllBytes(dict_file_path.c_str(), fileDictBuffer, &dictSize);
        if (res == 0)
            c_dict = ZSTD_createCDict(fileDictBuffer, (size_t)dictSize, cur_compressionLevel);
        delete fileDictBuffer;
    }
    else {
        c_dict = ZSTD_createCDict(dictBuffer, (size_t)dictSize, cur_compressionLevel);
    }

    //save new params
    if (res == 0 && c_dict) {
        initEncode = true;
        cur_compressionLevel = new_compressionLevel;
        encode_dict_file_path = std::move(dict_file_path);
    }
    else {
        res = -1;
    }
    return res;
}


//Init context for decode.
//If dictSize is 0 then load dictionary from file else from buffer.
int Context::initCtxDecode(string dict_file_path, const char* dictBuffer, int dictSize) {
    initDecode = false;

    if (d_dict) {
        ZSTD_freeDDict(d_dict);
        d_dict = nullptr;
    }

    //init dict
    int res = 0;
    if (dictSize == 0) {
        char* fileDictBuffer = nullptr;
        res = ReadAllBytes(dict_file_path.c_str(), fileDictBuffer, &dictSize);
        if (res == 0)
            d_dict = ZSTD_createDDict(fileDictBuffer, (size_t)dictSize);
        delete fileDictBuffer;
    }
    else {
        d_dict = ZSTD_createDDict(dictBuffer, (size_t)dictSize);
    }

    //save new params
    if (res == 0 && d_dict) {
        initDecode = true;
        decode_dict_file_path = std::move(dict_file_path);
    }
    else {
        res = -1;
    }
    return res;
}


//Check by dict_file_path if dict changes or if context not initialize and create context for encoder.
//If dictSize is 0 then load dictionary from file else from buffer.
//Return -1 if file dict read error or 0 if success.
int Context::checkCtxEncode(const char *dict_file_path, const char* dictBuffer, int dictSize, int new_compressionLevel) {
    int res = 0;
    string new_dict(dict_file_path);
    if (new_dict != encode_dict_file_path || initEncode == false || new_compressionLevel!=cur_compressionLevel) {
        res = initCtxEncode(new_dict, dictBuffer, dictSize, new_compressionLevel);
    }
    return res;
}


//Check by dict_file_path if dict changes or if context not initialize and create context for decoder.
//If dictSize is 0 then load dictionary from file else from buffer.
//Return -1 if file dict read error or 0 if success.
int Context::checkCtxDecode(const char *dict_file_path, const char* dictBuffer, int dictSize) {
    int res = 0;
    string new_dict(dict_file_path);
    if (new_dict != decode_dict_file_path || initDecode == false) {
        res = initCtxDecode(new_dict, dictBuffer, dictSize);
    }
    return res;
}
