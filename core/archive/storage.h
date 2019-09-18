#ifndef __ARCHIVE_STORAGE_H_
#define __ARCHIVE_STORAGE_H_

#pragma once

#include "errors.h"

namespace core
{
    namespace archive
    {
        union storage_mode
        {
            struct
            {
                uint32_t read_ : 1;
                uint32_t write_ : 1;
                uint32_t append_ : 1;
                uint32_t truncate_ : 1;

            } flags_;

            uint32_t value_;

            storage_mode()
                : value_(0)
            {
            }
        };

        class storage
        {
            const std::wstring file_name_;

            std::unique_ptr<std::fstream> active_file_stream_;

            archive::error last_error_;

        public:

            struct result_type
            {
                bool result_ = true;
                std::int32_t error_code_ = 0;

                constexpr operator bool() const noexcept { return result_; };
            };

            void clear();

            bool open(storage_mode _mode);
            void close();

            result_type write_data_block(core::tools::binary_stream& _data, int64_t& _offset);
            bool read_data_block(int64_t _offset, core::tools::binary_stream& _data);
            static bool fast_read_data_block(core::tools::binary_stream& buffer, int64_t& current_pos, int64_t& _begin, int64_t _end_position);

            archive::error get_last_error() const { return last_error_; }

            const std::wstring& get_file_name() const { return file_name_; }

            storage(std::wstring _file_name);
            ~storage();
        };

    }
}

#endif //__ARCHIVE_STORAGE_H_