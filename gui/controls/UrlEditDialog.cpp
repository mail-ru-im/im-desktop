#include "UrlEditDialog.h"

#include "TextEditEx.h"
#include "../fonts.h"
#include "../styles/ThemeParameters.h"

namespace
{
    auto getTextToLineSpacing() noexcept
    {
        return Utils::scale_value(8);
    }

    auto getHintSpacing() noexcept
    {
        return Utils::scale_value(8);
    }

    auto getUrlFont()
    {
        return Fonts::appFontScaled(16);
    }

    auto getHintFont()
    {
        return Fonts::appFontScaled(14);
    }

    auto getHintColorBadHex()
    {
        return Styling::getParameters().getColorHex(Styling::StyleVariable::SECONDARY_ATTENTION);
    }

    //! The same as on backend
    const QRegularExpression& urlRegexp()
    {
        static QRegularExpression nickRule;
        if (nickRule.pattern().isEmpty())
        {
            QString pattern;
            const auto pctEncoded = qsl("%[[:xdigit:]]{2}");
            const auto genDelims = qsl("\\:\\/\\?\\#\\[\\]\\@");
            const auto subDelims = qsl("\\!\\$\\&\\'\\(\\)\\*\\+\\,\\;\\=");
            const auto reserved = QString(genDelims % subDelims);
            const auto unreserved = qsl("\\pL\\pM\\p{So}\\pN\\_\\-\\.\\~");
            const auto pchar = QString(unreserved % subDelims % qsl("\\:\\@\\/"));
            const auto ipv6 = qsl("(?>[[:xdigit:]]{1,4}(?>\\:{1,2}[[:xdigit:]]{1,4})+)");

            const auto scheme = qsl("(?>[\\pL\\pM\\p{So}]{1}[\\pL\\pM\\p{So}\\pN\\+\\-\\.]*)");
            const auto userInfo = QString(qsl("(?>(?>[") % unreserved % subDelims % qsl("\\:]{1}|") % pctEncoded % qsl(")+)"));
            const auto label = qsl("(?>[\\pL\\pM\\p{So}\\pN]{1}(?:[\\pL\\pM\\p{So}\\pN-]*[\\pL\\pM\\p{So}\\pN]{1})?)");
            const auto host = QString(qsl("(?>") % label % qsl("(?>\\.") % label % qsl(")+)|(?>\\[") % ipv6 % qsl("\\])|(?>localhost)"));
            const auto port = qsl("(?>[[:digit:]]*)");
            const auto path = QString(qsl("(?>\\/(?>[") % pchar % qsl("]+|") % pctEncoded % qsl(")*)"));
            const auto query = QString(qsl("(?>(?>[") % pchar % qsl("\\?]+|") % pctEncoded % qsl(")*)"));
            const auto fragment = QString(qsl("(?>(?>[") % pchar % qsl("\\?]+|") % pctEncoded % qsl(")*)"));

            pattern = qsl("(?>(") % scheme % qsl("):\\/\\/)?(?>(") % userInfo % qsl(")@)?(") % host % qsl(")(?>:(")
                % port % qsl("))?(") % path % qsl(")?(?:\\?(") % query % qsl("))?(?:#(") % fragment % qsl("))?");

            nickRule.setPattern(pattern);
        }
        return nickRule;
    }
}

namespace Ui
{
    std::unique_ptr<UrlEditDialog> UrlEditDialog::create(QWidget* _mainWindow, QWidget* _parent, QStringView _linkDisplayName, InOut QString& _url)
    {
        auto mainWidget = new QWidget(_parent);
        mainWidget->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::Preferred);

        auto layout = Utils::emptyVLayout(mainWidget);
        layout->setContentsMargins(Utils::scale_value(16), Utils::scale_value(16), Utils::scale_value(16), 0);

