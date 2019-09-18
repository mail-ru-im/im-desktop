#include "stdafx.h"
#include "utils.h"

#include "tools/system.h"
#include "tools/hmac_sha_base64.h"
#include "../common.shared/version_info.h"
#include "../common.shared/common_defs.h"

#include "openssl/rand.h"
#include "openssl/evp.h"
#include "openssl/aes.h"


#if defined(__APPLE__)
#include<mach/mach.h>
#elif defined(__linux__)
#define STATM_PATH "/proc/self/statm"
typedef struct
{
    unsigned long size, resident, share, text, lib, data, dt;
} statm_t;
#endif

namespace core
{
    namespace utils
    {
        std::string_view get_dev_id()
        {
            if (platform::is_apple())
            {
                return std::string_view(build::get_product_variant("ic18eTwFBO7vAdt9","ic1gBBFr7Ir9EOA2","ic1iPK_NRh952CO-", "on2fah4R-mac"));
            }
            else if (platform::is_windows())
            {
                return std::string_view(build::get_product_variant("ic1nmMjqg7Yu-0hL", "ic1ReaqsoxgOBHFX", "ic1nQK338I3UIa_q", "on2fah4R-win"));
            }
            else
            {
                return std::string_view(build::get_product_variant("ic1fdj2RfSjLxBAs", "ic1pz70eh6TmSyL6", "ic11odzi_v6LYcz6", "on2fah4R-linux"));
            }
        }

        std::wstring get_product_data_path()
        {
            const wchar_t* product_path = build::get_product_variant(product_path_icq_w, (platform::is_apple() ? product_path_agent_mac_w : product_path_agent_w), product_path_biz_w, product_path_dit_w);

#ifdef __linux__
            return (core::tools::system::get_user_profile() + L"/.config/" + product_path);
#elif _WIN32
            return ::common::get_user_profile() + L'/' + product_path;
#else
            return (core::tools::system::get_user_profile() + L'/' + product_path);
#endif //__linux__
        }

        std::string get_product_name()
        {
            return "icq.desktop";
        }

        std::string get_app_name()
        {
            if constexpr (platform::is_windows())
            {
                return build::get_product_variant("Mail.ru Windows ICQ", "Mail.ru Windows Agent", "Myteam Windows", "Messenger Windows");
            }
            else if constexpr (platform::is_apple())
            {
                return build::get_product_variant("Mail.ru Mac OS X ICQ", "Mail.ru Mac OS X Agent", "Myteam OS X", "Messenger OS X");
            }
            else if constexpr (platform::is_linux())
            {
                return build::get_product_variant("Mail.ru Linux ICQ", "Mail.ru Linux Agent", "Myteam Linux", "Messenger Linux");
            }
            else
            {
                static_assert("unsupported platform");
                return build::get_product_variant("Mail.ru ICQ", "Mail.ru Agent", "Myteam", "Messenger");
            }
        }

        std::string_view get_ua_app_name()
        {
            return build::get_product_variant("ICQ", "Agent", "myteam", "Messenger");
        }

        constexpr std::string_view get_platform_name() noexcept
        {
            return std::string_view("Desktop");
        }

        constexpr std::string_view get_no_user_id() noexcept
        {
            return std::string_view("#no_user_id#");
        }

        constexpr std::string_view get_device_type() noexcept
        {
            return std::string_view("PC");
        }

        std::string get_ua_os_version()
        {
            auto version_string = tools::system::get_os_version_string();

            std::replace(std::begin(version_string), std::end(version_string), ' ', '_');

            return version_string;
        }

        std::string get_user_agent(const string_opt &_uin)
        {
            std::stringstream ss_ua;

            ss_ua << get_ua_app_name() << ' ' << get_platform_name() << ' ' << (_uin ? (*_uin) : get_no_user_id());
            ss_ua << ' ' << get_dev_id();
            ss_ua << ' ' << tools::version_info().get_ua_version();

            static const auto os_version = get_ua_os_version();
            assert(!os_version.empty());

            ss_ua << ' ' << os_version;
            ss_ua << ' ' << get_device_type();

            return ss_ua.str();
        }

        std::string get_platform_string()
        {
            if constexpr (platform::is_windows())
                return "Win";
            else if constexpr (platform::is_apple())
                return "Apple";
            else if constexpr (platform::is_linux())
                return "Linux";
            else
                return "Unknown";
        }

        std::string get_protocol_platform_string()
        {
            if constexpr (platform::is_apple())
                return "mac";
            else if constexpr (platform::is_windows())
                return "windows";
            else if constexpr (platform::is_linux())
                return "linux";

            return "unknown";
        }

