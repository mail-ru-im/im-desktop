#pragma once

#include "../../gui.shared/translator_base.h"

namespace Utils
{
    class Translator : public translate::translator_base
    {
    public:
        void init() override;
        QString getCurrentPhoneCode() const;
        void updateLocale();

        QString getLang() const override;
    };

    Translator* GetTranslator();
}