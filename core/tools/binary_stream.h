#ifndef __BINARYSTREAM_H_
#define __BINARYSTREAM_H_

#pragma once

#include <fstream>

#include "scope.h"

#include "zlib.h"

namespace core
{
    namespace tools
    {
        class stream
        {
        public:
            virtual ~stream() = default;

            virtual void write(const char* _data, int64_t _size) = 0;

            virtual int64_t all_size() const noexcept = 0;
            virtual int64_t available() const noexcept = 0;

            virtual char* get_data() const noexcept = 0;
            virtual const char* get_data_for_log() const noexcept = 0;

            virtual void swap(stream* _stream) noexcept = 0;

            virtual void close() {}
        };

        class file_output_stream final
            : public stream
        {
        public:
            explicit file_output_stream(std::ofstream&& _file)
                : file_(std::move(_file))
            {
            }

            void write(const char* _data, int64_t _size) override
            {
                file_.write(_data, _size);
                if (file_.good())
                    bytes_writed_ += _size;
            }

            int64_t all_size() const noexcept override
            {
                return bytes_writed_;
            }

            void close() override
            {
                file_.close();
            }

            int64_t available() const noexcept override
            {
                assert(false);
                return 0;
            }

            char* get_data() const noexcept override
            {
                assert(false);
                return nullptr;
            }

            const char* get_data_for_log() const noexcept override
            {
                assert(false);
                return nullptr;
            }

            void swap(stream* _stream) noexcept override
            {
                assert(false);
            }

        private:
            std::ofstream file_;
            int64_t bytes_writed_ = 0;
        };

        class binary_stream final
            : public stream
        {
            using data_buffer = std::vector<char>;

            mutable data_buffer buffer_;
            mutable int64_t input_cursor_ = 0;
            mutable int64_t output_cursor_ = 0;

        public:

            binary_stream() = default;

            binary_stream(const binary_stream& _stream)
            {
                copy(_stream);
            }

            binary_stream(binary_stream&& _stream) noexcept
            {
                swap(_stream);
            }

            binary_stream& operator=(binary_stream&& _stream) noexcept
            {
                swap(_stream);
                return *this;
            }

            binary_stream& operator=(const binary_stream& _stream)
            {
                copy(_stream);
                return *this;
            }

            char* get_data() const noexcept override
            {
                if (buffer_.empty())
                    return nullptr;

                return buffer_.data();
            }

            void copy(const binary_stream& _stream)
            {
                input_cursor_ = _stream.input_cursor_;
                output_cursor_ = _stream.output_cursor_;
                buffer_ = _stream.buffer_;
            }

            void swap(stream* _stream) noexcept override
            {
                if (auto bs = dynamic_cast<binary_stream*>(_stream))
                    swap(*bs);
            }

            void swap(binary_stream& _stream) noexcept
            {
                using std::swap;
                swap(_stream.input_cursor_, input_cursor_);
                swap(_stream.output_cursor_, output_cursor_);
                swap(_stream.buffer_, buffer_);
            }

            int64_t available() const noexcept override
            {
                return input_cursor_ - output_cursor_;
            }

            void reserve(int64_t _size)
            {
                if (buffer_.size() < _size)
                    resize(_size);
            }

            void resize(int64_t _size)
            {
                buffer_.resize(_size);
            }

            void resize(int64_t _size, const char& _val)
            {
                buffer_.resize(_size, _val);
            }

            void reset() noexcept
            {
                input_cursor_ = 0;
                output_cursor_ = 0;
            }

            void reset_out() const noexcept
            {
                output_cursor_ = 0;
            }

            char* alloc_buffer(int64_t _size)
            {
                const auto size_need = input_cursor_ + _size;
                if (size_need > buffer_.size())
                    buffer_.resize(size_need * 2);

                char* out = &buffer_[input_cursor_];

                input_cursor_ += _size;

                return out;
            }

            template <class t_> void write(const t_& _val)
            {
                write((const char*) &_val, sizeof(t_));
            }

            template <typename T, typename = std::enable_if_t< !std::is_same<T, std::ifstream>::value> > // for file streams use load_from_file
            void write_stream(T& _source)
            {
                const auto size = buffer_.size();
                std::copy(std::istreambuf_iterator<typename std::istream::char_type>(_source),
                          std::istreambuf_iterator<char>(),
                          std::back_inserter(buffer_));

                input_cursor_ += (buffer_.size() - size);
            }

            void write(const char* _data, int64_t _size) override
            {
                if (_size == 0)
                    return;

                const auto size_need = input_cursor_ + _size;
                if (size_need > buffer_.size())
                    buffer_.resize(size_need + 1, '\0'); // +1 it '\0' at the end of the buffer

                memcpy(&buffer_[input_cursor_], _data, _size);
                input_cursor_ += _size;
            }

            char* read(int64_t _size) const
            {
                if (_size == 0)
                {
                    assert(!"read from stream size = 0");
                    return nullptr;
                }

                if (available() < _size)
                {
                    assert(!"read from invalid size");
                    return nullptr;
                }

                char* out = &buffer_[output_cursor_];

                output_cursor_ += _size;

                return out;
            }

            char* read_available() const
            {
                const int64_t available_size = available();

                if (available_size == 0)
                {
                    assert(false);
                    return nullptr;
                }


                char* out = &buffer_[output_cursor_];

                output_cursor_ += available_size;

                return out;
            }

            template <class t_>
            t_ read() const
            {
                return  *((t_*) read(sizeof(t_)));
            }

            bool save_2_file(const std::wstring& _file_name) const;

            static bool save_2_file(std::string_view _buf, const std::wstring& _file_name);

            bool load_from_file(std::wstring_view _file_name);

            char* get_data_for_write() noexcept
            {
                return buffer_.data();
            }

            const char* get_data_for_log() const noexcept override
            {
                if (available() <= 0)
                    return nullptr;

                return &buffer_[output_cursor_];
            }

            void set_output(int64_t _value) noexcept
            {
                output_cursor_ = _value;
            }

            void set_input(int64_t _value) noexcept
            {
                input_cursor_ = _value;
            }

            int64_t all_size() const noexcept override
            {
                return buffer_.size();
            }

            bool is_zlib_compressed() const noexcept
            {
                return all_size() > 2
                    && static_cast<uint8_t>(buffer_[0]) == 0x78
                    && (static_cast<uint8_t>(buffer_[1]) == 0x9C
                        || static_cast<uint8_t>(buffer_[1]) == 0x01
                        || static_cast<uint8_t>(buffer_[1]) == 0xDA
                        || static_cast<uint8_t>(buffer_[1]) == 0x5E);
            }
            bool is_gzip_compressed() const noexcept
            {
                return all_size() > 2
                    && static_cast<uint8_t>(buffer_[0]) == 0x1F
                    && static_cast<uint8_t>(buffer_[1]) == 0x8B;
            }
            bool is_zstd_compressed() const noexcept
            {
                return all_size() > 4
                    && static_cast<uint8_t>(buffer_[0]) == 0x28
                    && static_cast<uint8_t>(buffer_[1]) == 0xB5
                    && static_cast<uint8_t>(buffer_[2]) == 0x2F
                    && static_cast<uint8_t>(buffer_[3]) == 0xFD;
            }
            bool is_compressed() const noexcept
            {
                return is_zlib_compressed() || is_gzip_compressed() || is_zstd_compressed();
            }

            bool encrypt_aes256(std::string_view _key);
            bool decrypt_aes256(std::string_view _key);
        };

