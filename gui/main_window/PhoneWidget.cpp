#include "stdafx.h"
#include "PhoneWidget.h"
#include "../core_dispatcher.h"
#include "../gui_settings.h"
#include "../controls/TextUnit.h"

#include "LoginPage.h"

#include "../controls/LineEditEx.h"
#include "../controls/CountrySearchCombobox.h"
#include "../controls/SizedView.h"
#include "../controls/DialogButton.h"
#include "../fonts.h"
#include "../utils/utils.h"
#include "../utils/InterConnector.h"
#include "../utils/gui_coll_helper.h"
#include "../utils/translator.h"
#include "styles/ThemeParameters.h"

namespace
{
    const int TOP_MARGIN = 40;
    const int LEFT_MARGIN = 44;
    const int RIGHT_MARGIN = 28;
    const int BOTTOM_MARGIN = 16;
    const int SPACING = 20;
    const int ACTIONS_SPACING = 20;
    const int PHONE_CALL_SPACING = 14;
    const int BUTTONS_OFFSET = 64;
    const int DIALOG_WIDTH = 360;
    const int DIALOG_HEIGHT = 478;
    const int SIM_HEIGHT = 60;
    const int SUPERSAFE_HEIGHT = 80;
    const int BUTTON_HEIGHT = 32;
    const int BUTTON_WIDTH = 116;
    const int BUTTON_HOR_PADDING = 20;
    const int BUTTON_MARGIN = 56;
    const int COUNTRY_CODE_WIDTH = 64;
    const int COUNTRY_CODE_LEFT_MARGIN = 32;
    const int PHONE_EDIT_MARGIN = 38;
    const int PHONE_EDIT_LEFT_MARGIN = 8;
    const int PHONE_EDIT_WIDTH = 224;
    const int ABOUT_PHONE_NUMBER_WIDTH = 296;
    const int ABOUT_PHONE_NUMBER_OFFSET = 52;
    const int COMPLETER_SPACING = 12;
    const int ENTERED_PHONE_NUMBER_TOP_MARGIN = 126;
    const int CODE_ACTIONS_TOP_MARGIN = 266;
    const int LABEL_OFFSET = 10;
    const int SPECIFY_NUMBER_HOR_OSSET = 64;
    const int SMS_CODE_SPACING = 38;
    const int SPACE_BETWEEN_BUTTONS = 16;
    const int LOGOUT_OFFSET = 370;
    const int HOR_MARGIN = 32;

    const int TIMER_INTERVAL = 1000;
    const int SECONDS_TO_RESEND = 60;
}

namespace Ui
{

    PhoneWidget::PhoneWidget(QWidget* _parent, const PhoneWidgetState& _state, const QString& _label, const QString& _about, bool _canClose, bool _forceAttach)
        : QWidget(_parent)
        , state_(_state)
        , next_(nullptr)
        , cancel_(nullptr)
        , countryCode_(nullptr)
        , enteredPhoneNumber_(nullptr)
        , smsCode_(nullptr)
        , sendSeq_(-1)
        , codeLength_(-1)
        , secRemaining_(SECONDS_TO_RESEND)
        , timer_(nullptr)
        , showSmsError_(false)
        , labelText_(_label)
        , aboutText_(_about)
        , canClose_(_canClose)
        , forceAttach_(_forceAttach)
        , loggedOut_(false)
        , showPhoneHint_(false)
    {
        init();
        setMouseTracking(true);
    }

    PhoneWidget::~PhoneWidget()
    {

    }

    void PhoneWidget::setState(const PhoneWidgetState& _state)
    {
        state_ = _state;
        update();
    }

    void PhoneWidget::paintEvent(QPaintEvent* _e)
    {
        QPainter p(this);
        label_->draw(p);

        if (state_ != PhoneWidgetState::ENTER_CODE_SATE)
            p.drawPixmap(QPoint(width() / 2 - icon_.width() / 2 / Utils::scale_bitmap(1), Utils::scale_value(TOP_MARGIN)), icon_);

        if (state_ == PhoneWidgetState::ABOUT_STATE)
        {
            p.drawPixmap(QPoint(Utils::scale_value(RIGHT_MARGIN) - mark_.width() / 2, about_first_->verOffset() + mark_.height() / 4), mark_);
            p.drawPixmap(QPoint(Utils::scale_value(RIGHT_MARGIN) - mark_.width() / 2, about_second_->verOffset() + mark_.height() / 4), mark_);

            about_first_->draw(p);
            about_second_->draw(p);
        }
        else if (state_ == PhoneWidgetState::ENTER_PHONE_STATE)
        {
            about_phone_enter_->draw(p);
            if (forceAttach_)
                logout_->draw(p);
        }
        else if (state_ == PhoneWidgetState::ENTER_CODE_SATE)
        {
            change_phone_->draw(p);
            if (showPhoneHint_)
                phone_hint_->draw(p);
            send_again_->draw(p);
            phone_call_->draw(p);
            if (showSmsError_)
                sms_error_->draw(p);
        }
        else if (state_ == PhoneWidgetState::FINISH_STATE)
        {
            about_phone_changed_->draw(p);
            changed_phone_number_->draw(p);
        }

        return QWidget::paintEvent(_e);
    }

