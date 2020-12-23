#include "stdafx.h"
#include "exif.h"
#include "system.h"
#include "strings.h"

namespace
{
    constexpr auto JPG_HEADER = 0xFFD8;
    constexpr auto APP0_HEADER = 0xFFE0;
    constexpr auto APP1_HEADER = 0xFFE1;
    constexpr auto EXIF_IFD = 0x8769;
    constexpr auto TYPE_SHORT = 3;
    constexpr auto TYPE_LONG = 4;
    constexpr auto ORIENTATION_TAG = 274;
    constexpr auto MAGIC_NUMBER_0 = 0;
    constexpr auto MAGIC_NUMBER_1 = 1;
    constexpr auto MAGIC_NUMBER_2 = 2;
    constexpr auto MAGIC_NUMBER_38 = 38;

    template <class T>
    void endswap(T* objp)
    {
        unsigned char* memp = reinterpret_cast<unsigned char*>(objp);
        std::reverse(memp, memp + sizeof(T));
    }

    uint16_t read_uint16(std::ifstream& stream)
    {
        uint16_t buffer = 0;
        stream.read(reinterpret_cast<char*>(&buffer), sizeof(buffer));
        endswap(&buffer);

        return buffer;
    }

    void write_uint16(std::ofstream& stream, uint16_t value)
    {
        endswap(&value);
        stream.write((char*)&value, sizeof(value));
    }

    uint16_t swap16(uint16_t _value)
    {
        endswap(&_value);
        return _value;
    }

    uint32_t swap32(uint32_t _value)
    {
        endswap(&_value);
        return _value;
    }

    void write_exif(core::tools::binary_stream& _stream, int _orientation)
    {
        _stream.write("MM", 2);
        _stream.write("\x00\x2A", 2);
        _stream.write("\x00\x00\x00\x08", 4);

        _stream.write(swap16(MAGIC_NUMBER_2));
        _stream.write(swap16(ORIENTATION_TAG) );
        _stream.write(swap16(TYPE_SHORT));
        _stream.write(swap32(MAGIC_NUMBER_1));
        _stream.write(swap16(_orientation));
        _stream.write(swap16(MAGIC_NUMBER_0));

        _stream.write(swap16(EXIF_IFD));
        _stream.write(swap16(TYPE_LONG));
        _stream.write(swap32(MAGIC_NUMBER_1));
        _stream.write(swap32(MAGIC_NUMBER_38));
        _stream.write(swap32(MAGIC_NUMBER_0));
        _stream.write(swap16(MAGIC_NUMBER_0));
    }
}

std::wstring core::tools::strip_exif(const std::wstring& _filename, int _orientation)
{
#ifdef _WIN32
    std::ifstream in(_filename, std::ios::binary);
#else
    std::ifstream in(tools::from_utf16(_filename), std::ios::binary);
#endif //_WIN32
    auto header = read_uint16(in);
    if (header != JPG_HEADER)
        return _filename;

    core::tools::binary_stream exif;
    {
        exif.write("Exif\0\0", 6);
        write_exif(exif, _orientation);
    }

    auto segment_id = read_uint16(in);
    auto segment_length = read_uint16(in);

    auto temp_filename = system::create_temp_file_path(boost::filesystem::wpath(_filename).filename().wstring());
    system::create_empty_file(temp_filename);

#ifdef _WIN32
    std::ofstream out(temp_filename, std::ios::binary);
#else
    std::ofstream out(tools::from_utf16(temp_filename), std::ios::binary);
#endif //_WIN32
    write_uint16(out, JPG_HEADER);

    auto get_segment = [](std::ifstream& _in, uint16_t _segment_length, std::string_view _to_find)
    {
        core::tools::binary_stream result;
        if (_segment_length < 6)
            return result;

        std::unique_ptr<char[]> buffer(new char[_segment_length - 2]);
        _in.read(buffer.get(), _segment_length - 2);
        if (std::string_view(buffer.get(), 4) != _to_find)
            return result;

        result.write(buffer.get(), _segment_length - 2);
        return result;
    };

    if (segment_id == APP0_HEADER)
    {        
        core::tools::binary_stream jfif = get_segment(in, segment_length, "JFIF");
        if (jfif.available() == 0)
            return _filename;

        segment_id = read_uint16(in);
        segment_length = read_uint16(in);
        if (segment_id == APP1_HEADER)
        {
            if (get_segment(in, segment_length, "Exif").available() == 0)
                return _filename;

            write_uint16(out, APP0_HEADER);
            write_uint16(out, jfif.available() + 2);
            out.write(jfif.get_data(), jfif.available());
        }
        else
        {
            return _filename;
        }
    }
    else if (segment_id == APP1_HEADER)
    {
        if (get_segment(in, segment_length, "Exif").available() == 0)
            return _filename;
    }
    else
    {
        return _filename;
    }
   
    write_uint16(out, APP1_HEADER);
    write_uint16(out, exif.available() + 2);
    out.write(exif.get_data(), exif.available());
    out << in.rdbuf();
    out.flush();

    return temp_filename;
}
