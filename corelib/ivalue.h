#pragma once

#include "namespaces.h"

#include "ibase.h"

CORE_NS_BEGIN

struct icollection;
struct iarray;
struct ihheaders_list;
struct istream;

struct ivalue : ibase
{
    virtual void set_as_int(int32_t) = 0;
    virtual int32_t get_as_int() const = 0;

    virtual void set_as_int64(int64_t) = 0;
    virtual int64_t get_as_int64() const = 0;

    virtual void set_as_string(const char*, int32_t) = 0;
    virtual const char* get_as_string() const = 0;

    virtual void set_as_double(double) = 0;
    virtual double get_as_double() const = 0;

    virtual void set_as_bool(bool) = 0;
    virtual bool get_as_bool() const = 0;

    virtual void set_as_collection(icollection*) = 0;
    virtual icollection* get_as_collection() const = 0;

    virtual void set_as_stream(istream*) = 0;
    virtual istream* get_as_stream() = 0;

    virtual void set_as_array(iarray*) = 0;
    virtual iarray* get_as_array() = 0;

    virtual void set_as_hheaders(ihheaders_list*) = 0;
    virtual ihheaders_list* get_as_hheaders() = 0;

    virtual void set_as_uint(uint32_t) = 0;
    virtual uint32_t get_as_uint() const = 0;

    virtual const char* log() const = 0;

    virtual ~ivalue() {}
};

CORE_NS_END