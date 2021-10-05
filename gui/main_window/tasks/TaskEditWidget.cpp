#include "stdafx.h"
#include "TaskEditWidget.h"

#include "styles/ThemeParameters.h"
#include "controls/TransparentScrollBar.h"
#include "../../controls/DateTimeWidget.h"
#include "../../controls/CheckboxList.h"
#include "../../app_config.h"

#include "controls/TextEditEx.h"
#include "controls/DialogButton.h"
#include "utils/InterConnector.h"
#include "utils/gui_coll_helper.h"
#include "utils/features.h"
#include "core_dispatcher.h"
#include "utils/utils.h"
#include "fonts.h"
#include "../input_widget/InputWidgetUtils.h"
#include "omicron/omicron_helper.h"
#include "../common.shared/omicron_keys.h"
#include "../../main_window/MainWindow.h"
#include "main_window/containers/LastseenContainer.h"

#include "AssigneePopup.h"
#include "AssigneeEdit.h"

namespace
{
    auto commonMargin() noexcept
    {
        return Utils::scale_value(20);
    }

    constexpr auto descriptionEditDocumentMargin() noexcept
    {
        return 2;
    }

    auto assigneeEditBottomMargin() noexcept
    {
        return Utils::scale_value(20);
    }

    constexpr auto assigneeEditHeight() noexcept
    {
        return 28;
    }

    auto dateTimeEditBottomMargin() noexcept
    {
        return Utils::scale_value(20);
    }

