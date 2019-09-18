#include "stdafx.h"

#include "mac_certs_cache.h"
#include "curl_tools/curl_ssl_callbacks_mac.h"

using namespace core;

mac_certs_cache::mac_certs_cache()
{

}

mac_certs_cache::~mac_certs_cache()
{
    clear();
}

void mac_certs_cache::invalidate()
{
    boost::unique_lock lock(mutex_);
    valid_ = false;
}

void mac_certs_cache::init_cache()
{
    if (valid_)
        return;

    clear();
    certs_ = get_system_certs();
    valid_ = true;
}

void mac_certs_cache::clear()
{
    for (auto & cert : certs_)
        X509_free(cert);

    certs_.clear();
}
