#include "stdafx.h"
#include "TaskEditWidget.h"

#include "styles/ThemeParameters.h"
#include "controls/TransparentScrollBar.h"
#include "../../controls/DateTimeWidget.h"
#include "../../controls/SwitcherCheckbox.h"
#include "../../controls/TextWidget.h"
#include "../../controls/GeneralDialog.h"
#include "../../app_config.h"

#include "controls/TextEditEx.h"
#include "controls/DialogButton.h"
#include "utils/InterConnector.h"
#include "core_dispatcher.h"
#include "utils/utils.h"
#include "fonts.h"
#include "main_window/containers/LastseenContainer.h"
#include "../MainWindow.h"

#include "AssigneePopup.h"
#include "AssigneeEdit.h"

namespace
{
    auto commonMargin() noexcept
    {
        return Utils::scale_value(20);
    }

    constexpr auto descriptionEditDocumentMarginNonScalable() noexcept
    {
        return 2; // must be non-scaled
    }

    auto descriptionEditBottomSpace() noexcept
    {
        return Utils::scale_value(8);
    }

    auto assigneeEditBottomMargin() noexcept
    {
        return Utils::scale_value(20);
    }

    auto assigneeEditBottomPadding() noexcept
    {
        return Utils::scale_value(8);
    }

    constexpr auto assigneeEditHeightNonScalable() noexcept
    {
        return 28; // must be non-scaled
    }

    auto dialogButtonWidth() noexcept
    {
        return Utils::scale_value(116);
    }

    Ui::DialogButton* createButton(QWidget* _parent, const QString& _text, const Ui::DialogButtonRole _role, bool _enabled = true)
    {
        auto button = new Ui::DialogButton(_parent, _text, _role);
        button->setFixedWidth(dialogButtonWidth());
        button->setEnabled(_enabled);
        return button;
    }

    constexpr int maxDescriptionLength() noexcept
    {
        return 140;
    }

    constexpr int maxLimitOverflow() noexcept
    {
        return 99;
    }

    auto remainedFont()
    {
        return Fonts::appFontScaled(11);
    }

    auto remainedColor()
    {
        return Styling::ThemeColorKey{ Styling::StyleVariable::BASE_PRIMARY };
    }

    auto errorColor()
    {
        return Styling::ThemeColorKey{ Styling::StyleVariable::SECONDARY_ATTENTION };
    }

    auto remainedTopMargin() noexcept
    {
        return Utils::scale_value(8);
    }

    auto remainedBottomMargin() noexcept
    {
        return Utils::scale_value(4);
    }

    auto deadlineCheckboxHeight() noexcept { return Utils::scale_value(30); }

    auto deadlineEditContentMargins() noexcept { return Utils::scale_value(QMargins(0, 10, 0, 10)); }

    auto deadlineTextUnitFont() noexcept
    {
        return Fonts::appFontScaled(15);
    }

    auto deadlineTextUnitColor() noexcept
    {
        return Styling::ThemeColorKey{ Styling::StyleVariable::TEXT_SOLID };
    }

    auto spacingAfterDeadlineEdit() noexcept { return Utils::scale_value(20); }

    constexpr std::chrono::milliseconds dateTimeAnimationDuration() noexcept { return std::chrono::milliseconds(300); }

    const QTime& getDefaultEndOfDay() noexcept
    {
        static const QTime endOfDay(18, 0, 0);
        return endOfDay;
    }

    const QTime& getDefaultDeadlineTime() noexcept
    {
        static const QTime deadline(19, 0, 0);
        return deadline;
    }

    auto buttonsSpacing() noexcept
    {
        return Utils::scale_value(20);
    }

    const auto deadlineTextUnitVerticalCentering() noexcept
    {
        return Utils::scale_value(2);
    }
} // namespace