    Ui::DialogButton* createButton(QWidget* _parent, const QString& _text, const Ui::DialogButtonRole _role, bool _enabled = true)
    {
        auto button = new Ui::DialogButton(_parent, _text, _role);
        button->setFixedWidth(Utils::scale_value(116));
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

    QColor remainedColor()
    {
        return Styling::getParameters().getColor(Styling::StyleVariable::BASE_PRIMARY);
    }

    QColor errorColor()
    {
        return Styling::getParameters().getColor(Styling::StyleVariable::SECONDARY_ATTENTION);
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
}


namespace Ui
{
    TaskEditWidget::TaskEditWidget(const QString& _contact, QWidget* _parent)
        : QWidget(_parent)
        , contact_{ _contact }
    {
        initialize();
        if (!(contact_.isEmpty() || Utils::isChat(contact_) || Logic::GetLastseenContainer()->isBot(contact_)))
            assigneeEdit_->selectContact(contact_);
        setDefaultDeadline();
        checkDate();
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

        if (const auto& date = _task.params_.date_; date.isValid())
        {
            deadlineEdit_->setDateTime(date);
        }
        else
        {
            deadlineEdit_->hide();
            deadlineCheckbox_->setChecked(true);
        }
        checkDate();
    }

    TaskEditWidget::~TaskEditWidget()
    {
        if (assigneePicker_)
            assigneePicker_->deleteLater();
    }

    void TaskEditWidget::setInputData(const QString& _text, const Data::QuotesVec& _quotes, const Data::MentionMap& _mentions)
    {
        quotes_ = _quotes;
        mentions_ = _mentions;

        descriptionEdit_->setMentions(_mentions);
        descriptionEdit_->setPlainText(_text, false);
        descriptionEdit_->moveCursor(QTextCursor::End);
    }

    void TaskEditWidget::setFocusOnTaskTitleInput()
    {
        descriptionEdit_->setFocus();
    }

    QString TaskEditWidget::getInputText() const
    {
        return descriptionEdit_->getPlainText();
    }

    int TaskEditWidget::getCursorPos() const
    {
        return descriptionEdit_->textCursor().position();
    }

    QString TaskEditWidget::getHeader() const
    {
        return isEditingTask() ? QT_TRANSLATE_NOOP("task_popup", "Edit task") : QT_TRANSLATE_NOOP("task_popup", "Create task");
    }

    void TaskEditWidget::paintEvent(QPaintEvent* _event)
    {
        QPainter p(this);

        const auto& descriptionGeometry = descriptionEdit_->geometry();
        remainedCount_->setOffsets(width() - remainedCount_->desiredWidth() - Utils::scale_value(20), descriptionGeometry.bottom() + remainedTopMargin());
        remainedCount_->draw(p);

        if (errorVisible_)
        {
            errorUnit_->elide(descriptionGeometry.width() - remainedCount_->cachedSize().width() - Utils::scale_value(20));
            errorUnit_->setOffsets(descriptionGeometry.left(), descriptionGeometry.bottom() + remainedTopMargin());
            errorUnit_->draw(p);
        }
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

    void TaskEditWidget::initialize()
    {
        auto layout = Utils::emptyVLayout(this);
        const auto dialogMargin = commonMargin();
        layout->setContentsMargins(dialogMargin, dialogMargin, dialogMargin, dialogMargin);

        descriptionEdit_ = new Ui::TextEditEx(this, Fonts::appFontScaled(15), Styling::getParameters().getColor(Styling::StyleVariable::TEXT_SOLID), true, true);
        descriptionEdit_->setPlaceholderText(QT_TRANSLATE_NOOP("task_popup", "Name"));
        descriptionEdit_->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        descriptionEdit_->setTextInteractionFlags(Qt::TextEditable | Qt::TextEditorInteraction);
        descriptionEdit_->setCursor(Qt::IBeamCursor);
        descriptionEdit_->viewport()->setCursor(Qt::IBeamCursor);
        descriptionEdit_->setEnterKeyPolicy(TextEditEx::EnterKeyPolicy::FollowSettingsRules);
        descriptionEdit_->setTabChangesFocus(true);
        descriptionEdit_->document()->setDocumentMargin(descriptionEditDocumentMargin());
        descriptionEdit_->addSpace(Utils::scale_value(8));

        Utils::ApplyStyle(descriptionEdit_, Styling::getParameters().getTextEditCommonQss(true));
        Testing::setAccessibleName(descriptionEdit_, qsl("AS TaskEditWidget nameEdit"));
        layout->addWidget(descriptionEdit_);

        connect(descriptionEdit_, &TextEditEx::textChanged, this, &TaskEditWidget::onNameChanged);
        connect(descriptionEdit_, &TextEditEx::enter, this, &TaskEditWidget::onSendClick);

        remainedCount_ = TextRendering::MakeTextUnit(QString());
        remainedCount_->init(remainedFont(), remainedColor());

        errorUnit_ = TextRendering::MakeTextUnit(QT_TRANSLATE_NOOP("task_popup", "Maximum length - %1 symbols").arg(maxDescriptionLength()));
        errorUnit_->init(remainedFont(), errorColor());
        errorUnit_->evaluateDesiredSize();
        layout->addSpacing(remainedTopMargin() + errorUnit_->cachedSize().height() + remainedBottomMargin());

        LineEditEx::Options lineEditOptions{ { Qt::Key_Up, Qt::Key_Down } };
        assigneeEdit_ = new AssigneeEdit(this, lineEditOptions);
        int left, top, right, bottom;
        assigneeEdit_->getTextMargins(&left, &top, &right, &bottom);
        QMargins lineEditMargins(left, top, right, Utils::scale_value(8));
        assigneeEdit_->setTextMargins(lineEditMargins);
        assigneeEdit_->setCustomPlaceholder(QT_TRANSLATE_NOOP("task_popup", "Assignee"));
        assigneeEdit_->setAlignment(Qt::AlignLeft | Qt::AlignBottom);
        assigneeEdit_->setAttribute(Qt::WA_MacShowFocusRect, false);
        assigneeEdit_->setFont(Fonts::appFontScaled(15));
        Utils::ApplyStyle(assigneeEdit_, Styling::getParameters().getLineEditCommonQss(false, assigneeEditHeight()));
        assigneeEdit_->setCustomPlaceholderColor(Styling::getParameters().getColor(Styling::StyleVariable::BASE_PRIMARY));
        Testing::setAccessibleName(assigneeEdit_, qsl("AS TaskEditWidget assigneeEdit"));
        assigneeEdit_->setContextMenuPolicy(Qt::ContextMenuPolicy::NoContextMenu);
        connect(assigneeEdit_, &LineEditEx::focusIn, this, &TaskEditWidget::showAssigneePicker);
        connect(assigneeEdit_, &LineEditEx::clicked, this, &TaskEditWidget::showAssigneePicker);

        connect(assigneeEdit_, &LineEditEx::textChanged, this, &TaskEditWidget::checkAssignee);
        connect(assigneeEdit_, &AssigneeEdit::selectedContactChanged, this, &TaskEditWidget::checkAssignee);
        connect(assigneeEdit_, &AssigneeEdit::enter, this, &TaskEditWidget::onSendClick);
        layout->addWidget(assigneeEdit_);
        layout->addSpacing(assigneeEditBottomMargin());

        deadlineCheckbox_ = new Ui::CheckBox(this);
        deadlineCheckbox_->setFixedHeight(deadlineCheckboxHeight());
        deadlineCheckbox_->setText(QT_TRANSLATE_NOOP("task_popup", "No deadline"));
        Testing::setAccessibleName(deadlineCheckbox_, qsl("AS TaskEditWidget deadlineCheckbox"));
        layout->addWidget(deadlineCheckbox_);
        deadlineEdit_ = new DateTimePicker(this, Ui::GetAppConfig().showSecondsInTimePicker());
        deadlineEdit_->setContentsMargins(deadlineEdit_->contentsMargins() + deadlineEditContentMargins());

        deadlineEdit_->setAccessibilityPrefix(qsl("TaskEditWidget"));
        connect(deadlineEdit_, &DateTimePicker::dateTimeChanged, this, &TaskEditWidget::checkDate);
        connect(deadlineEdit_, &DateTimePicker::dateTimeValidityChanged, this, &TaskEditWidget::checkDate);
        connect(deadlineEdit_, &DateTimePicker::enter, this, &TaskEditWidget::onSendClick);

        layout->addWidget(deadlineEdit_, Qt::AlignTop);
        layout->addSpacing(spacingAfterDeadlineEdit());

        deadlineAnimation_ = new QVariantAnimation(this);
        deadlineAnimation_->setDuration(dateTimeAnimationDuration().count());
        deadlineAnimation_->setStartValue(0);
        deadlineAnimation_->setEndValue(dateTimeWidgetHeight());
        connect(deadlineAnimation_, &QVariantAnimation::valueChanged, this, [this](const QVariant& _value)
        {
            const auto v = _value.toInt();
            deadlineEdit_->setMaximumHeight(v);
            setFixedHeight(heightWithoutDeadline_ + v);
        });
        connect(deadlineAnimation_, &QVariantAnimation::finished, this, &TaskEditWidget::checkDate);

        connect(deadlineCheckbox_, &Ui::CheckBox::clicked, this, [this](bool _isChecked)
        {
            if (0 == heightWithoutDeadline_)
                heightWithoutDeadline_ = height() - (deadlineEdit_->isVisible() ? dateTimeWidgetHeight() : 0);

            deadlineEdit_->setSizePolicy(deadlineEdit_->sizePolicy().horizontalPolicy(), QSizePolicy::Ignored);
            if (!_isChecked && deadlineEdit_->isHidden())
                deadlineEdit_->show();

            deadlineAnimation_->setDirection(_isChecked ? QAbstractAnimation::Backward : QAbstractAnimation::Forward);
            deadlineAnimation_->start();
        });

        auto backButton = createButton(this, QT_TRANSLATE_NOOP("task_popup", "Cancel"), DialogButtonRole::CANCEL);
        backButton->setFocusPolicy(Qt::TabFocus);
        Testing::setAccessibleName(backButton, qsl("AS TaskEditWidget backButton"));
        connect(backButton, &DialogButton::clicked, this, []()
        {
            Q_EMIT Utils::InterConnector::instance().closeAnyPopupWindow({});
        });
        nextButton_ = createButton(this, isEditingTask() ? QT_TRANSLATE_NOOP("task_popup", "Ok") : QT_TRANSLATE_NOOP("task_popup", "Create"), DialogButtonRole::CONFIRM, false);
        nextButton_->setFocusPolicy(Qt::TabFocus);
        Testing::setAccessibleName(nextButton_, qsl("AS TaskEditWidget doneButton"));
        connect(nextButton_, &DialogButton::clicked, this, &TaskEditWidget::onSendClick);

        auto buttonsLayout = Utils::emptyHLayout();
        buttonsLayout->addStretch();
        buttonsLayout->addWidget(backButton);
        buttonsLayout->addSpacing(Utils::scale_value(20));
        buttonsLayout->addWidget(nextButton_);
        buttonsLayout->addStretch();
        layout->addLayout(buttonsLayout);

        onNameChanged();
        checkAssignee();
        checkDate();

        Testing::setAccessibleName(this, qsl("AS TaskEditWidget"));
    }

    void TaskEditWidget::checkForm()
    {
        nextButton_->setEnabled(isFormCorrect());
        update();
    }

    bool TaskEditWidget::isFormCorrect() const
    {
        return taskTitleCorrect_ && assigneeCorrect_ && dateCorrect_;
    }

    int TaskEditWidget::dateTimeWidgetHeight() const
    {
        return deadlineEdit_->sizeHint().height();
    }

    void TaskEditWidget::setDefaultDeadline() noexcept
    {
        const auto currentDateTime = QDateTime::currentDateTime();
        const auto isPastEndOfDay = currentDateTime.time() > getDefaultEndOfDay();
        deadlineEdit_->setDateTime(QDateTime(currentDateTime.date().addDays(isPastEndOfDay ? 1 : 0), getDefaultDeadlineTime()));
    }

    void TaskEditWidget::onNameChanged()
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

        errorVisible_ = count > maxDescriptionLength();

        remainedCount_->setText(QString::number(maxDescriptionLength() - count), errorVisible_ ? errorColor() : remainedColor());

        if (const auto taskTitleCorrect = !QStringView(text).trimmed().isEmpty() && !errorVisible_; taskTitleCorrect != taskTitleCorrect_)
        {
            taskTitleCorrect_ = taskTitleCorrect;
            checkForm();
        }
        update();
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
            ? QDateTime()
            : QDateTime(deadlineEdit_->date(), deadlineEdit_->time());

        if (editTask)
        {
            GetDispatcher()->editTask(Data::TaskChange(*task_, task));
        }
        else
        {
            core_dispatcher::MessageData messageData;
            messageData.mentions_ = std::move(mentions_);
            messageData.quotes_ = std::move(quotes_);
            messageData.task_ = std::move(task);

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
        const auto isCorrect = deadlineCheckbox_->isChecked() || deadlineEdit_->isDateTimeValid();
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
}
