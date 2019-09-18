#pragma once

#include <curl.h>
#include <openssl/x509.h>

namespace core
{
    CURLcode ssl_ctx_callback_mac(CURL *curl, void *ssl_ctx, void *userptr);
    std::vector<X509*> get_system_certs();
}
