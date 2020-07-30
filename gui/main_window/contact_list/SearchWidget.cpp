#include "stdafx.h"
#include "SearchWidget.h"

#include "controls/CustomButton.h"
#include "controls/TextUnit.h"
#include "fonts.h"
#include "utils/utils.h"
#include "utils/InterConnector.h"
#include "styles/ThemeParameters.h"

namespace
{
    QSize iconSize()
    {
        return Utils::scale_value(QSize(16, 16));
    }

    QSize searchIconSize()
    {
        return Utils::scale_value(QSize(14, 14));
    }
}

namespace Ui
{
    SearchEdit::SearchEdit(QWidget* _parent)
        : LineEditEx(_parent)
        , drawPlaceholderAnyway_(false)
    {
        placeholderTextUnit_ = TextRendering::MakeTextUnit(QString());
        placeholderTextUnit_->init(font(),
                                   Styling::getParameters().getColor(Styling::StyleVariable::BASE_PRIMARY),
                                   QColor(), QColor(), QColor(), TextRendering::HorAligment::LEFT);

        setFocusPolicy(Qt::ClickFocus);
    }

    SearchEdit::~SearchEdit() = default;

    void SearchEdit::setPlaceholderTextEx(const QString& _text, bool _drawAnyway)
    {
        if (placeHolderText_ != _text)
        {
            placeHolderText_ = _text;
            placeholderTextUnit_->setText(placeHolderText_);
            drawPlaceholderAnyway_ = _drawAnyway;
            update();
        }
    }

    QString SearchEdit::getPlaceholder() const
    {
        return placeHolderText_;
    }

    void SearchEdit::paintEvent(QPaintEvent* _e)
    {
        LineEditEx::paintEvent(_e);

        if (!placeHolderText_.isEmpty() && (!hasFocus() || drawPlaceholderAnyway_) && text().isEmpty())
        {
            if (const auto f = font(); f != placeholderTextUnit_->getFont())
            {
                placeholderTextUnit_->init(f, Styling::getParameters().getColor(Styling::StyleVariable::BASE_PRIMARY), QColor(), QColor(), QColor(), TextRendering::HorAligment::LEFT, 1);
                placeholderTextUnit_->evaluateDesiredSize();
            }
            const auto r = rect();
            placeholderTextUnit_->setOffsets(cursorRect().center().x(), r.height() / 2);
            QPainter p(this);
            placeholderTextUnit_->draw(p, TextRendering::VerPosition::MIDDLE);
        }
    }

    void SearchEdit::keyPressEvent(QKeyEvent* _e)
    {
        LineEditEx::keyPressEvent(_e);
        if (_e->key() == Qt::Key_Escape)
            _e->accept();
    }


    SearchWidget::SearchWidget(QWidget* _parent, int _add_hor_space, int _add_ver_space)
        : QWidget(_parent)
        , active_(false)
        , needsClear_(false)
    {
        setFixedHeight(Utils::scale_value(32) + _add_ver_space * 2);

        vMainLayout_ = Utils::emptyVLayout(this);
        vMainLayout_->setContentsMargins(Utils::scale_value(12) + _add_hor_space, _add_ver_space, Utils::scale_value(12) + _add_hor_space, _add_ver_space);
        hMainLayout = Utils::emptyHLayout();

        searchWidget_ = new QWidget(this);
        searchWidget_->setPalette(Styling::getParameters().getColor(Styling::StyleVariable::BASE_BRIGHT_INVERSE));
        searchWidget_->setMouseTracking(true);
        searchWidget_->installEventFilter(this);

        hSearchLayout_ = Utils::emptyHLayout(searchWidget_);
        hSearchLayout_->setContentsMargins(Utils::scale_value(12), 0, Utils::scale_value(12), 0);

        searchIcon_ = new QLabel(this);
        searchIcon_->setFixedSize(searchIconSize());
        searchIcon_->setContentsMargins(0, 0, 0, 0);
        searchIcon_->setAttribute(Qt::WA_TranslucentBackground);
        hSearchLayout_->addWidget(searchIcon_);

        searchEdit_ = new SearchEdit(this);
        searchEdit_->setFixedHeight(Utils::scale_value(24));
        searchEdit_->setFont(Fonts::appFontScaled(14));
        searchEdit_->setFrame(QFrame::NoFrame);
        searchEdit_->setContentsMargins(Utils::scale_value(8), 0, 0, 0);
        searchEdit_->setAttribute(Qt::WA_MacShowFocusRect, false);
        searchEdit_->setStyleSheet(qsl("background:transparent"));

        hSearchLayout_->addSpacing(0);

        Testing::setAccessibleName(searchEdit_, qsl("AS Search searchEdit"));

        hSearchLayout_->addWidget(searchEdit_);

        clearIcon_ = new Ui::CustomButton(this, qsl(":/controls/reset_edit_icon"), QSize(14, 14));
        Styling::Buttons::setButtonDefaultColors(clearIcon_);
        clearIcon_->setAutoFillBackground(true);
        clearIcon_->setDisabledColor(Qt::red);
        clearIcon_->setContentsMargins(Utils::scale_value(8), 0, 0, 0);
        clearIcon_->setFixedSize(iconSize());
        clearIcon_->setStyleSheet(qsl("border: none;"));

        Testing::setAccessibleName(clearIcon_, qsl("AS Search clearIcon"));
        hSearchLayout_->addWidget(clearIcon_);
        clearIcon_->hide();

        hMainLayout->addWidget(searchWidget_);
        vMainLayout_->addLayout(hMainLayout);

        updateStyle();

        connect(searchEdit_, &LineEditEx::textChanged, this, &SearchWidget::searchChanged);
        connect(searchEdit_, &LineEditEx::clicked, this, &SearchWidget::searchStarted);
        connect(searchEdit_, &LineEditEx::escapePressed, this, &SearchWidget::onEscapePress);
        connect(searchEdit_, &LineEditEx::enter, this, &SearchWidget::enterPressed);
        connect(searchEdit_, &LineEditEx::upArrow, this, &SearchWidget::upPressed);
        connect(searchEdit_, &LineEditEx::downArrow, this, &SearchWidget::downPressed);
        connect(searchEdit_, &LineEditEx::focusOut, this, &SearchWidget::focusedOut);
        connect(searchEdit_, &LineEditEx::focusIn, this, &SearchWidget::focusedIn);

        connect(clearIcon_, &QPushButton::clicked, this, &SearchWidget::clearInput);
        connect(clearIcon_, &QPushButton::clicked, this, &SearchWidget::inputCleared);

        setDefaultPlaceholder();

        setFocusPolicy(Qt::TabFocus);
    }