namespace Ui
{
    TaskEditWidget::TaskEditWidget(const QString& _contact, const QString& _assignee, QWidget* _parent)
        : QWidget(_parent)
        , contact_{ _contact }
    {
        initialize();
        if (!(contact_.isEmpty() || Utils::isChat(contact_) || Logic::GetLastseenContainer()->isBot(contact_)))
            assigneeEdit_->selectContact(contact_);
        if (!(_assignee.isEmpty() || Utils::isChat(_assignee) || Logic::GetLastseenContainer()->isBot(_assignee)))
            assigneeEdit_->selectContact(_assignee);
        deadlineCheckbox_->setChecked(true);
        setDefaultDeadline();
    }

    TaskEditWidget::TaskEditWidget(const Data::TaskData& _task, const Data::MentionMap& _mentions, QWidget* _parent)
        : QWidget(_parent)
        , task_{ _task }
    {
        initialize();
        setInputData(_task.params_.title_, {}, _mentions);
        descriptionEdit_->moveCursor(QTextCursor::End);
        if (!_task.params_.assignee_.isEmpty())
            assigneeEdit_->selectContact(_task.params_.assignee_);
        
        deadlineCheckbox_->setChecked(!_task.params_.date_.isNull());
        if (const auto& date = _task.params_.date_; date.isValid())
            deadlineEdit_->setDateTime(date);
        else
            setDefaultDeadline();
    }

    TaskEditWidget::~TaskEditWidget()
    {
        if (assigneePicker_)
            assigneePicker_->deleteLater();
    }

    void TaskEditWidget::setInputData(const Data::FString& _text, const Data::QuotesVec& _quotes, const Data::MentionMap& _mentions)
    {
        descriptionEdit_->setMentions(_mentions);
        descriptionEdit_->setPlainText(_text.string().trimmed(), false);
        descriptionEdit_->moveCursor(QTextCursor::End);

        quotes_ = _quotes;
        mentions_ = _mentions;
        initialInputText_ = _text;
        initialCursorPosition_ = _text.size();
    }

    void TaskEditWidget::setFocusOnTaskTitleInput()
    {
        descriptionEdit_->setFocus();
    }

    Data::FString TaskEditWidget::getInputText() const
    {
        return initialInputText_.isEmpty() ? Data::FString(descriptionEdit_->getPlainText()) : initialInputText_;
    }

    int TaskEditWidget::getCursorPos() const
    {
        return initialCursorPosition_.value_or(descriptionEdit_->textCursor().position());
    }

    QString TaskEditWidget::getHeader() const
    {
        return isEditingTask() ? QT_TRANSLATE_NOOP("task_popup", "Edit task") : QT_TRANSLATE_NOOP("task_popup", "Create task");
    }

    void TaskEditWidget::keyPressEvent(QKeyEvent* _event)
    {
        if (_event->key() == Qt::Key_Escape && assigneePicker_ && assigneePicker_->isVisible())
            assigneePicker_->hide();
        else
            QWidget::keyPressEvent(_event);
    }

    void TaskEditWidget::resizeEvent(QResizeEvent* _event)
    {
        descriptionEdit_->adjustHeight(width() - 2 * commonMargin());
        QWidget::resizeEvent(_event);
    }

    void TaskEditWidget::showEvent(QShowEvent* _event)
    {
        updateContentSizeIfPossible();
        deadlineCheckbox_->setChecked(deadlineCheckbox_->isChecked());
        deadlineEdit_->setEnabledDate(deadlineCheckbox_->isChecked());
        deadlineEdit_->setEnabledTime(deadlineCheckbox_->isChecked());
        checkDate();
    }

