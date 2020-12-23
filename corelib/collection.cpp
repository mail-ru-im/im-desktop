#include "stdafx.h"
#include "collection.h"
#define __STDC_FORMAT_MACROS 1
#include <inttypes.h>

using namespace core;


core::collection_value::collection_value()
    :  type_(collection_value_type::vt_empty),
       log_data_(nullptr),
       ref_count_(1)
{

}

core::collection_value::~collection_value()
{
    clear();
}

int32_t core::collection_value::addref()
{
    return ++ref_count_;
}

int32_t core::collection_value::release()
{
    int32_t r = (--ref_count_);
    if (0 == r)
    {
        delete this;
        return 0;
    }
    return r;
}

void core::collection_value::clear()
{
    free(log_data_);
    log_data_ = nullptr;

    switch (type_)
    {
    case core::vt_empty:
        return;
    case core::vt_collection:
        {
            data__.collection_value_->release();
        }
        break;
    case core::vt_stream:
        {
            data__.istream_value_->release();
        }
        break;
    case core::vt_array:
        {
            data__.array_value_->release();
        }
        break;
    case core::vt_hheaders:
        {
            data__.ihheaders_value_->release();
        }
        break;
    case core::vt_string:
        {
            delete [] data__.string_value_;
        }
        break;
    case core::vt_int:
    case core::vt_double:
    case core::vt_bool:
    case core::vt_int64:
    case core::vt_uint:
        ::memset(&data__, 0, sizeof(data__));
        break;
    default:
        assert(!"clear data for this type");
        break;
    }

}

void core::collection_value::set_as_int(int32_t val)
{
    clear();
    type_ = vt_int;
    data__.int_value_ = val;
}

int32_t core::collection_value::get_as_int() const
{
    if (type_ != collection_value_type::vt_int)
    {
        assert(!"invalid value type");
        return 0;
    }

    return data__.int_value_;
}

void core::collection_value::set_as_int64(int64_t val)
{
    clear();
    type_ = vt_int64;
    data__.int64_value_ = val;
}

int64_t core::collection_value::get_as_int64() const
{
    if (type_ != collection_value_type::vt_int64)
    {
        assert(!"invalid value type");
        return 0;
    }

    return data__.int64_value_;
}


void core::collection_value::set_as_string(const char* val, int32_t len)
{
    clear();
    type_ = collection_value_type::vt_string;
    data__.string_value_ = new char[len + 1];
    if (len)
        memcpy(data__.string_value_, val, len);
    data__.string_value_[len] = '\0';
}

const char* core::collection_value::get_as_string() const
{
    if (type_ != collection_value_type::vt_string)
    {
        assert(!"invalid value type");
        return "";
    }

    return data__.string_value_;
}

void core::collection_value::set_as_double(double val)
{
    clear();
    type_ = collection_value_type::vt_double;
    data__.double_value_ = val;
}

double core::collection_value::get_as_double() const
{
    if (type_ != collection_value_type::vt_double)
    {
        assert(!"invalid value type");
        return 0.0;
    }

    return data__.double_value_;
}

void core::collection_value::set_as_bool(bool val)
{
    clear();
    type_ = collection_value_type::vt_bool;
    data__.bool_value_ = val;
}

bool core::collection_value::get_as_bool() const
{
    bool val = false;
    if (type_ != collection_value_type::vt_bool)
    {
        assert(!"invalid value type");
        return val;
    }

    return data__.bool_value_;
}

void core::collection_value::set_as_collection(icollection* val)
{
    clear();
    val->addref();
    type_ = collection_value_type::vt_collection;
    data__.collection_value_ = val;
}

icollection* core::collection_value::get_as_collection() const
{
    if (type_ != collection_value_type::vt_collection)
    {
        assert(!"invalid data type");
        return nullptr;
    }

    return data__.collection_value_;
}

void core::collection_value::set_as_stream(istream* val)
{
    clear();
    val->addref();
    type_ = collection_value_type::vt_stream;
    data__.istream_value_ = val;
}

istream* core::collection_value::get_as_stream()
{
    if (type_ != collection_value_type::vt_stream)
    {
        assert(!"invalid data type");
        return nullptr;
    }

    return data__.istream_value_;
}

