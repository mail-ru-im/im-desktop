#pragma once

namespace core
{
    namespace wim
    {
        class loader;

        struct wim_packet_params;

        class fs_loader_task
        {
        public:
            const wim_packet_params& get_wim_params() const;

            fs_loader_task(std::string _id, const wim_packet_params &_params);
            virtual ~fs_loader_task();

            virtual void on_result(int32_t _error) = 0;
            virtual void on_progress() = 0;

            virtual void resume(loader &_loader);

            const std::string& get_id() const;
            void set_last_error(int32_t _error);
            int32_t get_last_error() const;

        private:
            std::string id_;

            std::unique_ptr<wim_packet_params> wim_params_;

            int32_t error_;

        };

    }
}
