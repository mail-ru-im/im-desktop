#pragma once

#include "../../types/message.h"
#include "../../types/task.h"

namespace Ui
{
    class TextEditEx;
    class DialogButton;
    class DateTimePicker;
    class SwitcherCheckbox;
    class TextWidget;

    class AssigneeEdit;
    class AssigneePopup;

    class ScrollAreaWithTrScrollBar;

    class TaskEditWidget : public QWidget
    {
        Q_OBJECT
    public:
        explicit TaskEditWidget(const QString& _contact = {}, const QString& _assignee = {}, QWidget* _parent = nullptr);
        explicit TaskEditWidget(const Data::TaskData& _task, const Data::MentionMap& _mentions, QWidget* _parent = nullptr);
        ~TaskEditWidget();

        void setInputData(const Data::FString& _text, const Data::QuotesVec& _quotes, const Data::MentionMap& _mentions);
        void setFocusOnTaskTitleInput();

        Data::FString getInputText() const;
        int getCursorPos() const;

        QString getHeader() const;

    Q_SIGNALS:
        void createTask(qint64 _seq);

    protected:
        void keyPressEvent(QKeyEvent* _event) override;
        void resizeEvent(QResizeEvent* _event) override;
        void showEvent(QShowEvent* _event) override;

    private:
        void initialize();
        void initializeDescriptionEdit();
        void initializeAssigneeEdit();
        void initializeDeadlineCheckbox();
        void initializeDeadlineEdit();

        void checkName();

        void checkForm();
        bool isFormCorrect() const;
        bool isEditingTask() const noexcept { return task_.has_value(); }
        int dateTimeWidgetHeight() const;
        int dialogHeight() const;
        void setDefaultDeadline() noexcept;

    private Q_SLOTS:
        void updateContentSizeIfPossible();
        void onNameChanged();
        void onNameCursorPositionChanged();
        void onDeadlineCheckboxClick(bool _isChecked);
        void onSendClick();
        void checkAssignee();
        void checkDate();
        void showAssigneePicker();

    private:
        QWidget* scrollableContent_ = nullptr;
        TextEditEx* descriptionEdit_ = nullptr;
        TextWidget* remainedCount_ = nullptr;
        TextWidget* errorUnit_ = nullptr;
        AssigneeEdit* assigneeEdit_ = nullptr;
        AssigneePopup* assigneePicker_ = nullptr;
        DateTimePicker* deadlineEdit_ = nullptr;
        DialogButton* nextButton_ = nullptr;
        TextWidget* deadlineTextUnit_ = nullptr;
        SwitcherCheckbox* deadlineCheckbox_ = nullptr;
        Data::Task task_;
        QString contact_;
        Data::QuotesVec quotes_;
        Data::MentionMap mentions_;
        Data::FString initialInputText_;
        std::optional<int> initialCursorPosition_;
        bool taskTitleCorrect_ = false;
        bool assigneeCorrect_ = false;
        bool dateCorrect_ = true;
    };

} // namespace Ui
