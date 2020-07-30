#pragma once

#include "../namespaces.h"

CORE_DISK_CACHE_NS_BEGIN

class cache_entity;

class disk_cache;

enum class entity_type;

typedef std::shared_ptr<cache_entity> cache_entity_sptr;

typedef std::shared_ptr<disk_cache> disk_cache_sptr;

typedef std::function<void(cache_entity_sptr &_entity)> entity_get_callback;

typedef std::function<void()> entity_put_callback;

class disk_cache
{
public:
    static disk_cache_sptr make(std::wstring _path);

    virtual ~disk_cache() = 0;

    virtual void get(
        const entity_type _type,
        const std::string &_name,
        entity_get_callback &_on_entity_get) = 0;

    virtual void put(
        const entity_type _type,
        const std::string &_name,
        const void *_buf,
        const int64_t _buf_size,
        entity_put_callback &_on_entity_put) = 0;



};

CORE_DISK_CACHE_NS_END