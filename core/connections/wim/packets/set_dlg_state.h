#ifndef __ROBUSTO_SET_DLG_STATE
#define __ROBUSTO_SET_DLG_STATE

#pragma once

#include "../robusto_packet.h"

namespace core
{
    namespace tools
    {
        class http_request_simple;
    }
}


namespace core
{
    namespace wim
    {
        struct set_dlg_state_params
        {
            enum class exclude
            {
                call = 1,
                text = 2,
                mention = 4,
                ptt = 8
            };

            std::string aimid_;
            int64_t last_delivered_ = -1;
            int64_t last_read_ = -1;
            uint64_t excludes_ = 0;
            std::optional<bool> stranger_;

            void set_exclude(exclude _val, bool _on = true) noexcept
            {
                if (_on)
                    excludes_ |= static_cast<decltype(excludes_)>(_val);
                else
                    excludes_ &= ~static_cast<decltype(excludes_)>(_val);
            }

            bool test_exclude(exclude _val) const noexcept
            {
                return bool(excludes_ & static_cast<decltype(excludes_)>(_val));
            }

            bool has_exclude() const noexcept
            {
                return excludes_;
            }
        };


        class set_dlg_state : public robusto_packet
        {
            set_dlg_state_params params_;

            virtual int32_t init_request(std::shared_ptr<core::http_request_simple> _request) override;

        public:

            set_dlg_state(
                wim_packet_params _params,
                set_dlg_state_params _dlg_params);

            virtual ~set_dlg_state();
        };

    }

}


#endif// __ROBUSTO_SET_DLG_STATE
