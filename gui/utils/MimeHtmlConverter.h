#pragma once
#include "../types/fstring.h"

namespace Utils
{
    class MimeHtmlConverter
    {
    public:
        MimeHtmlConverter();
        ~MimeHtmlConverter();
        Data::FString fromHtml(const QString& _html) const;

    private:
        std::unique_ptr<class MimeHtmlConverterPrivate> d;
    };
}
