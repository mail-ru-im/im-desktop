#include "EditNicknameWidget.h"
#include "SidebarUtils.h"

#include "../MainWindow.h"
#include "../../controls/TooltipWidget.h"
#include "../../controls/TextEmojiWidget.h"
#include "../../controls/NickLineEdit.h"
#include "../../controls/DialogButton.h"
#include "../../controls/LoaderSpinner.h"
#include "../../controls/GeneralDialog.h"
#include "../../controls/SemitransparentWindowAnimated.h"

#include "../../core_dispatcher.h"
#include "../../utils/InterConnector.h"
#include "../../utils/features.h"
#include "../../styles/ThemeParameters.h"

#include "../../previewer/toast.h"

namespace
{
    auto getWidgetWidth() noexcept
    {
        return Utils::scale_value(340);
    }

    auto getHeaderVerOffset() noexcept
    {
        if constexpr (platform::is_apple())
            return Utils::scale_value(13);
        else
            return Utils::scale_value(11);
    }

    auto getHeaderHorOffset() noexcept
    {
        return Utils::scale_value(20);
    }

    auto getLeftMargin() noexcept
    {
        return Utils::scale_value(20);
    }

    auto getTopMargin() noexcept
    {
        return Utils::scale_value(68);
    }

    auto getRightMargin() noexcept
    {
        return Utils::scale_value(28);
    }

    auto getNickSpacing() noexcept
    {
        if constexpr (platform::is_apple())
            return Utils::scale_value(25);
        else
            return Utils::scale_value(20);
    }

    auto getTextSpacing() noexcept
    {
        if constexpr (platform::is_apple())
            return Utils::scale_value(15);
        else
            return Utils::scale_value(11);
    }

    auto getBottomSpacing() noexcept
    {
        if constexpr (platform::is_apple())
            return Utils::scale_value(9);
        else
            return Utils::scale_value(5);
    }

    auto getHeaderFont()
    {
        return Fonts::appFontScaled(23);
    }

    auto getTextFont()
    {
        return Fonts::appFontScaled(15);
    }

    auto getToastVerOffset() noexcept
    {
        return Utils::scale_value(10);
    }

    constexpr std::chrono::milliseconds getSetNickTimeout() noexcept
    {
        return std::chrono::milliseconds(200);
    }

    auto getButtonsTopMargin() noexcept
    {
        return Utils::scale_value(21);
    }

