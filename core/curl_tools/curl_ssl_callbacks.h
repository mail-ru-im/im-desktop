#pragma once

#include <curl.h>

namespace core
{
    CURLcode ssl_ctx_callback(CURL *curl, void *ssl_ctx, void *userptr);
}
