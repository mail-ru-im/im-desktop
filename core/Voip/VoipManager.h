#ifndef __VOIP_MANAGER_H__
#define __VOIP_MANAGER_H__

#ifdef _WIN32
    #include "libvoip/include/voip/voip2.h"
#else
    #include "libvoip/include/voip/voip2.h"
#endif

#include "VoipManagerDefines.h"
#include "voip_memory_reporter.h"

#include <vector>
#include <memory>

namespace core
{
    class core_dispatcher;
    class coll_helper;
}

namespace voip_manager
{
    class VoipManagerImpl;

    void addVoipStatistic(std::stringstream& s, const core::coll_helper& params);
    
    class VoipManager
    {
        std::shared_ptr<VoipManagerImpl> _impl;

    public:
        VoipManager(core::core_dispatcher& dispatcher);

        int64_t get_voip_initialization_memory() const;


    public:
        std::shared_ptr<ICallManager>       get_call_manager      ();
        std::shared_ptr<IWindowManager>     get_window_manager    ();
        std::shared_ptr<IMediaManager>      get_media_manager     ();
        std::shared_ptr<IDeviceManager>     get_device_manager    ();
        std::shared_ptr<IConnectionManager> get_connection_manager();
        std::shared_ptr<IVoipManager>       get_voip_manager      ();
        std::shared_ptr<IMaskManager>       get_mask_manager      ();

        int64_t _voip_initialization = -1;
    };
}

#endif//__VOIP_MANAGER_H__
