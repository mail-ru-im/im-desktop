#pragma once

namespace Ui
{
    class TextEditEx;
    class ProgressAnimation;

    namespace TextRendering
    {
        class TextUnit;
    }

    class NickLineEdit: public QWidget
    {
        Q_OBJECT

    public:
        explicit NickLineEdit(QWidget* _parent = nullptr, const QString& _initNick = QString(), const QString& _fixedPart = QString(), bool _groupMode = false);
        ~NickLineEdit();

        QSize sizeHint() const override;

        void setText(const QString& _nick, bool _silent = false);
        QString getText(bool full = false) const;

        void setNickRequest();
        void cancelSetNick();

        void resetInput();

        void setFocus();

        void updateCounter();

        void clearHint();

    Q_SIGNALS:
        void changed();
        void ready();
        void serverError(bool _repeateOn = false);
        void nickSet();
        void sameNick();
        void checkError();

    private Q_SLOTS:
        void onNickChanged();
        void onNickCursorPositionChanged();
        void onNickCheckResult(qint64 _reqId, int _error);
        void onCheckTimeout();

    protected:
        void paintEvent(QPaintEvent* _event) override;
        void mouseMoveEvent(QMouseEvent* _event) override;
        void mousePressEvent(QMouseEvent* _event) override;
        void mouseReleaseEvent(QMouseEvent* _event) override;
        void resizeEvent(QResizeEvent* _event) override;

    private:
        enum class ServerRequest
        {
            None,
            CheckNick,
            SetNick
        };

        enum class HintTextCode
        {
            InvalidNick,
            InvalidChar,
            NickAlreadyOccupied,
            NickNotAllowed,
            NickAvailable,
            NickMinLength,
            NickMaxLength,
            NickLatinChar,
            CheckNick,
            ServerError
        };

        void serverCheckNickname(bool _recheck = false);
        void updateHint(HintTextCode _code, const QColor& _color = QColor());
        void setServerErrorHint();
        void updateNickLine(bool _isError);
        bool checkForCorrectInput(bool _showHints = false);
        void startCheckAnimation();
        void stopCheckAnimation();
        void retryLastServerRequest();
        void moveCursorToTheEnd();

        QString makeHtml(QStringView _fixed, QStringView _text);

    private:
        QString originNick_;
        QString fixedPart_;
        QString previous_;
        QString previousPlain_;
        TextEditEx* nick_;
        std::unique_ptr<TextRendering::TextUnit> hintUnit_;
        std::unique_ptr<TextRendering::TextUnit> counterUnit_;
        int hintHorOffset_;
        ProgressAnimation* progressAnimation_;
        QTimer* checkTimer_;
        int checkResult_;
        int serverErrorsCounter_;
        qint64 lastReqId_;
        ServerRequest lastServerRequest_;
        bool linkPressed_;
        bool isFrendlyMinLengthError_;
        bool groupMode_;
    };
}
