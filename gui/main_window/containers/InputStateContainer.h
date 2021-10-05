#pragma once

#include "utils/utils.h"
#include "main_window/input_widget/InputWidgetState.h"

namespace Logic
{
    class InputStateContainer
    {
    public:
        static InputStateContainer& instance();
        InputStateContainer(InputStateContainer const&) = delete;
        InputStateContainer& operator=(InputStateContainer const&) = delete;
        ~InputStateContainer() = default;

        Ui::InputStatePtr createState(const QString& _contact); // creates temporary
        Ui::InputStatePtr getOrCreateState(const QString& _contact); // creates new if not exist
        const Ui::InputStatePtr& getState(const QString& _contact) const; // returns empty and != nullptr if not exist

        const Data::QuotesVec& getQuotes(const QString& _contact) const;
        int getCursorPos(const QString& _contact) const;

        void clearAll();
        void clear(const QString& _contact);

    private:
        InputStateContainer() = default;

        std::unordered_map<QString, Ui::InputStatePtr, Utils::QStringHasher> states_;
    };
}