        std::string get_client_string()
        {
            return build::get_product_variant("icq", "agent", "myteam", "dit");
        }

        std::wstring get_report_path()
        {
            return core::utils::get_product_data_path() + L"/reports";
        }

        std::wstring get_report_log_path()
        {
            return get_report_path() + L"/crash.log";
        }

        std::wstring get_report_mini_dump_path()
        {
            return get_report_path() + L"/crashdump.dmp";
        }

        std::wstring get_themes_path()
        {
            return (utils::get_product_data_path() + L"/themes");
        }

        std::wstring get_themes_meta_path()
        {
            return get_themes_path() + L"/themes.json";
        }

        boost::filesystem::wpath get_logs_path()
        {
            boost::filesystem::wpath logs_dir(get_product_data_path());

            logs_dir.append(L"logs");

            return logs_dir;
        }

        boost::filesystem::wpath get_app_ini_path()
        {
            boost::system::error_code error_code;
            auto product_data_root = boost::filesystem::canonical(utils::get_product_data_path(), Out error_code);

            return product_data_root / L"app.ini";
        }

        bool is_writable(const boost::filesystem::path &p)
        {
            std::ofstream of(p.string());
            return of.good();
        }

        boost::system::error_code remove_dir_contents_recursively(const boost::filesystem::path &_path_to_remove)
        {
            boost::system::error_code error;

            try
            {
                for (boost::filesystem::directory_iterator end_dir_it, it(_path_to_remove); it != end_dir_it && !error; it.increment(error))
                {
                    boost::system::error_code ec;
                    boost::filesystem::remove_all(it->path(), ec);
                    if (ec)
                        error = ec;
                }
            }
            catch (const std::exception&)
            {

            }

            return error;
        }

        boost::system::error_code remove_dir_contents_recursively2(const boost::filesystem::path& _path_to_remove, const std::unordered_set<std::string>& _skip_files)
        {
            boost::system::error_code error;

            for (boost::filesystem::directory_iterator end_dir_it, it(_path_to_remove); it != end_dir_it && !error; it.increment(error))
            {
                if (_skip_files.find(it->path().filename().string()) != _skip_files.end())
                    continue;

                boost::system::error_code ec;
                boost::filesystem::remove_all(it->path(), ec);
                if (ec)
                    error = ec;
            }

            return error;
        }

        boost::system::error_code remove_user_data_dirs()
        {
            boost::system::error_code error;

            for (boost::filesystem::directory_iterator end_dir_it, it(get_product_data_path()); it != end_dir_it && !error; it.increment(error))
            {
                const auto path = it->path();
                boost::system::error_code ec;
                if (boost::filesystem::is_directory(path, ec))
                {
                    const auto dir_name = path.filename().string();
                    if (std::all_of(dir_name.begin(), dir_name.end(), ::isdigit))
                        boost::filesystem::remove_all(path, ec);
                }

                if (ec)
                    error = ec;
            }

            return error;
        }

#ifdef _WIN32 // https://docs.microsoft.com/en-us/previous-versions/visualstudio/visual-studio-2008/xcb2z8hs(v=vs.90)
        void SetThreadName(DWORD dwThreadID, const char* threadName)
        {
#ifndef __MINGW32__
            constexpr DWORD MS_VC_EXCEPTION = 0x406D1388;

            #pragma pack(push,8)
            typedef struct tagTHREADNAME_INFO
            {
                DWORD dwType; // Must be 0x1000.
                LPCSTR szName; // Pointer to name (in user addr space).
                DWORD dwThreadID; // Thread ID (-1=caller thread).
                DWORD dwFlags; // Reserved for future use, must be zero.
            } THREADNAME_INFO;
            #pragma pack(pop)

            if (IsDebuggerPresent())
            {
                THREADNAME_INFO info;
                info.dwType = 0x1000;
                info.szName = threadName;
                info.dwThreadID = dwThreadID;
                info.dwFlags = 0;

                __try
                {
                    RaiseException(MS_VC_EXCEPTION, 0, sizeof(info) / sizeof(ULONG_PTR), (ULONG_PTR*)&info);
                }
                __except (EXCEPTION_EXECUTE_HANDLER)
                {
                }
            }
#endif
        }
#endif

        void set_this_thread_name(const std::string_view _name)
        {
#ifndef _WIN32
            constexpr auto NAME_MAX_LEN = 16;

            char tname[NAME_MAX_LEN];
            snprintf(tname, NAME_MAX_LEN, "%s", std::string(_name).c_str());
            tname[NAME_MAX_LEN - 1] = 0;

        #ifdef __linux__
            pthread_setname_np(pthread_self(), tname);
        #else
            pthread_setname_np(tname);
        #endif
#else
            SetThreadName(GetCurrentThreadId(), std::string(_name).c_str());
#endif
        }

