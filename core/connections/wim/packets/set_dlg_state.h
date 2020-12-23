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

            void update_last_read(int64_t _last_read)
            {
                last_read_ = std::max(last_read_, _last_read);
            }

            bool is_the_same_read_type(const set_dlg_state_params& _other) const noexcept
            {
                return aimid_ == _other.aimid_ && excludes_ == _other.excludes_;
            }

            bool is_the_same_read_state(const set_dlg_state_params& _other) const noexcept
            {
                return is_the_same_read_type(_other) && last_read_ == _other.last_read_;
            }

            bool is_empty() const noexcept
            {
                return aimid_.empty();
            }
        };


        class set_dlg_state : public robusto_packet
        {
            set_dlg_state_params params_;

            virtual int32_t init_request(const std::shared_ptr<core::http_request_simple>& _request) override;

        public:

            set_dlg_state(
                wim_packet_params _params,
                set_dlg_state_params _dlg_params);

            virtual ~set_dlg_state();

            virtual priority_t get_priority() const override;

            virtual std::string_view get_method() const override;

            virtual bool support_self_resending() const override;

            const set_dlg_state_params& get_params() const;

            void set_params(const set_dlg_state_params& _params);
        };

    }

}


#endif// __ROBUSTO_SET_DLG_STATE