    SearchWidget::~SearchWidget()
    {
    }

    void SearchWidget::setTabFocus()
    {
        QWidget::setFocus(Qt::TabFocusReason);
    }

    void SearchWidget::setFocus(Qt::FocusReason _reason)
    {
        searchEdit_->setFocus(_reason);

        if (_reason == Qt::MouseFocusReason)
            setActive(true);

        updateStyle();
    }

    void SearchWidget::clearFocus()
    {
        searchEdit_->clearFocus();
        setActive(false);
        updateStyle();
    }

    bool SearchWidget::isActive() const
    {
        return active_;
    }

    void SearchWidget::setDefaultPlaceholder()
    {
        setPlaceholderText(QT_TRANSLATE_NOOP("search_widget", "Search"));
    }

    void SearchWidget::setTempPlaceholderText(const QString & _text)
    {
        savedPlaceholder_ = searchEdit_->getPlaceholder();
        searchEdit_->setPlaceholderTextEx(_text, true);
    }

    void SearchWidget::setPlaceholderText(const QString & _text)
    {
        searchEdit_->setPlaceholderTextEx(_text);
    }

    void SearchWidget::setEditEventFilter(QObject* _obj)
    {
        if (searchEdit_)
            searchEdit_->installEventFilter(_obj);
    }

    void SearchWidget::clearSearch(const bool _force)
    {
        if (_force)
        {
            setNeedClear(true);
            Q_EMIT searchCompleted();
        }
        setNeedClear(false);
    }

    void SearchWidget::enterEvent(QEvent * _event)
    {
        updateStyle();
    }

    void SearchWidget::leaveEvent(QEvent * _event)
    {
        updateStyle();
    }

    bool SearchWidget::eventFilter(QObject * _obj, QEvent * _ev)
    {
        if (_obj == searchWidget_)
        {
            if (_ev->type() == QEvent::Enter || _ev->type() == QEvent::Leave)
            {
                updateStyle();
                return true;
            }
            else if (_ev->type() == QEvent::MouseButtonPress)
            {
                setFocus();
                return true;
            }
        }

        return QWidget::eventFilter(_obj, _ev);
    }

    void SearchWidget::focusInEvent(QFocusEvent* _e)
    {
        if (_e->reason() == Qt::ActiveWindowFocusReason)
        {
            if (const auto next = nextInFocusChain(); !next || next->focusPolicy() == Qt::NoFocus)
            {
                _e->accept();
                QWidget::clearFocus();
                return;
            }
        }
        else if (_e->reason() == Qt::OtherFocusReason)
        {
            _e->accept();
            QWidget::clearFocus();
            return;
        }

        updateStyle();
    }

    void SearchWidget::focusOutEvent(QFocusEvent*)
    {
        updateStyle();
        if (!savedPlaceholder_.isEmpty())
        {
            setPlaceholderText(savedPlaceholder_);
            savedPlaceholder_.clear();
        }
    }

