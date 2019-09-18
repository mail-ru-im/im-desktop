#pragma once

#include <QTextBrowser>
#include <QMargins>
#include "namespaces.h"

#include "fonts.h"
#include "styles/ThemeParameters.h"

namespace Ui {

// http://doc.qt.io/qt-5/qtextdocument.html#documentMargin-prop
constexpr int QTEXTDOCUMENT_DEFAULT_MARGIN = 4;

class TextBrowserEx: public QTextBrowser
{
    Q_OBJECT

public:
    struct Options
    {
        QColor linkColor_ = Styling::getParameters().getColor(Styling::StyleVariable::PRIMARY_INVERSE);
        QColor textColor_ = Styling::getParameters().getColor(Styling::StyleVariable::TEXT_SOLID);
        QColor backgroundColor_ = Styling::getParameters().getColor(Styling::StyleVariable::BASE_GLOBALWHITE);
        QFont font_ = Fonts::appFontScaled(15);
        bool borderless_ = true;
        bool openExternalLinks_ = true;
        qreal documentMargin_ = 0.;
        bool useDocumentMargin_ = false;
        QMargins bodyMargins_;
        bool noTextDecoration_ = false;

        QMargins getMargins() const;

        int leftBodyMargin() const { return bodyMargins_.left() > QTEXTDOCUMENT_DEFAULT_MARGIN ? (bodyMargins_.left() - QTEXTDOCUMENT_DEFAULT_MARGIN) : 0; }
        int rightBodyMargin() const { return bodyMargins_.right() > QTEXTDOCUMENT_DEFAULT_MARGIN ? (bodyMargins_.right() - QTEXTDOCUMENT_DEFAULT_MARGIN) : 0; }
        int bottomBodyMargin() const { return bodyMargins_.bottom() > QTEXTDOCUMENT_DEFAULT_MARGIN ? (bodyMargins_.bottom() - QTEXTDOCUMENT_DEFAULT_MARGIN) : 0; }
        int topBodyMargin() const { return bodyMargins_.top() > QTEXTDOCUMENT_DEFAULT_MARGIN ? (bodyMargins_.top() - QTEXTDOCUMENT_DEFAULT_MARGIN) : 0; }
    };

    TextBrowserEx(const Options& _options, QWidget* _parent =  nullptr);

    const Options& getOptions() const;

private:
    QString generateStyleSheet(const Options& _options);

private:
    Options options_;
};

namespace TextBrowserExUtils
{
    int setAppropriateHeight(TextBrowserEx& _textBrowser);
}

}