    void PhoneWidget::mouseReleaseEvent(QMouseEvent* _e)
    {
        if (state_ == PhoneWidgetState::ENTER_CODE_SATE && change_phone_->contains(_e->pos()))
        {
            setState(PhoneWidgetState::ENTER_PHONE_STATE);
            next_->setText(QT_TRANSLATE_NOOP("popup_window", "NEXT"));
            label_->setText(labelText_.isEmpty() ? QT_TRANSLATE_NOOP("phone_widget", "Your phone number") : labelText_);
            label_->setOffsets(Utils::scale_value(DIALOG_WIDTH) / 2 - label_->desiredWidth() / 2, Utils::scale_value(TOP_MARGIN + SUPERSAFE_HEIGHT + LABEL_OFFSET));
            next_->show();
            if (canClose_)
                next_->move(Utils::scale_value(DIALOG_WIDTH - BUTTON_MARGIN) - next_->width(), Utils::scale_value(DIALOG_HEIGHT - BOTTOM_MARGIN) - next_->height());
            else
                next_->move(Utils::scale_value(DIALOG_WIDTH) / 2 - next_->width() / 2, Utils::scale_value(DIALOG_HEIGHT - BOTTOM_MARGIN) - next_->height());
            cancel_->move(Utils::scale_value(BUTTON_MARGIN), Utils::scale_value(DIALOG_HEIGHT - BOTTOM_MARGIN) - cancel_->height());
            countryCode_->show();
            phoneNumber_->show();
            enteredPhoneNumber_->hide();
            smsCode_->hide();
            timer_->stop();
            phoneNumber_->setFocus();
            update();
        }
        else if (state_ == PhoneWidgetState::ENTER_CODE_SATE && secRemaining_ == 0 && send_again_->contains(_e->pos()))
        {
            sendCode();
        }
        else if (state_ == PhoneWidgetState::ENTER_CODE_SATE && secRemaining_ == 0 && phone_call_->contains(_e->pos()))
        {
            callPhone();
        }
        else if (state_ == PhoneWidgetState::ENTER_PHONE_STATE && forceAttach_ && logout_->contains(_e->pos()))
        {
            logout();
        }
        return QWidget::mouseReleaseEvent(_e);
    }

    void PhoneWidget::mouseMoveEvent(QMouseEvent* _e)
    {

        auto point = (state_ == PhoneWidgetState::ENTER_CODE_SATE && change_phone_->contains(_e->pos()));
        point |= (state_ == PhoneWidgetState::ENTER_CODE_SATE && secRemaining_ == 0 && (send_again_->contains(_e->pos()) || phone_call_->contains(_e->pos())));
        point |= (state_ == PhoneWidgetState::ENTER_PHONE_STATE && forceAttach_ && logout_->contains(_e->pos()));

        if (point)
            setCursor(Qt::PointingHandCursor);
        else
            setCursor(Qt::ArrowCursor);

        return QWidget::mouseMoveEvent(_e);
    }

    void PhoneWidget::showEvent(QShowEvent* _e)
    {
        if (state_ == PhoneWidgetState::ENTER_PHONE_STATE)
            phoneNumber_->setFocus();
        else
            setFocus();

        return QWidget::showEvent(_e);
    }

    void PhoneWidget::keyPressEvent(QKeyEvent* _e)
    {
        if (_e->key() == Qt::Key_Return)
        {
            if (next_->isVisible() && next_->isEnabled())
                nextClicked();
            return;
        }
        else if (_e->key() == Qt::Key_Escape && !canClose_)
        {
            return;
        }

        return QWidget::keyPressEvent(_e);
    }

