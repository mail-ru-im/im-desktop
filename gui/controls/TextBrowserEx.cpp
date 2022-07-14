#include "stdafx.h"

#include "TextBrowserEx.h"
#include "utils/Text.h"
#include "fonts.h"
#include "styles/ThemeParameters.h"
#include "styles/ThemesContainer.h"

namespace
{
    bool needUnderline(Ui::TextBrowserEx::Options::LinksUnderline _underline)
    {
        switch (_underline)
        {
        case Ui::TextBrowserEx::Options::LinksUnderline::NoUnderline:
            return false;
        case Ui::TextBrowserEx::Options::LinksUnderline::AlwaysUnderline:
            return true;
        case Ui::TextBrowserEx::Options::LinksUnderline::ThemeDependent:
            return Styling::getThemesContainer().getCurrentTheme()->isLinksUnderlined();
        default:
            im_assert(!"Bad underline parameter");
            break;
        }
        return true;
    }

    QString generateLinkStyleSheet(const Ui::TextBrowserEx::Options& _options)
    {
        const auto themeParameters = Styling::getParameters();
        QString styleSheet;
        styleSheet += ql1s("a:link { color: %1; }").arg(themeParameters.getColorHex(_options.linkColor_));
        if (!needUnderline(_options.linksUnderline_))
            styleSheet.append(QChar::Space).append(ql1s("a:link { text-decoration: none }"));
        return styleSheet;
    }

    QString generateStyleSheet(const Ui::TextBrowserEx::Options& _options)
    {
        const auto themeParameters = Styling::getParameters();
        QString styleSheet;

        if (_options.borderless_)
        {
            styleSheet.append(QChar::Space)
                .append(ql1s("QWidget { border: none; background-color: %1; color: %2; }")
                            .arg(themeParameters.getColorHex(_options.backgroundColor_), themeParameters.getColorHex(_options.textColor_)));
        }

        if (!_options.useDocumentMargin_)
        {
            styleSheet.append(QChar::Space)
                .append(qsl("body { margin-left: %1; margin-top: %2; margin-right: %3; margin-bottom: %4; }")
                            .arg(_options.leftBodyMargin())
                            .arg(_options.topBodyMargin())
                            .arg(_options.rightBodyMargin())
                            .arg(_options.bottomBodyMargin()));
        }

        return styleSheet;
    }
} // namespace

namespace Ui
{

TextBrowserEx::TextBrowserEx(const TextBrowserEx::Options& _options, QWidget *_parent)
    : QTextBrowser(_parent)
    , options_(_options)
{
    setFont(_options.font_);
    setOpenExternalLinks(_options.openExternalLinks_);
    setOpenLinks(_options.openExternalLinks_);
    updateStyle();

    // seems to affect margins set via stylesheet, see QTBUG-30831
    // workaround by subtracting default value of 4 in stylesheet
    if (options_.useDocumentMargin_)
        document()->setDocumentMargin(_options.documentMargin_);
}

const TextBrowserEx::Options &TextBrowserEx::getOptions() const
{
    return options_;
}

void TextBrowserEx::setLineSpacing(int lineSpacing)
{
    QTextBlockFormat * newFormat = new QTextBlockFormat();
    textCursor().clearSelection();
    textCursor().select(QTextCursor::Document);
    newFormat->setLineHeight(lineSpacing, QTextBlockFormat::FixedHeight);
    textCursor().setBlockFormat(*newFormat);
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

TextBrowserEx::Options::Options()
    : linkColor_(Styling::StyleVariable::PRIMARY_INVERSE)
    , textColor_(Styling::StyleVariable::TEXT_SOLID)
    , backgroundColor_(Styling::StyleVariable::BASE_GLOBALWHITE)
    , font_(Fonts::appFontScaled(15))
{
}

QMargins TextBrowserEx::Options::getMargins() const
{
    if (useDocumentMargin_)
        return { QTEXTDOCUMENT_DEFAULT_MARGIN, QTEXTDOCUMENT_DEFAULT_MARGIN,
                 QTEXTDOCUMENT_DEFAULT_MARGIN, QTEXTDOCUMENT_DEFAULT_MARGIN };

    return { leftBodyMargin() , topBodyMargin(), rightBodyMargin(), bottomBodyMargin() };
}

bool TextBrowserEx::event(QEvent* _event)
{
    const auto type = _event->type();
    if ((type == QEvent::UpdateLater || type == QEvent::Show) && themeChecker_.checkAndUpdateHash())
        updateStyle();
    return QTextBrowser::event(_event);
}
void TextBrowserEx::setHtmlSource(const QString& _html)
{
    html_ = _html;
    QTextEdit::setHtml(_html);
}

void TextBrowserEx::updateStyle()
{
    const auto textAlignment = alignment();
    setStyleSheet(generateStyleSheet(options_));
    document()->setDefaultStyleSheet(generateLinkStyleSheet(options_));
    QTextEdit::setHtml(html_);
    setAlignment(textAlignment);
}
} // namespace Ui
