#include "stdafx.h"
#include "PhoneFormatter.h"
#include "phonenumbers/phonenumberutil.h"
#include "phonenumbers/asyoutypeformatter.h"
#include "main_window/contact_list/CountryListModel.h"
#include "utils/utils.h"

namespace
{
    bool tryFormatWithLibPhonenumber(const QString& _phone, QString& _result);

    namespace FallbackFormatter
    {
        bool formatPhone(const QString &_phone, QString& _result);

        void trim(QString& _result);

        void convertFullNumber11Digits(const QString& _phone, QString& _result);
        void convertFullNumber12Digits(const QString& _phone, QString& _result);
        void convertFullNumber13Digits(const QString& _phone, QString& _result);

        QString convert6Digit(const QStringRef& _phone);
        QString convert7Digit(const QStringRef& _phone);

        std::map<QString, QString> getCountryCodes();

    } // FallbackFormatter

} // anonymous

namespace PhoneFormatter
{
    QString formatted(const QString& _phone)
    {
        QString formatted;

        if (_phone.isEmpty())
            return formatted;

        if (!tryFormatWithLibPhonenumber(_phone, formatted))
            FallbackFormatter::formatPhone(_phone, formatted);

        return formatted;
    }

    bool parse(const QString& _phone, QString& _code, QString& _number)
    {
        if (!_phone.isEmpty())
        {
            const auto phoneUtil = i18n::phonenumbers::PhoneNumberUtil::GetInstance();

            i18n::phonenumbers::PhoneNumber phoneNumber;
            const auto status = phoneUtil->ParseAndKeepRawInput(_phone.toStdString(), "ZZ", &phoneNumber);
            if (status == i18n::phonenumbers::PhoneNumberUtil::ErrorType::NO_PARSING_ERROR)
            {
                if (phoneNumber.has_country_code())
                    _code = ql1c('+') % QString::number(phoneNumber.country_code());

                if (phoneNumber.has_national_number())
                    _number = QString::number(phoneNumber.national_number());

                return true;
            }
        }

        return false;
    }

    QString getFormattedPhone(const QString& _code, const QString& _phone)
    {
        const auto phoneUtil = i18n::phonenumbers::PhoneNumberUtil::GetInstance();

        if (_code.isEmpty() || _phone.isEmpty())
            return _phone;

        const auto country = Utils::getCountryCodeByName(Logic::getCountryModel()->getCountryByCode(_code).name_).toUpper();
        if (country.isEmpty())
            return _phone;

        std::string result;
        auto formatter = phoneUtil->GetAsYouTypeFormatter(country.toStdString());

        for (const auto& d : _phone)
        {
            if (d.isDigit())
                formatter->InputDigit(d.toLatin1(), &result);
        }

        if (result.empty())
            return QString();

        return QString::fromStdString(result).remove(_code).trimmed();
    }

} // PhoneFormatter

namespace
{
    bool tryFormatWithLibPhonenumber(const QString& _phone, QString& _result)
    {
        const auto phoneUtil = i18n::phonenumbers::PhoneNumberUtil::GetInstance();

        i18n::phonenumbers::PhoneNumber phoneNumber;
        const auto error = phoneUtil->ParseAndKeepRawInput(_phone.toStdString(), "ZZ", &phoneNumber);

        if (error != i18n::phonenumbers::PhoneNumberUtil::ErrorType::NO_PARSING_ERROR)
            return false;

        std::string formatted;
        phoneUtil->FormatInOriginalFormat(phoneNumber, "ZZ", &formatted);

        _result = QString::fromStdString(formatted);

        return true;
    }

    namespace FallbackFormatter
    {
        bool formatPhone(const QString& _phone, QString& _result)
        {
            if (_phone.isEmpty())
                return false;

            auto phone = _phone;

            trim(phone);

            const auto phoneLength = phone.length();

            switch(phoneLength)
            {
                case 11:
                    convertFullNumber11Digits(phone, _result);
                    break;
                case 12:
                    convertFullNumber12Digits(phone, _result);
                    break;
                case 13:
                    convertFullNumber13Digits(phone, _result);
                    break;
                default:
                    _result = u'+' % phone;
                    break;
            }

            if (_result.isEmpty())
            {
                const auto prefix = !phone.isEmpty() ? QStringView(u"+") : QStringView();
                _result = prefix % phone;
            }

            return true;
        }

