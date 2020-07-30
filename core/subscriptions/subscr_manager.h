#pragma once

namespace core::wim::subscriptions
{
    using subscr_ptr = std::shared_ptr<class subscription_base>;
    using subscr_ptr_v = std::vector<subscr_ptr>;

    class manager
    {
    public:
        void add(subscr_ptr _subscription);
        void remove(const subscr_ptr& _subscription);

        subscr_ptr_v get_subscriptions_to_request() const;
        bool has_active_subscriptions() const;

    private:
        subscr_ptr_v active_subs_;
    };
}