        uint64_t get_current_process_ram_usage()
        {
#if defined(_WIN32)
            PROCESS_MEMORY_COUNTERS_EX pmc;
            ZeroMemory(&pmc, sizeof(PROCESS_MEMORY_COUNTERS_EX));
            GetProcessMemoryInfo(GetCurrentProcess(), (PROCESS_MEMORY_COUNTERS *)&pmc, sizeof(pmc));
            return static_cast<uint64_t>(pmc.PrivateUsage);
#elif defined(__APPLE__)
#if defined(TASK_VM_INFO) && TASK_VM_INFO >= 22
            task_vm_info vm_info = { 0, 0, 0 };
            mach_msg_type_number_t count = sizeof(vm_info) / sizeof(int);
            int err = task_info(mach_task_self(), TASK_VM_INFO, (int *)&vm_info, &count);
            UNUSED_ARG(err);
            return (uint64_t)(vm_info.internal + vm_info.compressed);
//            return (uint64_t)vm_info.phys_footprint;
#else
            return get_current_process_real_mem_usage();
#endif // TASK_VM_INFO
#else
            FILE *f = fopen(STATM_PATH, "r");
            if (!f)
                return 0;

            tools::auto_scope as([f]
            {
                fclose(f);
            });

            statm_t stat = {};
            if (7 != fscanf(f,"%ld %ld %ld %ld %ld %ld %ld", &stat.size, &stat.resident, &stat.share, &stat.text, &stat.lib, &stat.data, &stat.dt))
            {
                return 0;
            }

            return  stat.size * KILOBYTE;
#endif
        }

        uint64_t get_current_process_real_mem_usage()
        {
#if defined(_WIN32)
            PROCESS_MEMORY_COUNTERS_EX pmc;
            ZeroMemory(&pmc, sizeof(PROCESS_MEMORY_COUNTERS_EX));
            GetProcessMemoryInfo(GetCurrentProcess(), (PROCESS_MEMORY_COUNTERS *)&pmc, sizeof(pmc));
            return static_cast<uint64_t>(pmc.WorkingSetSize);
#elif defined(__APPLE__)
            struct task_basic_info t_info;
            mach_msg_type_number_t t_info_count = TASK_BASIC_INFO_COUNT;

            if (KERN_SUCCESS != task_info(mach_task_self(),
                                          TASK_BASIC_INFO,
                                          reinterpret_cast<task_info_t>(&t_info),
                                          &t_info_count))
            {
                return 0;
            }

            return t_info.resident_size;
#else
            return get_current_process_ram_usage();
#endif
        }

        template <typename T>
        std::string to_string_with_precision(const T a_value, const int n)
        {
            std::ostringstream out;
            out << std::setprecision(n) << a_value;
            return out.str();
        }

        std::string format_file_size(const int64_t size)
        {
            assert(size >= 0);

            const auto KiB = 1024;
            const auto MiB = 1024 * KiB;
            const auto GiB = 1024 * MiB;

            if (size >= GiB)
            {
                const auto gibSize = ((double)size / (double)GiB);
                return to_string_with_precision(gibSize, 1) + " GB";
            }

            if (size >= MiB)
            {
                const auto mibSize = ((double)size / (double)MiB);
                return to_string_with_precision(mibSize, 1) + " MB";
            }

            if (size >= KiB)
            {
                const auto kibSize = ((double)size / (double)KiB);
                return to_string_with_precision(kibSize, 1) + " MB";
            }

            return to_string_with_precision(size, 1) + " B";
        }

        size_t get_folder_size(const std::wstring& _path)
        {
            size_t result = 0;

            try
            {
                boost::filesystem::path path(_path);

                boost::system::error_code ec;
                if (!boost::filesystem::exists(path, ec))
                    return result;

                if (!boost::filesystem::is_directory(path, ec))
                    return result;

                for (boost::filesystem::recursive_directory_iterator it(path, ec);
                    it != boost::filesystem::recursive_directory_iterator() && !ec;
                    it.increment(ec))
                {
                    if (boost::filesystem::is_directory(*it, ec))
                        continue;

                    auto size = boost::filesystem::file_size(*it, ec);
                    if (ec.value() == boost::system::errc::success)
                        result += size_t(size);
                }
            }
            catch (const std::exception&)
            {

            }

            return result;
        }

        uint32_t rand_number()
        {
            uint32_t result;
            RAND_bytes((unsigned char *)&result, sizeof(result));

            return result;
        }

