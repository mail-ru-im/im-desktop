#ifndef __HMAC_SHA_BASE64_H_
#define __HMAC_SHA_BASE64_H_

#pragma once


namespace core
{
    namespace tools
    {
        void sha256(std::string_view _string, char _sha[65]);

        class base64
        {
        public:

            static uint8_t lobyte(uint16_t w);
            static uint8_t hibyte(uint16_t w);
            static uint16_t loword(uint32_t l);
            static uint16_t hiword(uint32_t l);

            static int32_t base64_decode(uint8_t* source, int32_t length, uint8_t* dst);
            static int32_t base64_encode(uint8_t* source, int32_t length, uint8_t* dst);

            static std::string hmac_base64(std::vector<uint8_t>& data, std::vector<uint8_t>& secret);

            static std::string decode64(const std::string& val);
            static std::string encode64(const std::string& val);
        };
    }
}


#endif //__HMAC_SHA_BASE64_H_