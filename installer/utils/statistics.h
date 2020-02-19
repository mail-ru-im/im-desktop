#pragma once

namespace installer
{
    class error;

    namespace statisctics
    {
        class SendTaskImpl;

        class SendTask : public QRunnable
        {
        public:
            SendTask(const error& _error, std::chrono::system_clock::time_point _start_time);
            ~SendTask();
            void run() override;

        private:
            std::unique_ptr<SendTaskImpl> impl_;
        };
    }
}