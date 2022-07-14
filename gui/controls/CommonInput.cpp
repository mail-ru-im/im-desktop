#include "stdafx.h"
#include "CommonInput.h"

#include "../utils/utils.h"
#include "../utils/PhoneFormatter.h"
#include "fonts.h"
#include "styles/ThemeParameters.h"
#include "main_window/contact_list/CountryListModel.h"
#include "../main_window/Placeholders.h"
#include "PhoneCodePicker.h"

namespace
{
    constexpr int defaultCodeLength = 6;

    constexpr int shadowWidth = 7;

    int getHorLeftMargin()
    {
        return Utils::scale_value(20);
    }

    int getHorLeftMarginMinor()
    {
        return Utils::scale_value(16);
    }

    int getHorRightMargin(const Ui::InputRole _role)
    {
        return Utils::scale_value(_role == Ui::InputRole::Code ? 12 : 20);
    }

    int getPadding()
    {
        return Utils::scale_value(4);
    }

    int getVerMargin()
    {
        return Utils::scale_value(12);
    }

    int getMinHeight()
    {
        return Utils::scale_value(88);
    }

    int getMaxHeight()
    {
        return Utils::scale_value(112);
    }

    constexpr int unknownCodeLength = 3;
    constexpr int fontSize = 16;

    QSize getControlSize()
    {
        return QSize(24, 24);
    }

    QSize getFlagSize()
    {
        return QSize(40, 40);
    }
}

namespace Ui
{
    BaseInput::BaseInput(QWidget *_parent)
        : QWidget(_parent)
        , currentInput_(0)
    {
        globalLayout_ = Utils::emptyHLayout(this);
        initInput(1);
    }

    void BaseInput::setEchoMode(QLineEdit::EchoMode  _mode)
    {
        for (auto input : inputs_)
            input->setEchoMode(_mode);
    }

    void BaseInput::setPlaceholderText(const QString &_text)
    {
        for (auto input : inputs_)
            input->setCustomPlaceholder(_text);
    }

    void BaseInput::clear()
    {
        for (auto input : inputs_)
            input->clear();
        currentInput_ = 0;
    }

    void BaseInput::setCursorMoveStyle(Qt::CursorMoveStyle _style)
    {
        for (auto input : inputs_)
            input->setCursorMoveStyle(_style);
    }

    int BaseInput::getWidth() const
    {
        if (inputs_.empty())
            return 0;

        auto curWidth = 0;
        for (auto input : inputs_)
            curWidth += input->width();

        return curWidth;
    }

    bool BaseInput::atBegin() const
    {
        return current()->text().isEmpty() || current()->cursorPosition() == 0;
    }

    bool BaseInput::atEnd() const
    {
        return current()->text().isEmpty() || current()->cursorPosition() == current()->text().size();
    }

    bool BaseInput::eventFilter(QObject* _obj, QEvent* _event)
    {
        if (_event->type() == QEvent::KeyPress)
        {
            auto keyEvent = static_cast<QKeyEvent*>(_event);
            if (keyEvent->key() == Qt::Key_Left)
            {
                if (keyEvent->modifiers() & Qt::ControlModifier)
                {
                    currentInput_ = 0;
                    current()->setFocus();
                }
                else if (atBegin())
                {
                    if (inputs_.size() == 1)
                        Q_EMIT leftArrow();
                    else
                        prevInput();
                }
                return false;
            }
            else if (keyEvent->key() == Qt::Key_Right)
            {
                if (keyEvent->modifiers() & Qt::ControlModifier)
                {
                    currentInput_ = lastInput();
                    current()->setFocus();
                }
                else if (atEnd())
                {
                    if (inputs_.size() == 1)
                        Q_EMIT rightArrow();
                    else
                        nextInput();
                }
                return false;
            }
            else if (keyEvent->key() == Qt::Key_Home)
            {
                currentInput_ = 0;
                current()->setFocus();
            }
            else if (keyEvent->key() == Qt::Key_End)
            {
                currentInput_ = lastInput();
                current()->setFocus();
            }
            else if (keyEvent->matches(QKeySequence::SelectAll))
            {
                selectAll();
                currentInput_ = lastInput();
                current()->setFocus();
                return false;
            }
            else if (keyEvent->key() == Qt::Key_Backspace || keyEvent->key() == Qt::Key_Delete)
            {
                if (isAllSelected())
                {
                    clear();
                    current()->setFocus();
                }
                else
                {
                    _event->accept();
                }
                return false;
            }

            return QWidget::eventFilter(_obj, _event);
        }

        return QWidget::eventFilter(_obj, _event);
    }

