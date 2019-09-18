#pragma once
#include <QtCore/qglobal.h>
#include <vector>

#ifdef __APPLE__
#   import <QtCore/qdatetime.h>
#   import <QtCore/qlocale.h>
#else
#   include <QDateTime>
#   include <QLocale>
#endif

#undef QT_TR_NOOP
#define QT_TR_NOOP(x) tr(x)
#undef QT_TRANSLATE_NOOP
#define QT_TRANSLATE_NOOP(scope, x) QApplication::translate(scope, x)
#undef QT_TRANSLATE_NOOP3
#define QT_TRANSLATE_NOOP3(scope, x, comment) QApplication::translate(scope, x, comment)

namespace translate
{
    class translator_base
    {
    public:
        virtual ~translator_base() {}
        virtual void init();
        QString formatDayOfWeek(const QDate& _date) const;
        QString formatDate(const QDate& target, bool currentYear) const;
        QString getNumberString(int number, const QString& one, const QString& two, const QString& five, const QString& twentyOne) const;

        const std::vector<QString>& getLanguages() const;

        virtual QString getLang() const;

        virtual QLocale getLocale() const;

    protected:
        QString getCurrentYearDateFormat() const;
        QString getOtherYearsDateFormat() const;

        void installTranslator(const QString& _lang);
    };
}
