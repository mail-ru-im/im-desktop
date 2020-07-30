#pragma once

#include "core_face.h"

namespace core
{

    struct core_instance : core::icore_interface, core::iconnector, core::icore_factory
    {
        iconnector*        gui_connector_;

    private:

        std::atomic<int32_t>    ref_count_;

        // ibase interface
        virtual int32_t addref() override;
        virtual int32_t release() override;

        // icoreinterface
        virtual iconnector* get_core_connector() override;
        virtual iconnector* get_gui_connector() override;
        virtual icore_factory* get_factory() override;

        // iconnector interface
        virtual void link(iconnector*, const common::core_gui_settings&) override;
        virtual void unlink() override;
        virtual void receive(std::string_view, int64_t, icollection*) override;

        // icore_factory
        virtual icollection* create_collection() override;

    public:

        core_instance();
        virtual ~core_instance();
    };

}

