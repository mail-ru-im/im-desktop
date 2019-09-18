#include "EditNameWidget.h"
#include "SidebarUtils.h"

#include "../../controls/TextUnit.h"
#include "../../controls/TextBrowserEx.h"
#include "../../controls/LineEditEx.h"
#include "../../controls/DialogButton.h"
#include "../../core_dispatcher.h"
#include "../../utils/utils.h"
#include "../../utils/Text.h"
#include "../../utils/InterConnector.h"
#include "../../gui_settings.h"
#include "../../styles/ThemesContainer.h"

namespace
{
    auto getWidgetWidth() noexcept
    {
        return Utils::scale_value(340);
    }

    auto getHeaderVerOffset() noexcept
    {
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
        return Utils::scale_value(79);
    }

    auto getRightMargin() noexcept
    {
        return Utils::scale_value(28);
    }

    auto getNamesSpacing() noexcept
    {
        return Utils::scale_value(27);
    }

    auto getTextToLineSpacing() noexcept
    {
        return Utils::scale_value(7);
    }

    auto getHeaderFont()
    {
        return Fonts::appFontScaled(23);
    }

    auto getNameFont()
    {
        return Fonts::appFontScaled(16);
    }
}

namespace Ui
{

    EditNameWidget::EditNameWidget(QWidget *_parent, const FormData& _initData)
        : QWidget(_parent)
        , okButton_(nullptr)
        , cancelButton_(nullptr)
    {
        setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
        setFixedWidth(getWidgetWidth());

        globalLayout_ = Utils::emptyVLayout(this);
        globalLayout_->setContentsMargins(getLeftMargin(), getTopMargin(), getRightMargin(), 0);

        headerUnit_ = TextRendering::MakeTextUnit(QT_TRANSLATE_NOOP("profile_edit_dialogs", "Edit name"));
        headerUnit_->init(getHeaderFont(), Styling::getParameters().getColor(Styling::StyleVariable::TEXT_SOLID));
        headerUnit_->evaluateDesiredSize();
        headerUnit_->setOffsets(getHeaderHorOffset(), getHeaderVerOffset());

        QFontMetrics fm(getNameFont());
        const auto nameEditWidth = getWidgetWidth() - getLeftMargin() - getRightMargin();
        const auto nameEditHeight = Utils::unscale_value(fm.height() + getTextToLineSpacing());

        LineEditEx::Options lineEditOptions;
        lineEditOptions.noPropagateKeys_ = { Qt::Key_Up, Qt::Key_Down, Qt::Key_Tab, Qt::Key_Backtab };

        firstName_ = new LineEditEx(this, lineEditOptions);
        firstName_->setPlaceholderText(QT_TRANSLATE_NOOP("profile_edit_dialogs", "First Name"));
        firstName_->setSizePolicy(QSizePolicy::Policy::Fixed, QSizePolicy::Policy::Fixed);
        firstName_->setFixedWidth(nameEditWidth);
        firstName_->setAlignment(Qt::AlignLeft | Qt::AlignTop);
        firstName_->setFont(getNameFont());
        firstName_->setText(_initData.firstName_);
        firstName_->setAttribute(Qt::WA_MacShowFocusRect, false);
        firstName_->setContextMenuPolicy(Qt::ContextMenuPolicy::NoContextMenu);
        Utils::ApplyStyle(firstName_, Styling::getParameters().getLineEditCommonQss(false, nameEditHeight));
        Testing::setAccessibleName(firstName_, qsl("AS ped firstName_"));

        lastName_ = new LineEditEx(this, lineEditOptions);
        lastName_->setPlaceholderText(QT_TRANSLATE_NOOP("profile_edit_dialogs", "Last name"));
        lastName_->setSizePolicy(QSizePolicy::Policy::Fixed, QSizePolicy::Policy::Fixed);
        lastName_->setFixedWidth(nameEditWidth);
        lastName_->setAlignment(Qt::AlignLeft | Qt::AlignTop);
        lastName_->setFont(getNameFont());
        lastName_->setText(_initData.lastName_);
        lastName_->setContextMenuPolicy(Qt::ContextMenuPolicy::NoContextMenu);
        lastName_->setAttribute(Qt::WA_MacShowFocusRect, false);
        Utils::ApplyStyle(lastName_, Styling::getParameters().getLineEditCommonQss(false, nameEditHeight));
        Testing::setAccessibleName(lastName_, qsl("AS ped lastName_"));

        connect(firstName_, &LineEditEx::textChanged, this, &EditNameWidget::onFormChanged);
        connect(lastName_, &LineEditEx::textChanged, this, &EditNameWidget::onFormChanged);

        globalLayout_->addWidget(firstName_);
        globalLayout_->addSpacing(getNamesSpacing());
        globalLayout_->addWidget(lastName_);
        globalLayout_->addSpacerItem(new QSpacerItem(0, getNamesSpacing() + Utils::scale_value(2), QSizePolicy::Fixed, QSizePolicy::Expanding));

        firstName_->setFocus();

        connectToLineEdit(firstName_, lastName_, lastName_);
        connectToLineEdit(lastName_, firstName_, firstName_);
    }

