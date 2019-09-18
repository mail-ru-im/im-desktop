#include "stdafx.h"
#include "device_id.h"
#include "hmac_sha_base64.h"

std::string core::tools::get_device_id()
{
    char sha_buffer[65] = { 0 };
    core::tools::sha256(impl::get_device_id_impl(), sha_buffer);
    const auto result = std::string(sha_buffer);

    return result.substr(result.length() / 2);  // take the last part of sha256 returned by get_device_id_impl
}
