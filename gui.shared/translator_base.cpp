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

    QString translator_base::getCurrentYearDateFormat() const
    {
        const QString lang = getLang();
        if (lang == ql1s("ru"))
            return qsl("d MMM");
        else if (lang == ql1s("de"))
            return qsl("d. MMM");
        else if (lang == ql1s("pt"))
            return qsl("d 'de' MMMM");
        else if (lang == ql1s("uk"))
            return qsl("d MMM");
        else if (lang == ql1s("cs"))
            return qsl("d. MMM");
        else if (lang == ql1s("fr"))
            return qsl("Le d MMM");
        else if (lang == ql1s("zh"))
            return ql1s("MMMd'") % QT_TRANSLATE_NOOP("date", "day") % ql1c('\'');
        else if (lang == ql1s("es"))
            return qsl("d 'de' MMM");
        else if (lang == ql1s("tr"))
            return qsl("d MMM");
        else if (lang == ql1s("vi"))
            return ql1c('\'') % QT_TRANSLATE_NOOP("date", "day") % ql1s("' d MMM");

        return qsl("MMM d");
    }

    QString translator_base::getOtherYearsDateFormat() const
    {
        const QString lang = getLang();
        if (lang == ql1s("ru"))
            return qsl("d MMM yyyy");
        else if (lang == ql1s("de"))
            return qsl("d. MMM yyyy");
        else if (lang == ql1s("pt"))
            return qsl("d 'de' MMMM 'de' yyyy");
        else if (lang == ql1s("uk"))
            return qsl("d MMM yyyy");
        else if (lang == ql1s("cs"))
            return qsl("d. MMM yyyy");
        else if (lang == ql1s("fr"))
            return qsl("Le d MMM yyyy");
        else if (lang == ql1s("zh"))
            return ql1s("yyyy'") % QT_TRANSLATE_NOOP("date", "year") % ql1s("'MMMd'") % QT_TRANSLATE_NOOP("date", "day") % ql1c('\'');
        else if (lang == ql1s("es"))
            return qsl("d 'de' MMM 'de' yyyy");
        else if (lang == ql1s("tr"))
            return qsl("d MMM yyyy");
        else if (lang == ql1s("vi"))
            return ql1c('\'') % QT_TRANSLATE_NOOP("date", "day") % ql1s("' d MMM '") % QT_TRANSLATE_NOOP("date", "year") % ql1s("' yyyy");

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
}
