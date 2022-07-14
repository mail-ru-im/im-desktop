#ifndef __CORE_MD5_H_
#define __CORE_MD5_H_

#pragma once

namespace core
{
    namespace tools
    {
        std::string md5(const void* _data, size_t _size);
        std::string md5(std::istream& _input);
    }
}


#endif //__CORE_MD5_H_