#include "stdafx.h"

#include <tools/hmac_sha_base64.h>
#include "tools/device_id.h"

uint16_t getVolumeHash()
{
   DWORD serialNum = 0;

   // Determine if this volume uses an NTFS file system.
   GetVolumeInformationA( "c:\\", NULL, 0, &serialNum, NULL, NULL, NULL, 0 );
   uint16_t hash = (uint16_t)(( serialNum + ( serialNum >> 16 )) & 0xFFFF );

   return hash;
}

uint16_t getCpuHash()
{
   int cpuinfo[4] = { 0, 0, 0, 0 };
   __cpuid( cpuinfo, 0 );
   uint16_t hash = 0;
   uint16_t* ptr = (uint16_t*)(&cpuinfo[0]);
   for (uint32_t i = 0; i < 8; i++ )
      hash += ptr[i];

   return hash;
}

const char* getMachineName()
{
   static char computerName[1024];
   DWORD size = 1024;
   GetComputerNameA( computerName, &size );
   return &(computerName[0]);
}

std::string core::tools::impl::get_device_id_impl()
{
    std::string device_id;
    device_id += std::to_string(getVolumeHash());
    device_id += std::to_string(getCpuHash());
    device_id += getMachineName();

    return device_id;
}