    void BaseInput::focusInEvent(QFocusEvent * _e)
    {
        auto currentInput = current();
        currentInput->setFocus();
        if (_e->reason() != Qt::ShortcutFocusReason)
            currentInput->setCursorPosition(currentInput->text().size());
    }

    void BaseInput::initInput(const int _size)
    {
        if (!inputs_.empty())
        {
            for (auto input_ : inputs_)
            {
                globalLayout_->removeWidget(input_);
                delete input_;
            }
            inputs_.clear();
        }

        inputs_.reserve(_size);

        for (auto i = 0; i < _size; ++i)
        {
            auto input_ = new LineEditEx(this);
            input_->setFont(Fonts::appFontScaled(fontSize));
            input_->changeTextColor(Styling::ThemeColorKey{ Styling::StyleVariable::TEXT_SOLID });
            input_->setCustomPlaceholderColor(Styling::ThemeColorKey{ Styling::StyleVariable::BASE_PRIMARY });
            Utils::ApplyStyle(input_, Styling::getParameters().getLineEditCustomQss(Styling::StyleVariable::INVALID, Styling::StyleVariable::INVALID));
            input_->installEventFilter(this);
            inputs_.push_back(input_);
        }

        for (auto input_ : inputs_)
        {
            globalLayout_->addWidget(input_);
            connect(input_, &LineEditEx::enter, this, &BaseInput::enterPressed);
            connect(input_, &LineEditEx::textChanged, this, [this]()
            {
                Q_EMIT textChanged(text());
            });
            connect(input_, &LineEditEx::textEdited, this, &BaseInput::textEdited);
            connect(input_, &LineEditEx::textEdited, this, &BaseInput::setText);
            connect(input_, &LineEditEx::emptyTextBackspace, this, &BaseInput::emptyTextBackspace);
            connect(this, &BaseInput::rightArrow, this, &BaseInput::nextInput);
            connect(this, &BaseInput::leftArrow, this, &BaseInput::prevInput);
        }

        for (auto iter = inputs_.begin(); iter != inputs_.end(); ++iter)
        {
            connect(*iter, &LineEditEx::focusIn, this, [this, iter]()
            {
                currentInput_ = std::distance(inputs_.begin(), iter);
            });
        }
    }

    LineEditEx* BaseInput::current() const
    {
        return inputs_[currentInput_];
    }

    bool BaseInput::isLast() const
    {
        return inputs_.size() > 1 && currentInput_ == inputs_.size() - 1;
    }

    void BaseInput::prevInput()
    {
        if (inputs_.empty() || currentInput_ <= 0)
            return;

        --currentInput_;
        current()->setFocus();
        current()->setCursorPosition(text().length());
        current()->deselect();
    }

    void BaseInput::nextInput()
    {
        if (inputs_.empty() || currentInput_ >= inputs_.size() - 1)
            return;

        ++currentInput_;
        current()->setFocus(Qt::ShortcutFocusReason);
        current()->setCursorPosition(text().length());
        current()->deselect();
    }

    void BaseInput::selectAll()
    {
        for (const auto input : inputs_)
            input->selectAll();
    }

    void BaseInput::deselectAll()
    {
        if (isAllSelected())
            currentInput_ = 0;

        for (const auto input : inputs_)
            input->deselect();
    }

    bool BaseInput::isAllSelected() const
    {
        return std::all_of(inputs_.begin(), inputs_.end(), [](const auto input) { return input->selectedText() == input->text(); });
    }

    int BaseInput::lastInput() const
    {
        for (auto iter = inputs_.rbegin(); iter != inputs_.rend(); ++iter)
        {
            if (!(*iter)->text().isEmpty())
                return (inputs_.size() - std::distance(inputs_.rbegin(), iter) - 1);

        }
        return 0;
    }

    void BaseInput::setValidator(const QValidator * _v)
    {
        for (auto input : inputs_)
            input->setValidator(_v);
    }

    void BaseInput::setMaxLength(const int _len)
    {
        for (auto input : inputs_)
            input->setMaxLength(_len);
    }

    void BaseInput::setText(const QString &_text)
    {
        auto currentInput = current();
        const auto pos = currentInput->text().size() - currentInput->cursorPosition();
        currentInput->setText(_text);
        currentInput->setCursorPosition(currentInput->text().size() - pos);
    }