        const auto textColor = Styling::ThemeColorKey{ Styling::StyleVariable::TEXT_SOLID };
        auto url = new TextEditEx(mainWidget, Fonts::appFontScaled(16), textColor, true, true);
        Utils::ApplyStyle(url, Styling::getParameters().getTextEditCommonQss(true));
        Testing::setAccessibleName(url, qsl("AS General urlEdit"));
        url->setPlaceholderText(QT_TRANSLATE_NOOP("popup_window", "Link"));
        url->setAlignment(Qt::AlignLeft | Qt::AlignTop);
        url->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        url->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        url->setFontUnderline(false);
        url->setWordWrapMode(QTextOption::WrapMode::WrapAnywhere);
        url->setSizePolicy(QSizePolicy::Policy::Fixed, QSizePolicy::Policy::MinimumExpanding);
        url->setMinimumHeight(QFontMetrics(getUrlFont()).height() + getTextToLineSpacing());
        url->setMinimumWidth(Utils::scale_value(300));
        url->setHeightSupplement(Utils::scale_value(4));
        url->setEnterKeyPolicy(TextEditEx::EnterKeyPolicy::CatchEnterAndNewLine);
        url->setContentsMargins(QMargins());
        url->setAttribute(Qt::WA_MacShowFocusRect, false);
        url->setDocumentMargin(0);
        url->setTextInteractionFlags(Qt::TextEditorInteraction);
        url->setUndoRedoEnabled(true);

        auto hint = new QLabel(mainWidget);
        hint->setFont(getHintFont());
        hint->setAlignment(Qt::AlignLeft);
        hint->setMinimumWidth(Utils::scale_value(300));
        hint->setMinimumHeight(QFontMetrics(getUrlFont()).height() + getTextToLineSpacing());
        hint->setContentsMargins(QMargins());

        layout->addWidget(url);
        layout->addSpacing(getHintSpacing());
        layout->addWidget(hint);

        auto updateControls = [hint, url](bool _isCorrect)
        {
            const auto hintText = _isCorrect ? QString() : QT_TRANSLATE_NOOP("popup_window", "Invalid url");
            const auto coloredText = QString(u"<font color=" % getHintColorBadHex() % u'>' % hintText % u"</font>");
            hint->setText(coloredText);

            Utils::ApplyStyle(url, Styling::getParameters().getTextLineEditCommonQssNoHeight(!_isCorrect));
        };

        auto dialog = std::make_unique<Ui::UrlEditDialog>(mainWidget, _mainWindow, url, hint);
        dialog->addLabel(QT_TRANSLATE_NOOP("popup_window", "Add link"));
        dialog->addText(
            QT_TRANSLATE_NOOP("popup_window", "The link will be displayed as \"%1\"").arg(_linkDisplayName),
            Utils::scale_value(12));
        auto okButton = dialog->addButtonsPair(QT_TRANSLATE_NOOP("popup_window", "Cancel"), QT_TRANSLATE_NOOP("popup_window", "Ok")).first;

        QObject::connect(url, &QTextEdit::textChanged, dialog.get(), [dialog = dialog.get(), okButton]()
        {
            dialog->updateControls(true);
            okButton->setEnabled(!dialog->getUrl().isEmpty());
        });

        updateControls(true);
        url->setText(_url);
        url->selectAll();
        url->setFocus();

        return dialog;
    }
    bool UrlEditDialog::isUrlValid() const
    {
        const auto url = getUrl();
        return urlRegexp().match(url).capturedLength() == url.size();
    }

    UrlEditDialog::UrlEditDialog(QWidget* _mainWidget, QWidget* _parent, Ui::TextEditEx* _urlEdit, QLabel* _hint)
        : GeneralDialog(_mainWidget, _parent)
    {
        url_ = _urlEdit;
        hint_ = _hint;

        connect(_urlEdit, &Ui::TextEditEx::enter, this, &UrlEditDialog::accept);
    }

    void UrlEditDialog::updateControls(bool _isCorrect)
    {
        const auto hintText = _isCorrect ? QString() : QT_TRANSLATE_NOOP("popup_window", "Invalid url");
        const auto coloredText = QString(u"<font color=" % getHintColorBadHex() % u'>' % hintText % u"</font>");
        hint_->setText(coloredText);

        Utils::ApplyStyle(url_, Styling::getParameters().getTextLineEditCommonQssNoHeight(!_isCorrect));
    }

    QString UrlEditDialog::getUrl() const
    {
        return url_->toPlainText().trimmed();
    }

    void UrlEditDialog::accept()
    {
        const auto url = getUrl();
        const auto isValid = isUrlValid();
        updateControls(isValid);
        if (!url.isEmpty() && isValid)
            GeneralDialog::acceptDialog();
    }
}
