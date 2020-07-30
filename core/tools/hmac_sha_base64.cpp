#include "stdafx.h"
#include "hmac_sha_base64.h"
#include "openssl/sha.h"

#include <boost/archive/iterators/binary_from_base64.hpp>
#include <boost/archive/iterators/base64_from_binary.hpp>
#include <boost/archive/iterators/transform_width.hpp>
#include <boost/algorithm/string.hpp>

using namespace core;
using namespace tools;

uint8_t *coding=(uint8_t*)"ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";                   //52...63

enum
{
    UNABLE_TO_OPEN_INPUT_FILE,
    UNABLE_TO_OPEN_OUTPUT_FILE,
    UNABLE_TO_CREATE_OUTPUTBUFFER
};

uint8_t base64::lobyte(uint16_t w)
{
    return ((uint8_t)(((uint32_t)(w)) & 0xff));
}

uint8_t base64::hibyte(uint16_t w)
{
    return ((uint8_t)((((uint32_t)(w)) >> 8) & 0xff));
}

uint16_t base64::loword(uint32_t l)
{
    return ((uint16_t)(((uint32_t)(l)) & 0xffff));
}

uint16_t base64::hiword(uint32_t l)
{
    return ((uint16_t)((((uint32_t)(l)) >> 16) & 0xffff));
}

int32_t fchr(uint8_t c)
{
    int32_t i=0;
    for (i=0;i < 64;i++) if (coding[i]==c) return (i);
    return (-1);
}

int32_t base64::base64_decode(uint8_t *source,int32_t length, uint8_t *dst)
{
    int32_t temp =0;
    int32_t i=0;
    int32_t size =0;
    int32_t cursor = 0;

    for (;i<length/4;)
    {
        int32_t zeroes;
        int32_t j;
        cursor = 0;
        zeroes = 0;

        *((uint8_t*)&cursor+3) = source[i*4];
        *((uint8_t*)&cursor+2) = source[i*4+1];
        *((uint8_t*)&cursor+1) = source[i*4+2];
        *((uint8_t*)&cursor) = source[i*4+3];
        for (j =0 ;j <4; j++)
        {
            if (*((uint8_t*)&cursor +j) == '=')
            {
                *((uint8_t*)&cursor +j) =0;
                zeroes++;
                continue;
            }
            temp  = fchr( *((uint8_t*)&cursor +j) );
            temp <<=6*j;
            cursor &= ~(0xFF << j*8);
            cursor ^= temp;
        }
        for (j = 0; j<3; j++)
        {
            dst[i*3+j] = *((uint8_t*)&cursor+2);
            cursor <<=8;
            size++;
        }
        i++;
        size-=zeroes;
    }
    return size;
}

int32_t base64::base64_encode(uint8_t* source, int32_t length, uint8_t* dst)
{
    uint32_t cursor =0x0;
    int32_t i=0,size =0, j =0,ostatok = (length%3);

    for (;i<length - (length%3);)
    {
        *((uint8_t*)&cursor) = source[i+2];
        *((uint8_t*)&cursor+1) = source[i+1];
        *((uint8_t*)&cursor+2) = source[i];
        i+=3;

        //shift
        for (j =0; j<4; j++ )
        {
            uint8_t zna4; // delete after debug
            cursor <<=6;
            zna4 = core::tools::base64::hibyte(core::tools::base64::hiword(cursor));
            dst[size+j] = coding[zna4];
            cursor &= 0x0FFFFFF;
        }
        size +=j;
    }
    if (ostatok)
    {
        cursor =0;
        if (ostatok>1)
        {
            *((uint8_t*)&cursor) = source[i+1];
            *((uint8_t*)&cursor+1) = source[i];
        }
        else
            *((uint8_t*)&cursor) = source[i];

        for (i = 0; i<ostatok+1; i++)
        {
            cursor<<=6;
            if (ostatok == 1)
            {
                dst[size+i] = coding[core::tools::base64::hibyte(core::tools::base64::loword(cursor))];
                cursor &= 0xFF;
            }
            else
            {
                dst[size+i] = coding[core::tools::base64::lobyte(core::tools::base64::hiword(cursor))];
                cursor &= 0xFFFF;
            }

        }
        size +=i;
        for (j=0; j < 4 - i;j++)
        {
            dst[size] = '=';
            size++;
        }
    }
    return (size);
}