    QString BaseInput::text() const
    {
        QString text;
        for (auto input : inputs_)
            text += input->text();
        return text;
    }

    QFontMetrics BaseInput::getFontMetrics() const
    {
        return inputs_.front()->fontMetrics();
    }

    void BaseInput::setFont(const QFont& _font)
    {
        for (auto input : inputs_)
            input->setFont(_font);
    }

    CommonInput::CommonInput(QWidget *_parent, const InputRole _role)
        : BaseInput(_parent)
        , role_(_role)
        , codeLength_(defaultCodeLength)
    {
        if (role_ == InputRole::Password)
        {
            setEchoMode(QLineEdit::Password);
        }
        else if (role_ == InputRole::Code)
        {
            setCodeLength(codeLength_);
            setCursorMoveStyle(Qt::VisualMoveStyle);
            connect(this, &BaseInput::emptyTextBackspace, this, [this]()
            {
                prevInput();
                current()->clear();
            });
        }
    }

    void CommonInput::setCodeLength(int _len)
    {
        if (role_ == InputRole::Code)
        {
            if (codeLength_ != _len)
                codeLength_ = _len;
            initInput(codeLength_);
            static const auto reg = QRegularExpression(qsl("[\\d]*"));
            static const auto validator = new QRegularExpressionValidator(reg);
            setValidator(validator);
            setFont(Fonts::appFontScaled(fontSize, platform::is_apple() ? Fonts::FontWeight::Medium : Fonts::FontWeight::Normal));
            const auto mask = qsl("_");
            setPlaceholderText(mask);
            setFixedWidth((getFontMetrics().horizontalAdvance(mask) + 2 * getPadding()) * codeLength_);
        }
    }

    void CommonInput::setText(const QString& _text)
    {
        if (role_ != InputRole::Code)
        {
            BaseInput::setText(_text);
            return;
        }

        if (!_text.isEmpty())
        {
            if (isLast())
            {
                current()->setText(_text[0]);
            }
            else
            {
                auto i = 0;
                while (i < _text.size())
                {
                    current()->setText(_text[i]);
                    nextInput();
                    ++i;
                }
            }
        }
        else if (_text.isEmpty())
        {
            prevInput();
        }
    }

    PhoneInput::PhoneInput(QWidget *_parent)
        : BaseInput(_parent)
    {
        static const auto reg = QRegularExpression(qsl("[\\d\\s\\-]*"));
        static const auto validator = new QRegularExpressionValidator(reg);
        setValidator(validator);
        setMaxLength(18);
        setFont(Fonts::appFontScaled(fontSize, platform::is_apple() ? Fonts::FontWeight::Medium : Fonts::FontWeight::Normal));
        connect(this, &BaseInput::textChanged, this, &PhoneInput::setPhone);
    }

    void PhoneInput::setPhone(const QString & _phone)
    {
        if (phone_ != _phone)
        {
            phone_ = _phone;
            BaseInput::setText(phone_);
        }
    }

    CodeInput::CodeInput(QWidget * _parent, const QString & _code)
        : BaseInput(_parent)
        , code_(_code)
    {
        setValidator(new QRegularExpressionValidator(QRegularExpression(qsl("\\+?\\d*"))));
        setFont(Fonts::appFontScaled(fontSize, platform::is_apple() ? Fonts::FontWeight::Medium : Fonts::FontWeight::Normal));
        setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    }

    void CodeInput::setText(const QString & _code)
    {
        auto code = _code == u"8" ? qsl("+7") : _code;
        if (!code.isEmpty() && !code.startsWith(u'+'))
            code = u'+' % code;
        if (code_ != code)
        {
            code_ = code;
            auto m = getFontMetrics();
            setFixedWidth(m.horizontalAdvance(code_) + Utils::scale_value(2));
            Q_EMIT codeChanged(code_);
        }
        BaseInput::setText(code_);
    }

