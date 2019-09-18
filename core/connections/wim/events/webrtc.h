#ifndef __FETCH_WEBRTC_EVENT_H__
#define __FETCH_WEBRTC_EVENT_H__

#include "fetch_event.h"

namespace core {
    namespace wim {

        class webrtc_event : public fetch_event {
            std::string _raw;

        public:
            webrtc_event();
            virtual ~webrtc_event();

            using fetch_event::parse;

            int32_t parse(const std::string& raw);
            virtual void on_im(std::shared_ptr<core::wim::im> _im, std::shared_ptr<auto_callback> _on_complete) override;
        };

    }
}



#endif//__FETCH_WEBRTC_EVENT_H__
