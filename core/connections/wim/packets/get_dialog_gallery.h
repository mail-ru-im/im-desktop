#pragma once

#include "../robusto_packet.h"
#include "../../../archive/gallery_cache.h"


namespace core::tools
{
    class http_request_simple;
}

namespace core::wim
{
    class get_dialog_gallery: public robusto_packet
    {
    public:
        get_dialog_gallery(wim_packet_params _params,
                           const std::string& _aimid,
                           const std::string& _path_version,
                           const core::archive::gallery_entry_id& _from,
                           const core::archive::gallery_entry_id& _till,
                           const int _count, bool _parse_for_patches = false);

        virtual ~get_dialog_gallery();

        core::archive::gallery_storage get_gallery() const;

    private:
        virtual int32_t init_request(std::shared_ptr<core::http_request_simple> _request) override;
        virtual int32_t parse_results(const rapidjson::Value& _node_results) override;

        core::archive::gallery_storage gallery_;

        const std::string aimid_;
        const std::string path_version_;
        const core::archive::gallery_entry_id from_;
        const core::archive::gallery_entry_id till_;
        const int count_;
        const std::string my_aimid_;
        bool parse_for_patches_;
    };
}