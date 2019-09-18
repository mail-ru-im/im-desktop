#include "stdafx.h"
#include "binary_stream.h"
#include "strings.h"
#include "system.h"
#include "utils.h"

using namespace core;
using namespace tools;

bool binary_stream::save_2_file(const std::wstring& _file_name) const
{
    boost::filesystem::wpath path_for_file(_file_name);
    std::wstring folder_name = path_for_file.parent_path().wstring();

    if (!core::tools::system::is_exist(folder_name))
    {
        if (!core::tools::system::create_directory(folder_name))
            return false;
    }

    std::wstring temp_file_name = _file_name + L".tmp";

    {
        auto outfile = tools::system::open_file_for_write(temp_file_name, std::ofstream::binary | std::ofstream::trunc);
        if (!outfile.is_open())
            return false;

        auto_scope end_scope_call([&outfile] { outfile.close(); });

        uint32_t size = available();

        if (size > 0)
        {
            outfile.write(read(size), size);
        }
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

    return (!error);
}

bool binary_stream::load_from_file(const std::wstring& _file_name)
{
    if (!core::tools::system::is_exist(_file_name))
        return false;

    auto infile = tools::system::open_file_for_read(_file_name, std::ifstream::in | std::ifstream::binary |std::ifstream::ate);
    if (!infile.is_open())
        return false;

    constexpr int32_t mem_block_size = 1024 * 64;

    uint64_t size = infile.tellg();
    infile.seekg (0, std::ifstream::beg);
    uint64_t size_read = 0;

    char buffer[mem_block_size];

    while (size_read < size)
    {
        uint64_t bytes_to_read = mem_block_size;
        uint64_t tail = size - size_read;
        if (tail < mem_block_size)
            bytes_to_read = tail;

        infile.read(buffer, bytes_to_read);
        if (!infile.good())
        {
            assert(!"read stream to file error");
            return false;
        }

        write((const char*)buffer, (int32_t) bytes_to_read);

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

template <> void binary_stream::write<std::string>(const std::string& _val)
{
    if (!_val.empty())
        write(_val.c_str(), (uint32_t)_val.size());
}

template <> void binary_stream::write<std::string_view>(const std::string_view& _val)
{
    if (!_val.empty())
        write(_val.data(), (uint32_t)_val.size());
}

template <> std::string core::tools::binary_stream::read<std::string>() const
{
    std::string val;

    uint32_t sz = available();
    if (!sz)
        return val;

    val.assign((const char*) read(sz), sz);

    return val;
}

bool core::tools::compress(const char* _data, size_t _size, binary_stream& _compressed_bs, int _level)
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

    return true;
}

bool core::tools::compress(const binary_stream& _bs, binary_stream& _compressed_bs, int _level)
{
    size_t size = _bs.all_size();
    if (size > max_compress_bytes())
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

    deflate_s.next_in = reinterpret_cast<Bytef*>(_bs.get_data());
    deflate_s.avail_in = static_cast<unsigned int>(size);

    _compressed_bs.reset();

    size_t size_compressed = 0;
    do
    {
        size_t increase = size / 2 + 1024;

        _compressed_bs.reserve(size_compressed + increase);

        deflate_s.avail_out = static_cast<unsigned int>(increase);
        deflate_s.next_out = reinterpret_cast<Bytef*>((_compressed_bs.get_data()+ size_compressed));

        deflate(&deflate_s, Z_FINISH);

        size_compressed += (increase - deflate_s.avail_out);
    } while (deflate_s.avail_out == 0);

    deflateEnd(&deflate_s);
    _compressed_bs.resize(size_compressed);

    return true;
}

bool core::tools::decompress(const binary_stream& _bs, binary_stream& _uncompressed_bs)
{
    size_t size = _bs.all_size();
    if (size > max_decompress_bytes() ||
        (2 * size) > max_decompress_bytes())
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

    inflate_s.next_in = reinterpret_cast<Bytef*>(_bs.get_data());
    inflate_s.avail_in = static_cast<unsigned int>(size);

    _uncompressed_bs.reset();

    size_t size_uncompressed = 0;
    do
    {
        size_t resize_to = size_uncompressed + 2 * size;
        if (resize_to > max_decompress_bytes())
        {
            inflateEnd(&inflate_s);
            return false;
        }
        _uncompressed_bs.resize(resize_to);
        inflate_s.avail_out = static_cast<unsigned int>(2 * size);
        inflate_s.next_out = reinterpret_cast<Bytef*>(_uncompressed_bs.get_data() + size_uncompressed);
        auto ret = inflate(&inflate_s, Z_FINISH);
        if (ret != Z_STREAM_END && ret != Z_OK && ret != Z_BUF_ERROR)
        {
            // std::string error_msg = inflate_s.msg;
            inflateEnd(&inflate_s);
            return false;
        }

        size_uncompressed += (2 * size - inflate_s.avail_out);
    } while (inflate_s.avail_out == 0);

    inflateEnd(&inflate_s);
    _uncompressed_bs.resize(size_uncompressed);

    return true;
}
