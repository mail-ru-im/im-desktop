#include "stdafx.h"

#include "InputTextContainer.h"

namespace Logic
{
    InputTextContainer& InputTextContainer::instance()
    {
        static InputTextContainer instance;
        return instance;
    }

    const InputText& InputTextContainer::getInputText(const QString& _contact) const
    {
        assert(!_contact.isEmpty());
        if (!_contact.isEmpty())
            if (auto it = inputTexts_.find(_contact); it != inputTexts_.end())
                return it->second;

        static const InputText empty;
        return empty;
    }

    void InputTextContainer::setInputText(const QString& _contact, QString _text, int _pos)
    {
        assert(!_contact.isEmpty());
        if (!_contact.isEmpty())
            inputTexts_[_contact] = InputText(std::move(_text), _pos);
    }

    const QString& InputTextContainer::getText(const QString& _contact) const
    {
        return getInputText(_contact).text_;
    }

    void InputTextContainer::setText(const QString& _contact, QString _text)
    {
        assert(!_contact.isEmpty());
        if (!_contact.isEmpty())
            inputTexts_[_contact].text_ = std::move(_text);
    }

    int InputTextContainer::getCursorPos(const QString& _contact) const
    {
        return getInputText(_contact).cursorPos_;
    }

    void InputTextContainer::setCursorPos(const QString& _contact, int _pos)
    {
        assert(!_contact.isEmpty());
        if (!_contact.isEmpty())
            inputTexts_[_contact].cursorPos_ = _pos;
    }

    void InputTextContainer::clear(const QString& _contact)
    {
        assert(!_contact.isEmpty());
        if (!_contact.isEmpty())
            if (auto it = inputTexts_.find(_contact); it != inputTexts_.end())
                inputTexts_.erase(it);
    }

    void InputTextContainer::clearAll()
    {
        inputTexts_.clear();
    }
}