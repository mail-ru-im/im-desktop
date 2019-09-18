#pragma once

#ifdef OMICRON_USE_LIBCURL
#include <curl.h>

namespace omicronlib
{
    CURLcode ssl_ctx_callback_mac(CURL *curl, void *ssl_ctx, void *userptr);
}
#endif