    auto getButtonsBottomMargin() noexcept
    {
        return Utils::scale_value(19);
    }
}
namespace Ui
{
    EditNicknameWidget::EditNicknameWidget(QWidget* _parent, const FormData& _initData)
        : QWidget(_parent)
        , okButton_(nullptr)
        , cancelButton_(nullptr)
        , nickReady_(true)
        , setNickTimeout_(new QTimer(this))
        , toastParent_(Utils::InterConnector::instance().getMainWindow())
        , nickSet_(false)
        , groupMode_(_initData.groupMode_)
        , fixedSize_(_initData.fixedSize_)
    {
        if (fixedSize_)
        {
            setSizePolicy(QSizePolicy::Policy::Fixed, QSizePolicy::Policy::Expanding);
            setFixedWidth(getWidgetWidth());
        }

        if (!_initData.margins_.isNull())
            margins_ = _initData.margins_;
        else
            margins_ = QMargins(getLeftMargin(), getTopMargin(), getRightMargin(), 0);

        auto globalLayout = Utils::emptyVLayout(this);
        globalLayout->setContentsMargins(margins_);

        headerUnit_ = TextRendering::MakeTextUnit(QT_TRANSLATE_NOOP("profile_edit_dialogs", "Nickname"));
        headerUnit_->init(getHeaderFont(), Styling::getParameters().getColor(Styling::StyleVariable::TEXT_SOLID));
        headerUnit_->evaluateDesiredSize();
        headerUnit_->setOffsets(getHeaderHorOffset(), getHeaderVerOffset());

        const auto widgetWidth = fixedSize_ ? getWidgetWidth() : width();
        const auto textMaxWidth = widgetWidth - (margins_.left() + margins_.right());

        if (!groupMode_)
        {
            nickLabelText_ = new TextWidget(this, QT_TRANSLATE_NOOP("profile_edit_dialogs", "People who do not have your phone number can search for you by nickname, by specifying"));
            nickLabelText_->init(getTextFont(), Styling::getParameters().getColor(Styling::StyleVariable::BASE_PRIMARY));
            nickLabelText_->setMaxWidthAndResize(textMaxWidth);
            Testing::setAccessibleName(nickLabelText_, qsl("AS ProfilePage nickLabelText"));
            globalLayout->addWidget(nickLabelText_);
        }

        if constexpr (platform::is_apple())
            globalLayout->addSpacing(Utils::scale_value(4));

        if (!groupMode_)
        {
            const QString initNick = _initData.nickName_.isEmpty() ? _initData.nickName_ : Utils::makeNick(_initData.nickName_);
            auto nickUnit = Ui::TextRendering::MakeTextUnit(initNick, {}, TextRendering::LinksVisible::DONT_SHOW_LINKS);
            nickUnit->init(getTextFont(), Styling::getParameters().getColor(Styling::StyleVariable::TEXT_PRIMARY), QColor(), QColor(), QColor(),
                TextRendering::HorAligment::LEFT, 2, Ui::TextRendering::LineBreakType::PREFER_SPACES);
            nickLabel_ = new TextUnitLabel(this, std::move(nickUnit), Ui::TextRendering::VerPosition::TOP, textMaxWidth, true);
            nickLabel_->setCursor(Qt::PointingHandCursor);
            Testing::setAccessibleName(nickLabel_, qsl("AS ProfilePage nickLabel"));
            nickLabel_->setFixedHeight(nickLabel_->sizeHint().height());
            globalLayout->addWidget(nickLabel_);
            globalLayout->addSpacing(getNickSpacing());
        }

        nickName_ = new NickLineEdit(this, _initData.nickName_, _initData.fixedPart_, groupMode_);
        globalLayout->addWidget(nickName_);
        globalLayout->addSpacing(getNickSpacing());

        ruleText_ = new TextWidget(this, QT_TRANSLATE_NOOP("profile_edit_dialogs", "Acceptable characters: the Roman alphabet (a-z), digits (0-9), period \".\", underscore \"_\""));
        ruleText_->init(getTextFont(), Styling::getParameters().getColor(Styling::StyleVariable::BASE_PRIMARY));
        ruleText_->setMaxWidthAndResize(textMaxWidth);
        Testing::setAccessibleName(ruleText_, qsl("AS ProfilePage nickRuleText"));
        globalLayout->addWidget(ruleText_);
        globalLayout->addSpacing(getTextSpacing());

        if (!groupMode_)
        {
            auto nickWidget = new QWidget();
            auto nickWidgetLayout = Utils::emptyVLayout(nickWidget);
            urlLabelText_ = new TextWidget(this, QT_TRANSLATE_NOOP("profile_edit_dialogs", "This link will open a chat with you in ICQ"));
            urlLabelText_->init(getTextFont(), Styling::getParameters().getColor(Styling::StyleVariable::BASE_PRIMARY));
            urlLabelText_->setMaxWidthAndResize(textMaxWidth);
            Testing::setAccessibleName(urlLabelText_, qsl("AS ProfilePage nickUrlLabelText"));

            nickWidgetLayout->addWidget(urlLabelText_);

            auto urlUnit = Ui::TextRendering::MakeTextUnit(getNickUrl(_initData.nickName_), Data::MentionMap(), TextRendering::LinksVisible::DONT_SHOW_LINKS);
            urlUnit->init(getTextFont(), Styling::getParameters().getColor(Styling::StyleVariable::TEXT_PRIMARY), QColor(), QColor(), QColor(),
                TextRendering::HorAligment::LEFT, 2, Ui::TextRendering::LineBreakType::PREFER_SPACES);
            urlLabel_ = new TextUnitLabel(this, std::move(urlUnit), Ui::TextRendering::VerPosition::TOP, textMaxWidth, true);
            urlLabel_->setCursor(Qt::PointingHandCursor);
            Testing::setAccessibleName(urlLabel_, qsl("AS ProfilePage nickUrlLabel"));
            urlLabel_->setFixedHeight(urlLabel_->sizeHint().height());
            nickWidgetLayout->addWidget(urlLabel_);
            nickWidget->setFixedHeight(nickWidget->sizeHint().height());
            globalLayout->addWidget(nickWidget);
            globalLayout->addSpacing(getBottomSpacing());
        }

        setNickSpinner_ = new LoaderSpinner(this, QSize(), LoaderSpinner::Option::Cancelable);
        setNickSpinner_->hide();

        setNickTimeout_->setSingleShot(true);
        setNickTimeout_->setInterval(getSetNickTimeout());

        connect(nickName_, &NickLineEdit::changed, this, &EditNicknameWidget::onFormChanged);
        connect(nickName_, &NickLineEdit::ready, this, &EditNicknameWidget::onFormReady);
        connect(nickName_, &NickLineEdit::serverError, this, &EditNicknameWidget::onServerError);
        connect(nickName_, &NickLineEdit::nickSet, this, &EditNicknameWidget::onNickSet);
        connect(nickName_, &NickLineEdit::sameNick, this, &EditNicknameWidget::onSameNick);
        connect(nickName_, &NickLineEdit::checkError, this, &EditNicknameWidget::error);

        connect(setNickSpinner_, &LoaderSpinner::clicked, this, &EditNicknameWidget::cancelSetNickClicked);
        connect(setNickTimeout_, &QTimer::timeout, this, &EditNicknameWidget::onSetNickTimeout);

        if (!groupMode_)
        {
            connect(nickLabel_, &TextUnitLabel::clicked, this, [this]() {
                QApplication::clipboard()->setText(nickLabel_->text());
                showToast(QT_TRANSLATE_NOOP("sidebar", "Nickname copied"));
                Ui::GetDispatcher()->post_stats_to_core(core::stats::stats_event_names::nickeditscr_edit_action, { {"from", statFrom_}, {"do", "nicktap"} });
            });

            connect(urlLabel_, &TextUnitLabel::clicked, this, [this]() {
                QApplication::clipboard()->setText(urlLabel_->text());
                showToast(QT_TRANSLATE_NOOP("sidebar", "Link copied"));
                Ui::GetDispatcher()->post_stats_to_core(core::stats::stats_event_names::nickeditscr_edit_action, { {"from", statFrom_}, {"do", "linktap"} });
            });
        }

        nickName_->setFocus();
    }

