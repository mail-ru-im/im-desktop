#include "stdafx.h"

#include "main_window/MainWindow.h"
#include "user_activity/user_activity.h"
#include "controls/ContactAvatarWidget.h"
#include "controls/TooltipWidget.h"
#include "controls/DialogButton.h"

#include "controls/LineEditEx.h"
#include "controls/TextEditEx.h"
#include "styles/ThemeParameters.h"
#include "utils/InterConnector.h"
#include "utils/utils.h"
#include "core_dispatcher.h"
#include "gui_settings.h"
#include "LocalPIN.h"
#include "my_info.h"
#include "fonts.h"

namespace
{
    constexpr std::chrono::milliseconds activityCheckInterval = std::chrono::seconds(5);
    constexpr auto lineEditHeight = 32;
    constexpr auto lineEditWidth = 260;
    constexpr auto lineEditVerifyWidth = 180;
}

namespace Ui
{

class LocalPINWidget_p
{
public:

    LineEditEx* createLineEdit(QWidget* _parent, const QString& _placeholder = QString())
    {
        auto lineEdit = new LineEditEx(_parent);
        lineEdit->setCustomPlaceholder(_placeholder);
        lineEdit->setCustomPlaceholderColor(Styling::getParameters().getColor(Styling::StyleVariable::BASE_PRIMARY));
        lineEdit->setFixedWidth(Utils::scale_value(mode_ == LocalPINWidget::Mode::VerifyPIN ? lineEditVerifyWidth : lineEditWidth));
        lineEdit->setFont(Fonts::appFontScaled(16));
        lineEdit->setAttribute(Qt::WA_MacShowFocusRect, false);
        lineEdit->setEchoMode(QLineEdit::Password);
        lineEdit->setFocusPolicy(Qt::StrongFocus);

        Utils::ApplyStyle(lineEdit, Styling::getParameters().getLineEditCommonQss(false, lineEditHeight));

        return lineEdit;
    }

    DialogButton* createButton(QWidget* _parent, const QString& _text, const DialogButtonRole _role, bool _enabled = true)
    {
        auto button = new DialogButton(_parent, _text, _role);
        button->setFixedWidth(Utils::scale_value(116));
        button->setEnabled(_enabled);
        return button;
    }

    bool isOkButtonEnabled() const
    {
        auto codeNotEmpty = code_ && !code_->text().isEmpty();
        auto newCodeNotEmpty = newCode_ && !newCode_->text().isEmpty();
        auto newCodeRepeatNotEmpty = newCodeRepeat_ && !newCodeRepeat_->text().isEmpty();

        switch(mode_)
        {
            case LocalPINWidget::Mode::RemovePIN:
            case LocalPINWidget::Mode::VerifyPIN:
                return codeNotEmpty;
                break;
            case LocalPINWidget::Mode::SetPIN:
                return newCodeNotEmpty && newCodeRepeatNotEmpty;
                break;
            case LocalPINWidget::Mode::ChangePIN:
                return codeNotEmpty && newCodeNotEmpty && newCodeRepeatNotEmpty;
                break;
        }
        return false;
    }

    bool validateInputs()
    {
        auto isValid = true;
        switch(mode_)
        {
            case LocalPINWidget::Mode::SetPIN:
            case LocalPINWidget::Mode::ChangePIN:
                if (newCode_->text() != newCodeRepeat_->text())
                {
                    isValid = false;
                    onPasswordsDontMatch();
                }
                break;
            default:
                break;
        }

        return isValid;
    }

    void onWrongPassword()
    {
        const auto wrongPasswordDescription = QT_TRANSLATE_NOOP("local_pin", "Wrong passcode");
        error_->setText(wrongPasswordDescription);
        if (code_)
            Utils::ApplyStyle(code_, Styling::getParameters().getLineEditCommonQss(true, lineEditHeight));
    }