    BaseInputContainer::BaseInputContainer(QWidget * _parent, InputRole _role)
        : QWidget(_parent)
        , leftWidget_(nullptr)
        , rightWidget_(nullptr)
        , code_(nullptr)
        , input_(nullptr)
        , leftSpacer_(nullptr)
        , rightSpacer_(nullptr)
    {
        setSizePolicy(QSizePolicy::Policy::Expanding, QSizePolicy::Policy::Fixed);
        setFixedHeight(Utils::scale_value(62));

        globalLayout_ = Utils::emptyHLayout(this);
        globalLayout_->setContentsMargins(getHorLeftMargin(), 0, getHorRightMargin(_role), 0);
        globalLayout_->setAlignment(Qt::AlignBaseline | Qt::AlignLeft);

        if (_role == InputRole::Phone)
        {
            leftWidget_ = new PhoneCodePicker(_parent);
            leftWidget_->setFixedSize(Utils::scale_value(getFlagSize()));
            Testing::setAccessibleName(leftWidget_, qsl("AS General phoneCodePicker"));
            globalLayout_->addWidget(leftWidget_);
            globalLayout_->addItem(new QSpacerItem(Utils::scale_value(5), 0, QSizePolicy::Fixed, QSizePolicy::Minimum));

            code_ = new CodeInput(this);
            setPhoneCode(getPhoneCodePicker()->getCode());
            Testing::setAccessibleName(code_, qsl("AS General phoneCodeInput"));
            globalLayout_->addWidget(code_);

            input_ = new PhoneInput(this);
            Testing::setAccessibleName(input_, qsl("AS General phoneInput"));

            connect(getPhoneCodePicker(), &PhoneCodePicker::countrySelected, code_, &CodeInput::setText);
            connect(getPhoneCodePicker(), &PhoneCodePicker::countrySelected, this, [this]()
            {
                setPhone(input_->text());
            });

            connect(code_, &BaseInput::textEdited, this, &BaseInputContainer::updateCountryCode);
            connect(code_, &BaseInput::textChanged, this, &BaseInputContainer::updateCountryCode);
            connect(code_, &BaseInput::rightArrow, this, [this]()
            {
                if (code_->atEnd())
                    input_->setFocus(Qt::ShortcutFocusReason);
            });
        }
        else
        {
            input_ = new CommonInput(this, _role);

            if (_role == InputRole::Password)
            {
                Testing::setAccessibleName(input_, qsl("AS General passwordInput"));
            }
            else if (_role == InputRole::Common)
            {
                Testing::setAccessibleName(input_, qsl("AS General commonInput"));
            }

            if (_role == InputRole::Code)
            {
                globalLayout_->setAlignment(Qt::AlignVCenter | Qt::AlignRight);
                leftSpacer_ = new QSpacerItem(0, 0, QSizePolicy::Fixed, QSizePolicy::Fixed);
                globalLayout_->addItem(leftSpacer_);

                rightWidget_ = new RotatingSpinner(this);
                rightWidget_->setFixedSize(Utils::scale_value(getControlSize()));
                rightWidget_->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
                Testing::setAccessibleName(rightWidget_, qsl("AS General code"));
            }
        }

        input_->setAttribute(Qt::WA_MacShowFocusRect, false);

        globalLayout_->addWidget(input_);

        if (rightWidget_)
        {
            rightSpacer_ = new QSpacerItem(0, 0, QSizePolicy::Preferred, QSizePolicy::Expanding);
            globalLayout_->addItem(rightSpacer_);
            globalLayout_->addWidget(rightWidget_);
            rightWidget_->setVisible(false);
        }

        connect(input_, &BaseInput::enterPressed, this, &BaseInputContainer::enterPressed);
        connect(input_, &BaseInput::textEdited, this, &CommonInputContainer::textEdited);
        connect(input_, &BaseInput::textEdited, this, [this](){updateSpacerWidth(ResizeMode::Common);});
        if (_role == InputRole::Phone)
            connect(input_, &BaseInput::textChanged, this, &CommonInputContainer::setPhone);
        connect(input_, &BaseInput::textChanged, this, &CommonInputContainer::textChanged);

        connect(input_, &BaseInput::textChanged, this, [this](){updateSpacerWidth(ResizeMode::Common);});
        connect(input_, &BaseInput::emptyTextBackspace, this, [this]()
        {
            if (code_)
                code_->setFocus();
        });
        connect(input_, &BaseInput::leftArrow, this, [this]()
        {
            if (code_ && input_->atBegin())
                code_->setFocus();
        });

        updateSpacerWidth(ResizeMode::Common);

        auto shadowEffect = new QGraphicsDropShadowEffect(this);
        shadowEffect->setBlurRadius(Utils::scale_value(3 * shadowWidth));
        shadowEffect->setOffset(0, Utils::scale_value(1));
        auto shadowColor = QColor(0, 0, 0, 255 * 0.22);
        shadowEffect->setColor(shadowColor);

        setGraphicsEffect(shadowEffect);
    }

    void BaseInputContainer::setPlaceholderText(const QString &_text)
    {
        input_->setPlaceholderText(_text);
    }

