#include "stdafx.h"
#include "md5.h"
#include <openssl/md5.h>

namespace core
{
    namespace tools
    {
        template<size_t N>
        std::string digest_string(unsigned char (&_digest)[N])
        {
            char format_buffer[4];
            char result[N * 2 + 1] = { 0 };

            char* ptr = result;
            for (size_t i = 0; i < N; ++i)
            {
#ifdef _WIN32
                const size_t n = sprintf_s(format_buffer, "%02x", _digest[i]);
#else
                const size_t n = sprintf(format_buffer, "%02x", _digest[i]);
#endif
                ptr = std::copy_n(format_buffer, n, ptr);
            }

            return result;
        }

        std::string md5(const void* _data, size_t _size)
        {
            MD5_CTX md5handler;
            unsigned char md5digest[MD5_DIGEST_LENGTH];

            MD5_Init(&md5handler);
            MD5_Update(&md5handler, _data, _size);
            MD5_Final(md5digest,&md5handler);

            return digest_string(md5digest);
        }
        std::string md5(std::istream& _input)
        {
            if (!_input)
                return {};

            char buffer[4096];
            unsigned char md5digest[MD5_DIGEST_LENGTH];

            MD5_CTX md5handler;
            MD5_Init(&md5handler);

            while (_input.good())
            {
                _input.read(buffer, std::size(buffer));
                MD5_Update(&md5handler, buffer, _input.gcount());
            }

            MD5_Final(md5digest, &md5handler);

            return digest_string(md5digest);
        }
    }
}


