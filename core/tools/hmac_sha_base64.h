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

            [[nodiscard]] static uint8_t lobyte(uint16_t w);
            [[nodiscard]] static uint8_t hibyte(uint16_t w);
            [[nodiscard]] static uint16_t loword(uint32_t l);
            [[nodiscard]] static uint16_t hiword(uint32_t l);

            static int32_t base64_decode(uint8_t* source, int32_t length, uint8_t* dst);
            [[nodiscard]]static int32_t base64_encode(uint8_t* source, int32_t length, uint8_t* dst);

            [[nodiscard]] static std::string decode64(std::string_view val);
            [[nodiscard]] static std::string encode64(std::string_view val);
        };
    }
}


#endif //__HMAC_SHA_BASE64_H_