        void trim(QString& _result)
        {
            QString buf;
            for (auto &p : _result)
            {
                if (p.isDigit())
                    buf += p;
            }
            _result = buf;
        }

        void convertFullNumber11Digits(const QString& _phone, QString& _result)
        {
            auto countryCodes = getCountryCodes();
            if (_phone.startsWith(countryCodes[qsl("RU_KZ")])
             || _phone.startsWith(countryCodes[qsl("US")]))
            {
                _result = ql1c('+') % _phone[0] % ql1c(' ');
                for (auto i = 0; i < 3; i++)
                    _result += _phone[1+i];
                _result += convert7Digit(_phone.midRef(4));
            }
            else if (_phone.startsWith(countryCodes[qsl("CZ")]))
            {
                _result = ql1c('+') % countryCodes[qsl("CZ")] % ql1c(' ');
                for (auto i = 0; i < 2; i++)
                    _result += _phone[3+i];
                _result += convert6Digit(_phone.midRef(5));
            }
        }

        void convertFullNumber12Digits(const QString& _phone, QString& _result)
        {
            auto countryCodes = getCountryCodes();
            if (_phone.startsWith(countryCodes[qsl("CZ")]))
            {
                _result = ql1c('+') % countryCodes[qsl("CZ")] % ql1c(' ');
                for (auto i = 0; i < 3; i++)
                    _result += _phone[3+i];
                _result += convert6Digit(_phone.midRef(6));
            }
            else if (_phone.startsWith(countryCodes[qsl("UA")])
                  || _phone.startsWith(countryCodes[qsl("BY")])
                  || _phone.startsWith(countryCodes[qsl("UZ")]))
            {
                _result = ql1c('+');
                for (auto i = 0; i < 3; i++)
                    _result += _phone[i];
                _result += ql1c(' ');
                for (auto i = 0; i < 2; i++)
                    _result += _phone[3+i];
                _result += convert7Digit(_phone.midRef(5));
            }
            else if (_phone.startsWith(countryCodes[qsl("UK")]))
            {
                _result = ql1c('+') % countryCodes[qsl("UK")] % ql1c(' ');
                for (auto i = 0; i < 4; i++)
                    _result += _phone[2+i];
                _result += convert6Digit(_phone.midRef(6));
            }
        }

        void convertFullNumber13Digits(const QString& _phone, QString& _result)
        {
            auto countryCodes = getCountryCodes();
            if (_phone.startsWith(countryCodes[qsl("NG")]))
            {
                _result = ql1c('+') % countryCodes[qsl("NG")] % ql1c(' ');
                for (auto i = 0; i < 3; i++)
                    _result += _phone[3+i];
                _result += convert7Digit(_phone.midRef(6));
            }
            else if (_phone.startsWith(countryCodes[qsl("DE")]))
            {
                _result = ql1c('+') % countryCodes[qsl("DE")] % ql1c(' ');
                for (auto i = 0; i < 4; i++)
                    _result += _phone[2+i];
                _result += convert7Digit(_phone.midRef(6));
            }
        }

        QString convert6Digit(const QStringRef& _phone)
        {
            QString formatted(qsl(" "));
            for (auto i = 0; i < 3; i++)
                formatted += _phone.at(i);
            formatted += ql1c('-');
            for (auto i = 0; i < 3; i++)
                formatted += _phone.at(3+i);

            return formatted;
        }

        QString convert7Digit(const QStringRef& _phone)
        {
            QString formatted(qsl(" "));
            for (auto i = 0; i < 3; i++)
                formatted += _phone.at(i);
            formatted += ql1c('-');
            for (auto i = 0; i < 2; i++)
                formatted += _phone.at(3+i);
            formatted += ql1c('-');
            for (auto i = 0; i < 2; i++)
                formatted += _phone.at(5+i);

            return formatted;
        }

        std::map<QString, QString> getCountryCodes()
        {
            static std::map<QString, QString> countryCodes =
            {
                { qsl("RU_KZ"), qsl("7")},
                { qsl("UA"), qsl("380")},
                { qsl("BY"), qsl("375")},
                { qsl("UZ"), qsl("998")},
                { qsl("US"), qsl("1")},
                { qsl("UK"), qsl("44")},
                { qsl("NG"), qsl("234")},
                { qsl("DE"), qsl("49")},
                { qsl("CZ"), qsl("420")}
            };

            return  countryCodes;
        }

    } // FallbackFormatter

} // anonymous