    void PhoneWidget::init()
    {
        next_ = new DialogButton(this, QT_TRANSLATE_NOOP("popup_window", "NEXT"), DialogButtonRole::CONFIRM);
        next_->setCursor(QCursor(Qt::PointingHandCursor));
        next_->setFixedSize(Utils::scale_value(BUTTON_WIDTH), Utils::scale_value(BUTTON_HEIGHT));
        connect(next_, &QPushButton::clicked, this, &PhoneWidget::nextClicked);

        cancel_ = new DialogButton(this, QT_TRANSLATE_NOOP("popup_window", "CANCEL"), DialogButtonRole::CANCEL);
        cancel_->setCursor(QCursor(Qt::PointingHandCursor));
        cancel_->setFixedSize(Utils::scale_value(BUTTON_WIDTH), Utils::scale_value(BUTTON_HEIGHT));
        connect(cancel_, &QPushButton::clicked, this, &PhoneWidget::cancelClicked);
        if (!canClose_)
            cancel_->hide();

        icon_ = Utils::loadPixmap(forceAttach_ ? qsl(":/phone_widget/supersafe_100") : qsl(":/phone_widget/sim_100"));
        mark_ = Utils::renderSvg(qsl(":/phone_widget/list_marker"), Utils::scale_value(QSize(10, 10)), Styling::getParameters().getColor(Styling::StyleVariable::PRIMARY));

        label_ = TextRendering::MakeTextUnit(QT_TRANSLATE_NOOP("phone_widget", "Change number"));
        label_->init(Fonts::appFontScaled(22, Fonts::FontWeight::SemiBold), Styling::getParameters().getColor(Styling::StyleVariable::TEXT_SOLID));

        const auto label_height = label_->getHeight(label_->desiredWidth());
        auto margin = Utils::scale_value(TOP_MARGIN + SUPERSAFE_HEIGHT + LABEL_OFFSET);

        label_->setOffsets(Utils::scale_value(DIALOG_WIDTH) / 2 - label_->desiredWidth() / 2, margin);
        margin += label_height;

        const auto labelOffset = margin;

        about_first_ = TextRendering::MakeTextUnit(QT_TRANSLATE_NOOP("phone_widget", "Your account, contact details, chats and messages will be moved to your new number."));
        about_first_->init(Fonts::appFontScaled(13), Styling::getParameters().getColor(Styling::StyleVariable::TEXT_SOLID));

        margin += Utils::scale_value(SPACING);
        about_first_->setOffsets(Utils::scale_value(LEFT_MARGIN), margin);
        margin += about_first_->getHeight(Utils::scale_value(DIALOG_WIDTH - LEFT_MARGIN - RIGHT_MARGIN));

        about_second_ = TextRendering::MakeTextUnit(QT_TRANSLATE_NOOP("phone_widget", "Before starting the transfer, ensure that you can receive SMS messages or calls to your new number."));
        about_second_->init(Fonts::appFontScaled(13), Styling::getParameters().getColor(Styling::StyleVariable::TEXT_SOLID));

        margin += Utils::scale_value(SPACING);
        about_second_->setOffsets(Utils::scale_value(LEFT_MARGIN), margin);
        about_second_->getHeight(Utils::scale_value(DIALOG_WIDTH - LEFT_MARGIN - RIGHT_MARGIN));

        countryCode_ = new LineEditEx(this);
        countryCode_->setText(Utils::GetTranslator()->getCurrentPhoneCode());
        countryCode_->setFont(Fonts::appFontScaled(18));
        countryCode_->setFixedWidth(Utils::scale_value(COUNTRY_CODE_WIDTH));
        // TODO: fix lineEdits
        Utils::ApplyStyle(countryCode_, Styling::getParameters().getLineEditCommonQss());
        countryCode_->setAttribute(Qt::WA_MacShowFocusRect, false);
        countryCode_->setAlignment(Qt::AlignCenter);
        countryCode_->move(Utils::scale_value(COUNTRY_CODE_LEFT_MARGIN), labelOffset + Utils::scale_value(PHONE_EDIT_MARGIN));
        countryCode_->hide();
        countryCode_->setValidator(new Utils::PhoneValidator(this, true));

        auto countries = Utils::getCountryCodes();
        auto model = new QStandardItemModel(this);
        model->setColumnCount(2);
        int i = 0;
        for (auto it = countries.begin(), end = countries.end(); it != end; ++it)
        {
            QStandardItem* firstCol = new QStandardItem(it.key());
            firstCol->setFlags(firstCol->flags() & ~Qt::ItemIsEditable);
            QStandardItem* secondCol = new QStandardItem(it.value());
            secondCol->setFlags(secondCol->flags() & ~Qt::ItemIsEditable);
            secondCol->setTextAlignment(Qt::AlignRight);
            model->setItem(i, 0, firstCol);
            model->setItem(i, 1, secondCol);
            ++i;
        }

        auto view_ = new SizedView(this);
        view_->setSelectionBehavior(QAbstractItemView::SelectRows);
        view_->header()->hide();
        view_->setRootIsDecorated(false);
        view_->setFixedWidth(Utils::scale_value(ABOUT_PHONE_NUMBER_WIDTH));
        view_->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        Utils::ApplyStyle(view_, Styling::getParameters().getPhoneComboboxQss());

        completer_ = new QCompleter(this);
        completer_->setCaseSensitivity(Qt::CaseInsensitive);
        completer_->setModel(model);
        completer_->setPopup(view_);
        completer_->setCompletionColumn(0);
        completer_->setCompletionMode(QCompleter::PopupCompletion);
        completer_->setMaxVisibleItems(6);
        completer_->setCompletionColumn(1);
        completer_->setCompletionPrefix(countryCode_->text());
        countryCode_->setCompleter(completer_);

        connect(completer_, Utils::QOverload<const QString&>::of(&QCompleter::activated), this, &PhoneWidget::countrySelected);

        connect(countryCode_, &QLineEdit::textChanged, this, &PhoneWidget::codeChanged);
        connect(countryCode_, &Ui::LineEditEx::clicked, this, &PhoneWidget::codeFocused);

        phoneNumber_ = new LineEditEx(this);
        // TODO: fix lineEdits
        phoneNumber_->setPlaceholderText(QT_TRANSLATE_NOOP("phone_widget", "phone number"));
        phoneNumber_->setFont(Fonts::appFontScaled(18));
        phoneNumber_->setFixedWidth(Utils::scale_value(PHONE_EDIT_WIDTH));
        Utils::ApplyStyle(phoneNumber_, Styling::getParameters().getLineEditCommonQss());
        phoneNumber_->setAttribute(Qt::WA_MacShowFocusRect, false);
        phoneNumber_->setValidator(new Utils::PhoneValidator(this, false));
        phoneNumber_->move(countryCode_->x() + countryCode_->width() + Utils::scale_value(PHONE_EDIT_LEFT_MARGIN), labelOffset + Utils::scale_value(PHONE_EDIT_MARGIN));
        phoneNumber_->hide();

        connect(phoneNumber_, &QLineEdit::textChanged, this, &PhoneWidget::phoneChanged);
        connect(phoneNumber_, &LineEditEx::emptyTextBackspace, this, [this]() { countryCode_->setFocus(); completer_->complete(); });

        enteredPhoneNumber_ = new LineEditEx(this);
        enteredPhoneNumber_->setFont(Fonts::appFontScaled(18));
        enteredPhoneNumber_->setFixedWidth(Utils::scale_value(ABOUT_PHONE_NUMBER_WIDTH));
        Utils::ApplyStyle(enteredPhoneNumber_, Styling::getParameters().getLineEditCommonQss());
        enteredPhoneNumber_->changeTextColor(Styling::getParameters().getColor(Styling::StyleVariable::BASE_PRIMARY));
        enteredPhoneNumber_->setAttribute(Qt::WA_MacShowFocusRect, false);
        enteredPhoneNumber_->setEnabled(false);
        enteredPhoneNumber_->move(Utils::scale_value(DIALOG_WIDTH) / 2 - Utils::scale_value(ABOUT_PHONE_NUMBER_WIDTH) / 2, Utils::scale_value(ENTERED_PHONE_NUMBER_TOP_MARGIN));
        enteredPhoneNumber_->hide();

        smsCode_ = new LineEditEx(this);
        smsCode_->setFont(Fonts::appFontScaled(18));
        smsCode_->setFixedWidth(Utils::scale_value(ABOUT_PHONE_NUMBER_WIDTH));
        Utils::ApplyStyle(smsCode_, Styling::getParameters().getLineEditCommonQss());
        smsCode_->setAttribute(Qt::WA_MacShowFocusRect, false);
        smsCode_->setValidator(new Utils::PhoneValidator(this, false));
        smsCode_->move(Utils::scale_value(DIALOG_WIDTH) / 2 - Utils::scale_value(ABOUT_PHONE_NUMBER_WIDTH) / 2, Utils::scale_value(ENTERED_PHONE_NUMBER_TOP_MARGIN) + enteredPhoneNumber_->height() + Utils::scale_value(SMS_CODE_SPACING));
        smsCode_->hide();

        connect(smsCode_, &QLineEdit::textChanged, this, &PhoneWidget::smsCodeChanged);

        change_phone_ = TextRendering::MakeTextUnit(QT_TRANSLATE_NOOP("phone_widget", "Change"));
        change_phone_->init(Fonts::appFontScaled(13), Styling::getParameters().getColor(Styling::StyleVariable::TEXT_PRIMARY));
        auto change_phone_offset = change_phone_->getHeight(change_phone_->desiredWidth());
        if (platform::is_apple())
            change_phone_offset += Utils::scale_value(4);
        change_phone_->setOffsets(Utils::scale_value(DIALOG_WIDTH) / 2 + Utils::scale_value(ABOUT_PHONE_NUMBER_WIDTH) / 2 - change_phone_->desiredWidth(), Utils::scale_value(ENTERED_PHONE_NUMBER_TOP_MARGIN) + change_phone_offset);

        about_phone_enter_ = TextRendering::MakeTextUnit(aboutText_.isEmpty() ? QT_TRANSLATE_NOOP("phone_widget", "Enter your new number") : aboutText_);
        about_phone_enter_->init(Fonts::appFontScaled(13), Styling::getParameters().getColor(Styling::StyleVariable::BASE_PRIMARY), QColor(), QColor(), QColor(), TextRendering::HorAligment::CENTER);
        about_phone_enter_->getHeight(Utils::scale_value(DIALOG_WIDTH - SPECIFY_NUMBER_HOR_OSSET * 2));
        about_phone_enter_->setOffsets(Utils::scale_value(SPECIFY_NUMBER_HOR_OSSET), phoneNumber_->y() + phoneNumber_->height() + Utils::scale_value(ABOUT_PHONE_NUMBER_OFFSET));

        phone_hint_ = TextRendering::MakeTextUnit(QString());
        phone_hint_->init(Fonts::appFontScaled(13), Styling::getParameters().getColor(Styling::StyleVariable::TEXT_SOLID), QColor(), QColor(), QColor(), TextRendering::HorAligment::CENTER);
        phone_hint_->evaluateDesiredSize();
        phone_hint_->setOffsets(Utils::scale_value(HOR_MARGIN), Utils::scale_value(CODE_ACTIONS_TOP_MARGIN));

        send_again_ = TextRendering::MakeTextUnit(QT_TRANSLATE_NOOP("phone_widget", "Resend code") + ql1s(" 1:00"));
        send_again_->init(Fonts::appFontScaled(13), Styling::getParameters().getColor(Styling::StyleVariable::BASE_PRIMARY));
        send_again_->evaluateDesiredSize();
        send_again_->setOffsets((Utils::scale_value(DIALOG_WIDTH) - send_again_->desiredWidth()) / 2, Utils::scale_value(CODE_ACTIONS_TOP_MARGIN));

        phone_call_ = TextRendering::MakeTextUnit(QT_TRANSLATE_NOOP("phone_widget", "Dictate over the phone"));
        phone_call_->init(Fonts::appFontScaled(13), Styling::getParameters().getColor(Styling::StyleVariable::BASE_PRIMARY));
        phone_call_->evaluateDesiredSize();
        phone_call_->setOffsets((Utils::scale_value(DIALOG_WIDTH) - phone_call_->desiredWidth()) / 2, send_again_->verOffset() + send_again_->cachedSize().height() + Utils::scale_value(PHONE_CALL_SPACING));

        sms_error_ = TextRendering::MakeTextUnit(QT_TRANSLATE_NOOP("phone_widget", "Incorrect code"));
        sms_error_->init(Fonts::appFontScaled(13), Styling::getParameters().getColor(Styling::StyleVariable::SECONDARY_ATTENTION)
                         , QColor()
                         , QColor()
                         , QColor()
                         , TextRendering::HorAligment::CENTER);
        sms_error_->evaluateDesiredSize();
        sms_error_->setOffsets((Utils::scale_value(DIALOG_WIDTH) - sms_error_->desiredWidth()) / 2, Utils::scale_value(CODE_ACTIONS_TOP_MARGIN));

        logout_ = TextRendering::MakeTextUnit(QT_TRANSLATE_NOOP("phone_widget", "Sign out"));
        logout_->init(Fonts::appFontScaled(13), Styling::getParameters().getColor(Styling::StyleVariable::TEXT_PRIMARY));
        logout_->evaluateDesiredSize();
        logout_->setOffsets((Utils::scale_value(DIALOG_WIDTH) - logout_->desiredWidth()) / 2, Utils::scale_value(LOGOUT_OFFSET));

        if (canClose_)
            next_->move(Utils::scale_value(DIALOG_WIDTH - BUTTON_MARGIN) - next_->width(), Utils::scale_value(DIALOG_HEIGHT - BOTTOM_MARGIN) - next_->height());
        else
            next_->move(Utils::scale_value(DIALOG_WIDTH) /2 - next_->width() / 2, Utils::scale_value(DIALOG_HEIGHT - BOTTOM_MARGIN) - next_->height());

        cancel_->move(Utils::scale_value(BUTTON_MARGIN), Utils::scale_value(DIALOG_HEIGHT - BOTTOM_MARGIN) - cancel_->height());

        about_phone_changed_ = TextRendering::MakeTextUnit(QT_TRANSLATE_NOOP("phone_widget", "Your new account number is"));
        about_phone_changed_->init(Fonts::appFontScaled(16), Styling::getParameters().getColor(Styling::StyleVariable::BASE_PRIMARY), QColor(), QColor(), QColor(), TextRendering::HorAligment::CENTER);
        auto h = about_phone_changed_->getHeight(Utils::scale_value(DIALOG_WIDTH - LEFT_MARGIN * 2));
        about_phone_changed_->setOffsets(Utils::scale_value(LEFT_MARGIN), labelOffset + Utils::scale_value(PHONE_EDIT_MARGIN));


        changed_phone_number_ = TextRendering::MakeTextUnit(countryCode_->text() + QString());
        // FONT COLOR
        changed_phone_number_->init(Fonts::appFontScaled(18), Styling::getParameters().getColor(Styling::StyleVariable::BASE_SECONDARY));
        changed_phone_number_->evaluateDesiredSize();
        changed_phone_number_->setOffsets(Utils::scale_value(DIALOG_WIDTH) / 2 - changed_phone_number_->desiredWidth() / 2, labelOffset + Utils::scale_value(PHONE_EDIT_MARGIN) + h + Utils::scale_value(ACTIONS_SPACING * 2));

        setFixedSize(Utils::scale_value(DIALOG_WIDTH), Utils::scale_value(DIALOG_HEIGHT));

        timer_ = new QTimer(this);
        timer_->setInterval(TIMER_INTERVAL);
        timer_->setSingleShot(false);

        QObject::connect(Ui::GetDispatcher(), &Ui::core_dispatcher::phoneInfoResult, this, &PhoneWidget::phoneInfoResult);
        QObject::connect(Ui::GetDispatcher(), &Ui::core_dispatcher::getSmsResult, this, &PhoneWidget::getSmsResult);
        QObject::connect(Ui::GetDispatcher(), &Ui::core_dispatcher::loginResult, this, &PhoneWidget::loginResult);
        QObject::connect(Ui::GetDispatcher(), &Ui::core_dispatcher::loginResultAttachPhone, this, &PhoneWidget::loginResult);

        QObject::connect(timer_, &QTimer::timeout, this, &PhoneWidget::onTimer);

        if (state_ == PhoneWidgetState::ENTER_PHONE_STATE)
        {
            next_->setText(QT_TRANSLATE_NOOP("popup_window", "NEXT"));
            label_->setText(labelText_.isEmpty() ? QT_TRANSLATE_NOOP("phone_widget", "Your phone number") : labelText_);
            label_->setOffsets(Utils::scale_value(DIALOG_WIDTH) / 2 - label_->desiredWidth() / 2, label_->verOffset());
            next_->setEnabled(false);
            countryCode_->show();
            phoneNumber_->show();
        }
    }