    void BaseInputContainer::clear()
    {
        input_->clear();
    }

    void BaseInputContainer::paintEvent(QPaintEvent *_event)
    {
        QPainter p(this);
        p.setRenderHint(QPainter::Antialiasing);

        const auto adjust = Utils::scale_value(shadowWidth);
        QRect rRect = rect().adjusted(0, adjust, 0, -adjust);
        const auto radius = rRect.height()/2;

        QPainterPath path;
        path.addRoundedRect(rRect, radius, radius);
        Utils::drawBubbleShadow(p, path, radius, Utils::scale_value(1), QColor(0, 0, 0, 255 * 0.01));

        p.setCompositionMode(QPainter::CompositionMode_Source);
        auto bgColor = Styling::getParameters().getColor(Styling::StyleVariable::BASE_GLOBALWHITE_SECONDARY);
        p.setPen(Qt::NoPen);
        p.fillPath(path, bgColor);
    }

    void BaseInputContainer::focusInEvent(QFocusEvent * _event)
    {
        input_->setFocus();
    }

    void BaseInputContainer::mouseReleaseEvent(QMouseEvent * _event)
    {
        setFocus();
    }

    void BaseInputContainer::updateCountryCode()
    {
        if (auto picker = getPhoneCodePicker())
        {
            const auto currentCode = code_->text();
            if (currentCode.isEmpty())
            {
                code_->setFocus();
                return;
            }

            static QString lastKnownCode;
            picker->setCode(currentCode);
            if (currentCode.size() > unknownCodeLength + 1 && !picker->isKnownCode())
            {
                const auto codeLength = lastKnownCode.isEmpty() ? unknownCodeLength + 1 : lastKnownCode.size();
                const auto tmpCode = currentCode.left(codeLength);
                const auto phone = currentCode.mid(codeLength);
                code_->setText(tmpCode);
                setPhone(phone);
                input_->setFocus();
            }
            else if (currentCode != u"+")
            {
                lastKnownCode = currentCode;
                if (picker->isKnownCode(lastKnownCode) || currentCode.size() >= unknownCodeLength + 1)
                {
                    input_->setFocus();
                    setPhone(input_->text());
                }
                else
                {
                    code_->setFocus();
                }
            }
        }
    }

    void BaseInputContainer::setText(const QString &_text)
    {
        input_->setText(_text);
    }

    QString BaseInputContainer::text() const
    {
        return input_->text();
    }
    void BaseInputContainer::setPhone(const QString& _phone)
    {
        QString code = getPhoneCode();
        QString formattedPhone = _phone;

        if (!code.isEmpty())
            formattedPhone = PhoneFormatter::getFormattedPhone(code, _phone);

        input_->setPhone(formattedPhone);
    }

    void BaseInputContainer::setPhoneCode(const QString& _code)
    {
        if (code_)
            code_->setText(_code);

        if (auto picker = getPhoneCodePicker())
            picker->setCode(_code);
    }

    QString BaseInputContainer::getPhoneCode()
    {
        if (code_)
            return code_->text();

        return QString();
    }

    QString BaseInputContainer::getPhone()
    {
        return input_->text().remove(QRegularExpression(qsl("[\\s\\-]")));
    }

    void BaseInputContainer::setCodeLength(int _len)
    {
        input_->setCodeLength(_len);
    }

    void BaseInputContainer::startSpinner() const
    {
        if (rightWidget_)
        {
            rightWidget_->show();
            updateSpacerWidth(ResizeMode::Force);
            rightWidget_->startAnimation();
        }
    }

    void BaseInputContainer::stopSpinner() const
    {
        if (rightWidget_)
        {
            rightWidget_->hide();
            updateSpacerWidth(ResizeMode::Common);
            rightWidget_->stopAnimation();
        }
    }

    void BaseInputContainer::setPhoneFocus() const
    {
        input_->setFocus();
    }

    PhoneCodePicker *BaseInputContainer::getPhoneCodePicker() const
    {
        return qobject_cast<PhoneCodePicker*>(leftWidget_);
    }

    void BaseInputContainer::updateSpacerWidth(const ResizeMode _force) const
    {
        if (leftSpacer_ && rightSpacer_)
        {
            int dX = _force == ResizeMode::Force ? getControlSize().width() - getHorLeftMargin() : 0;

            auto spacerW = (width() - input_->getWidth() - getHorRightMargin(InputRole::Code) - getHorLeftMargin()) / 2;
            leftSpacer_->changeSize(spacerW, 0, QSizePolicy::Fixed, QSizePolicy::Expanding);
            rightSpacer_->changeSize(spacerW - dX, 0, QSizePolicy::Preferred, QSizePolicy::Expanding);
            layout()->invalidate();
        }
    }

