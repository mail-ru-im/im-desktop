#pragma once

#include "../../types/message.h"
#include "../../types/task.h"


namespace Ui
{
    class TextEditEx;
    class DialogButton;
    class DateTimePicker;
    class CheckBox;

    namespace TextRendering
    {
        class TextUnit;
        using TextUnitPtr = std::unique_ptr<TextUnit>;
    }

    class AssigneeEdit;
    class AssigneePopup;

    class TaskEditWidget : public QWidget
    {
        Q_OBJECT
    public:
        explicit TaskEditWidget(const QString& _contact = {}, QWidget* _parent = nullptr);
        explicit TaskEditWidget(const Data::TaskData& _task, const Data::MentionMap& _mentions, QWidget* _parent = nullptr);
        ~TaskEditWidget();

        void setInputData(const QString& _text, const Data::QuotesVec& _quotes, const Data::MentionMap& _mentions);
        void setFocusOnTaskTitleInput();

        QString getInputText() const;
        int getCursorPos() const;

        QString getHeader() const;

    Q_SIGNALS:
        void createTask(qint64 seq);

    protected:
        void paintEvent(QPaintEvent* _event) override;
        void keyPressEvent(QKeyEvent* _event) override;
        void resizeEvent(QResizeEvent* _event) override;

    private:
        void initialize();
        void checkForm();
        bool isFormCorrect() const;
        bool isEditingTask() const noexcept { return task_.has_value(); }
        int dateTimeWidgetHeight() const;
        void setDefaultDeadline() noexcept;

    private Q_SLOTS:
        void onNameChanged();
        void onSendClick();
        void checkAssignee();
        void checkDate();
        void showAssigneePicker();

    private:
        TextEditEx* descriptionEdit_ = nullptr;
        TextRendering::TextUnitPtr remainedCount_;
        TextRendering::TextUnitPtr errorUnit_;
        AssigneeEdit* assigneeEdit_ = nullptr;
        AssigneePopup* assigneePicker_ = nullptr;
        DateTimePicker* deadlineEdit_ = nullptr;
        QVariantAnimation* deadlineAnimation_ = nullptr;
        int heightWithoutDeadline_ = 0;
        DialogButton* nextButton_ = nullptr;
        CheckBox* deadlineCheckbox_ = nullptr;
        Data::Task task_;
        QString contact_;
        Data::QuotesVec quotes_;
        Data::MentionMap mentions_;
        bool taskTitleCorrect_ = false;
        bool assigneeCorrect_ = false;
        bool dateCorrect_ = true;
        bool errorVisible_ = false;
    };




}