    void TaskEditWidget::initialize()
    {
        auto layout = Utils::emptyVLayout(this);
        const auto dialogMargin = commonMargin();
        layout->setContentsMargins(0, dialogMargin, 0, dialogMargin);

        scrollableContent_ = new QWidget(this);
        auto contentLayout = Utils::emptyVLayout(scrollableContent_);

        initializeDescriptionEdit();
        contentLayout->addWidget(descriptionEdit_);

        remainedCount_ = new TextWidget(this, QString{});
        remainedCount_->init({ remainedFont(), remainedColor() });
        errorUnit_ = new TextWidget(this, QT_TRANSLATE_NOOP("task_popup", "Maximum length - %1 symbols").arg(maxDescriptionLength()));
        errorUnit_->init({ remainedFont(), errorColor() });
        auto labelsLayout = Utils::emptyHLayout();
        labelsLayout->addWidget(errorUnit_);
        labelsLayout->addStretch();
        labelsLayout->addWidget(remainedCount_);

        contentLayout->addSpacing(remainedTopMargin());
        contentLayout->addLayout(labelsLayout);
        contentLayout->addSpacing(remainedBottomMargin());

        initializeAssigneeEdit();
        contentLayout->addWidget(assigneeEdit_);
        contentLayout->addSpacing(assigneeEditBottomMargin());

        initializeDeadlineCheckbox();
        deadlineTextUnit_ = new TextWidget(this, QT_TRANSLATE_NOOP("task_popup", "Deadline for the task"));
        deadlineTextUnit_->init({ deadlineTextUnitFont(), deadlineTextUnitColor() });

        // vertical centering
        auto deadlineTextUnitVLayout = Utils::emptyVLayout();
        deadlineTextUnitVLayout->addStretch();
        deadlineTextUnitVLayout->addWidget(deadlineTextUnit_);
        deadlineTextUnitVLayout->addSpacing(deadlineTextUnitVerticalCentering());

        auto deadlineContentLayout = Utils::emptyHLayout();
        deadlineContentLayout->addLayout(deadlineTextUnitVLayout);
        deadlineContentLayout->addStretch();
        deadlineContentLayout->addWidget(deadlineCheckbox_);
        contentLayout->addLayout(deadlineContentLayout);

        initializeDeadlineEdit();
        contentLayout->addWidget(deadlineEdit_);

        contentLayout->setSizeConstraint(QLayout::SetMinAndMaxSize);
        contentLayout->setContentsMargins(dialogMargin, 0, dialogMargin, 0);

        auto scrollArea = CreateScrollAreaAndSetTrScrollBarV(this);
        scrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        scrollArea->setFocusPolicy(Qt::NoFocus);
        scrollArea->setWidgetResizable(true);
        scrollArea->setStyleSheet(qsl("background-color: transparent; border: none"));
        scrollArea->setWidget(scrollableContent_);
        scrollArea->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
        scrollArea->setContentsMargins(0, 0, 0, 0);
        layout->addWidget(scrollArea);

        layout->addSpacing(spacingAfterDeadlineEdit());

        auto backButton = createButton(this, QT_TRANSLATE_NOOP("task_popup", "Cancel"), DialogButtonRole::CANCEL);
        backButton->setFocusPolicy(Qt::TabFocus);
        Testing::setAccessibleName(backButton, qsl("AS TaskEditWidget backButton"));
        connect(backButton, &DialogButton::clicked, this, []()
        {
            Q_EMIT Utils::InterConnector::instance().closeAnyPopupWindow({});
        });
        nextButton_ = createButton(this, isEditingTask() ? QT_TRANSLATE_NOOP("task_popup", "Ok") : QT_TRANSLATE_NOOP("task_popup", "Create"), DialogButtonRole::CONFIRM, false);
        nextButton_->setFocusPolicy(Qt::TabFocus);
        connect(nextButton_, &DialogButton::clicked, this, &TaskEditWidget::onSendClick);
        Testing::setAccessibleName(nextButton_, qsl("AS TaskEditWidget doneButton"));

        auto buttonsLayout = Utils::emptyHLayout();
        buttonsLayout->setContentsMargins(dialogMargin, 0, dialogMargin, 0);
        buttonsLayout->addStretch();
        buttonsLayout->addWidget(backButton);
        buttonsLayout->addSpacing(buttonsSpacing());
        buttonsLayout->addWidget(nextButton_);
        buttonsLayout->addStretch();
        layout->addLayout(buttonsLayout);

        connect(&Utils::InterConnector::instance(), &Utils::InterConnector::mainWindowResized, this, &TaskEditWidget::updateContentSizeIfPossible);

        checkName();
        checkAssignee();
        checkDate();

        Testing::setAccessibleName(this, qsl("AS TaskEditWidget"));
    }

