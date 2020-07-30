#include "stdafx.h"

#include "subscr_manager.h"
#include "subscription.h"

namespace core::wim::subscriptions
{
    void manager::add(subscr_ptr _subscription)
    {
        if (!_subscription)
            return;

        const auto it = std::find_if(active_subs_.begin(), active_subs_.end(), [&_subscription](const auto& s) { return *s == *_subscription; });

        if (it != active_subs_.end())
            (*it)->reset_request_time();
        else
            active_subs_.push_back(std::move(_subscription));
    }

    void manager::remove(const subscr_ptr& _subscription)
    {
        if (!_subscription)
            return;

        const auto it = std::find_if(active_subs_.begin(), active_subs_.end(), [&_subscription](const auto& s) { return *s == *_subscription; });

        if (it != active_subs_.end())
            active_subs_.erase(it);
    }

    subscr_ptr_v manager::get_subscriptions_to_request() const
    {
        if (has_active_subscriptions())
        {
            subscr_ptr_v res;
            res.reserve(active_subs_.size());

            const auto cur_time = subscr_clock_t::now();
            for (const auto& s : active_subs_)
            {
                if (!s->is_requested() || cur_time - s->request_time() >= s->renew_interval())
                {
                    s->set_requested();
                    res.push_back(s);
                }
            }
            return res;
        }

        return {};
    }

    bool manager::has_active_subscriptions() const
    {
        return !active_subs_.empty();
    }
}
