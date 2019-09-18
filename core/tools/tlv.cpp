#include "stdafx.h"
#include "tlv.h"
#include "binary_stream.h"

using namespace core;
using namespace tools;

core::tools::tlvpack::tlvpack()
{
    cursor_ = tlvlist_.begin();
}


void tlvpack::copy(const tlvpack& _pack)
{
    tlvlist_.clear();

    for (const auto& tlv_cur : _pack.tlvlist_)
        tlvlist_.push_back(std::make_shared<tlv>(*tlv_cur));
}

tlvpack::tlvpack(const tlvpack& _pack)
{
    copy(_pack);
}

tlvpack& tlvpack::operator=(const tlvpack& _pack)
{
    copy(_pack);
    return (*this);
}

core::tools::tlvpack::~tlvpack()
{

}

uint32_t core::tools::tlvpack::size() const
{
    return (uint32_t)tlvlist_.size();
}

bool core::tools::tlvpack::empty() const
{
    return tlvlist_.empty();
}

bool core::tools::tlvpack::unserialize(const binary_stream& _stream)
{
    while (_stream.available())
    {
        auto tlv = std::make_shared<core::tools::tlv>();
        if (!tlv->unserialize(_stream))
            return false;

        tlvlist_.push_back(tlv);
    }

    return true;
}

void core::tools::tlvpack::serialize(binary_stream& _stream) const
{
    for (const auto &x : tlvlist_)
        x->serialize(_stream);
}

void core::tools::tlvpack::push_child(const std::shared_ptr<tlv>& _tlv)
{
    tlvlist_.push_back(_tlv);
}

void core::tools::tlvpack::push_child(tlv _tlv)
{
    tlvlist_.push_back(std::make_shared<tlv>(std::move(_tlv)));
}


std::shared_ptr<tlv> core::tools::tlvpack::get_item(uint32_t _type) const
{
    for (const auto& x : tlvlist_)
    {
        if (x->get_type() == _type)
            return x;
    }

    return std::shared_ptr<tlv>();
}

std::shared_ptr<tlv> core::tools::tlvpack::get_first()
{
    if (tlvlist_.empty())
        return std::shared_ptr<tlv>();

    cursor_ = tlvlist_.begin();

    return (*cursor_);
}

std::shared_ptr<tlv> core::tools::tlvpack::get_next()
{
    if (tlvlist_.empty() || cursor_ == tlvlist_.end() || tlvlist_.end() == (++cursor_))
        return std::shared_ptr<tlv>();

    return (*cursor_);
}

iserializable_tlv::~iserializable_tlv()
{

}




tlv::tlv()
    : type_(0)
{
}

tlv::~tlv()
{
}


uint32_t core::tools::tlv::get_type() const
{
    return type_;
}

void core::tools::tlv::set_type(uint32_t _type)
{
    type_ = _type;
}

void core::tools::tlv::serialize(core::tools::binary_stream& _stream) const
{
    _stream.write((uint32_t) type_);
    _stream.write((uint32_t) value_stream_.available());
    if (value_stream_.available())
    {
        uint32_t size = value_stream_.available();
        _stream.write(value_stream_.read(size), size);
        value_stream_.reset_out();
    }

}

bool core::tools::tlv::unserialize(const binary_stream& _stream)
{
    if (_stream.available() < sizeof(uint32_t)*2)
        return false;

    type_ = _stream.read<uint32_t>();
    uint32_t length = _stream.read<uint32_t>();

    if (length == 0)
        return true;

    if (_stream.available() < (length))
        return false;

    value_stream_.write(_stream.read(length), length);

    return true;
}

bool core::tools::tlv::try_get_field_with_type(const binary_stream& _stream, const uint32_t _type, uint32_t& _length)
{
    _length = 0;
    if (_stream.available() < sizeof(uint32_t)*2)
        return false;

    auto type = _stream.read<uint32_t>();
    auto length = _stream.read<uint32_t>();

    if (length == 0)
        return true;

    if (_stream.available() < (length))
        return false;

    if (type == _type)
    {
        _length = length;
    }
    else
    {
        _stream.read(length);
    }

    return true;
}

template<> binary_stream tlv::get_value() const
{
    return value_stream_;
}

template<> tlvpack tlv::get_value() const
{
    tlvpack pack;
    pack.unserialize(value_stream_);

    return pack;
}

template<> std::string tlv::get_value<std::string>(const std::string& _default_value) const
{
    std::string val = _default_value;
    if (const uint32_t size = value_stream_.available(); size > 0)
    {
        try
        {
            if (const auto chars = (const char*)value_stream_.read(size))
                val.assign(chars, size);
        }
        catch (...)
        {
        }
        value_stream_.reset_out();
    }

    return val;
}

template<> std::string tlv::get_value<std::string>() const
{
    return get_value<std::string>(std::string());
}

template<> void tlv::set_value<std::string>(const std::string& _value)
{
    value_stream_.reset();
    if (_value.empty())
        return;
    value_stream_.write((char*)_value.c_str(), (uint32_t)_value.size());
}

template<> void tlv::set_value<binary_stream>(const binary_stream& _value)
{
    value_stream_.copy(_value);
}

template<> void tlv::set_value<tlvpack>(const tlvpack& _value)
{
    _value.serialize(value_stream_);
}

template<> void tlv::set_value<iserializable_tlv>(const iserializable_tlv& _value)
{
    tlvpack pack;
    _value.serialize(Out pack);
    set_value<tlvpack>(pack);
}
