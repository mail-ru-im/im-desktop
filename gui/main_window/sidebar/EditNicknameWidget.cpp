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

    auto getNickFont()
    {
        return Fonts::appFontScaled(16);
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
    {
        setSizePolicy(QSizePolicy::Policy::Fixed, QSizePolicy::Policy::Expanding);
        setFixedWidth(getWidgetWidth());

        auto globalLayout = Utils::emptyVLayout(this);
        globalLayout->setContentsMargins(getLeftMargin(), getTopMargin(), getRightMargin(), 0);

        headerUnit_ = TextRendering::MakeTextUnit(QT_TRANSLATE_NOOP("profile_edit_dialogs", "Nickname"));
        headerUnit_->init(getHeaderFont(), Styling::getParameters().getColor(Styling::StyleVariable::TEXT_SOLID));
        headerUnit_->evaluateDesiredSize();
        headerUnit_->setOffsets(getHeaderHorOffset(), getHeaderVerOffset());

        const auto textMaxWidth = getWidgetWidth() - getLeftMargin() - getRightMargin();

        nickLabelText_ = new TextWidget(this, QT_TRANSLATE_NOOP("profile_edit_dialogs", "People who do not have your phone number can search for you by nickname, by specifying"));
        nickLabelText_->init(getTextFont(), Styling::getParameters().getColor(Styling::StyleVariable::BASE_PRIMARY));
        nickLabelText_->setMaxWidthAndResize(textMaxWidth);
        Testing::setAccessibleName(nickLabelText_, qsl("AS ped nickLabelText_"));
        globalLayout->addWidget(nickLabelText_);

        if constexpr (platform::is_apple())
            globalLayout->addSpacing(Utils::scale_value(4));

        const QString initNick = _initData.nickName_.isEmpty() ? _initData.nickName_ : Utils::makeNick(_initData.nickName_);
        auto nickUnit = Ui::TextRendering::MakeTextUnit(initNick, {}, TextRendering::LinksVisible::DONT_SHOW_LINKS);
        nickUnit->init(getTextFont(), Styling::getParameters().getColor(Styling::StyleVariable::TEXT_PRIMARY), QColor(), QColor(), QColor(),
                       TextRendering::HorAligment::LEFT, 2, Ui::TextRendering::LineBreakType::PREFER_SPACES);
        nickLabel_ = new TextUnitLabel(this, std::move(nickUnit), Ui::TextRendering::VerPosition::TOP, textMaxWidth, true);
        nickLabel_->setCursor(Qt::PointingHandCursor);
        Testing::setAccessibleName(nickLabel_, qsl("AS ped nickLabel_"));
        globalLayout->addWidget(nickLabel_);
        globalLayout->addSpacing(getNickSpacing());

        nickName_ = new NickLineEdit(this, _initData.nickName_);
        globalLayout->addWidget(nickName_);
        globalLayout->addSpacing(getNickSpacing());

        ruleText_ = new TextWidget(this, QT_TRANSLATE_NOOP("profile_edit_dialogs", "Acceptable characters: the Roman alphabet (a-z), digits (0-9), period \".\", underscore \"_\""));
        ruleText_->init(getTextFont(), Styling::getParameters().getColor(Styling::StyleVariable::BASE_PRIMARY));
        ruleText_->setMaxWidthAndResize(textMaxWidth);
        Testing::setAccessibleName(ruleText_, qsl("AS ped ruleText_"));
        globalLayout->addWidget(ruleText_);
        globalLayout->addSpacing(getTextSpacing());

        auto urlTextLayout = Utils::emptyHLayout();
        {
            urlLabelText_ = new TextWidget(this, QT_TRANSLATE_NOOP("profile_edit_dialogs", "This link will open a chat with you in ICQ"));
            urlLabelText_->init(getTextFont(), Styling::getParameters().getColor(Styling::StyleVariable::BASE_PRIMARY));
            urlLabelText_->setMaxWidthAndResize(textMaxWidth);
            Testing::setAccessibleName(urlLabelText_, qsl("AS ped urlLabelText_"));

            auto heightHolder = new QWidget(this);
            heightHolder->setFixedHeight(urlLabelText_->height());

            urlTextLayout->addWidget(urlLabelText_);
            urlTextLayout->addWidget(heightHolder);
        }
        globalLayout->addLayout(urlTextLayout);

        auto urlUnit = Ui::TextRendering::MakeTextUnit(getNickUrl(_initData.nickName_), Data::MentionMap(), TextRendering::LinksVisible::DONT_SHOW_LINKS);
        urlUnit->init(getTextFont(), Styling::getParameters().getColor(Styling::StyleVariable::TEXT_PRIMARY), QColor(), QColor(), QColor(),
                      TextRendering::HorAligment::LEFT, 2, Ui::TextRendering::LineBreakType::PREFER_SPACES);
        urlLabel_ = new TextUnitLabel(this, std::move(urlUnit), Ui::TextRendering::VerPosition::TOP, textMaxWidth, true);
        urlLabel_->setCursor(Qt::PointingHandCursor);
        Testing::setAccessibleName(urlLabel_, qsl("AS ped urlLabel_"));
        globalLayout->addWidget(urlLabel_);
        globalLayout->addSpacing(getBottomSpacing());

        setNickSpinner_ = new LoaderSpinner(this, QSize(), false, false);
        setNickSpinner_->hide();

        setNickTimeout_->setSingleShot(true);
        setNickTimeout_->setInterval(getSetNickTimeout().count());

        connect(nickName_, &NickLineEdit::changed, this, &EditNicknameWidget::onFormChanged);
        connect(nickName_, &NickLineEdit::ready, this, &EditNicknameWidget::onFormReady);
        connect(nickName_, &NickLineEdit::serverError, this, &EditNicknameWidget::onServerError);
        connect(nickName_, &NickLineEdit::nickSet, this, &EditNicknameWidget::onNickSet);
        connect(nickName_, &NickLineEdit::sameNick, this, &EditNicknameWidget::onSameNick);

        connect(setNickSpinner_, &LoaderSpinner::clicked, this, &EditNicknameWidget::cancelSetNickClicked);
        connect(setNickTimeout_, &QTimer::timeout, this, &EditNicknameWidget::onSetNickTimeout);

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

        nickName_->setFocus();
    }

    EditNicknameWidget::~EditNicknameWidget()
    {
        if (!nickSet_)
            Ui::GetDispatcher()->post_stats_to_core(core::stats::stats_event_names::nickeditscr_edit_action, { {"from", statFrom_}, {"do", "cancel"} });
    }

    EditNicknameWidget::FormData EditNicknameWidget::getFormData() const
    {
        return { nickName_->getText() };
    }

    void EditNicknameWidget::clearData()
    {
        nickName_->resetInput();
    }

    void EditNicknameWidget::setButtonsPair(const QPair<DialogButton*, DialogButton*>& _buttonsPair)
    {
        assert(!okButton_ && !cancelButton_);

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

    void EditNicknameWidget::onFormChanged()
    {
        nickReady_ = false;

        auto nick = nickName_->getText();
        if (!nick.isEmpty())
            nick.prepend(ql1c('@'));

        nickLabel_->setText(nick, Styling::getParameters().getColor(Styling::StyleVariable::TEXT_SOLID));
        nickLabel_->setEnabled(false);

        urlLabel_->setText(QString());
        urlLabel_->setEnabled(false);
        urlLabelText_->hide();

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
        if (!nick.isEmpty())
        {
            nickLabel_->setText(Utils::makeNick(nick), Styling::getParameters().getColor(Styling::StyleVariable::TEXT_SOLID));
            urlLabel_->setText(getNickUrl(nick), Styling::getParameters().getColor(Styling::StyleVariable::TEXT_SOLID));
            urlLabelText_->show();
        }

        if (okButton_)
            okButton_->setEnabled(true);
    }

    void EditNicknameWidget::onSameNick()
    {
        auto color = Styling::getParameters().getColor(Styling::StyleVariable::TEXT_PRIMARY);
        nickLabel_->setText(nickLabel_->text(), color);
        urlLabel_->setText(urlLabel_->text(), color);
        nickLabel_->setEnabled(true);
        urlLabel_->setEnabled(true);
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
    }

    void EditNicknameWidget::onNickSet()
    {
        setNickTimeout_->stop();

        setNickSpinner_->stopAnimation();
        setNickSpinner_->hide();

        nickSet_ = true;
        Ui::GetDispatcher()->post_stats_to_core(core::stats::stats_event_names::nickeditscr_edit_action, { {"from", statFrom_}, {"do", "save"} });

        emit changed();
        emit Utils::InterConnector::instance().closeAnyPopupWindow(Utils::CloseWindowInfo());
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
        okButton_->setEnabled(false);
        nickName_->setNickRequest();
        setNickTimeout_->start();
    }

    void EditNicknameWidget::cancelClicked()
    {
        emit Utils::InterConnector::instance().closeAnyPopupWindow(Utils::CloseWindowInfo());
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

    bool EditNicknameWidget::isFormComplete() const
    {
        return nickReady_;
    }

    QString EditNicknameWidget::getNickUrl(const QString& _nick) const
    {
        if (_nick.isEmpty())
            return QString();

        return qsl("https://") % Features::getProfileDomain() % ql1c('/') % _nick;
    }

    void EditNicknameWidget::showToast(const QString& _text) const
    {
        if (toastParent_)
        {
            auto toast = new Ui::Toast(_text, toastParent_);
            Ui::ToastManager::instance()->showToast(toast, QPoint(toastParent_->width() / 2, toastParent_->height() - toast->height() - getToastVerOffset()));
        }
    }

    void showEditNicknameDialog(EditNicknameWidget* _widget)
    {
        Ui::GeneralDialog::Options options;
        options.preferredSize_ = QSize(_widget->width(), -1);
        options.ignoreRejectDlgPairs_.emplace_back(Utils::CloseWindowInfo::Initiator::MacEventFilter, Utils::CloseWindowInfo::Reason::MacEventFilter);

        auto gd = std::make_unique<Ui::GeneralDialog>(_widget, Utils::InterConnector::instance().getMainWindow(), false, true, true, true, options);
        gd->setIgnoredKeys({ Qt::Key_Return, Qt::Key_Enter });

        auto buttonsPair = gd->addButtonsPair(QT_TRANSLATE_NOOP("popup_window", "CANCEL"), QT_TRANSLATE_NOOP("popup_window", "APPLY"), true, false, false);
        _widget->setButtonsPair(buttonsPair);
        gd->setButtonsAreaMargins(QMargins(0, getButtonsTopMargin(), 0, getButtonsBottomMargin()));
        gd->showInCenter();
    }
}
