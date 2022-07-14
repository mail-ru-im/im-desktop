#include "stdafx.h"
#include "device_id.h"
#include "hmac_sha_base64.h"
#include "../../common.shared/config/config.h"

std::string core::tools::get_device_id()
{
    char sha_buffer[65] = { 0 };
    core::tools::sha256(impl::get_device_id_impl(), sha_buffer);
    const auto result = std::string_view(sha_buffer);

    const size_t half_size = result.size() / 2;
    const bool has_develop_flag = config::get().has_develop_cli_flag();
    const size_t from = has_develop_flag ? 0 : half_size;
    const size_t to = has_develop_flag ? half_size : std::string_view::npos;

    return std::string(result.substr(from, to));  // take the last part of sha256 returned by get_device_id_impl
}
