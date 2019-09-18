#include "stdafx.h"
#include "TextBrowserEx.h"
#include "utils/Text.h"

namespace Ui
{

TextBrowserEx::TextBrowserEx(const TextBrowserEx::Options& _options, QWidget *_parent)
    : QTextBrowser(_parent),
      options_(_options)
{
    setFont(_options.font_);
    setOpenExternalLinks(_options.openExternalLinks_);
    setStyleSheet(generateStyleSheet(_options));
    document()->setDefaultStyleSheet(generateStyleSheet(_options));

    // seems to affect margins set via stylesheet, see QTBUG-30831
    // workaround by subtracting default value of 4 in stylesheet
    if (options_.useDocumentMargin_)
        document()->setDocumentMargin(_options.documentMargin_);
}

const TextBrowserEx::Options &TextBrowserEx::getOptions() const
{
    return options_;
}

QString TextBrowserEx::generateStyleSheet(const TextBrowserEx::Options &_options)
{
    QString styleSheet = qsl("a { color: %1; }").arg(_options.linkColor_.name());

    if (_options.borderless_)
        styleSheet.append(qsl(" ")).append(qsl("QWidget { border: none; background-color: %1; color: %2; }").arg(_options.backgroundColor_.name()).arg(_options.textColor_.name()));

    if (_options.noTextDecoration_)
        styleSheet.append(qsl(" ")).append(qsl("a:link { text-decoration: none }"));

    if (!_options.useDocumentMargin_)
        styleSheet.append(qsl(" ")).append(qsl("body { margin-left: %1; margin-top: %2; margin-right: %3; margin-bottom: %4; }")
                                            .arg(_options.leftBodyMargin()).arg(_options.topBodyMargin())
                                            .arg(_options.rightBodyMargin()).arg(_options.bottomBodyMargin()));

    return styleSheet;
}

namespace TextBrowserExUtils
{

int setAppropriateHeight(TextBrowserEx &_textBrowser)
{
    const auto& margins = _textBrowser.getOptions().getMargins();
    auto textHeightMetrics = Utils::evaluateTextHeightMetrics(_textBrowser.toPlainText(),
                                                              _textBrowser.width() - margins.left() - margins.right(),
                                                              QFontMetrics(_textBrowser.getOptions().font_));

    auto maxHeight = textHeightMetrics.getHeightPx() + margins.top() + margins.bottom();
    _textBrowser.setMaximumHeight(maxHeight);

    return maxHeight;
}

}

QMargins TextBrowserEx::Options::getMargins() const
{
    if (useDocumentMargin_)
        return { QTEXTDOCUMENT_DEFAULT_MARGIN, QTEXTDOCUMENT_DEFAULT_MARGIN,
                 QTEXTDOCUMENT_DEFAULT_MARGIN, QTEXTDOCUMENT_DEFAULT_MARGIN };

    return { leftBodyMargin() , topBodyMargin(), rightBodyMargin(), bottomBodyMargin() };
}

}
