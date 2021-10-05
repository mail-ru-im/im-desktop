#pragma once

#include "utils/utils.h"

namespace Logic
{
    struct InputText
    {
        InputText() = default;

        InputText(Data::FString _text, int _pos)
            : text_(std::move(_text))
            , cursorPos_(_pos)
        {}

        void clear()
        {
            text_.clear();
            cursorPos_ = 0;
        }

        Data::FString text_;
        int cursorPos_ = 0;
    };

    class InputTextContainer
    {
    public:
        static InputTextContainer& instance();
        ~InputTextContainer() = default;

        const InputText& getInputText(const QString& _contact) const;
        void setInputText(const QString& _contact, Data::FString _text, int _pos);

        const Data::FString& getText(const QString& _contact) const;
        void setText(const QString& _contact, Data::FString _text);

        int getCursorPos(const QString& _contact) const;
        void setCursorPos(const QString& _contact, int _pos);

        void clear(const QString& _contact);
        void clearAll();

    private:
        InputTextContainer() = default;

        std::unordered_map<QString, InputText, Utils::QStringHasher> inputTexts_;
    };
}