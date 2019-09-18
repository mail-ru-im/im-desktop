#pragma once

namespace core
{
    namespace utils
    {
        typedef std::optional<std::string> string_opt;

        std::string_view get_dev_id();
        std::wstring get_product_data_path();
        std::string get_product_name();
        std::string get_app_name();
        std::string get_user_agent(const string_opt &_uin = string_opt());
        std::string get_platform_string();
        std::string get_protocol_platform_string();
        std::string get_client_string();

        std::wstring get_report_path();
        std::wstring get_report_log_path();
        std::wstring get_report_mini_dump_path();
        std::wstring get_themes_path();
        std::wstring get_themes_meta_path();

        boost::filesystem::wpath get_logs_path();
        boost::filesystem::wpath get_app_ini_path();

        bool is_writable(const boost::filesystem::path &p);
        boost::system::error_code remove_dir_contents_recursively(const boost::filesystem::path &_path_to_remove);
        boost::system::error_code remove_dir_contents_recursively2(const boost::filesystem::path& _path_to_remove, const std::unordered_set<std::string>& _skip_files);

        boost::system::error_code remove_user_data_dirs();

        void set_this_thread_name(const std::string_view _name);

        template <typename T>
        std::string to_string_with_precision(const T a_value, const int n = 6);

        size_t get_folder_size(const std::wstring& _path);

        uint64_t get_current_process_ram_usage(); // phys_footprint on macos, "Mem" in Activity Monitor
        uint64_t get_current_process_real_mem_usage(); // resident size on macos, "Real Mem" in Activity Monitor

        uint32_t rand_number();

        std::string calc_local_pin_hash(const std::string& _password, const std::string& _salt);

        namespace aes
        {
            bool encrypt(const std::vector<char>& _source, std::vector<char>& _encrypted, std::string_view _key);

            bool decrypt(const std::vector<char>& _encrypted, std::vector<char>& _decrypted, std::string_view _key);

            int encrypt_impl(const unsigned char *plaintext, int plaintext_len, const unsigned char *key,
                             const unsigned char *iv, unsigned char *ciphertext);

            int decrypt_impl(const unsigned char *ciphertext, int ciphertext_len, const unsigned char *key,
                             const unsigned char *iv, unsigned char *plaintext);
        }
    }
}