    void TaskEditWidget::initializeDescriptionEdit()
    {
        descriptionEdit_ = new Ui::TextEditEx(this, Fonts::appFontScaled(15), Styling::ThemeColorKey{ Styling::StyleVariable::TEXT_SOLID }, true, true);
        descriptionEdit_->setPlaceholderText(QT_TRANSLATE_NOOP("task_popup", "Name"));
        descriptionEdit_->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        descriptionEdit_->setTextInteractionFlags(Qt::TextEditable | Qt::TextEditorInteraction);
        descriptionEdit_->setCursor(Qt::IBeamCursor);
        descriptionEdit_->viewport()->setCursor(Qt::IBeamCursor);
        descriptionEdit_->setEnterKeyPolicy(TextEditEx::EnterKeyPolicy::FollowSettingsRules);
        descriptionEdit_->setTabChangesFocus(true);
        descriptionEdit_->document()->setDocumentMargin(descriptionEditDocumentMarginNonScalable());
        descriptionEdit_->setHeightSupplement(descriptionEditBottomSpace());
        descriptionEdit_->setMaxHeight(GeneralDialog::maximumHeight());
        Utils::ApplyStyle(descriptionEdit_, Styling::getParameters().getTextEditCommonQss(true, Styling::StyleVariable::BASE_SECONDARY));

        connect(descriptionEdit_, &TextEditEx::cursorPositionChanged, this, &TaskEditWidget::onNameCursorPositionChanged);
        connect(descriptionEdit_, &TextEditEx::textChanged, this, &TaskEditWidget::onNameChanged);
        connect(descriptionEdit_, &TextEditEx::enter, this, &TaskEditWidget::onSendClick);
        connect(descriptionEdit_, &TextEditEx::setSize, this, &TaskEditWidget::updateContentSizeIfPossible);

        Testing::setAccessibleName(descriptionEdit_, qsl("AS TaskEditWidget nameEdit"));
    }

    void TaskEditWidget::initializeAssigneeEdit()
    {
        LineEditEx::Options lineEditOptions{ { Qt::Key_Up, Qt::Key_Down } };
        assigneeEdit_ = new AssigneeEdit(this, lineEditOptions);
        auto lineEditMargins = assigneeEdit_->textMargins();
        lineEditMargins.setBottom(assigneeEditBottomPadding());
        assigneeEdit_->setTextMargins(lineEditMargins);
        assigneeEdit_->setCustomPlaceholder(QT_TRANSLATE_NOOP("task_popup", "Assignee"));
        assigneeEdit_->setAlignment(Qt::AlignLeft | Qt::AlignBottom);
        assigneeEdit_->setAttribute(Qt::WA_MacShowFocusRect, false);
        assigneeEdit_->setFont(Fonts::appFontScaled(15));
        assigneeEdit_->setCustomPlaceholderColor(Styling::ThemeColorKey{ Styling::StyleVariable::BASE_PRIMARY });
        assigneeEdit_->setContextMenuPolicy(Qt::ContextMenuPolicy::NoContextMenu);
        Utils::ApplyStyle(assigneeEdit_, Styling::getParameters().getLineEditCommonQss(false, assigneeEditHeightNonScalable(), Styling::StyleVariable::BASE_SECONDARY));

        connect(assigneeEdit_, &LineEditEx::focusIn, this, &TaskEditWidget::showAssigneePicker);
        connect(assigneeEdit_, &LineEditEx::clicked, this, &TaskEditWidget::showAssigneePicker);
        connect(assigneeEdit_, &LineEditEx::textChanged, this, &TaskEditWidget::checkAssignee);
        connect(assigneeEdit_, &AssigneeEdit::selectedContactChanged, this, &TaskEditWidget::checkAssignee);
        connect(assigneeEdit_, &AssigneeEdit::enter, this, &TaskEditWidget::onSendClick);

        Testing::setAccessibleName(assigneeEdit_, qsl("AS TaskEditWidget assigneeEdit"));
    }

