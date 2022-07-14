#pragma once

#include "main_window/input_widget/FileToSend.h"
#include "types/message.h"
#include "main_window/mediatype.h"

namespace Ui
{
    enum class InputView
    {
        Default,
        Readonly,
        Disabled,
        Edit,
        Ptt,
        Multiselect,
    };

    struct InputText
    {
        InputText() = default;

        InputText(Data::FString _text, int _pos)
            : text_(std::move(_text))
            , cursorPos_(_pos)
        {}

        void clear()
        {
            text_.clear();
            cursorPos_ = 0;
        }

        Data::FString text_;
        int cursorPos_ = 0;
    };

    struct InputWidgetState
    {
        Data::QuotesVec quotes_;
        Data::MentionMap mentions_;
        Data::FilesPlaceholderMap files_;

        Data::Draft draft_;
        std::optional<Data::Draft> queuedDraft_;
        bool draftLoaded_ = false;

        FilesToSend filesToSend_;
        Data::FString description_;

        struct EditData
        {
            Data::MessageBuddySptr message_;
            Data::FString buffer_;
            MediaType type_ = MediaType::noMedia;
            Data::FString editableText_;

            EditData() : message_(std::make_shared<Data::MessageBuddy>()) {}
            EditData(const Data::MessageBuddySptr& _msg) : message_(_msg) {}
        };
        std::optional<EditData> edit_;

        bool canBeEmpty_ = false;
        bool sendAsFile_ = false;

        InputView view_ = InputView::Default;
        std::vector<InputView> viewHistory_;
        int mentionSignIndex_ = -1;

        bool msgHistoryReady_ = false;

        InputText text_;

        void clear(InputView _newView = InputView::Default)
        {
            quotes_.clear();
            mentions_.clear();
            filesToSend_.clear();
            description_.clear();
            edit_.reset();
            view_ = _newView;
            viewHistory_.clear();
            mentionSignIndex_ = -1;
            canBeEmpty_ = false;
            sendAsFile_ = false;
            text_.clear();
        }

        [[nodiscard]] bool isDisabled() const noexcept
        {
            return view_ == InputView::Disabled || view_ == InputView::Readonly;
        }

        [[nodiscard]] bool hasQuotes() const
        {
            return !quotes_.isEmpty();
        }

        [[nodiscard]] bool isEditing() const noexcept { return edit_ && edit_->message_; }
        void startEdit(const Data::MessageBuddySptr& _msg);

        [[nodiscard]] Data::MentionMap& getMentionsRef() noexcept { return isEditing() ? edit_->message_->Mentions_ : mentions_; }
        [[nodiscard]] const Data::MentionMap& getMentions() const noexcept { return isEditing() ? edit_->message_->Mentions_ : mentions_; }

        [[nodiscard]] Data::FilesPlaceholderMap& getFilesRef() noexcept { return isEditing() ? edit_->message_->Files_ : files_; }
        [[nodiscard]] const Data::FilesPlaceholderMap& getFiles() const noexcept { return isEditing() ? edit_->message_->Files_ : files_; }

        [[nodiscard]] Data::QuotesVec& getQuotesRef() noexcept { return isEditing() ? edit_->message_->Quotes_ : quotes_; }
        [[nodiscard]] const Data::QuotesVec& getQuotes() const noexcept { return isEditing() ? edit_->message_->Quotes_ : quotes_; }

        void setText(Data::FString _text, int _cursorPos = 0);
    };

    using InputStatePtr = std::shared_ptr<InputWidgetState>;
}
