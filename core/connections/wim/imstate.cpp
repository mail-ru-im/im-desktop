#include "stdafx.h"
#include "imstate.h"

namespace core
{
    namespace wim
    {
        imstate::imstate()
            :   hist_msg_id_(-1),
            before_hist_msg_id_(-1),
            state_(imstate_sent_state::unknown),
            error_code_(200)
        {

        }

        const std::string& imstate::get_request_id() const
        {
            return request_id_;
        }

        void imstate::set_request_id(std::string _request_id)
        {
            request_id_ = std::move(_request_id);
        }

        const std::string& imstate::get_msg_id() const
        {
            return msg_id_;
        }

        void imstate::set_msg_id(std::string _msg_id)
        {
            msg_id_ = std::move(_msg_id);
        }

        int64_t imstate::get_hist_msg_id() const
        {
            return hist_msg_id_;
        }

        void imstate::set_hist_msg_id(const int64_t _hist_msg_id)
        {
            hist_msg_id_ = _hist_msg_id;
        }

        int64_t imstate::get_before_hist_msg_id() const
        {
            return before_hist_msg_id_;
        }

        void imstate::set_before_hist_msg_id(const int64_t _before_hist_msg_id)
        {
            before_hist_msg_id_ = _before_hist_msg_id;
        }

        imstate_sent_state imstate::get_state() const
        {
            return state_;
        }

        void imstate::set_state(const imstate_sent_state _state)
        {
            state_ = _state;
        }

        int32_t imstate::get_error_code() const
        {
            return error_code_;
        }

        void imstate::set_error_code(const int32_t _error_code)
        {
            error_code_ = _error_code;
        }
    }
}

