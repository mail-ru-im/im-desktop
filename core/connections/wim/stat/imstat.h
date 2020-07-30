#pragma once

namespace core
{
    namespace wim
    {
        class wim_packet;
        struct wim_packet_params;
    }

    namespace statistic
    {
        class imstat
        {
            std::chrono::system_clock::time_point		last_start_time_;

        public:

            bool need_send() const;
            std::shared_ptr<wim::wim_packet> get_statistic(const wim::wim_packet_params& _params);

            imstat();
            virtual ~imstat();
        };

    }
}

