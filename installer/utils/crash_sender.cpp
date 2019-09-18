#include "stdafx.h"

#ifdef _WIN32

#include "../../common.shared/win32/common_crash_sender.h"

#include "../../external/curl/include/curl.h"
#include "../../common.shared/keys.h"
#include "../logic/tools.h"

#include <openssl/ssl.h>
#include <openssl/x509.h>

#include <wincrypt.h>

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

using namespace installer;

namespace common_crash_sender
{
    const std::string& get_hockeyapp_url()
    {
        auto app_id = build::is_agent() ? agent_installer_hockey_app_id : hockey_app_id_installer;
        static std::string hockeyapp_url = "https://rink.hockeyapp.net/api/2/apps/" + app_id + "/crashes/upload";
        return hockeyapp_url;
    }

    void post_dump_to_hockey_app_and_delete()
    {
        auto log_file_path = logic::get_product_folder() + "/reports/crash.log";
        auto dump_file_path = logic::get_product_folder() +"/reports/crashdump.dmp";


        CURL *curl;
        CURLcode res;

        curl_global_init(CURL_GLOBAL_ALL);

        curl = curl_easy_init();
        if(curl)
        {
            struct curl_httppost *formpost=NULL;
            struct curl_httppost *lastptr=NULL;

            auto temp = logic::get_product_folder();

            curl_formadd(&formpost,
                &lastptr,
                CURLFORM_COPYNAME, "log",
                CURLFORM_FILE, log_file_path.toStdString().c_str(),
                CURLFORM_END);

            curl_formadd(&formpost,
                &lastptr,
                CURLFORM_COPYNAME, "attachment0",
                CURLFORM_FILE, dump_file_path.toStdString().c_str(),
                CURLFORM_END);

            curl_easy_setopt(curl, CURLOPT_SSL_CTX_FUNCTION, ssl_ctx_callback_impl);

            curl_easy_setopt(curl, CURLOPT_HTTPPOST, formpost);

            curl_easy_setopt(curl, CURLOPT_URL, get_hockeyapp_url().c_str());

            res = curl_easy_perform(curl);
            if(res != CURLE_OK)
                fprintf(stderr, "curl_easy_perform() failed: %s\n",
                curl_easy_strerror(res));

            curl_easy_cleanup(curl);

            curl_formfree(formpost);
        }
        curl_global_cleanup();

        wchar_t log_path[1024];
        log_file_path.toWCharArray(log_path);
        log_path[log_file_path.length()] = '\0';

        wchar_t dump_path[1024];
        dump_file_path.toWCharArray(dump_path);
        dump_path[dump_file_path.length()] = '\0';

        wchar_t path[1024];
        (logic::get_product_folder() + "/reports/").toWCharArray(path);
        path[(logic::get_product_folder() + "/reports/").length()] = '\0';

        DeleteFile(log_path);
        DeleteFile(dump_path);
        RemoveDirectory(path);
    }

    void start_sending_process()
    {
        PROCESS_INFORMATION pi = {0};
        STARTUPINFO si = {0};
        si.cb = sizeof(si);

        wchar_t arg[100];
        send_dump_arg.toWCharArray(arg);
        arg[send_dump_arg.length()] = '\0';

        wchar_t buffer[1025];
        ::GetModuleFileName(0, buffer, 1024);

        wchar_t command[1024];
        swprintf_s(command, 1023, L"\"%s\" %s", buffer, arg);

        if (::CreateProcess(0, command, 0, 0, 0, 0, 0, 0, &si, &pi))
        {
            ::CloseHandle( pi.hProcess );
            ::CloseHandle( pi.hThread );
        }
    }
}

#endif // _WIN32
