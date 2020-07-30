#include "stdafx.h"

#include "dir_cache.h"

#include "disk_cache.h"

CORE_DISK_CACHE_NS_BEGIN

disk_cache_sptr disk_cache::make(std::wstring _path)
{
    assert(!_path.empty());

    return std::make_shared<dir_cache>(std::move(_path));
}

disk_cache::~disk_cache()
{

}

CORE_DISK_CACHE_NS_END