    EditNicknameWidget::~EditNicknameWidget()
    {
        if (!nickSet_ && !statFrom_.empty())
            Ui::GetDispatcher()->post_stats_to_core(core::stats::stats_event_names::nickeditscr_edit_action, { {"from", statFrom_}, {"do", "cancel"} });
    }

    EditNicknameWidget::FormData EditNicknameWidget::getFormData() const
    {
        EditNicknameWidget::FormData formData;
        formData.nickName_ = nickName_->getText();
        return formData;
    }

    void EditNicknameWidget::clearData()
    {
        nickName_->resetInput();
    }

    void EditNicknameWidget::setButtonsPair(const QPair<DialogButton*, DialogButton*>& _buttonsPair)
    {
        im_assert(!okButton_ && !cancelButton_);

        okButton_ = _buttonsPair.first;
        cancelButton_ = _buttonsPair.second;

        // without Qt::QueuedConnection doesn't work setEnabled(false) in okClicked()
        connect(okButton_, &DialogButton::clicked, this, &EditNicknameWidget::okClicked, Qt::QueuedConnection);
        connect(cancelButton_, &DialogButton::clicked, this, &EditNicknameWidget::cancelClicked);

        okButton_->setEnabled(true);
    }

    void EditNicknameWidget::setToastParent(QWidget* _parent)
    {
        toastParent_ = _parent;
    }