    void PhoneWidget::sendCode()
    {
        ivrUrl_.clear();
        gui_coll_helper collection(GetDispatcher()->create_collection(), true);
        collection.set_value_as_qstring("country", countryCode_->text());
        collection.set_value_as_qstring("phone", phoneNumber_->text());
        collection.set_value_as_qstring("locale", Utils::GetTranslator()->getLang());
        collection.set_value_as_bool("is_login", false);
        sendSeq_ = GetDispatcher()->post_message_to_core("login_get_sms_code", collection.get());

        timer_->start();
        secRemaining_ = SECONDS_TO_RESEND;
        showSmsError_ = false;
        resetActions();
        Utils::ApplyStyle(smsCode_, Styling::getParameters().getLineEditCommonQss());
        smsCode_->setEnabled(true);
        smsCode_->clear();
    }

    void PhoneWidget::callPhone()
    {
        if (ivrUrl_.isEmpty())
            return;

        Ui::GetDispatcher()->getCodeByPhoneCall(ivrUrl_);

        timer_->start();
        secRemaining_ = SECONDS_TO_RESEND;
        showSmsError_ = false;
        resetActions();
        Utils::ApplyStyle(smsCode_, Styling::getParameters().getLineEditCommonQss());
        smsCode_->setEnabled(true);
        smsCode_->clear();
    }