    void TaskEditWidget::initializeDeadlineCheckbox()
    {
        deadlineCheckbox_ = new Ui::SwitcherCheckbox(this);
        deadlineCheckbox_->setFixedHeight(deadlineCheckboxHeight());

        connect(deadlineCheckbox_, &Ui::SwitcherCheckbox::clicked, this, &TaskEditWidget::onDeadlineCheckboxClick);

        Testing::setAccessibleName(deadlineCheckbox_, qsl("AS TaskEditWidget deadlineCheckbox"));
    }

    void TaskEditWidget::initializeDeadlineEdit()
    {
        deadlineEdit_ = new DateTimePicker(this, Ui::GetAppConfig().showSecondsInTimePicker());
        deadlineEdit_->setContentsMargins(deadlineEdit_->contentsMargins() + deadlineEditContentMargins());
        deadlineEdit_->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
        deadlineEdit_->setFixedHeight(dateTimeWidgetHeight());

        connect(deadlineEdit_, &DateTimePicker::dateTimeChanged, this, &TaskEditWidget::checkDate);
        connect(deadlineEdit_, &DateTimePicker::dateTimeValidityChanged, this, &TaskEditWidget::checkDate);
        connect(deadlineEdit_, &DateTimePicker::enter, this, &TaskEditWidget::onSendClick);

        deadlineEdit_->setAccessibilityPrefix(qsl("TaskEditWidget"));
    }

    void TaskEditWidget::checkName()
    {
        auto text = descriptionEdit_->getPlainText();

        auto count = 0;
        auto finder = QTextBoundaryFinder(QTextBoundaryFinder::Grapheme, text);
        while (count - maxDescriptionLength() < maxLimitOverflow() && finder.toNextBoundary() != -1)
            count++;

        if (finder.position() > 0)
        {
            QSignalBlocker sb(descriptionEdit_);
            text.truncate(finder.position());
            descriptionEdit_->setPlainText(text);
            descriptionEdit_->moveCursor(QTextCursor::End);
        }

        const bool errorVisible = count > maxDescriptionLength();
        errorUnit_->setVisible(errorVisible);

        remainedCount_->setText(QString::number(maxDescriptionLength() - count), errorVisible ? errorColor() : remainedColor());

        if (const auto taskTitleCorrect = !QStringView(text).trimmed().isEmpty() && !errorVisible; taskTitleCorrect != taskTitleCorrect_)
        {
            taskTitleCorrect_ = taskTitleCorrect;
            checkForm();
        }
    }

    void TaskEditWidget::checkForm()
    {
        nextButton_->setEnabled(isFormCorrect());
    }

    bool TaskEditWidget::isFormCorrect() const
    {
        return taskTitleCorrect_ && assigneeCorrect_ && dateCorrect_;
    }

    int TaskEditWidget::dateTimeWidgetHeight() const
    {
        return deadlineEdit_->sizeHint().height();
    }

    int TaskEditWidget::dialogHeight() const
    {
        auto h = 2 * commonMargin();

        h += descriptionEdit_->height();
        h += remainedTopMargin();
        h += remainedCount_->height();
        h += remainedBottomMargin();
        h += assigneeEdit_->height();
        h += assigneeEditBottomMargin();
        h += deadlineCheckbox_->height();
        h += deadlineEdit_->isVisible() ? deadlineEdit_->height() : 0;

        h += spacingAfterDeadlineEdit();
        h += nextButton_->height();
        return h;
    }