    void Ui::EditNicknameWidget::setStatFrom(std::string_view _from)
    {
        statFrom_ = _from;
    }

    void Ui::EditNicknameWidget::setNick(const QString& _nick)
    {
        nickName_->setText(_nick, true);
        nickName_->updateCounter();
    }

    QString Ui::EditNicknameWidget::getNick() const
    {
        return nickName_->getText();
    }

    void EditNicknameWidget::clearHint()
    {
        nickName_->clearHint();
    }

    void EditNicknameWidget::onFormChanged()
    {
        nickReady_ = false;

        auto nick = nickName_->getText();
        if (!nick.isEmpty())
            nick.prepend(u'@');

        if (!groupMode_)
        {
            nickLabel_->setText(nick, Styling::getParameters().getColor(Styling::StyleVariable::TEXT_SOLID));
            nickLabel_->setEnabled(false);
            urlLabel_->setText(QString());
            urlLabel_->setEnabled(false);
            urlLabelText_->hide();
        }

        if (okButton_)
        {
            if (!changeOkButtonText_.isEmpty())
            {
               okButton_->setText(changeOkButtonText_);
               changeOkButtonText_.clear();
            }
            okButton_->setEnabled(isFormComplete());
        }
    }

    void EditNicknameWidget::onFormReady()
    {
        nickReady_ = true;

        const auto nick = nickName_->getText();
        if (!nick.isEmpty() && !groupMode_)
        {
            nickLabel_->setText(Utils::makeNick(nick), Styling::getParameters().getColor(Styling::StyleVariable::TEXT_SOLID));
            urlLabel_->setText(getNickUrl(nick), Styling::getParameters().getColor(Styling::StyleVariable::TEXT_SOLID));
            urlLabelText_->show();
        }

        if (okButton_)
            okButton_->setEnabled(true);

        Q_EMIT ready();
    }

    void EditNicknameWidget::onSameNick()
    {
        if (groupMode_)
            return;

        auto color = Styling::getParameters().getColor(Styling::StyleVariable::TEXT_PRIMARY);
        nickLabel_->setText(nickLabel_->text(), color);
        nickLabel_->setEnabled(true);
        urlLabel_->setText(urlLabel_->text(), color);
        urlLabel_->setEnabled(true);

        Q_EMIT theSame();
    }

    void EditNicknameWidget::onServerError(bool _repeateOn)
    {
        setNickTimeout_->stop();

        setNickSpinner_->stopAnimation();
        setNickSpinner_->hide();

        if (okButton_)
        {
            if (_repeateOn)
            {
                if (changeOkButtonText_.isEmpty())
                    changeOkButtonText_ = okButton_->text();
                okButton_->setText(QT_TRANSLATE_NOOP("popup_window", "REPEAT"));
                okButton_->setEnabled(true);
            }
            else
            {
                okButton_->setEnabled(false);
            }
        }

        Q_EMIT error();
    }

    void EditNicknameWidget::onNickSet()
    {
        setNickTimeout_->stop();

        setNickSpinner_->stopAnimation();
        setNickSpinner_->hide();

        nickSet_ = true;
        Ui::GetDispatcher()->post_stats_to_core(core::stats::stats_event_names::nickeditscr_edit_action, { {"from", statFrom_}, {"do", "save"} });

        Q_EMIT changed();
        Q_EMIT Utils::InterConnector::instance().closeAnyPopupWindow(Utils::CloseWindowInfo());
        close();
    }

    void EditNicknameWidget::onSetNickTimeout()
    {
        setNickSpinner_->setFixedSize(width(), height() - getTopMargin());
        setNickSpinner_->move(x(), getTopMargin());
        setNickSpinner_->show();
        setNickSpinner_->startAnimation(Styling::getParameters().getColor(Styling::StyleVariable::PRIMARY),
                                        Styling::getParameters().getColor(Styling::StyleVariable::BASE_BRIGHT));
    }

