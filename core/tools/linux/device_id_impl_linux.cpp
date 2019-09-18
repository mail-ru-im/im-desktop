#include "stdafx.h"

#include <sys/utsname.h>
#include "tools/device_id.h"
#include "cpuid.h"


static const char* getMachineName()
{
   static struct utsname u;

   if ( uname( &u ) < 0 )
      return "unknown";

   return u.nodename;
}

static void getCpuid(unsigned int* p)
{
    __get_cpuid(0, &p[0], &p[1], &p[2], &p[3]);
}

static unsigned short getCpuHash()
{
    unsigned int cpuinfo[4] = { 0, 0, 0, 0 };
    getCpuid(cpuinfo);
    unsigned short hash = 0;
    unsigned int* ptr = (&cpuinfo[0]);
    for ( unsigned int i = 0; i < 4; i++ )
        hash += (ptr[i] & 0xFFFF) + ( ptr[i] >> 16 );

    return hash;
}

std::string core::tools::impl::get_device_id_impl()
{
    std::string device_id = getMachineName() + std::to_string(getCpuHash());
    return device_id;
}

