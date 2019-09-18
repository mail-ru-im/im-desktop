#include "stdafx.h"
#include "storage.h"
#include "history_message.h"
#include "../tools/system.h"

using namespace core;
using namespace archive;

namespace
{
    constexpr uint32_t max_data_block_size() noexcept { return 1024 * 1024; };
}

storage::storage(std::wstring _file_name)
    : file_name_(std::move(_file_name)), last_error_(archive::error::ok)
{
}


storage::~storage() = default;

void storage::clear()
{

}

bool storage::open(storage_mode _mode)
{
    last_error_ = archive::error::ok;

    if (active_file_stream_)
    {
        assert(!"file stream already opened");
        return false;
    }

    boost::filesystem::wpath path_for_file(file_name_);
    std::wstring forder_name = path_for_file.parent_path().wstring();

    if (_mode.flags_.read_ && !_mode.flags_.write_ && !core::tools::system::is_exist(file_name_))
    {
        last_error_ = archive::error::file_not_exist;
        return false;
    }

    if (!core::tools::system::is_exist(forder_name))
    {
        if (!core::tools::system::create_directory(forder_name))
        {
            last_error_ = archive::error::create_directory_error;
            return false;
        }
    }

    std::ios_base::openmode open_mode = std::fstream::binary;

    if (_mode.flags_.read_)
        open_mode |= std::fstream::in;
    if (_mode.flags_.write_)
        open_mode |= std::fstream::out;
    if (_mode.flags_.append_)
        open_mode |= std::fstream::app;
    if (_mode.flags_.truncate_)
        open_mode |= std::fstream::trunc;

#ifdef _WIN32
    active_file_stream_ = std::make_unique<std::fstream>(file_name_, open_mode);
#else
    active_file_stream_ = std::make_unique<std::fstream>(tools::from_utf16(file_name_), open_mode);
#endif

    if (!active_file_stream_->is_open())
    {
        last_error_ = archive::error::open_file_error;
        active_file_stream_.reset();
        return false;
    }

    if (_mode.flags_.append_ && _mode.flags_.write_)
        active_file_stream_->seekp(0, std::ios::end);

    return true;
}

void storage::close()
{
    if (!active_file_stream_)
    {
        assert(!"file stream not opened");
        return;
    }

    active_file_stream_->close();
    active_file_stream_.reset();
}

static std::int32_t get_last_error()
{
#ifdef _WIN32
    return std::int32_t(GetLastError());
#else //  Win
    return errno;
#endif
}

storage::result_type storage::write_data_block(core::tools::binary_stream& _data, int64_t& _offset)
{
    if (!active_file_stream_)
    {
        assert(!"file stream not opened");
        return { false, 0 };
    }

    _offset = active_file_stream_->tellp();

    if (const uint32_t data_size = _data.available(); data_size)
    {
        active_file_stream_->write((const char*)&data_size, sizeof(data_size));
        if (active_file_stream_->fail())
            return { false, get_last_error() };
        active_file_stream_->write((const char*)&data_size, sizeof(data_size));
        if (active_file_stream_->fail())
            return { false, get_last_error() };

        active_file_stream_->write((const char*)_data.read(data_size), data_size);
        if (active_file_stream_->fail())
            return { false, get_last_error() };

        active_file_stream_->write((const char*)&data_size, sizeof(data_size));
        if (active_file_stream_->fail())
            return { false, get_last_error() };
        active_file_stream_->write((const char*)&data_size, sizeof(data_size));
        if (active_file_stream_->fail())
            return { false, get_last_error() };
        return { true, 0 };
    }

    return { false, std::numeric_limits<std::int32_t>::max() };
}

bool storage::read_data_block(int64_t _offset, core::tools::binary_stream& _data)
{
    if (_offset != -1)
        active_file_stream_->seekp(_offset);

    if (active_file_stream_->peek() == EOF)
    {
        last_error_ = archive::error::end_of_file;
        return false;
    }

    uint32_t sz1 = 0, sz2 = 0;
    active_file_stream_->read((char*) &sz1, sizeof(sz1));
    if (!active_file_stream_->good())
        return false;

    active_file_stream_->read((char*) &sz2, sizeof(sz2));
    if (!active_file_stream_->good())
        return false;

    if (sz1 != sz2 || sz1 > max_data_block_size())
        return false;

    if (sz1 != 0)
    {
        active_file_stream_->read(_data.alloc_buffer(sz1), sz1);
        if (!active_file_stream_->good())
            return false;
    }

    uint32_t sz3 = 0, sz4 = 0;
    active_file_stream_->read((char*) &sz3, sizeof(sz3));
    if (!active_file_stream_->good())
        return false;

    active_file_stream_->read((char*) &sz4, sizeof(sz4));
    if (!active_file_stream_->good())
        return false;

    if (sz1 != sz3 || sz1 != sz4)
        return false;

    return true;
}

bool storage::fast_read_data_block(core::tools::binary_stream& buffer, int64_t& current_pos, int64_t& _begin, int64_t _end_position)
{
    bool is_small_file = false;

    constexpr auto step = sizeof(uint32_t) / sizeof(char);
    if (current_pos + 4 * step >= buffer.all_size())
    {
        is_small_file = true;
    }

    if (!is_small_file)
    {
        uint32_t sz1 = 0, sz2 = 0, sz3 = 0, sz4 = 0;

        while (!is_small_file)
        {
            if (current_pos + 4 * step >= _end_position)
            {
                is_small_file = true;
                break;
            }

            sz1 = *((uint32_t*)(&(buffer.get_data())[current_pos]));
            sz2 = *((uint32_t*)(&(buffer.get_data())[current_pos + step]));

            if (sz1 == 0 || sz1 != sz2 || sz1 > max_data_block_size())
            {
                ++current_pos;
                continue;
            }

            if (current_pos + 4 * step + sz1 <= _end_position)
            {
                sz3 = *((uint32_t*)(&(buffer.get_data())[current_pos + 2 * step + sz1]));
                sz4 = *((uint32_t*)(&(buffer.get_data())[current_pos + 3 * step + sz1]));

                if (sz3 != sz1 || sz3 != sz4)
                {
                    ++current_pos;
                    continue;
                }
                else
                    break;
            }
            else
            {
                is_small_file = true;
                break;
            }
        };

        if (!is_small_file)
        {
            current_pos += 2 * step;
            _begin = current_pos;

            buffer.set_output(current_pos);
            buffer.set_input(current_pos + sz1);

            current_pos += sz1 + 2 * step;
        }
    }

    return !is_small_file;
}
