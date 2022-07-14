#pragma once

namespace config
{
    class configuration;
}

namespace core
{
    namespace utils
    {
        std::wstring get_product_data_path();
        std::wstring get_local_product_data_path();
    } // namespace utils
} // namespace core
