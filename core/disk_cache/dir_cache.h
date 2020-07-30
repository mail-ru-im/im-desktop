#pragma once

#include "disk_cache.h"

CORE_DISK_CACHE_NS_BEGIN

class dir_cache : public disk_cache
{
public:
    dir_cache(std::wstring _root_dir_path);

    virtual ~dir_cache() override;

    virtual void get(
        const entity_type _type,
        const std::string &_name,
        entity_get_callback &_on_entity_get) override;

    virtual void put(
        const entity_type _type,
        const std::string &_name,
        const void *_buf,
        const int64_t _buf_size,
        entity_put_callback &_on_entity_put) override;

private:
    const std::wstring &root_dir_path_;

};

CORE_DISK_CACHE_NS_END