    void PhoneWidget::logout()
    {
        hide();
        QString text = QT_TRANSLATE_NOOP("popup_window", "Are you sure you want to sign out?");
        auto confirm = Utils::GetConfirmationWithTwoButtons(
            QT_TRANSLATE_NOOP("popup_window", "CANCEL"),
            QT_TRANSLATE_NOOP("popup_window", "YES"),
            text,
            QT_TRANSLATE_NOOP("popup_window", "Sign out"),
            nullptr);

        if (confirm)
        {
            loggedOut_ = true;
            get_gui_settings()->set_value(settings_feedback_email, QString());
            GetDispatcher()->post_message_to_core("logout", nullptr);
            emit Utils::InterConnector::instance().logout();
            emit Utils::InterConnector::instance().closeAnyPopupWindow(Utils::CloseWindowInfo());
            emit requestClose();
        }
        else
        {
            show();
        }
    }

    void PhoneWidget::updateTextOnGetSmsResult()
    {
        auto getCallStr = [](auto codeLength)
        {
            return Utils::GetTranslator()->getNumberString(
                codeLength,
                QT_TRANSLATE_NOOP3("phone_widget", "Enter last %1 digit", "1"),
                QT_TRANSLATE_NOOP3("phone_widget", "Enter last %1 digits", "2"),
                QT_TRANSLATE_NOOP3("phone_widget", "Enter last %1 digits", "5"),
                QT_TRANSLATE_NOOP3("phone_widget", "Enter last %1 digits", "21")).arg(codeLength);
        };
        const auto callCheck = LoginPage::isCallCheck(checks_);
        label_->setText(callCheck ? QT_TRANSLATE_NOOP("phone_widget", "Change number") : QT_TRANSLATE_NOOP("phone_widget", "Code from SMS"));
        label_->setOffsets(Utils::scale_value(DIALOG_WIDTH) / 2 - label_->desiredWidth() / 2, Utils::scale_value(TOP_MARGIN - LABEL_OFFSET));
        showPhoneHint_ = callCheck;

        if (codeLength_ != -1)
        {
            phone_hint_->setText(callCheck ? getCallStr(codeLength_) : QT_TRANSLATE_NOOP("phone_widget", "Code from SMS"));
            phone_hint_->getHeight(width() - Utils::scale_value(HOR_MARGIN * 2));
            phone_hint_->setOffsets(Utils::scale_value(HOR_MARGIN), Utils::scale_value(CODE_ACTIONS_TOP_MARGIN));
        }
    }