        template <> void core::tools::binary_stream::write<std::string>(const std::string& _val);
        template <> void core::tools::binary_stream::write<std::string_view>(const std::string_view& _val);

        template <> std::string core::tools::binary_stream::read<std::string>() const;

        inline constexpr size_t max_compress_bytes() noexcept
        {
            return 2'000'000'000U;
        }
        inline constexpr size_t max_decompress_bytes() noexcept
        {
            return 1'000'000'000U;
        }

        bool compress_gzip(const char* _data, size_t _size, binary_stream& _compressed_bs, int _level = Z_BEST_COMPRESSION);
        bool compress_gzip(const stream& _bs, binary_stream& _compressed_bs, int _level = Z_BEST_COMPRESSION);
        bool decompress_gzip(const char* _data, size_t _size, binary_stream& _uncompressed_bs);
        bool decompress_gzip(const stream& _bs, binary_stream& _uncompressed_bs);
        inline bool is_compressed(const binary_stream& _bs)
        {
            return _bs.is_compressed();
        }
#ifndef STRIP_ZSTD
        bool compress_zstd(const char* _data, size_t _size, std::string_view _dict, binary_stream& _compressed_bs);
        bool compress_zstd(const stream& _bs, std::string_view _dict, binary_stream& _compressed_bs);
        bool decompress_zstd(const char* _data, size_t _size, std::string_view _dict, binary_stream& _uncompressed_bs);
        bool decompress_zstd(const stream& _bs, std::string_view _dict, binary_stream& _uncompressed_bs);
#endif // !STRIP_ZSTD
    }

}

#endif //__BINARYSTREAM_H_