std::string base64::hmac_base64(std::vector<uint8_t>& data, std::vector<uint8_t>& secret)
{
    const unsigned SHA_BLOCKSIZE = 64;

    uint8_t* text = &data[0];
    uint32_t textlen = (uint32_t)data.size();
    uint8_t* key = &secret[0];
    uint32_t keylen = (uint32_t)secret.size();

#define MIR_SHA256_HASH_SIZE 32

    unsigned char mdkey[MIR_SHA256_HASH_SIZE];
    unsigned char k_ipad[SHA_BLOCKSIZE], k_opad[SHA_BLOCKSIZE];

    SHA256_CTX ctx;

    uint8_t res[MIR_SHA256_HASH_SIZE];

    if ( keylen > SHA_BLOCKSIZE )
    {
        SHA256_Init( &ctx );
        SHA256_Update( &ctx, key, keylen );
        SHA256_Final( mdkey, &ctx );

        keylen = 32;
        key = mdkey;
    }

    memcpy( k_ipad, key, keylen );
    memcpy( k_opad, key, keylen );
    memset( k_ipad+keylen, 0x36, SHA_BLOCKSIZE - keylen );
    memset( k_opad+keylen, 0x5c, SHA_BLOCKSIZE - keylen );

    for ( uint32_t i = 0; i < keylen; i++ )
    {
        k_ipad[i] ^= 0x36;
        k_opad[i] ^= 0x5c;
    }

    SHA256_Init( &ctx );
    SHA256_Update( &ctx, k_ipad, SHA_BLOCKSIZE );
    SHA256_Update( &ctx, text, (int32_t) textlen );
    SHA256_Final( res, &ctx );

    SHA256_Init( &ctx );
    SHA256_Update( &ctx, k_opad,SHA_BLOCKSIZE );
    SHA256_Update( &ctx, res, (int32_t)MIR_SHA256_HASH_SIZE );
    SHA256_Final( res, &ctx );

    std::vector<uint8_t> temp_buffer(MIR_SHA256_HASH_SIZE*2);
    int32_t encoded = core::tools::base64::base64_encode((uint8_t*) res, MIR_SHA256_HASH_SIZE, &temp_buffer[0]);

    return std::string((char*) &temp_buffer[0], encoded);
}

using namespace boost::archive::iterators;

std::string base64::decode64(const std::string& val)
{
    typedef transform_width<binary_from_base64<std::string::const_iterator>, 8, 6> it;
    return boost::algorithm::trim_right_copy_if(std::string(it(std::begin(val)), it(std::end(val))), [](char c) {
        return c == '\0';
    });
}

std::string base64::encode64(const std::string& val)
{
    typedef base64_from_binary<transform_width<std::string::const_iterator, 6, 8>> it;
    auto tmp = std::string(it(std::begin(val)), it(std::end(val)));
    return tmp.append((3 - val.size() % 3) % 3, '=');
}

void core::tools::sha256(std::string_view _string, char _sha[65])
{
    unsigned char hash[SHA256_DIGEST_LENGTH];

    SHA256_CTX sha256;
    SHA256_Init(&sha256);
    SHA256_Update(&sha256, _string.data(), _string.size());
    SHA256_Final(hash, &sha256);

    for (int i = 0; i < SHA256_DIGEST_LENGTH; ++i)
    {
        sprintf(_sha + (i * 2), "%02x", hash[i]);
    }

    _sha[64] = 0;
}
