#include "stdafx.h"

#ifdef OMICRON_USE_LIBCURL
#include "curl_ssl_callbacks.h"

#include <openssl/ssl.h>
#include <openssl/x509.h>

#ifdef _WIN32
#include <wincrypt.h>
#endif

#ifdef __APPLE__
#include "curl_ssl_callbacks_mac.h"
#endif


#ifdef _WIN32
static int X509_ñallback(int ok, X509_STORE_CTX *ctx)
{
    if (!ok)
    {
        if (X509_STORE_CTX_get_error(ctx) == X509_V_ERR_UNABLE_TO_GET_ISSUER_CERT_LOCALLY)
        {
            if (auto x509 = X509_STORE_CTX_get_current_cert(ctx))
            {
                unsigned char *buf = nullptr;
                const int len = i2d_X509(x509, &buf);
                if (len > 0)
                {
                    const std::vector<unsigned char> cert_data(buf, buf + len);
                    OPENSSL_free(buf);
                    PCCERT_CONTEXT win_cert = CertCreateCertificateContext(X509_ASN_ENCODING, (const BYTE *)cert_data.data(), cert_data.size());
                    if (win_cert)
                    {
                        CERT_CHAIN_PARA parameters;
                        memset(&parameters, 0, sizeof(parameters));
                        parameters.cbSize = sizeof(parameters);
                        // set key usage constraint
                        parameters.RequestedUsage.dwType = USAGE_MATCH_TYPE_AND;
                        parameters.RequestedUsage.Usage.cUsageIdentifier = 1;
                        LPSTR oid = (LPSTR)(szOID_PKIX_KP_SERVER_AUTH);
                        parameters.RequestedUsage.Usage.rgpszUsageIdentifier = &oid;

                        PCCERT_CHAIN_CONTEXT chain;
                        BOOL result = CertGetCertificateChain(
                            nullptr, //default engine
                            win_cert,
                            nullptr, //current date/time
                            nullptr, //default store
                            &parameters,
                            0, //default dwFlags
                            nullptr, //reserved
                            &chain);

                        if (result)
                        {
                            //based on http://msdn.microsoft.com/en-us/library/windows/desktop/aa377182%28v=vs.85%29.aspx
                            //about the final chain rgpChain[cChain-1] which must begin with a trusted root to be valid
                            if (chain->TrustStatus.dwErrorStatus == CERT_TRUST_NO_ERROR && chain->cChain > 0)
                            {
                                const PCERT_SIMPLE_CHAIN final_chain = chain->rgpChain[chain->cChain - 1];
                                // http://msdn.microsoft.com/en-us/library/windows/desktop/aa377544%28v=vs.85%29.aspx
                                // rgpElement[0] is the end certificate chain element. rgpElement[cElement-1] is the self-signed "root" certificate element.
                                if (final_chain->TrustStatus.dwErrorStatus == CERT_TRUST_NO_ERROR && final_chain->cElement > 0)
                                {
                                    // x509 - here is valid root cert. Due to curl close connection after X509_V_ERR_UNABLE_TO_GET_ISSUER_CERT_LOCALLY we do nothing here.
                                    /*auto x509 = d2i_X509(nullptr, (const unsigned char **)&(final_chain->rgpElement[final_chain->cElement - 1]->pCertContext->pbCertEncoded),
                                        final_chain->rgpElement[final_chain->cElement - 1]->pCertContext->cbCertEncoded);
                                    if (x509)
                                    {
                                    }*/
                                }
                            }
                            CertFreeCertificateChain(chain);
                        }
                        CertFreeCertificateContext(win_cert);
                    }
                }
            }
        }
    }
    return 1;
}


static CURLcode ssl_ctx_callback_impl(CURL *curl, void *ssl_ctx, void *userptr)
{
    auto ctx = static_cast<SSL_CTX*>(ssl_ctx);
    auto store = SSL_CTX_get_cert_store(ctx);
    if (!store)
    {
        store = X509_STORE_new();
        SSL_CTX_set_cert_store(ctx, store);
    }

    if (auto h_system_store = CertOpenSystemStore(0, L"ROOT"))
    {
        PCCERT_CONTEXT pc = nullptr;
        while (1)
        {
            pc = CertFindCertificateInStore(h_system_store, X509_ASN_ENCODING, 0, CERT_FIND_ANY, nullptr, pc);
            if (!pc)
                break;

            if (CertVerifyTimeValidity(NULL, pc->pCertInfo) != 0)
                continue;

            auto x509 = d2i_X509(nullptr, (const unsigned char **)&pc->pbCertEncoded, pc->cbCertEncoded);
            if (x509)
            {
                X509_STORE_add_cert(store, x509);
                X509_free(x509);
            }
        }
        CertCloseStore(h_system_store, 0);
    }

    X509_STORE_set_verify_cb(store, X509_ñallback);

    return CURLE_OK;
}
#elif __linux__
static CURLcode ssl_ctx_callback_impl(CURL *curl, void *ssl_ctx, void *userptr)
{
    auto ctx = static_cast<SSL_CTX*>(ssl_ctx);
    SSL_CTX_load_verify_locations(ctx, "/etc/ssl/certs/ca-certificates.crt", nullptr);                 // Debian/Ubuntu/Gentoo etc.
    SSL_CTX_load_verify_locations(ctx, "/etc/pki/tls/certs/ca-bundle.crt", nullptr);                   // Fedora/RHEL 6
    SSL_CTX_load_verify_locations(ctx, "/etc/ssl/ca-bundle.pem", nullptr);                             // OpenSUSE
    SSL_CTX_load_verify_locations(ctx, "/etc/pki/tls/cacert.pem", nullptr);                            // OpenELEC
    SSL_CTX_load_verify_locations(ctx, "/etc/pki/ca-trust/extracted/pem/tls-ca-bundle.pem", nullptr);  // CentOS/RHEL 7

    return CURLE_OK;
}
#elif __APPLE__
static CURLcode ssl_ctx_callback_impl(CURL *curl, void *ssl_ctx, void *userptr)
{
    return omicronlib::ssl_ctx_callback_mac(curl, ssl_ctx, userptr);
}
#endif

namespace omicronlib
{
    CURLcode ssl_ctx_callback(CURL *curl, void *ssl_ctx, void *userptr)
    {
        return ssl_ctx_callback_impl(curl, ssl_ctx, userptr);
    }
}
#endif // OMICRON_USE_LIBCURL
