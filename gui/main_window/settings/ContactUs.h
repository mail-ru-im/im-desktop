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
    class CommonInputContainer;
    class TextEditInputContainer;
    class ScrollAreaWithTrScrollBar;

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
        QWidget* errorWidget_;
        QVBoxLayout* attachLayout_;
        QVBoxLayout* filesLayout_;
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
        void reset();

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

    enum class ClearData
    {
        No,
        Yes,
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
        TextEditInputContainer* suggestioner_;
        CommonInputContainer* email_;
        DialogButton* sendButton_;
        GetDebugInfoWidget* debugInfoWidget_;

        TextEmojiWidget* suggestionSizeError_;
        TextEmojiWidget* emailError_;
        QWidget* sendWidget_;
        TextEmojiWidget* errorOccuredSign_;
        QWidget*         errorOccuredWidget_;
        RotatingSpinner* sendSpinner_;
        ScrollAreaWithTrScrollBar* scrollArea_;
        QWidget* mainWidget_;

        bool hasProblemDropper_;
        bool isPlain_;
        enum class State
        {
            Feedback,
            Success
        };

    public:
        ContactUsWidget(QWidget* _parent, bool _isPlain = false);
        void resetState(ClearData _mode = ClearData::No);
    protected:
        void resizeEvent(QResizeEvent* _event) override;
        void focusInEvent(QFocusEvent* _event) override;
        bool eventFilter(QObject* _obj, QEvent* _event) override;
    private:
        void init();
        void initFeedback();
        void initSuccess();

        void setState(const ContactUsWidget::State& _state);
        void sendFeedback();

        enum class ErrorReason
        {
            Feedback,
            Email
        };
        void showError(ErrorReason _reason);
    };
}