    void PhoneWidget::resetActions()
    {
        showPhoneHint_ = LoginPage::isCallCheck(checks_);
        send_again_->setText((showPhoneHint_ ? QT_TRANSLATE_NOOP("phone_widget", "Recall") : QT_TRANSLATE_NOOP("phone_widget", "Resend code")) + ql1s(" 1:00"), Styling::getParameters().getColor(Styling::StyleVariable::BASE_PRIMARY));
        phone_call_->setText(QT_TRANSLATE_NOOP("phone_widget", "Dictate over the phone"), Styling::getParameters().getColor(Styling::StyleVariable::BASE_PRIMARY));

        phone_hint_->setOffsets(Utils::scale_value(HOR_MARGIN), Utils::scale_value(CODE_ACTIONS_TOP_MARGIN));
        send_again_->setOffsets((Utils::scale_value(DIALOG_WIDTH) - send_again_->desiredWidth()) / 2, showPhoneHint_ ? (phone_hint_->verOffset() + phone_hint_->cachedSize().height() + Utils::scale_value(ACTIONS_SPACING)) : Utils::scale_value(CODE_ACTIONS_TOP_MARGIN));
        phone_call_->setOffsets((Utils::scale_value(DIALOG_WIDTH) - phone_call_->desiredWidth()) / 2, send_again_->verOffset() + send_again_->cachedSize().height() + Utils::scale_value(PHONE_CALL_SPACING));
    }

    void PhoneWidget::nextClicked()
    {
        timer_->stop();
        if (state_ == PhoneWidgetState::ABOUT_STATE)
        {
            setState(PhoneWidgetState::ENTER_PHONE_STATE);
            next_->setText(QT_TRANSLATE_NOOP("popup_window", "NEXT"));
            label_->setText(labelText_.isEmpty() ? QT_TRANSLATE_NOOP("phone_widget", "Your phone number") : labelText_);
            label_->setOffsets(Utils::scale_value(DIALOG_WIDTH) / 2 - label_->desiredWidth() / 2, label_->verOffset());
            next_->setEnabled(false);
            countryCode_->show();
            phoneNumber_->show();
            phoneNumber_->setFocus();
        }
        else if (state_ == PhoneWidgetState::ENTER_PHONE_STATE)
        {
            setState(PhoneWidgetState::ENTER_CODE_SATE);
            next_->hide();
            updateTextOnGetSmsResult();
            enteredPhoneNumber_->setText(countryCode_->text() + phoneNumber_->text());
            enteredPhoneNumber_->show();
            smsCode_->show();
            smsCode_->clear();
            smsCode_->setFocus();
            countryCode_->hide();
            phoneNumber_->hide();
            cancel_->move(Utils::scale_value(DIALOG_WIDTH) / 2 - cancel_->width() / 2, Utils::scale_value(DIALOG_HEIGHT - BOTTOM_MARGIN) - cancel_->height());
            sendCode();
            update();
        }
        else if (state_ == PhoneWidgetState::FINISH_STATE)
        {
            emit Utils::InterConnector::instance().acceptGeneralDialog();
        }
    }

    void PhoneWidget::cancelClicked()
    {
        emit Utils::InterConnector::instance().closeAnyPopupWindow(Utils::CloseWindowInfo());
    }

    void PhoneWidget::codeChanged(const QString& _code)
    {
        if (!_code.startsWith(ql1c('+')))
            countryCode_->setText(ql1c('+') + _code);

        if (completer_->completionCount() == 0)
        {
            phoneNumber_->setText(_code.mid(_code.length() - 1));
            phoneNumber_->setFocus();

            countryCode_->blockSignals(true);
            countryCode_->setText(_code.left(_code.length() - 1));
            countryCode_->blockSignals(false);
        }
        else if (completer_->completionCount() == 1 && completer_->currentCompletion() == _code)
        {
            phoneNumber_->setFocus();
        }
    }

    void PhoneWidget::codeFocused()
    {
        completer_->setCompletionPrefix(QString());
        completer_->complete();
    }

    void PhoneWidget::phoneChanged(const QString& _phone)
    {
        if (_phone.length() < 3)
        {
            next_->setText(QT_TRANSLATE_NOOP("popup_window", "NEXT"));
            next_->setFixedWidth(Utils::scale_value(BUTTON_WIDTH));
            next_->setEnabled(false);
            if (canClose_)
                next_->move(Utils::scale_value(DIALOG_WIDTH - BUTTON_MARGIN) - next_->width(), Utils::scale_value(DIALOG_HEIGHT - BOTTOM_MARGIN) - next_->height());
            else
                next_->move(Utils::scale_value(DIALOG_WIDTH) / 2 - next_->width() / 2, Utils::scale_value(DIALOG_HEIGHT - BOTTOM_MARGIN) - next_->height());
            cancel_->move(Utils::scale_value(BUTTON_MARGIN), Utils::scale_value(DIALOG_HEIGHT - BOTTOM_MARGIN) - cancel_->height());

            return;
        }

        Ui::gui_coll_helper collection(Ui::GetDispatcher()->create_collection(), true);
        collection.set_value_as_qstring("phone", countryCode_->text() % _phone);
        collection.set_value_as_qstring("gui_locale", get_gui_settings()->get_value(settings_language, QString()));
        GetDispatcher()->post_message_to_core("phoneinfo", collection.get());
    }

    void PhoneWidget::smsCodeChanged(const QString& _code)
    {
        if (_code.length() == codeLength_)
        {
            gui_coll_helper collection(GetDispatcher()->create_collection(), true);
            collection.set_value_as_qstring("phone", phoneNumber_->text());
            collection.set_value_as_qstring("sms_code", _code);
            collection.set_value_as_bool("save_auth_data", true);
            collection.set_value_as_bool("is_login", false);
            sendSeq_ = GetDispatcher()->post_message_to_core("login_by_phone", collection.get());

            smsCode_->setEnabled(false);
            Utils::ApplyStyle(smsCode_, Styling::getParameters().getLineEditDisabledQss());
            smsCode_->changeTextColor(Styling::getParameters().getColor(Styling::StyleVariable::BASE_PRIMARY));
        }
        else
        {
            smsCode_->setEnabled(true);
            Utils::ApplyStyle(smsCode_, Styling::getParameters().getLineEditCommonQss());
            smsCode_->changeTextColor(Styling::getParameters().getColor(Styling::StyleVariable::TEXT_SOLID));
            showSmsError_ = false;
            phone_hint_->setOffsets(Utils::scale_value(HOR_MARGIN), Utils::scale_value(CODE_ACTIONS_TOP_MARGIN));
            send_again_->setOffsets((Utils::scale_value(DIALOG_WIDTH) - send_again_->desiredWidth()) / 2, showPhoneHint_ ? (phone_hint_->verOffset() + phone_hint_->cachedSize().height() + Utils::scale_value(ACTIONS_SPACING)) : Utils::scale_value(CODE_ACTIONS_TOP_MARGIN));
            phone_call_->setOffsets((Utils::scale_value(DIALOG_WIDTH) - phone_call_->desiredWidth()) / 2, send_again_->verOffset() + send_again_->cachedSize().height() + Utils::scale_value(PHONE_CALL_SPACING));
            update();
        }
    }