    EditNameWidget::~EditNameWidget()
    {
    }

    EditNameWidget::FormData EditNameWidget::getFormData() const
    {
        FormData formData;

        formData.firstName_ = firstName_->text();
        formData.lastName_ = lastName_->text();

        return formData;
    }

    void EditNameWidget::clearData()
    {
        firstName_->clear();
        lastName_->clear();
    }

    void EditNameWidget::setButtonsPair(const QPair<DialogButton*, DialogButton*> &_buttonsPair)
    {
        assert(!okButton_ && !cancelButton_);

        okButton_ = _buttonsPair.first;
        cancelButton_ = _buttonsPair.second;

        connect(okButton_, &DialogButton::clicked, this, &EditNameWidget::okClicked);
        connect(cancelButton_, &DialogButton::clicked, this, &EditNameWidget::cancelClicked);

        okButton_->setEnabled(isFormComplete());
    }

    void EditNameWidget::onFormChanged()
    {
        if (okButton_)
            okButton_->setEnabled(isFormComplete());
    }

    void EditNameWidget::okClicked()
    {
        Utils::UpdateProfile({std::make_pair(std::string("firstName"), firstName_->text()),
                              std::make_pair(std::string("lastName"), lastName_->text())});
        emit changed();
        emit Utils::InterConnector::instance().closeAnyPopupWindow(Utils::CloseWindowInfo());
        close();
    }

    void EditNameWidget::cancelClicked()
    {
        emit Utils::InterConnector::instance().closeAnyPopupWindow(Utils::CloseWindowInfo());
        close();
    }

    void EditNameWidget::paintEvent(QPaintEvent *_event)
    {
        QWidget::paintEvent(_event);

        QPainter p(this);
        headerUnit_->draw(p, Ui::TextRendering::VerPosition::TOP);
    }

    void EditNameWidget::keyPressEvent(QKeyEvent *_event)
    {
        QWidget::keyPressEvent(_event);

        bool isEnter = _event->key() == Qt::Key_Return || _event->key() == Qt::Key_Enter;
        if (isEnter && isFormComplete())
        {
            okClicked();
            _event->accept();
        }
    }

    void EditNameWidget::showEvent(QShowEvent* _e)
    {
        QWidget::showEvent(_e);
        firstName_->setFocus();
    }

    bool EditNameWidget::isFormComplete() const
    {
        return !firstName_->text().isEmpty();
    }

    void EditNameWidget::connectToLineEdit(LineEditEx *_lineEdit, LineEditEx *_onUp, LineEditEx *_onDown) const
    {
        if (_onUp)
        {
            connect(_lineEdit, &LineEditEx::upArrow, _onUp, Utils::QOverload<>::of(&LineEditEx::setFocus));
            connect(_lineEdit, &LineEditEx::backtab, _onUp, [_onUp]() { _onUp->setFocus(Qt::BacktabFocusReason); });
        }

        if (_onDown)
        {
            connect(_lineEdit, &LineEditEx::downArrow, _onDown, Utils::QOverload<>::of(&LineEditEx::setFocus));
            connect(_lineEdit, &LineEditEx::tab, _onDown, [_onDown]() { _onDown->setFocus(Qt::TabFocusReason); });
        }
    }

}
