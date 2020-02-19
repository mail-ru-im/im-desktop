#pragma once

#include "../../external/curl/include/curl.h"

namespace curl_utils
{
    CURLcode ssl_ctx_callback_impl(CURL*, void* ssl_ctx, void*);
}