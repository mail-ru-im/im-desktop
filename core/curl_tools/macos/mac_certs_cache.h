#pragma once

#include "stdafx.h"

#include <openssl/x509.h>

namespace core
{

class mac_certs_cache
{
public:
    mac_certs_cache();
    ~mac_certs_cache();

    void invalidate();

    template<typename Fn>
    void for_each(Fn && fn)
    {
        mutex_.lock_shared();

        if (!valid_)
        {
            mutex_.unlock_shared();
            mutex_.lock();

            init_cache();

            mutex_.unlock();
            mutex_.lock_shared();
        }

        for (auto & cert : certs_)
            fn(cert);

        mutex_.unlock_shared();
    }

private:
    void init_cache();
    void clear();

    bool valid_ = false;
    std::vector<X509*> certs_;
    boost::shared_mutex mutex_;
};

}