        std::string calc_local_pin_hash(const std::string& _password, const std::string& _salt)
        {
            const auto salted_password = _salt + _password;
            char buffer[65] = { 0 };
            core::tools::sha256(salted_password, buffer);

            return std::string(buffer);
        }

        namespace aes
        {
            bool encrypt(const std::vector<char>& _source, std::vector<char>& _encrypted, std::string_view _key)
            {
                _encrypted.clear();
                _encrypted.resize(_source.size() + AES_BLOCK_SIZE, 0);

                char iv_buffer[65] = { 0 };
                core::tools::sha256(_key, iv_buffer); // generate iv from key

                // since iv size is 16 byte, take the last half of sha256 (iv_buffer[32])
                auto res = encrypt_impl((const unsigned char*)&_source[0], _source.size(),
                        (const unsigned char*)_key.data(), (const unsigned char*)&iv_buffer[32], (unsigned char*)&_encrypted[0]);

                _encrypted.resize(res);

                return res > 0;
            }

            bool decrypt(const std::vector<char>& _encrypted, std::vector<char>& _decrypted, std::string_view _key)
            {
                _decrypted.clear();
                _decrypted.resize(_encrypted.size() + AES_BLOCK_SIZE, 0);

                char iv_buffer[65] = { 0 };
                core::tools::sha256(_key, iv_buffer);  // generate iv from key

                // since iv size is 16 byte, take the last half of sha256 (iv_buffer[32])
                auto res = decrypt_impl((const unsigned char*)&_encrypted[0], _encrypted.size(),
                        (const unsigned char*)_key.data(), (const unsigned char*)&iv_buffer[32], (unsigned char*)&_decrypted[0]);

                _decrypted.resize(res);

                return res > 0;
            }

            int encrypt_impl(const unsigned char *plaintext, int plaintext_len, const unsigned char *key,
                             const unsigned char *iv, unsigned char *ciphertext)
            {
                EVP_CIPHER_CTX *ctx;

                int len;

                int ciphertext_len;

                /* Create and initialise the context */
                if(!(ctx = EVP_CIPHER_CTX_new()))
                    return 0;

                /* Initialise the encryption operation. IMPORTANT - ensure you use a key
               * and IV size appropriate for your cipher
               * In this example we are using 256 bit AES (i.e. a 256 bit key). The
               * IV size for *most* modes is the same as the block size. For AES this
               * is 128 bits */
                if(1 != EVP_EncryptInit_ex(ctx, EVP_aes_256_cbc(), NULL, key, iv))
                    return 0;

                /* Provide the message to be encrypted, and obtain the encrypted output.
               * EVP_EncryptUpdate can be called multiple times if necessary
               */
                if(1 != EVP_EncryptUpdate(ctx, ciphertext, &len, plaintext, plaintext_len))
                    return 0;
                ciphertext_len = len;

                /* Finalise the encryption. Further ciphertext bytes may be written at
               * this stage.
               */
                if(1 != EVP_EncryptFinal_ex(ctx, ciphertext + len, &len))
                    return 0;
                ciphertext_len += len;

                /* Clean up */
                EVP_CIPHER_CTX_free(ctx);

                return ciphertext_len;
            }

            int decrypt_impl(const unsigned char *ciphertext, int ciphertext_len, const unsigned char *key,
                             const unsigned char *iv, unsigned char *plaintext)
            {
                EVP_CIPHER_CTX *ctx;

                int len;

                int plaintext_len;

                /* Create and initialise the context */
                if(!(ctx = EVP_CIPHER_CTX_new()))
                    return 0;

                /* Initialise the decryption operation. IMPORTANT - ensure you use a key
               * and IV size appropriate for your cipher
               * In this example we are using 256 bit AES (i.e. a 256 bit key). The
               * IV size for *most* modes is the same as the block size. For AES this
               * is 128 bits */
                if(1 != EVP_DecryptInit_ex(ctx, EVP_aes_256_cbc(), NULL, key, iv))
                    return 0;

                /* Provide the message to be decrypted, and obtain the plaintext output.
               * EVP_DecryptUpdate can be called multiple times if necessary
               */
                if(1 != EVP_DecryptUpdate(ctx, plaintext, &len, ciphertext, ciphertext_len))
                    return 0;
                plaintext_len = len;

                /* Finalise the decryption. Further plaintext bytes may be written at
               * this stage.
               */
                if(1 != EVP_DecryptFinal_ex(ctx, plaintext + len, &len))
                    return 0;
                plaintext_len += len;

                /* Clean up */
                EVP_CIPHER_CTX_free(ctx);

                return plaintext_len;
            }

        } // aes
    }
}
