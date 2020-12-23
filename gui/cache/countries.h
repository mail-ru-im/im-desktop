#pragma once

namespace Ui
{
    namespace countries
    {
        struct country
        {
            const int phone_code_;
            const QString name_;
            const QStringView iso_code_;

            country(int _phone_code, const QString& _name, QStringView _iso_code)
                : phone_code_(_phone_code)
                , name_(_name)
                , iso_code_(_iso_code)
            {
            }
        };

        using countries_list = std::vector<country>;

        const countries_list& get();
    }
}