    void onPasswordsDontMatch()
    {
        static auto passwordsDontMatchDescription = QT_TRANSLATE_NOOP("local_pin", "Passcodes don't match");
        error_->setText(passwordsDontMatchDescription);
        if (newCode_)
            Utils::ApplyStyle(newCode_, Styling::getParameters().getLineEditCommonQss(true, lineEditHeight));
        if (newCodeRepeat_)
            Utils::ApplyStyle(newCodeRepeat_, Styling::getParameters().getLineEditCommonQss(true, lineEditHeight));
    }


    QLabel* error_ = nullptr;
    TextWidget* name_ = nullptr;
    LineEditEx* code_ = nullptr;
    LineEditEx* newCode_ = nullptr;
    LineEditEx* newCodeRepeat_ = nullptr;
    ContactAvatarWidget* avatar_ = nullptr;
    DialogButton* cancelButton_ = nullptr;
    DialogButton* okButton_ = nullptr;
    QTextBrowser* footer_ = nullptr;
    LocalPINWidget::Mode mode_;
    qint64 seq_;

    QString enteredCode_;
};

LocalPINWidget::LocalPINWidget(LocalPINWidget::Mode _mode, QWidget* _parent)
    : QFrame(_parent)
    , d(std::make_unique<LocalPINWidget_p>())
{
    d->mode_ = _mode;

    if (_parent)
        _parent->installEventFilter(this);

    auto mainLayout = Utils::emptyVLayout(this);

    if (_mode == Mode::VerifyPIN)
        mainLayout->addStretch(0);

    // avatar
    d->avatar_ = new ContactAvatarWidget(this, MyInfo()->aimId(), MyInfo()->friendly(), Utils::scale_value(80), true);

    d->name_ = new Ui::TextWidget(this, MyInfo()->friendly(), Data::MentionMap(), TextRendering::LinksVisible::DONT_SHOW_LINKS);
    d->name_->init(
        Fonts::appFontScaled(22, Fonts::FontWeight::SemiBold),
        Styling::getParameters().getColor(Styling::StyleVariable::TEXT_SOLID),
        QColor(), QColor(), QColor(), TextRendering::HorAligment::CENTER, 1);
    d->name_->setMaxWidthAndResize(Utils::scale_value(260));

    connect(MyInfo(), &my_info::received, this, &LocalPINWidget::onMyInfoLoaded);

    mainLayout->addSpacing(Utils::scale_value(40));
    mainLayout->addWidget(d->avatar_, 0, Qt::AlignHCenter);
    mainLayout->addSpacing(Utils::scale_value(8));
    mainLayout->addWidget(d->name_, 0, Qt::AlignHCenter);
    mainLayout->addSpacing(Utils::scale_value(30));

    // inputs
    if(_mode == Mode::VerifyPIN || _mode == Mode::RemovePIN || _mode == Mode::ChangePIN)
    {
        d->code_ = d->createLineEdit(this, QT_TRANSLATE_NOOP("local_pin", "Current passcode"));
        connect(d->code_, &LineEditEx::textChanged, this, &LocalPINWidget::onInputChanged);
        mainLayout->addWidget(d->code_, 0, Qt::AlignHCenter);

        connect(GetDispatcher(), &core_dispatcher::localPINChecked, this, &LocalPINWidget::onPINChecked);
    }
    if(_mode == Mode::ChangePIN || _mode == Mode::SetPIN)
    {
        d->newCode_ = d->createLineEdit(this, QT_TRANSLATE_NOOP("local_pin", "New passcode"));
        d->newCodeRepeat_ = d->createLineEdit(this, QT_TRANSLATE_NOOP("local_pin", "Re-enter new passcode"));

        connect(d->newCode_, &LineEditEx::textChanged, this, &LocalPINWidget::onInputChanged);
        connect(d->newCodeRepeat_, &LineEditEx::textChanged, this, &LocalPINWidget::onInputChanged);

        if(_mode == Mode::ChangePIN)
            mainLayout->addSpacing(Utils::scale_value(20));
        mainLayout->addWidget(d->newCode_, 0, Qt::AlignHCenter);
        mainLayout->addSpacing(Utils::scale_value(20));
        mainLayout->addWidget(d->newCodeRepeat_, 0, Qt::AlignHCenter);
    }
    mainLayout->addSpacing(Utils::scale_value(8));

    // error
    d->error_ = new QLabel(this);
    d->error_->setFont(Fonts::appFontScaled(14, Fonts::FontWeight::Normal));
    d->error_->setStyleSheet(qsl("color: #ef5350"));
    d->error_->setMinimumHeight(Utils::scale_value(20));
    mainLayout->addWidget(d->error_, 0, Qt::AlignHCenter);

    mainLayout->addSpacing(Utils::scale_value(24));

    // buttons
    auto buttonsFrame = new QFrame(this);
    buttonsFrame->setFixedWidth(Utils::scale_value(260));
    auto buttonsLayout = Utils::emptyHLayout(buttonsFrame);

    if(_mode == Mode::ChangePIN || _mode == Mode::SetPIN || _mode == Mode::RemovePIN)
    {
        d->cancelButton_ = d->createButton(this, QT_TRANSLATE_NOOP("local_pin", "CANCEL"), DialogButtonRole::CANCEL);
        connect(d->cancelButton_, &DialogButton::clicked, this, &LocalPINWidget::onCancel);

        buttonsLayout->addWidget(d->cancelButton_);
        buttonsLayout->addSpacing(Utils::scale_value(16));
    }

    d->okButton_ = d->createButton(this, QT_TRANSLATE_NOOP("local_pin", "APPLY"), DialogButtonRole::CONFIRM, false);
    connect(d->okButton_, &DialogButton::clicked, this, &LocalPINWidget::onOk);

    if (d->mode_ == Mode::VerifyPIN)
        d->okButton_->setFixedWidth(Utils::scale_value(lineEditVerifyWidth));

    buttonsLayout->addWidget(d->okButton_);

    mainLayout->addWidget(buttonsFrame, 0, Qt::AlignHCenter);

    if(_mode == Mode::VerifyPIN)
    {
        mainLayout->addStretch(0);

        auto footer = new FooterWidget(this);
        QSizePolicy sp(QSizePolicy::Expanding, QSizePolicy::Preferred);
        footer->setSizePolicy(sp);

        connect(footer, &FooterWidget::logOutClicked, this, &LocalPINWidget::onLogOut);

        mainLayout->addWidget(footer);
    }

    mainLayout->addSpacing(Utils::scale_value(16));

    Utils::setDefaultBackground(this);

    setAttribute(Qt::WA_DeleteOnClose, _mode != Mode::VerifyPIN);
}

LocalPINWidget::~LocalPINWidget()
{

}

bool LocalPINWidget::eventFilter(QObject* _object, QEvent* _event)
{
    if (_object == parent() && _event->type() == QEvent::Resize)
    {
        auto resizeEvent = static_cast<QResizeEvent*>(_event);
        setFixedSize(resizeEvent->size());
    }

    return false;
}

void LocalPINWidget::onLogOut()
{
    const QString text = QT_TRANSLATE_NOOP("popup_window", "Are you sure you want to sign out?");
    auto confirm = Utils::GetConfirmationWithTwoButtons(
        QT_TRANSLATE_NOOP("popup_window", "CANCEL"),
        QT_TRANSLATE_NOOP("popup_window", "YES"),
        text,
        QT_TRANSLATE_NOOP("popup_window", "Sign out"),
        nullptr);

    if (confirm)
    {
        get_gui_settings()->set_value(settings_feedback_email, QString());
        GetDispatcher()->post_message_to_core("logout", nullptr);
        emit Utils::InterConnector::instance().logout();
        close();
    }
    else
    {
        if (d->code_)
            d->code_->setFocus();
    }
}

void LocalPINWidget::onCancel()
{
    emit Utils::InterConnector::instance().closeAnyPopupWindow(Utils::CloseWindowInfo());
}

void LocalPINWidget::onOk()
{
    if (!d->validateInputs())
        return;

    if (d->mode_ == Mode::VerifyPIN || d->mode_ == Mode::RemovePIN || d->mode_ == Mode::ChangePIN)
    {
        if (d->mode_ == Mode::ChangePIN)
            d->enteredCode_ = d->newCode_->text();

        d->seq_ = GetDispatcher()->localPINEntered(d->code_->text());
    }
    else if (d->mode_ == Mode::SetPIN)
    {
        GetDispatcher()->setLocalPIN(d->newCode_->text());
        LocalPIN::instance()->setEnabled(true);
        emit Utils::InterConnector::instance().closeAnyPopupWindow(Utils::CloseWindowInfo());
        close();
    }
}

void LocalPINWidget::onInputChanged(const QString& _text)
{
    Q_UNUSED(_text)

    d->error_->setText(QString());
    d->error_->setMinimumHeight(Utils::scale_value(20));

    auto enableOk = d->isOkButtonEnabled();
    d->okButton_->setEnabled(enableOk);

    if (d->code_)
        Utils::ApplyStyle(d->code_, Styling::getParameters().getLineEditCommonQss(false, lineEditHeight));
    if (d->newCode_)
        Utils::ApplyStyle(d->newCode_, Styling::getParameters().getLineEditCommonQss(false, lineEditHeight));
    if (d->newCodeRepeat_)
        Utils::ApplyStyle(d->newCodeRepeat_, Styling::getParameters().getLineEditCommonQss(false, lineEditHeight));
}

void LocalPINWidget::onPINChecked(qint64 _seq, bool _result)
{
    if (_seq != d->seq_)
        return;

    if (!_result)
    {
        d->onWrongPassword();
        return;
    }

    switch (d->mode_)
    {
        case Mode::ChangePIN:
            GetDispatcher()->setLocalPIN(d->enteredCode_);
            LocalPIN::instance()->setEnabled(true);
            break;
        case Mode::RemovePIN:
            GetDispatcher()->disableLocalPIN();
            LocalPIN::instance()->setEnabled(false);
        case Mode::VerifyPIN:
            LocalPIN::instance()->setLocked(false);
            break;
        default:
            break;
    }

    if (d->mode_ != Mode::VerifyPIN)
    {
        emit Utils::InterConnector::instance().closeAnyPopupWindow(Utils::CloseWindowInfo());
        close();
    }
    else
    {
        d->code_->clear();
        LocalPIN::instance()->unlock();
    }
}

void LocalPINWidget::onMyInfoLoaded()
{
    d->name_->setText(MyInfo()->friendly());
    d->avatar_->UpdateParams(MyInfo()->aimId(), MyInfo()->friendly());
}

void LocalPINWidget::keyPressEvent(QKeyEvent* _event)
{
    if ((_event->key() == Qt::Key_Return || _event->key() == Qt::Key_Enter) && d->isOkButtonEnabled())
    {
        onOk();
        _event->accept();
    }
    else
    {
        _event->ignore();
    }

    return QWidget::keyPressEvent(_event);
}

void LocalPINWidget::showEvent(QShowEvent* _event)
{
    if (d->mode_ == Mode::SetPIN)
        d->newCode_->setFocus();
    else
        d->code_->setFocus();

    QWidget::showEvent(_event);
}

LocalPIN* LocalPIN::instance()
{
    static LocalPIN instance;
    return &instance;
}

void LocalPIN::setEnabled(bool _enabled)
{
    enabled_ = _enabled;

    setLockTimeout(lockTimeoutMinutes());
}

void LocalPIN::setLocked(bool _locked)
{
    locked_ = _locked;
}

void LocalPIN::lock()
{
    if (std::exchange(locked_, true))
        return;

    auto mainWindow = Utils::InterConnector::instance().getMainWindow();
    if (!mainWindow)
    {
        assert(!"null MainWindow in LocalPIN");
        return;
    }

    mainWindow->lock();
}

void LocalPIN::unlock()
{
    auto mainWindow = Utils::InterConnector::instance().getMainWindow();
    if (!mainWindow)
    {
        assert(!"null MainWindow in LocalPIN");
        return;
    }
    mainWindow->showMainPage();

    if (deferrredAction_)
    {
        deferrredAction_->execute();
        deferrredAction_->deleteLater();
    }
}

bool LocalPIN::locked() const
{
    return locked_;
}

bool LocalPIN::enabled() const
{
    return enabled_;
}

bool LocalPIN::autoLockEnabled() const
{
    return enabled() && lockTimeoutMinutes().count() > 0;
}

void LocalPIN::setLockTimeout(std::chrono::minutes _minutes)
{
    if (_minutes.count() > 0 && enabled_)
        checkActivityTimer_.start();
    else
        checkActivityTimer_.stop();

    get_gui_settings()->set_value<int>(setting_local_pin_timeout, _minutes.count());
}

std::chrono::minutes LocalPIN::lockTimeoutMinutes() const
{
    return std::chrono::minutes(get_gui_settings()->get_value<int>(setting_local_pin_timeout, 0));
}

void LocalPIN::setDeferredAction(DeferredAction* _action)
{
    if (deferrredAction_)
        deferrredAction_->deleteLater();

    deferrredAction_ = _action;
}

void LocalPIN::onCheckActivityTimeout()
{
    auto timeout = lockTimeoutMinutes();

    if (timeout.count() > 0)
    {
        if (UserActivity::getLastAppActivityMs() > std::chrono::duration_cast<std::chrono::milliseconds>(timeout) && enabled_)
            lock();
    }
}

LocalPIN::LocalPIN()
{
    checkActivityTimer_.setInterval(activityCheckInterval.count());
    connect(&checkActivityTimer_, &QTimer::timeout, this, &LocalPIN::onCheckActivityTimeout);
}

class FooterWidget_p
{
public:
    TextRendering::TextUnitPtr textUnit_;
    bool pressed_ = false;
};

FooterWidget::FooterWidget(QWidget* _parent)
    : QWidget(_parent)
    , d(std::make_unique<FooterWidget_p>())
{

    const auto textBegin = QT_TRANSLATE_NOOP("local_pin", "If you forgot your passcode, you can ");
    const auto logOutText = QT_TRANSLATE_NOOP("local_pin", "log out");
    const auto textEnd = QT_TRANSLATE_NOOP("local_pin", " and authorize again.");

    d->textUnit_ = TextRendering::MakeTextUnit(textBegin % logOutText % textEnd);
    d->textUnit_->init(
        Fonts::appFontScaled(14, Fonts::FontWeight::Normal),
        Styling::getParameters().getColor(Styling::StyleVariable::BASE_PRIMARY),
        Styling::getParameters().getColor(Styling::StyleVariable::PRIMARY),
        QColor(), QColor(), TextRendering::HorAligment::CENTER);

    const std::map<QString, QString> links = {{logOutText, QString()}};
    d->textUnit_->applyLinks(links);
    d->textUnit_->getHeight(d->textUnit_->desiredWidth());

    setMouseTracking(true);
}

FooterWidget::~FooterWidget()
{

}

QSize FooterWidget::sizeHint() const
{
    const auto height = d->textUnit_->getHeight(width());
    return QSize(width(), height);
}

void FooterWidget::paintEvent(QPaintEvent* _event)
{
    QPainter p(this);
    d->textUnit_->draw(p);

    QWidget::paintEvent(_event);
}

void FooterWidget::mouseMoveEvent(QMouseEvent* _event)
{
    const auto overLink = d->textUnit_->isOverLink(_event->pos());
    setCursor(overLink ? Qt::PointingHandCursor : Qt::ArrowCursor);

    QWidget::mouseMoveEvent(_event);
}

void FooterWidget::mousePressEvent(QMouseEvent* _event)
{
    d->pressed_ = d->textUnit_->isOverLink(_event->pos());

    _event->accept();
    QWidget::mousePressEvent(_event);
}

void FooterWidget::mouseReleaseEvent(QMouseEvent* _event)
{
    const auto overLink = d->textUnit_->isOverLink(_event->pos());

    if (overLink && d->pressed_)
        emit logOutClicked();

    d->pressed_ = false;

    _event->accept();
    QWidget::mouseReleaseEvent(_event);
}

void FooterWidget::resizeEvent(QResizeEvent* _event)
{
    adjustSize();
    updateGeometry();
    QWidget::resizeEvent(_event);
}

}
