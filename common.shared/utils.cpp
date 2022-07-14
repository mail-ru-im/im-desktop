#include "stdafx.h"

#include "utils.h"
#include "string_utils.h"
#include "common_defs.h"

#include "config/config.h"
#include "../core/tools/strings.h"
#include "../core/tools/system.h"

namespace core
{
    namespace utils
    {
        std::wstring get_product_data_path(std::wstring_view _relative)
        {
#ifdef __linux__
            return su::wconcat(core::tools::system::get_user_profile(), L"/.config/", _relative);
#elif _WIN32
            return su::wconcat(::common::get_user_profile(), L'/', _relative);
#else
            return su::wconcat(core::tools::system::get_user_profile(), L'/', _relative);
#endif //__linux__
        }

        std::wstring get_local_product_data_path(std::wstring_view _relative)
        {
#ifdef _WIN32
            return su::wconcat(::common::get_local_user_profile(), L'/', _relative);
#else
            return get_product_data_path(_relative);
#endif
        }

        std::wstring get_product_data_path_impl(std::wstring (*_path_function)(std::wstring_view))
        {
            constexpr auto v = platform::is_apple() ? config::values::product_path_mac : config::values::product_path;
            const auto relative_path = config::get().string(v);
            if (config::get().has_develop_cli_flag())
                return _path_function(tools::from_utf8(std::string { su::concat(relative_path, " develop") }));
            return _path_function(tools::from_utf8(relative_path));
        }

        std::wstring get_product_data_path()
        {
            return get_product_data_path_impl(&get_product_data_path);
        }

        std::wstring get_local_product_data_path()
        {
            return get_product_data_path_impl(&get_local_product_data_path);
        }
    } // namespace utils
} // namespace core