    void PhoneWidget::phoneInfoResult(qint64 _seq, const Data::PhoneInfo& _data)
    {
        if (state_ != PhoneWidgetState::ENTER_PHONE_STATE || _data.remaining_lengths_.empty())
            return;

        auto cleanErrors = [this]()
        {
            Utils::ApplyStyle(phoneNumber_, Styling::getParameters().getLineEditCommonQss());
            about_phone_enter_->setText(aboutText_.isEmpty() ? QT_TRANSLATE_NOOP("phone_widget", "Enter your new number") : aboutText_, Styling::getParameters().getColor(Styling::StyleVariable::BASE_PRIMARY));
            about_phone_enter_->getHeight(Utils::scale_value(DIALOG_WIDTH - SPECIFY_NUMBER_HOR_OSSET * 2));
            about_phone_enter_->setOffsets(Utils::scale_value(SPECIFY_NUMBER_HOR_OSSET), about_phone_enter_->verOffset());
            update();
        };

        if (_data.is_phone_valid_)
        {
            cleanErrors();
            next_->setFixedWidth(Utils::scale_value(BUTTON_WIDTH));
            next_->setEnabled(true);
            next_->setText(QT_TRANSLATE_NOOP("popup_window", "NEXT"));

            if (canClose_)
                next_->move(Utils::scale_value(DIALOG_WIDTH - BUTTON_MARGIN) - next_->width(), Utils::scale_value(DIALOG_HEIGHT - BOTTOM_MARGIN) - next_->height());
            else
                next_->move(Utils::scale_value(DIALOG_WIDTH) / 2 - next_->width() / 2, Utils::scale_value(DIALOG_HEIGHT - BOTTOM_MARGIN) - next_->height());
            cancel_->move(Utils::scale_value(BUTTON_MARGIN), Utils::scale_value(DIALOG_HEIGHT - BOTTOM_MARGIN) - cancel_->height());

            return;
        }

        auto remaining = _data.remaining_lengths_[0];
        if (remaining != 0)
        {
            cleanErrors();
            next_->setEnabled(false);

            QString text;
            if (remaining > 0)
            {
                text = Utils::GetTranslator()->getNumberString(
                    remaining,
                    QT_TRANSLATE_NOOP3("phone_widget", "%1 DIGIT LEFT", "1"),
                    QT_TRANSLATE_NOOP3("phone_widget", "%1 DIGITS LEFT", "2"),
                    QT_TRANSLATE_NOOP3("phone_widget", "%1 DIGITS LEFT", "5"),
                    QT_TRANSLATE_NOOP3("phone_widget", "%1 DIGITS LEFT", "21")).arg(remaining);
            }
            else
            {
                text = Utils::GetTranslator()->getNumberString(
                    -remaining,
                    QT_TRANSLATE_NOOP3("phone_widget", "%1 EXTRA DIGIT", "1"),
                    QT_TRANSLATE_NOOP3("phone_widget", "%1 EXTRA DIGITS", "2"),
                    QT_TRANSLATE_NOOP3("phone_widget", "%1 EXTRA DIGITS", "5"),
                    QT_TRANSLATE_NOOP3("phone_widget", "%1 EXTRA DIGITS", "21")).arg(-remaining);
            }

            next_->setText(text);
            QFontMetrics m(Fonts::appFontScaled(14));
            next_->setFixedWidth(Utils::scale_value(BUTTON_HOR_PADDING * 2) + m.width(text));
            if (!canClose_)
            {
                next_->move(Utils::scale_value(DIALOG_WIDTH) / 2 - next_->width() / 2, Utils::scale_value(DIALOG_HEIGHT - BOTTOM_MARGIN) - next_->height());
            }
            else
            {
                auto full = cancel_->width() + next_->width() + Utils::scale_value(SPACE_BETWEEN_BUTTONS);
                cancel_->move((Utils::scale_value(DIALOG_WIDTH) - full) / 2, Utils::scale_value(DIALOG_HEIGHT - BOTTOM_MARGIN) - cancel_->height());
                next_->move(cancel_->x() + cancel_->width() + Utils::scale_value(SPACE_BETWEEN_BUTTONS), Utils::scale_value(DIALOG_HEIGHT - BOTTOM_MARGIN) - next_->height());
            }
            return;
        }

        about_phone_enter_->setText(QT_TRANSLATE_NOOP("phone_widget", "Check that number is correct"), Styling::getParameters().getColor(Styling::StyleVariable::SECONDARY_ATTENTION));
        about_phone_enter_->getHeight(Utils::scale_value(DIALOG_WIDTH - SPECIFY_NUMBER_HOR_OSSET * 2));
        about_phone_enter_->setOffsets(Utils::scale_value(SPECIFY_NUMBER_HOR_OSSET), about_phone_enter_->verOffset());
        Utils::ApplyStyle(phoneNumber_, Styling::getParameters().getLineEditCommonQss(true));
        next_->setFixedWidth(Utils::scale_value(BUTTON_WIDTH));
        next_->setEnabled(false);
        next_->setText(QT_TRANSLATE_NOOP("popup_window", "NEXT"));

        if (canClose_)
            next_->move(Utils::scale_value(DIALOG_WIDTH - BUTTON_MARGIN) - next_->width(), Utils::scale_value(DIALOG_HEIGHT - BOTTOM_MARGIN) - next_->height());
        else
            next_->move(Utils::scale_value(DIALOG_WIDTH) / 2 - next_->width() / 2, Utils::scale_value(DIALOG_HEIGHT - BOTTOM_MARGIN) - next_->height());
        cancel_->move(Utils::scale_value(BUTTON_MARGIN), Utils::scale_value(DIALOG_HEIGHT - BOTTOM_MARGIN) - cancel_->height());

        update();
    }

