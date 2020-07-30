#include "stdafx.h"
#include "binary_stream.h"
#include "strings.h"
#include "system.h"
#include "utils.h"

#include "../core.h"
#include "../zstd_helper.h"
#include "../zstd_wrap/zstd_wrap.h"

#include "../common.shared/string_utils.h"

using namespace core;
using namespace tools;

bool binary_stream::save_2_file(const std::wstring& _file_name) const
{
    if (const auto size = available(); size > 0)
        return save_2_file({ read(size), size_t(size) }, _file_name);
    return save_2_file({}, _file_name);
}

bool core::tools::binary_stream::save_2_file(std::string_view _buf, const std::wstring& _file_name)
{
    boost::filesystem::wpath path_for_file(_file_name);
    std::wstring folder_name = path_for_file.parent_path().wstring();

    if (!core::tools::system::is_exist(folder_name))
    {
        if (!core::tools::system::create_directory(folder_name))
            return false;
    }

    std::wstring temp_file_name = su::wconcat(_file_name, L".tmp");

    {
        auto outfile = tools::system::open_file_for_write(temp_file_name, std::ofstream::binary | std::ofstream::trunc);
        if (!outfile.is_open())
            return false;

        auto_scope end_scope_call([&outfile] { outfile.close(); });
        if (const auto size = _buf.size(); size > 0)
            outfile.write(_buf.data(), size);
        outfile.flush();

        if (!outfile.good())
        {
            assert(!"save stream to file error");
            return false;
        }
    }

    boost::filesystem::path from(std::move(temp_file_name));
    boost::filesystem::path target(_file_name);
    boost::system::error_code error;
    boost::filesystem::rename(from, target, error);

    return !error;
}

bool binary_stream::load_from_file(std::wstring_view _file_name)
{
    if (!core::tools::system::is_exist(_file_name))
        return false;

    auto infile = tools::system::open_file_for_read(_file_name, std::ifstream::in | std::ifstream::binary | std::ifstream::ate);
    if (!infile.is_open())
        return false;

    constexpr int64_t mem_block_size = 1024 * 64;

    int64_t size = infile.tellg();
    infile.seekg(0, std::ifstream::beg);
    int64_t size_read = 0;

    char buffer[size_t(mem_block_size)];

    while (size_read < size)
    {
        int64_t bytes_to_read = mem_block_size;
        int64_t tail = size - size_read;
        if (tail < mem_block_size)
            bytes_to_read = tail;

        infile.read(buffer, bytes_to_read);
        if (!infile.good())
        {
            assert(!"read stream to file error");
            return false;
        }

        write((const char*)buffer, bytes_to_read);

        size_read += bytes_to_read;
    }

    return true;
}

bool binary_stream::encrypt_aes256(std::string_view _key)
{
    if (buffer_.empty())
        return false;

    reset();

    std::vector<char> encrypted;
    buffer_.pop_back();         // remove '\0' at the end
    auto result = utils::aes::encrypt(buffer_, encrypted, _key);

    buffer_.clear();
    write(&encrypted[0], encrypted.size());

    return result;
}

bool binary_stream::decrypt_aes256(std::string_view _key)
{
    if (buffer_.empty())
        return false;

    reset();

    std::vector<char> decrypted;
    buffer_.pop_back();         // remove '\0' at the end
    auto result = utils::aes::decrypt(buffer_, decrypted, _key);

    buffer_.clear();
    write(&decrypted[0], decrypted.size());

    return result;
}

template <> void binary_stream::write<std::string_view>(const std::string_view& _val)
{
    if (!_val.empty())
        write(_val.data(), _val.size());
}

template <> void binary_stream::write<std::string>(const std::string& _val)
{
    write(std::string_view(_val));
}

template <> std::string core::tools::binary_stream::read<std::string>() const
{
    std::string val;

    auto sz = available();
    if (!sz)
        return val;

    val.assign((const char*) read(sz), sz);

    return val;
}

bool core::tools::compress_gzip(const char* _data, size_t _size, binary_stream& _compressed_bs, int _level)
{
    if (_size > max_compress_bytes())
        return false;

    z_stream deflate_s;
    deflate_s.zalloc = Z_NULL;
    deflate_s.zfree = Z_NULL;
    deflate_s.opaque = Z_NULL;
    deflate_s.avail_in = 0;
    deflate_s.next_in = Z_NULL;

    constexpr int window_bits = 15 + 16; // gzip with windowbits of 15
    constexpr int mem_level = 8;
    // The memory requirements for deflate are (in bytes):
    // (1 << (window_bits+2)) + (1 << (mem_level+9))
    // with a default value of 8 for mem_level and our window_bits of 15
    // this is 128Kb

    if (deflateInit2(&deflate_s, _level, Z_DEFLATED, window_bits, mem_level, Z_DEFAULT_STRATEGY) != Z_OK)
        return false;

    deflate_s.next_in = reinterpret_cast<Bytef*>((char*)_data);
    deflate_s.avail_in = static_cast<unsigned int>(_size);

    _compressed_bs.reset();

    size_t size_compressed = 0;
    do
    {
        size_t increase = _size / 2 + 1024;

        _compressed_bs.reserve(size_compressed + increase);

        deflate_s.avail_out = static_cast<unsigned int>(increase);
        deflate_s.next_out = reinterpret_cast<Bytef*>((_compressed_bs.get_data() + size_compressed));

        deflate(&deflate_s, Z_FINISH);

        size_compressed += (increase - deflate_s.avail_out);
    } while (deflate_s.avail_out == 0);

    deflateEnd(&deflate_s);
    _compressed_bs.resize(size_compressed);
    _compressed_bs.set_input(size_compressed);

    return true;
}

