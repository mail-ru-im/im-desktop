#pragma once

namespace PhoneFormatter
{
    QString formatted(const QString& _phone);

    bool parse(const QString& _phone, QString& _code, QString& _number);

    QString getFormattedPhone(const QString& _code, const QString& _phone);
}