    void EditNicknameWidget::okClicked()
    {
        if (groupMode_)
            return;

        okButton_->setEnabled(false);
        nickName_->setNickRequest();
        setNickTimeout_->start();
    }

    void EditNicknameWidget::cancelClicked()
    {
        Q_EMIT Utils::InterConnector::instance().closeAnyPopupWindow(Utils::CloseWindowInfo());
        close();
    }

    void EditNicknameWidget::cancelSetNickClicked()
    {
        if (okButton_)
            okButton_->setEnabled(true);

        setNickSpinner_->stopAnimation();
        setNickSpinner_->hide();

        nickName_->cancelSetNick();
    }

    void EditNicknameWidget::paintEvent(QPaintEvent* _event)
    {
        QWidget::paintEvent(_event);

        QPainter p(this);
        if (!groupMode_)
            headerUnit_->draw(p, Ui::TextRendering::VerPosition::TOP);
    }

    void EditNicknameWidget::keyPressEvent(QKeyEvent* _event)
    {
        QWidget::keyPressEvent(_event);

        bool isEnter = _event->key() == Qt::Key_Return || _event->key() == Qt::Key_Enter;
        if (isEnter && isFormComplete())
        {
            okClicked();
            _event->accept();
        }
    }

    void EditNicknameWidget::showEvent(QShowEvent* _e)
    {
        QWidget::showEvent(_e);
        nickName_->setFocus();
    }

    void EditNicknameWidget::resizeEvent(QResizeEvent* _event)
    {
        const auto widgetWidth = fixedSize_ ? getWidgetWidth() : width();
        const auto textMaxWidth = widgetWidth - (margins_.left() + margins_.right());

        if (!groupMode_)
        {
            nickLabelText_->setMaxWidthAndResize(textMaxWidth);
            urlLabelText_->setMaxWidthAndResize(textMaxWidth);
        }

        ruleText_->setMaxWidthAndResize(textMaxWidth);

        QWidget::resizeEvent(_event);
    }

    bool EditNicknameWidget::isFormComplete() const
    {
        return nickReady_;
    }

    QString EditNicknameWidget::getNickUrl(const QString& _nick) const
    {
        if (_nick.isEmpty())
            return QString();

        return u"https://" % Features::getProfileDomain() % u'/' % _nick;
    }

    void EditNicknameWidget::showToast(const QString& _text) const
    {
        if (toastParent_)
        {
            auto toast = new Ui::Toast(_text, toastParent_);
            Testing::setAccessibleName(toast, qsl("AS General toast"));
            Ui::ToastManager::instance()->showToast(toast, QPoint(toastParent_->width() / 2, toastParent_->height() - toast->height() - getToastVerOffset()));
        }
    }

    void showEditNicknameDialog(EditNicknameWidget* _widget)
    {
        Ui::GeneralDialog::Options options;
        options.preferredWidth_ = _widget->width();
        options.ignoredInfos_.emplace_back(Utils::CloseWindowInfo::Initiator::MacEventFilter, Utils::CloseWindowInfo::Reason::MacEventFilter);

        auto gd = std::make_unique<Ui::GeneralDialog>(_widget, Utils::InterConnector::instance().getMainWindow(), options);
        gd->setIgnoredKeys({ Qt::Key_Return, Qt::Key_Enter });

        auto buttonsPair = gd->addButtonsPair(QT_TRANSLATE_NOOP("popup_window", "Cancel"), QT_TRANSLATE_NOOP("popup_window", "OK"), true, false, false);
        _widget->setButtonsPair(buttonsPair);
        gd->setButtonsAreaMargins(QMargins(0, getButtonsTopMargin(), 0, getButtonsBottomMargin()));
        gd->showInCenter();
    }
}