    void SearchWidget::keyPressEvent(QKeyEvent* _e)
    {
        _e->ignore();
        if (hasFocus())
        {
            if (_e->key() == Qt::Key_Enter || _e->key() == Qt::Key_Return)
            {
                setFocus(Qt::MouseFocusReason);
                _e->accept();
            }
            else if (_e->key() == Qt::Key_Down)
            {
                _e->accept();
                Q_EMIT selectFirstInRecents(QPrivateSignal());
                QWidget::clearFocus();
            }
            else if (_e->key() == Qt::Key_Escape)
            {
                _e->accept();
                Utils::InterConnector::instance().setFocusOnInput();
                QWidget::clearFocus();
            }
        }
    }

    void SearchWidget::updateStyle()
    {
        const auto makeIcon = [](const auto _var)
        {
            return Utils::renderSvg(qsl(":/search_input"), searchIconSize(), Styling::getParameters().getColor(_var));
        };

        const auto getBackground = [](const bool _active, const bool _hover)
        {
            auto bgVar = Styling::StyleVariable::BASE_BRIGHT_INVERSE;
            if (_active)
                bgVar = Styling::StyleVariable::BASE_BRIGHT_INVERSE;
            else if (_hover)
                bgVar = Styling::StyleVariable::BASE_BRIGHT_INVERSE_HOVER;

            return Styling::getParameters().getColor(bgVar);
        };

        static const auto normalSearchIcon = makeIcon(Styling::StyleVariable::BASE_PRIMARY);
        static const auto hoverSearchIcon = makeIcon(Styling::StyleVariable::BASE_SECONDARY);
        static const auto activeSearchIcon = makeIcon(Styling::StyleVariable::BASE_PRIMARY);

        const auto hover = searchWidget_->underMouse();
        searchIcon_->setPixmap(active_ ? activeSearchIcon : (hover ? hoverSearchIcon : normalSearchIcon));

        const auto focused = hasFocus() && !active_;
        const auto bg = focused? Styling::getParameters().getPrimaryTabFocusColor() : getBackground(active_, hover);

        const QString style = u"QWidget { border-radius: 16dip; background-color: "
            % bg.name(QColor::HexArgb) % u";color: " % Styling::getParameters().getColorHex(Styling::StyleVariable::TEXT_SOLID)
            % u'}';
        Utils::ApplyStyle(this, style);
    }

    void SearchWidget::setActive(bool _active)
    {
        if (active_ == _active)
            return;

        active_ = _active;
        updateStyle();

        updateClearIconVisibility();

        Q_EMIT activeChanged(active_);

        if (active_)
            Utils::InterConnector::instance().setCurrentMultiselectMessage(-1);
    }

    void SearchWidget::updateClearIconVisibility()
    {
        clearIcon_->setVisible((active_ || searchEdit_->hasFocus()) && !searchEdit_->text().isEmpty());
    }

    void SearchWidget::searchStarted()
    {
        setActive(true);
    }

    QString SearchWidget::getText() const
    {
        return searchEdit_->text();
    }

    void SearchWidget::setText(const QString& _text)
    {
        searchEdit_->setText(_text);
    }

    bool SearchWidget::isEmpty() const
    {
        return getText().isEmpty();
    }

    void SearchWidget::clearInput()
    {
        searchEdit_->clear();
    }

    void SearchWidget::searchCompleted()
    {
        if (needsClear_)
            searchCompletedImpl(SetFocusToInput::Yes);
    }

    void SearchWidget::searchCompletedImpl(const SetFocusToInput _setFocus)
    {
        clearInput();
        setActive(false);
        searchEdit_->clearFocus();
        clearIcon_->hide();
        searchChanged(QString());
        Q_EMIT searchEnd();

        if (Utils::InterConnector::instance().isMultiselect())
            Q_EMIT Utils::InterConnector::instance().multiSelectCurrentElementChanged();

        if (_setFocus == SetFocusToInput::Yes)
            Utils::InterConnector::instance().setFocusOnInput();
    }

    void SearchWidget::onEscapePress()
    {
        setNeedClear(true);
        searchCompleted();
        Q_EMIT escapePressed();
    }

    void SearchWidget::searchChanged(const QString& _text)
    {
        if (searchedText_ == _text)
            return;

        if (!_text.isEmpty())
            setActive(true);

        updateClearIconVisibility();

        searchedText_ = _text;

        if (!_text.isEmpty())
            Q_EMIT searchBegin();
        else
            Q_EMIT inputEmpty();

        Q_EMIT search(searchedText_);
    }

    void SearchWidget::focusedOut()
    {
        if (needsClear_)
        {
            searchCompletedImpl(SetFocusToInput::No);
            updateClearIconVisibility();
        }
        updateStyle();
    }

    void SearchWidget::focusedIn()
    {
        updateClearIconVisibility();
        updateStyle();
    }
}