bool core::tools::compress_gzip(const stream& _bs, binary_stream& _compressed_bs, int _level)
{
    return compress_gzip(_bs.get_data(), _bs.available(), _compressed_bs, _level);
}

bool core::tools::decompress_gzip(const char* _data, size_t _size, binary_stream& _uncompressed_bs)
{
    if (_size > max_decompress_bytes() || (2 * _size) > max_decompress_bytes())
        return false;

    z_stream inflate_s;
    inflate_s.zalloc = Z_NULL;
    inflate_s.zfree = Z_NULL;
    inflate_s.opaque = Z_NULL;
    inflate_s.avail_in = 0;
    inflate_s.next_in = Z_NULL;

    constexpr int window_bits = 15 + 32; // auto with windowbits of 15

    if (inflateInit2(&inflate_s, window_bits) != Z_OK)
        return false;

    inflate_s.next_in = reinterpret_cast<Bytef*>((char*)_data);
    inflate_s.avail_in = static_cast<unsigned int>(_size);

    _uncompressed_bs.reset();

    size_t size_uncompressed = 0;
    do
    {
        size_t resize_to = size_uncompressed + 2 * _size;
        if (resize_to > max_decompress_bytes())
        {
            inflateEnd(&inflate_s);
            return false;
        }
        _uncompressed_bs.resize(resize_to);
        inflate_s.avail_out = static_cast<unsigned int>(2 * _size);
        inflate_s.next_out = reinterpret_cast<Bytef*>(_uncompressed_bs.get_data() + size_uncompressed);
        auto ret = inflate(&inflate_s, Z_FINISH);
        if (ret != Z_STREAM_END && ret != Z_OK && ret != Z_BUF_ERROR)
        {
            // std::string error_msg = inflate_s.msg;
            inflateEnd(&inflate_s);
            return false;
        }

        size_uncompressed += (2 * _size - inflate_s.avail_out);
    } while (inflate_s.avail_out == 0);

    inflateEnd(&inflate_s);
    _uncompressed_bs.resize(size_uncompressed + 1, '\0'); // +1 for zero-terminating
    _uncompressed_bs.set_input(size_uncompressed);

    return true;
}

bool core::tools::decompress_gzip(const stream& _bs, binary_stream& _uncompressed_bs)
{
    return decompress_gzip(_bs.get_data(), _bs.available(), _uncompressed_bs);
}

bool core::tools::compress_zstd(const char* _data, size_t _size, std::string_view _dict, binary_stream& _compressed_bs)
{
    if (!_data || _size == 0 || _dict.empty())
        return false;

    const auto zstd_helper = g_core->get_zstd_helper();
    if (!zstd_helper)
        return false;

    _compressed_bs.reset();
    _compressed_bs.reserve(_size * 2);

    size_t data_out_written = 0;
    int zstd_status = ZSTDW_OK;
    bool is_output_buffer_increased = false;
    do
    {
        zstd_status = zstd_helper->compress(_data, _size, _compressed_bs.get_data(), _compressed_bs.all_size(), &data_out_written, _dict);
        if (zstd_status != ZSTDW_OK)
        {
            if (zstd_status == ZSTDW_OUTPUT_BUFFER_TOO_SMALL)
            {
                if (is_output_buffer_increased)
                    return false;

                _compressed_bs.reserve(data_out_written);
                is_output_buffer_increased = true;
            }
            else
            {
                if (zstd_status == ZSTDW_DICT_FILE_READ_ERROR)
                    zstd_helper->reinit_dicts(_dict);

                return false;
            }
        }

    } while (zstd_status != ZSTDW_OK);

    _compressed_bs.resize(data_out_written);
    _compressed_bs.set_input(data_out_written);

    return true;
}

bool core::tools::compress_zstd(const stream& _bs, std::string_view _dict, binary_stream& _compressed_bs)
{
    return compress_zstd(_bs.get_data(), _bs.available(), _dict, _compressed_bs);
}

bool core::tools::decompress_zstd(const char* _data, size_t _size, std::string_view _dict, binary_stream& _uncompressed_bs)
{
    if (!_data || _size == 0 || _dict.empty())
        return false;

    const auto zstd_helper = g_core->get_zstd_helper();
    if (!zstd_helper)
        return false;

    _uncompressed_bs.reset();
    _uncompressed_bs.reserve(_size * 3);

    size_t data_out_written = 0;
    int zstd_status = ZSTDW_OK;
    bool is_output_buffer_increased = false;
    do
    {
        zstd_status = zstd_helper->decompress(_data, _size, _uncompressed_bs.get_data(), _uncompressed_bs.all_size(), &data_out_written, _dict);
        if (zstd_status != ZSTDW_OK)
        {
            if (zstd_status == ZSTDW_OUTPUT_BUFFER_TOO_SMALL)
            {
                if (is_output_buffer_increased)
                    return false;

                _uncompressed_bs.reserve(data_out_written);
                is_output_buffer_increased = true;
            }
            else
            {
                if (zstd_status == ZSTDW_DICT_FILE_READ_ERROR)
                    zstd_helper->reinit_dicts(_dict);

                return false;
            }
        }

    } while (zstd_status != ZSTDW_OK);

    _uncompressed_bs.resize(data_out_written + 1, '\0'); // +1 for zero-terminating
    _uncompressed_bs.set_input(data_out_written);

    return true;
}

bool core::tools::decompress_zstd(const stream& _bs, std::string_view _dict, binary_stream& _uncompressed_bs)
{
    return decompress_zstd(_bs.get_data(), _bs.available(), _dict, _uncompressed_bs);
}
