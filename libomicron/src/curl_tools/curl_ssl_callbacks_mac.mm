#include "stdafx.h"

#include "curl_ssl_callbacks_mac.h"

#import <Cocoa/Cocoa.h>
#import <Security/SecKeychainItem.h>
#import <Security/CodeSigning.h>
#import <objc/runtime.h>

#include <openssl/ssl.h>
#include <openssl/x509.h>

namespace
{
template <typename T>
class RefDeleter
{
public:
    RefDeleter(T _ref) : ref(_ref) {}
    ~RefDeleter() { if (ref) CFRelease(ref);}

    RefDeleter(const RefDeleter&) = delete;
    RefDeleter(RefDeleter&&) = delete;
    RefDeleter& operator=(const RefDeleter&) = delete;
    RefDeleter& operator=(RefDeleter&&) = delete;

private:
    T ref = {};
};
}

static bool hasTrustedSslServerPolicy(SecPolicyRef policy, CFDictionaryRef props) {
    CFDictionaryRef policyProps = SecPolicyCopyProperties(policy);
    RefDeleter deleter(policyProps);
    // only accept certificates with policies for SSL server validation for now
    if (CFEqual(CFDictionaryGetValue(policyProps, kSecPolicyOid), kSecPolicyAppleSSL))
    {
        CFBooleanRef policyClient;
        if (CFDictionaryGetValueIfPresent(policyProps, kSecPolicyClient, reinterpret_cast<const void**>(&policyClient)) &&
            CFEqual(policyClient, kCFBooleanTrue))
        {
            return false; // no client certs
        }
        if (!CFDictionaryContainsKey(props, kSecTrustSettingsResult))
        {
            // as per the docs, no trust settings result implies full trust
            return true;
        }
        CFNumberRef number = static_cast<CFNumberRef>(CFDictionaryGetValue(props, kSecTrustSettingsResult));
        SecTrustSettingsResult settingsResult;
        CFNumberGetValue(number, kCFNumberSInt32Type, &settingsResult);
        switch (settingsResult)
        {
        case kSecTrustSettingsResultTrustRoot:
        case kSecTrustSettingsResultTrustAsRoot:
            return true;
        default:
            return false;
        }
    }
    return false;
}

static bool isCaCertificateTrusted(SecCertificateRef cfCert, int domain)
{
    CFArrayRef cfTrustSettings = nullptr;
    OSStatus status = SecTrustSettingsCopyTrustSettings(cfCert, SecTrustSettingsDomain(domain), &cfTrustSettings);
    RefDeleter deleter(cfTrustSettings);
    if (status == noErr) {
        CFIndex size = CFArrayGetCount(cfTrustSettings);
        // if empty, trust for everything (as per the Security Framework documentation)
        if (size == 0)
        {
            return true;
        }
        else
        {
            for (CFIndex i = 0; i < size; ++i)
            {
                CFDictionaryRef props = static_cast<CFDictionaryRef>(CFArrayGetValueAtIndex(cfTrustSettings, i));
                if (CFDictionaryContainsKey(props, kSecTrustSettingsPolicy))
                {
                    if (hasTrustedSslServerPolicy((SecPolicyRef)CFDictionaryGetValue(props, kSecTrustSettingsPolicy), props))
                        return true;
                }
            }
        }
    }
    return false;
}

namespace omicronlib
{
    CURLcode ssl_ctx_callback_mac(CURL *curl, void *ssl_ctx, void *userptr)
    {
        auto ctx = static_cast<SSL_CTX*>(ssl_ctx);
        auto store = SSL_CTX_get_cert_store(ctx);
        if (!store)
        {
            store = X509_STORE_new();
            SSL_CTX_set_cert_store(ctx, store);
        }

        for (int dom = kSecTrustSettingsDomainUser; dom <= int(kSecTrustSettingsDomainSystem); ++dom)
        {
            CFArrayRef cfCerts = nullptr;
            OSStatus status = SecTrustSettingsCopyCertificates(SecTrustSettingsDomain(dom), &cfCerts);
            RefDeleter deleter(cfCerts);
            if (status == noErr)
            {
                const CFIndex size = CFArrayGetCount(cfCerts);
                for (CFIndex i = 0; i < size; ++i)
                {
                    SecCertificateRef cfCert = (SecCertificateRef)CFArrayGetValueAtIndex(cfCerts, i);
                    CFDataRef derData = SecCertificateCopyData(cfCert);
                    RefDeleter dataDeleter(derData);
                    if (isCaCertificateTrusted(cfCert, dom))
                    {
                        if (derData != NULL)
                        {
                            const unsigned char* pData = CFDataGetBytePtr(derData);
                            auto x509_cert = d2i_X509(nullptr, &pData, CFDataGetLength(derData));
                            if (x509_cert)
                            {
                                X509_STORE_add_cert(store, x509_cert);
                                X509_free(x509_cert);
                            }
                        }
                    }
                }
            }
        }
        return CURLE_OK;
    }
}
