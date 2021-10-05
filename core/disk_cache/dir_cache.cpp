#include "stdafx.h"

#include "cache_entity_type.h"

#include "dir_cache.h"

CORE_DISK_CACHE_NS_BEGIN

dir_cache::dir_cache(std::wstring _root_dir_path)
    : root_dir_path_(std::move(_root_dir_path))
{
    im_assert(!root_dir_path_.empty());
}

dir_cache::~dir_cache()
{

}

void dir_cache::get(
    const entity_type _type,
    const std::string &_name,
    entity_get_callback &_on_entity_get)
{
    im_assert(_type > entity_type::min);
    im_assert(_type < entity_type::max);
    im_assert(!_name.empty());
    im_assert(_on_entity_get);
}

void dir_cache::put(
    const entity_type _type,
    const std::string &_name,
    const void *_buf,
    const int64_t _buf_size,
    entity_put_callback &_on_entity_put)
{
    im_assert(_type > entity_type::min);
    im_assert(_type < entity_type::max);
    im_assert(!_name.empty());
    im_assert(_buf);
    im_assert(_buf_size > 0);
}

CORE_DISK_CACHE_NS_END