#pragma once

namespace core
{
    namespace tools
    {
        std::string encode_url(std::string_view _url);
        std::string_view extract_protocol_prefix(std::string_view _url);
    }
}
