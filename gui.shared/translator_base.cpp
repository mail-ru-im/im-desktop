#include "stdafx.h"

#include "translator_base.h"

namespace translate
{
    void translator_base::init()
    {
        installTranslator(getLang());
    }

    QString translator_base::formatDayOfWeek(const QDate& _date) const
    {
        switch (_date.dayOfWeek())
        {
        case Qt::Monday:
            return QT_TRANSLATE_NOOP("date", "on Monday");
        case Qt::Tuesday:
            return QT_TRANSLATE_NOOP("date", "on Tuesday");
        case Qt::Wednesday:
            return QT_TRANSLATE_NOOP("date", "on Wednesday");
        case Qt::Thursday:
            return QT_TRANSLATE_NOOP("date", "on Thursday");
        case Qt::Friday:
            return QT_TRANSLATE_NOOP("date", "on Friday");
        case Qt::Saturday:
            return QT_TRANSLATE_NOOP("date", "on Saturday");
        case Qt::Sunday:
            return QT_TRANSLATE_NOOP("date", "on Sunday");

        default:
            return QT_TRANSLATE_NOOP("date", "recently");
        }
    }

    QString translator_base::formatDate(const QDate& target, bool currentYear) const
    {
        const QString format = currentYear ? getCurrentYearDateFormat() : getOtherYearsDateFormat();
        return QLocale().toString(target, format);
    }

    QString translator_base::getNumberString(int number, const QString& one, const QString& two, const QString& five, const QString& twentyOne) const
    {
        if (number == 1)
            return one;

        QString result;
        int strCase = number % 10;
        if (strCase == 1 && number % 100 != 11)
        {
            result = twentyOne;
        }
        else if (strCase > 4 || strCase == 0 || (number % 100 > 10 && number % 100 < 20))
        {
            result = five;
        }
        else
        {
            result = two;
        }

        return result;
    }

    QString translator_base::formatDifferenceToNow(const QDateTime& _dt, bool _withLongTimeAgo) const
    {
        QString result;

        QDateTime now = QDateTime::currentDateTime();
        const auto days = _dt.daysTo(now);
        const auto sameYear = _dt.date().year() == now.date().year();

        if (days == 0)
            result = QT_TRANSLATE_NOOP("date", "today");
        else if (days == 1)
            result = QT_TRANSLATE_NOOP("date", "yesterday");
        else if (days < 7)
            result = formatDayOfWeek(_dt.date());
        else if (days > 365 && _withLongTimeAgo)
            result = QT_TRANSLATE_NOOP("date", "a long time ago");
        else
            result = formatDate(_dt.date(), sameYear);

        if (sameYear || days < 7)
            result += QT_TRANSLATE_NOOP("date", " at ") % _dt.time().toString(Qt::SystemLocaleShortDate);

        return result;
    }

    QString translator_base::getCurrentYearDateFormat() const
    {
        const QString lang = getLang();
        if (lang == u"ru")
            return qsl("d MMM");
        else if (lang == u"de")
            return qsl("d. MMM");
        else if (lang == u"pt")
            return qsl("d 'de' MMMM");
        else if (lang == u"uk")
            return qsl("d MMM");
        else if (lang == u"cs")
            return qsl("d. MMM");
        else if (lang == u"fr")
            return qsl("Le d MMM");
        else if (lang == u"zh")
            return u"MMMd'" % QT_TRANSLATE_NOOP("date", "day") % u'\'';
        else if (lang == u"es")
            return qsl("d 'de' MMM");
        else if (lang == u"tr")
            return qsl("d MMM");
        else if (lang == u"vi")
            return u'\'' % QT_TRANSLATE_NOOP("date", "day") % u"' d MMM";

        return qsl("MMM d");
    }

    QString translator_base::getOtherYearsDateFormat() const
    {
        const QString lang = getLang();
        if (lang == u"ru")
            return qsl("d MMM yyyy");
        else if (lang == u"de")
            return qsl("d. MMM yyyy");
        else if (lang == u"pt")
            return qsl("d 'de' MMMM 'de' yyyy");
        else if (lang == u"uk")
            return qsl("d MMM yyyy");
        else if (lang == u"cs")
            return qsl("d. MMM yyyy");
        else if (lang == u"fr")
            return qsl("Le d MMM yyyy");
        else if (lang == u"zh")
            return u"yyyy'" % QT_TRANSLATE_NOOP("date", "year") % u"'MMMd'" % QT_TRANSLATE_NOOP("date", "day") % u'\'';
        else if (lang == u"es")
            return qsl("d 'de' MMM 'de' yyyy");
        else if (lang == u"tr")
            return qsl("d MMM yyyy");
        else if (lang == u"vi")
            return u'\'' % QT_TRANSLATE_NOOP("date", "day") % u"' d MMM '" % QT_TRANSLATE_NOOP("date", "year") % u"' yyyy";

        return qsl("MMM d, yyyy");
    }

    void translator_base::installTranslator(const QString& _lang)
    {
        QLocale::setDefault(QLocale(_lang));
        static QTranslator translator;
        translator.load(_lang, qsl(":/translations"));

        QApplication::installTranslator(&translator);
    }

    const std::vector<QString>& translator_base::getLanguages() const
    {
        static const std::vector<QString> clist =
        {
            qsl("ru"),
            qsl("en"),
            qsl("uk"),
            qsl("de"),
            qsl("pt"),
            qsl("cs"),
            qsl("fr"),
            qsl("zh"),
            qsl("tr"),
            qsl("vi"),
            qsl("es")
        };
        return clist;
    }

    static QString getLocalLang()
    {
        if (platform::is_apple())
        {
            const auto uiLangs = QLocale().uiLanguages();
            if (!uiLangs.isEmpty())
            {
                const auto& first = uiLangs.first();
                const auto idx = first.indexOf(ql1c('-'));
                if (idx == -1)
                    return first;
                return first.left(idx);
            }
            else
            {
                return QString();
            }
        }
        else
        {
            return QLocale::system().name().left(2);
        }
    }

    QString translator_base::getLang() const
    {
        QString lang;

        QString localLang = getLocalLang();
        if (localLang.isEmpty())
            lang = qsl("en");
        else
            lang = std::move(localLang);

        const auto& langs = getLanguages();
        if (std::all_of(langs.begin(), langs.end(), [&lang](const QString& _l) { return _l != lang; }))
            lang = qsl("en");

        return lang;
    }

    QLocale translator_base::getLocale() const
    {
        return QLocale(getLang());
    }

    QString translator_base::getLocaleStr() const
    {
        QString localeStr = getLocale().name();
        localeStr.replace(ql1c('_'), ql1c('-'));

        return std::move(localeStr).toLower();
    }
}