    void PhoneWidget::getSmsResult(int64_t _seq, int _result, int _codeLength, const QString& _ivrUrl, const QString& _ckecks)
    {
        if (_seq != sendSeq_)
            return;

        if (_result == core::le_success)
        {
            if (_codeLength != 0)
                codeLength_ = _codeLength;

            checks_ = _ckecks;

            ivrUrl_ = _ivrUrl;

            updateTextOnGetSmsResult();
        }
    }

    void PhoneWidget::loginResult(int64_t _seq, int _result)
    {
        if (_seq != sendSeq_)
            return;

        if (_result == 0)
        {
            setState(PhoneWidgetState::FINISH_STATE);
            cancel_->hide();
            label_->setText(QT_TRANSLATE_NOOP("phone_widget", "Number changed"));
            label_->setOffsets(Utils::scale_value(DIALOG_WIDTH) / 2 - label_->desiredWidth() / 2, Utils::scale_value(TOP_MARGIN + SUPERSAFE_HEIGHT + LABEL_OFFSET));
            enteredPhoneNumber_->hide();
            smsCode_->hide();
            next_->setText(QT_TRANSLATE_NOOP("popup_window", "OK"));
            next_->move(Utils::scale_value(DIALOG_WIDTH) / 2 - next_->width() / 2, Utils::scale_value(DIALOG_HEIGHT - BOTTOM_MARGIN) - next_->height());
            next_->show();
            changed_phone_number_->setText(countryCode_->text() + phoneNumber_->text());
            changed_phone_number_->evaluateDesiredSize();
            changed_phone_number_->setOffsets(Utils::scale_value(DIALOG_WIDTH) / 2 - changed_phone_number_->desiredWidth() / 2, changed_phone_number_->verOffset());
        }
        else
        {
            showSmsError_ = true;
            if (_result == core::login_error::le_attach_error_busy_phone)
            {
                sms_error_->setText(QT_TRANSLATE_NOOP("sidebar", "This phone number is already attached to another account.\nPlease edit phone number and try again."));
                sms_error_->getHeight(std::min(Utils::scale_value(DIALOG_WIDTH), sms_error_->desiredWidth()));
                sms_error_->setOffsets((Utils::scale_value(DIALOG_WIDTH) - sms_error_->desiredWidth()) / 2, Utils::scale_value(CODE_ACTIONS_TOP_MARGIN));
                timer_->stop();
                secRemaining_ = SECONDS_TO_RESEND;
                send_again_->setText(QString());
                phone_call_->setText(QString());
            }
            auto offset = (sms_error_->cachedSize().height() + Utils::scale_value(ACTIONS_SPACING));

            smsCode_->setEnabled(true);
            Utils::ApplyStyle(smsCode_, Styling::getParameters().getLineEditCommonQss(true));
            smsCode_->setFocus();

            phone_hint_->setOffsets(Utils::scale_value(HOR_MARGIN), phone_hint_->verOffset() + offset);
            send_again_->setOffsets((Utils::scale_value(DIALOG_WIDTH) - send_again_->desiredWidth()) / 2, send_again_->verOffset() + offset);
            phone_call_->setOffsets((Utils::scale_value(DIALOG_WIDTH) - phone_call_->desiredWidth()) / 2, phone_call_->verOffset() + offset);
        }

        update();
    }

    void PhoneWidget::onTimer()
    {
        --secRemaining_;

        const auto callCheck = LoginPage::isCallCheck(checks_);

        if (secRemaining_ == 0)
        {
            send_again_->setText(callCheck ? QT_TRANSLATE_NOOP("phone_widget", "Recall") : QT_TRANSLATE_NOOP("phone_widget", "Resend code"), Styling::getParameters().getColor(Styling::StyleVariable::TEXT_PRIMARY));
            phone_call_->setText(QT_TRANSLATE_NOOP("phone_widget", "Dictate over the phone"), Styling::getParameters().getColor(Styling::StyleVariable::TEXT_PRIMARY));
            timer_->stop();
        }
        else
        {
            auto ar = secRemaining_ >= 10 ? qsl(" 0:%1") : qsl(" 0:0%1");
            send_again_->setText((callCheck ? QT_TRANSLATE_NOOP("phone_widget", "Recall") : QT_TRANSLATE_NOOP("phone_widget", "Resend code")) + ar.arg(secRemaining_), Styling::getParameters().getColor(Styling::StyleVariable::BASE_PRIMARY));
            phone_call_->setText(QT_TRANSLATE_NOOP("phone_widget", "Dictate over the phone"), Styling::getParameters().getColor(Styling::StyleVariable::BASE_PRIMARY));
        }

        auto offset = 0;
        if (showSmsError_)
            offset += (sms_error_->cachedSize().height() + Utils::scale_value(ACTIONS_SPACING));

        phone_hint_->setOffsets(Utils::scale_value(HOR_MARGIN), Utils::scale_value(CODE_ACTIONS_TOP_MARGIN) + offset);
        send_again_->setOffsets((Utils::scale_value(DIALOG_WIDTH) - send_again_->desiredWidth()) / 2, showPhoneHint_ ? (phone_hint_->verOffset() + phone_hint_->cachedSize().height() + Utils::scale_value(ACTIONS_SPACING)) : Utils::scale_value(CODE_ACTIONS_TOP_MARGIN) + offset);
        phone_call_->setOffsets((Utils::scale_value(DIALOG_WIDTH) - phone_call_->desiredWidth()) / 2, send_again_->verOffset() + send_again_->cachedSize().height() + Utils::scale_value(PHONE_CALL_SPACING));

        update();
    }

    void PhoneWidget::countrySelected(const QString&)
    {
        phoneNumber_->setFocus();
    }
}
