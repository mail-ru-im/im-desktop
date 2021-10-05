#include "stdafx.h"

#include "InputStateContainer.h"

namespace Logic
{
    InputStateContainer& InputStateContainer::instance()
    {
        static InputStateContainer instance;
        return instance;
    }

    Ui::InputStatePtr InputStateContainer::createState(const QString& _contact)
    {
        if (_contact.isEmpty())
            return {};

        return std::make_shared<Ui::InputWidgetState>();
    }

    Ui::InputStatePtr InputStateContainer::getOrCreateState(const QString& _contact)
    {
        if (_contact.isEmpty())
            return {};

        auto& state = states_[_contact];
        if (!state)
            state = std::make_shared<Ui::InputWidgetState>();

        return state;
    }

    const Ui::InputStatePtr& InputStateContainer::getState(const QString& _contact) const
    {
        if (!_contact.isEmpty())
        {
            if (auto it = states_.find(_contact); it != states_.end())
                return it->second;
        }

        static const Ui::InputStatePtr empty = std::make_shared<Ui::InputWidgetState>();
        return empty;
    }

    int InputStateContainer::getCursorPos(const QString& _contact) const
    {
        return getState(_contact)->text_.cursorPos_;
    }

    const Data::QuotesVec& InputStateContainer::getQuotes(const QString& _contact) const
    {
        return getState(_contact)->quotes_;
    }

    void InputStateContainer::clearAll()
    {
        states_.clear();
    }

    void InputStateContainer::clear(const QString& _contact)
    {
        if (auto it = states_.find(_contact); it != states_.end())
        {
            im_assert(it->second.use_count() == 1);
            states_.erase(it);
        }
    }
}