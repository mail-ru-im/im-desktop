#ifndef __IMSTATE_H_
#define __IMSTATE_H_

#pragma once

namespace core
{
    namespace wim
    {
        enum imstate_sent_state
        {
            unknown = 0,
            failed = 1,
            sent = 2,
            delivered = 3
        };

        class imstate
        {
            std::string request_id_;
            std::string msg_id_;
            int64_t hist_msg_id_;
            int64_t before_hist_msg_id_;
            imstate_sent_state state_;
            int32_t error_code_;

        public:

            imstate();

            const std::string& get_request_id() const;
            void set_request_id(std::string _request_id);

            const std::string& get_msg_id() const;
            void set_msg_id(std::string _msg_id);

            int64_t get_hist_msg_id() const;
            void set_hist_msg_id(const int64_t _hist_msg_id);

            int64_t get_before_hist_msg_id() const;
            void set_before_hist_msg_id(const int64_t _before_hist_msg_id);

            imstate_sent_state get_state() const;
            void set_state(const imstate_sent_state _state);

            int32_t get_error_code() const;
            void set_error_code(const int32_t _error_code);
        };

        typedef std::vector<imstate> imstate_list;
    }
}


#endif//__IMSTATE_H_
