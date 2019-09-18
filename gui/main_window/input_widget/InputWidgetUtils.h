#pragma once

namespace core
{
    namespace stats
    {
        enum class stats_event_names;
    }
}

namespace Styling
{
    enum class StyleVariable;
}

namespace Ui
{
    class CustomButton;
    class MentionCompleter;
    class HistoryTextEdit;

    std::string getStatsChatType();

    int getInputTextLeftMargin();

    int getDefaultInputHeight();
    int getHorMargin();
    int getVerMargin();
    int getHorSpacer();
    int getEditHorMarginLeft();

    int getQuoteRowHeight();

    int getBubbleCornerRadius();

    QSize getCancelBtnIconSize();
    QSize getCancelButtonSize();

    void drawInputBubble(QPainter& _p, const QRect& _widgetRect, const QColor& _color, const int _topMargin, const int _botMargin);

    bool isMarkdown(const QStringRef& _text, const int _position);

    MentionCompleter* getMentionCompleter();
    bool showMentionCompleter(const QString& _initialPattern, const QPoint& _pos);

    bool isEmoji(const QStringRef& _text, int& _pos);

    std::pair<QString, int> getLastWord(HistoryTextEdit* _textEdit);
    bool replaceEmoji(HistoryTextEdit* _textEdit);
    void RegisterTextChange(HistoryTextEdit* _textEdit);

    class BubbleWidget : public QWidget
    {
    public:
        BubbleWidget(QWidget* _parent, const int _topMargin, const int _botMargin, const Styling::StyleVariable _bgColor);

    protected:
        void paintEvent(QPaintEvent*) override;

    private:
        const int topMargin_;
        const int botMargin_;
        const Styling::StyleVariable bgColor_;
    };

    enum class UpdateMode
    {
        Force,
        IfChanged,
    };

    enum class StateTransition
    {
        Animated,
        Force,
    };

    enum class InputStyleMode
    {
        Default,
        CustomBg
    };

    class StyledInputElement
    {
    public:
        void updateStyle(const InputStyleMode _mode)
        {
            if (styleMode_ != _mode)
            {
                styleMode_ = _mode;
                updateStyleImpl(_mode);
            }
        }

        InputStyleMode currentStyle() const noexcept
        {
            return styleMode_;
        }

        virtual ~StyledInputElement() = default;

    protected:
        virtual void updateStyleImpl(const InputStyleMode _mode) = 0;

    protected:
        InputStyleMode styleMode_ = InputStyleMode::Default;
    };

    void updateButtonColors(CustomButton* _button, const InputStyleMode _mode);

    QColor getPopupHoveredColor();
    QColor getPopupPressedColor();

    QColor focusColorPrimary();
    QColor focusColorAttention();

    void sendStat(const core::stats::stats_event_names _event, const std::string_view _from);
    void sendShareStat(bool _sent);
}

namespace Styling
{
    enum class StyleVariable;

    namespace InputButtons
    {
        namespace Default
        {
            void setColors(Ui::CustomButton* _button);
            QColor defaultColor();
            QColor hoverColor();
            QColor pressedColor();
            QColor activeColor();
        }

        namespace Alternate
        {
            void setColors(Ui::CustomButton* _button);
            QColor defaultColor();
            QColor hoverColor();
            QColor pressedColor();
            QColor activeColor();
        }
    }
}