    TextEditInputContainer::TextEditInputContainer(QWidget* _parent)
        :QWidget(_parent)
    {
        input_ = new TextEditEx(this, Fonts::appFontScaled(fontSize), Styling::ThemeColorKey{ Styling::StyleVariable::TEXT_SOLID }, true, false);
        input_->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
        input_->setMinimumHeight(getMinHeight());
        input_->setMaxHeight(getMaxHeight() - getVerMargin());
        input_->setTextInteractionFlags(Qt::TextEditable | Qt::TextEditorInteraction);
        input_->setSizePolicy(QSizePolicy::Policy::Expanding, QSizePolicy::Policy::Preferred);
        input_->setEnterKeyPolicy(TextEditEx::EnterKeyPolicy::CatchNewLine);
        Utils::transparentBackgroundStylesheet(input_);
        Testing::setAccessibleName(input_, qsl("AS input textEdit"));

        auto globalLayout = Utils::emptyHLayout(this);
        globalLayout->setAlignment(Qt::AlignTop | Qt::AlignLeft);
        globalLayout->setContentsMargins(getHorLeftMarginMinor(), getVerMargin(), getHorRightMargin(Ui::InputRole::Code), 0);
        globalLayout->addWidget(input_);

        auto shadowEffect = new QGraphicsDropShadowEffect(this);
        shadowEffect->setBlurRadius(Utils::scale_value(3 * shadowWidth));
        shadowEffect->setOffset(0, Utils::scale_value(1));
        auto shadowColor = QColor(0, 0, 0, 255 * 0.22);
        shadowEffect->setColor(shadowColor);

        setGraphicsEffect(shadowEffect);

        connect(input_->document(), &QTextDocument::contentsChange, this, &TextEditInputContainer::updateInputHeight);
        connect(input_, &TextEditEx::textChanged, this, &TextEditInputContainer::updateInputHeight);
        setFixedHeight(getMaxHeight());
        setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
        updateInputHeight();
    }

    void TextEditInputContainer::setText(const QString& _text)
    {
        input_->setPlainText(_text);
    }

    QString TextEditInputContainer::getPlainText() const
    {
        return input_->getPlainText();
    }

    QTextDocument* TextEditInputContainer::document() const
    {
        return input_->document();
    }

    void TextEditInputContainer::setPlaceholderText(const QString& _text)
    {
        input_->setPlaceholderText(_text);
    }

    void TextEditInputContainer::clear()
    {
        input_->clear();
    }

    void TextEditInputContainer::paintEvent(QPaintEvent *_event)
    {
        QPainter p(this);
        p.setRenderHint(QPainter::Antialiasing);

        const auto adjust = Utils::scale_value(shadowWidth);
        QRect rRect = rect().adjusted(0, adjust, 0, -adjust);
        const auto radius = Utils::scale_value(16);

        QPainterPath path;
        path.addRoundedRect(rRect, radius, radius);
        Utils::drawBubbleShadow(p, path, radius, Utils::scale_value(1), QColor(0, 0, 0, 255 * 0.01));

        p.setCompositionMode(QPainter::CompositionMode_Source);
        auto bgColor = Styling::getParameters().getColor(Styling::StyleVariable::BASE_GLOBALWHITE_SECONDARY);
        p.setPen(Qt::NoPen);
        p.fillPath(path, bgColor);
    }

    void TextEditInputContainer::focusInEvent(QFocusEvent *_event)
    {
        input_->setFocus();
    }

    void TextEditInputContainer::mouseReleaseEvent(QMouseEvent *_event)
    {
        setFocus();
    }

    void TextEditInputContainer::updateInputHeight()
    {
        const int textControlHeight = input_->document()->size().height();
        if (textControlHeight > input_->maximumHeight())
            input_->setMinimumHeight(input_->maximumHeight());
        else if (textControlHeight > getMinHeight())
            input_->setMinimumHeight(textControlHeight);
        else
            input_->setMinimumHeight(getMinHeight());
        const auto newHeight = std::min(std::max(textControlHeight, getMinHeight()), getMaxHeight() - 2 * getVerMargin());
        input_->setFixedHeight(newHeight);
        update();
    }
}
