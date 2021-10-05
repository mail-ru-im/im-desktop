#include "stdafx.h"
#include "InputWidgetState.h"
#include "../corelib/enumerations.h"


namespace Ui
{
    void InputWidgetState::startEdit(const Data::MessageBuddySptr& _msg)
    {
        edit_ = std::make_optional(_msg);
    }

    void InputWidgetState::setText(Data::FString _text, int _cursorPos)
    {
        if (isEditing())
        {
            if (edit_->message_->GetType() == core::message_type::file_sharing)
            {
                auto builder = edit_->message_->GetSourceText() % qsl("\n");
                builder %= _text;
                edit_->buffer_ = builder.finalize();
            }
            else
            {
                edit_->buffer_ = std::move(_text);
            }
        }
        else
        {
            text_ = { std::move(_text), _cursorPos };
        }
    }
}