void core::collection_value::set_as_array(iarray* val)
{
    clear();
    val->addref();
    type_ = collection_value_type::vt_array;
    data__.array_value_ = val;
}

iarray* core::collection_value::get_as_array()
{
    if (type_ != collection_value_type::vt_array)
    {
        assert(!"invalid data type");
        return nullptr;
    }

    return data__.array_value_;
}

void core::collection_value::set_as_hheaders(ihheaders_list* _val)
{
    clear();
    _val->addref();
    type_ = collection_value_type::vt_hheaders;
    data__.ihheaders_value_ = _val;
}

ihheaders_list* core::collection_value::get_as_hheaders()
{
    if (type_ != collection_value_type::vt_hheaders)
    {
        assert(!"invalid data type");
        return nullptr;
    }

    return data__.ihheaders_value_;
}

void core::collection_value::set_as_uint(uint32_t val)
{
    clear();
    type_ = vt_uint;
    data__.uint_value_ = val;
}

uint32_t core::collection_value::get_as_uint() const
{
    if (type_ != collection_value_type::vt_uint)
    {
        assert(!"invalid data type");
        return 0;
    }

    return data__.uint_value_;
}

const char* core::collection_value::log() const
{
    free(log_data_);
    log_data_ = nullptr;

    switch (type_)
    {
    case core::vt_string:
        return data__.string_value_;
    case core::vt_int:
        log_data_ = (char*) malloc(20);
        sprintf(log_data_, "%d", data__.int_value_);
        break;
    case core::vt_double:
        log_data_ = (char*) malloc(40);
        sprintf(log_data_, "%d", data__.int_value_);
        break;
    case core::vt_bool:
        log_data_ = (char*) malloc(20);
        sprintf(log_data_, "%s", data__.bool_value_ ? "true" : "false");
        break;
    case core::vt_int64:
        log_data_ = (char*) malloc(40);
        sprintf(log_data_, "%" PRId64, data__.int64_value_);
        break;
    case core::vt_uint:
        log_data_ = (char*) malloc(20);
        sprintf(log_data_, "%d", data__.uint_value_);
        break;
    case core::vt_collection:
        return "<collection>";
    case core::vt_stream:
        log_data_ = (char*) malloc(40);
        sprintf(log_data_, "<stream size=" "%" PRId64 ">", data__.istream_value_->size());
        break;
    case core::vt_array:
        log_data_ = (char*) malloc(40);
        sprintf(log_data_, "<array size=%d>", data__.array_value_->size());
        break;
    case core::vt_hheaders:
        return "<headers>";
    default:
        return "<unknown>";
    }

    return log_data_;
}

//////////////////////////////////////////////////////////////////////////
//
//////////////////////////////////////////////////////////////////////////
coll_stream::coll_stream()
    :    ref_count_(1)
{

}

coll_stream::~coll_stream() = default;

uint8_t* coll_stream::read(int64_t _size)
{
    return (uint8_t*) stream_.read( _size);
}

void coll_stream::reset()
{
    stream_.reset();
}

void coll_stream::write(std::istream& _source)
{
    stream_.write_stream(_source);
}

void coll_stream::write(const uint8_t* _buffer, int64_t _size)
{
    stream_.write((const char*) _buffer, _size);
}

bool coll_stream::empty() const
{
    return !stream_.available();
}

int64_t coll_stream::size() const
{
    return stream_.available();
}

int32_t coll_stream::addref()
{
    return ++ref_count_;
}

int32_t coll_stream::release()
{
    int32_t r = (--ref_count_);
    if (0 == r)
    {
        delete this;
        return 0;
    }
    return r;
}






//////////////////////////////////////////////////////////////////////////
// collection
//////////////////////////////////////////////////////////////////////////
collection::collection()
    :    ref_count_(1), cursor_(values_.end()), log_data_(nullptr)
{
}


collection::~collection()
{
    clear();
}



int32_t core::collection::addref()
{
    return ++ref_count_;
}

int32_t core::collection::release()
{
    int32_t r = (--ref_count_);
    if (0 == r)
    {
        delete this;
        return 0;
    }
    return r;
}

ivalue* core::collection::create_value()
{
    return (new core::collection_value());
}

