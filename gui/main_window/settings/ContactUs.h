#pragma once
#include "GeneralSettingsWidget.h"

namespace Ui
{
    class CustomButton;
    class DialogButton;
    class LineEditEx;
    class TextEditEx;
    class TextEmojiWidget;
    class RotatingSpinner;
    class ProgressAnimation;

    class AttachFileWidget : public QWidget
    {
        Q_OBJECT
    public:
        AttachFileWidget(QWidget* _parent);
        void hideError() const;
        const std::map<QString, QString>& getFiles() const { return filesToSend_; }
        void clearFileList();
    private:
        QWidget* attachWidget_;
        QVBoxLayout* attachLayout_;
        TextEmojiWidget* attachSizeError_;
        std::map<QString, QString> filesToSend_;
        double totalFileSize_;
    private:
        void processFile(const QString& _filePath);
        void attachFile();
    };

    class GetDebugInfoWidget : public QWidget
    {
        Q_OBJECT
    public:
        GetDebugInfoWidget(QWidget* _parent = nullptr);

    private:
        void onClick();
        void onTimeout();
        void onResult(const QString& _fileName);

        void startAnimation();
        void stopAnimation();

    private:
        CustomButton* icon_;
        ProgressAnimation* progress_;
        QTimer* checkTimer_;
    };

    class ContactUsWidget : public QWidget
    {
        Q_OBJECT
    private:
        QWidget* sendingPage_;
        QWidget* successPage_;
        AttachFileWidget* attachWidget_;
        QComboBox* dropper_;
        QString selectedProblem_;
        TextEditEx* suggestioner_;
        LineEditEx* email_;
        DialogButton* sendButton_;
        GetDebugInfoWidget* debugInfoWidget_;

        TextEmojiWidget* suggestionSizeError_;
        TextEmojiWidget* emailError_;
        TextEmojiWidget* errorOccuredSign_;
        RotatingSpinner* sendSpinner_;

        bool hasProblemDropper_;
        enum class State
        {
            Feedback,
            Success
        };

    public:
        ContactUsWidget(QWidget* _parent);
    private:
        void init();
        void initFeedback();
        void initSuccess();

        void setState(const ContactUsWidget::State& _state);
        void sendFeedback();

        void updateSuggesionHeight();
        void updateSendButton(bool _state);
    };
}
