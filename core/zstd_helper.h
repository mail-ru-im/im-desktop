#pragma once

#include "tools/spin_lock.h"

namespace core
{
    class im_container;

    class zstd_helper : public std::enable_shared_from_this<zstd_helper>
    {
    public:
        enum class dict_location
        {
            internal,
            external
        };

        struct dict_info
        {
            dict_location location_ = dict_location::internal;
            std::string path_;
            const char* data_ = nullptr;
            size_t size_ = 0;

            std::string name_;
            std::string timestamp_;

            dict_info() = default;
            dict_info(dict_location _location, std::string_view _path, const char* _data = nullptr, size_t _size = 0);
        };

        zstd_helper(std::wstring_view _dirname);
        ~zstd_helper();

        void start_auto_updater(std::weak_ptr<im_container> _im_cont);
        void stop_auto_updater();

        bool add_dict(dict_info _info);
        void reinit_dicts(std::string_view _fault_dict_name = {});
        bool is_dict_exist(std::string_view _dict_name) const;

        std::string get_last_request_dict() const;
        std::string get_last_response_dict() const;

        int compress(const char* _data_in, size_t _data_in_size, char* _data_out, size_t _data_out_size, size_t* _data_size_written, std::string_view _dict_name, int _compress_level = -1) const;
        int decompress(const char* _data_in, size_t _data_in_size, char* _data_out, size_t _data_out_size, size_t* _data_size_written, std::string_view _dict_name) const;

    private:
        enum class dict_type
        {
            request,
            response
        };

        using dicts_list = std::vector<dict_info>;
        using dicts_list_sptr = std::shared_ptr<dicts_list>;

        void init_dicts();
        void update_dicts();

        std::optional<dict_info> get_dict_info(std::string_view _dict_name) const;

        bool add_dict_common(dict_type _type, dict_info _info);

        std::string get_dicts_list() const;

        dicts_list_sptr get_request_dicts() const;
        dicts_list_sptr get_response_dicts() const;

        std::wstring dicts_dirname_;

        dicts_list_sptr request_dicts_;
        dicts_list_sptr response_dicts_;
        mutable tools::spin_lock request_dicts_lock_;
        mutable tools::spin_lock response_dicts_lock_;

        uint32_t timer_id_;

        std::weak_ptr<im_container> im_cont_;

        std::atomic_bool is_reinited_;
    };
}