icollection* core::collection::create_collection()
{
    return (new core::collection());
}

iarray* core::collection::create_array()
{
    return (new core::coll_array());
}

istream* core::collection::create_stream()
{
    return (new core::coll_stream());
}

ihheaders_list* core::collection::create_hheaders_list()
{
    return (new core::hheaders_list());
}

void core::collection::set_value(std::string_view name, ivalue* value)
{
    auto iter = values_.find(name);
    if (iter != values_.end())
    {
        iter->second->release();
        values_.erase(iter);
    }

    values_[std::string(name)] = value;
    value->addref();
}

ivalue* core::collection::get_value(std::string_view name) const
{
    const auto iter_value = values_.find(name);
    if (iter_value == values_.end())
    {
        assert(!"value doesn't exist");
#if defined(DEBUG) || defined(_DEBUG)
        puts(std::string(name).c_str());
#endif // defined(DEBUG) || defined(_DEBUG)
        return nullptr;
    }

    return iter_value->second;
}

void core::collection::clear()
{
    free(log_data_);

    for (const auto& x : values_)
        x.second->release();
}

ivalue* core::collection::first()
{
    if (values_.empty())
        return nullptr;

    cursor_ = values_.begin();

    return cursor_->second;
}

ivalue* core::collection::next()
{
    const auto end = values_.end();
    if (cursor_ == end)
        return nullptr;

    ++cursor_;
    if (cursor_ == end)
        return nullptr;

    return cursor_->second;
}

int32_t core::collection::count() const
{
    return (int32_t) values_.size();
}

bool core::collection::empty() const
{
    return values_.empty();
}

bool core::collection::is_value_exist(std::string_view name) const
{
    return values_.find(name) != values_.end();
}

const char* core::collection::log() const
{
    if (const auto iter_val = values_.find("not_log"); iter_val != values_.end())
        return "";

    std::string s;

    for (const auto& [name, value] : values_)
    {
        s += name;
        s += '=';
        s += value->log();
        s += '\n';
    }

    if (s.empty())
        return "";

    const auto text_size = s.size();
    free(log_data_);
    log_data_ = (char*) malloc(text_size + 1);

    memcpy(log_data_, s.data(), text_size);
    log_data_[text_size] = 0;

    return log_data_;
}







core::coll_array::coll_array()
    :    ref_count_(1)
{

}

core::coll_array::~coll_array()
{
    for (auto x : vec_)
        x->release();
}

int32_t core::coll_array::addref()
{
    return ++ref_count_;
}

int32_t coll_array::release()
{
    int32_t r = (--ref_count_);
    if (0 == r)
    {
        delete this;
        return 0;
    }
    return r;
}


void core::coll_array::push_back(ivalue* val)
{
    vec_.push_back(val);
    val->addref();
}

const ivalue* core::coll_array::get_at(size_type pos) const
{
    return vec_[pos];
}

void core::coll_array::reserve(size_type sz)
{
    vec_.reserve(sz);
}

core::iarray::size_type core::coll_array::size() const
{
    return size_type(vec_.size());
}

bool core::coll_array::empty() const
{
    return vec_.empty();
}


hheaders_list::hheaders_list()
    : cursor_(headers_.end()), ref_count_(1)
{

}

hheaders_list::~hheaders_list()
{
    for (auto x : headers_)
        delete x;
}

int32_t hheaders_list::addref()
{
    return ++ref_count_;
}

int32_t hheaders_list::release()
{
    int32_t r = (--ref_count_);
    if (0 == r)
    {
        delete this;
        return 0;
    }
    return r;
}

void hheaders_list::push_back(hheader* _header)
{
    headers_.push_back(_header);
}

hheader* hheaders_list::first()
{
    if (headers_.empty())
        return nullptr;

    cursor_ = headers_.begin();

    return (*cursor_);
}

hheader* hheaders_list::next()
{
    if (cursor_ == headers_.end())
        return nullptr;

    ++cursor_;
    if (cursor_ == headers_.end())
        return nullptr;

    return (*cursor_);
}

int32_t hheaders_list::count() const
{
    return (int32_t) headers_.size();
}

bool hheaders_list::empty() const
{
    return headers_.empty();
}