    void TaskEditWidget::setDefaultDeadline() noexcept
    {
        const auto currentDateTime = QDateTime::currentDateTime();
        const auto isPastEndOfDay = currentDateTime.time() > getDefaultEndOfDay();
        deadlineEdit_->setDateTime(QDateTime(currentDateTime.date().addDays(isPastEndOfDay ? 1 : 0), getDefaultDeadlineTime()));
    }

    void TaskEditWidget::updateContentSizeIfPossible()
    {
        if (!parentWidget())
            return;

        auto mw = Utils::InterConnector::instance().getMainWindow();
        const auto titleHeight = mw->getTitleHeight();
        const auto dialogVerticalMargins = 2 * (GeneralDialog::verticalMargin() + GeneralDialog::shadowWidth());
        const auto headerHeight = parentWidget()->height() - height();
        const auto maxAvailableHeight = mw->height() - titleHeight - dialogVerticalMargins - headerHeight;
        const auto maxDialogHeight = GeneralDialog::maximumHeight() - headerHeight;
        const auto desiredHeight = dialogHeight();
        const auto h = std::min(std::min(desiredHeight, maxAvailableHeight), maxDialogHeight);
        setFixedHeight(h);
    }

    void TaskEditWidget::onNameChanged()
    {
        initialInputText_.clear();
        initialCursorPosition_.reset();
        checkName();
    }

    void TaskEditWidget::onNameCursorPositionChanged()
    {
        if (initialCursorPosition_)
            initialCursorPosition_ = descriptionEdit_->textCursor().position();
    }

    void TaskEditWidget::onDeadlineCheckboxClick(bool _isChecked)
    {
        deadlineEdit_->setEnabledDate(_isChecked);
        deadlineEdit_->setEnabledTime(_isChecked);
        checkDate();
    }

    void TaskEditWidget::onSendClick()
    {
        if (assigneePicker_ && assigneePicker_->isVisible())
            return;
        if (!isFormCorrect())
            return;

        const auto editTask = isEditingTask();
        Data::TaskData task;
        if (editTask)
            task = *task_;
        task.params_.title_ = descriptionEdit_->getPlainText().trimmed();
        if (auto contact = assigneeEdit_->selectedContact())
            task.params_.assignee_ = *contact;
        else
            task.params_.assignee_.clear();

        task.params_.date_ = deadlineCheckbox_->isChecked()
            ? QDateTime(deadlineEdit_->date(), deadlineEdit_->time())
            : QDateTime();

        if (editTask)
        {
            GetDispatcher()->editTask(Data::TaskChange(*task_, task));
        }
        else
        {
            core_dispatcher::MessageData messageData;
            messageData.task_ = std::move(task);
            messageData.quotes_ = std::move(quotes_);

            if (contact_.isEmpty())
            {
                qint64 seq = GetDispatcher()->createTask(std::move(messageData.task_));
                Q_EMIT createTask(seq);
            }
            else
            {
                GetDispatcher()->sendMessageToContact(contact_, std::move(messageData));
            }
        }

        Q_EMIT Utils::InterConnector::instance().acceptGeneralDialog();
    }

    void TaskEditWidget::checkAssignee()
    {
        if (const auto assigneeCorrect = assigneeEdit_->text().isEmpty() || assigneeEdit_->selectedContact(); assigneeCorrect != assigneeCorrect_)
        {
            assigneeCorrect_ = assigneeCorrect;
            checkForm();
        }
    }

    void TaskEditWidget::checkDate()
    {
        const auto isCorrect = !deadlineCheckbox_->isChecked() || deadlineEdit_->isDateTimeValid();
        if (isCorrect != dateCorrect_)
        {
            dateCorrect_ = isCorrect;
            checkForm();
        }
    }

    void TaskEditWidget::showAssigneePicker()
    {
        if (!assigneePicker_)
            assigneePicker_ = new AssigneePopup(assigneeEdit_);
        assigneePicker_->show();
    }
} // namespace Ui
