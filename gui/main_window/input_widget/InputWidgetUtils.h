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
    class HistoryTextEdit;
    enum class AttachMediaType;

    std::string getStatsChatType(const QString& _contact);

    int getInputTextLeftMargin() noexcept;

    int getDefaultInputHeight() noexcept;
    int getHorMargin() noexcept;
    int getVerMargin() noexcept;
    int getHorSpacer() noexcept;
    int getEditHorMarginLeft() noexcept;

    int getQuoteRowHeight() noexcept;

    int getBubbleCornerRadius() noexcept;

    QSize getCancelBtnIconSize() noexcept;
    QSize getCancelButtonSize() noexcept;

    void drawInputBubble(QPainter& _p, const QRect& _widgetRect, const QColor& _color, const int _topMargin, const int _botMargin);

    bool isEmoji(QStringView _text, qsizetype& _pos);

    class BubbleWidget : public QWidget
    {
        Q_OBJECT
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

    void sendStat(const QString& _contact, core::stats::stats_event_names _event, const std::string_view _from);
    void sendShareStat(const QString& _contact, bool _sent);
    void sendAttachStat(const QString& _contact, AttachMediaType _mediaType);

    class TransitionAnimation : public QVariantAnimation
    {
        Q_OBJECT

    public:
        TransitionAnimation(QObject* parent = nullptr) : QVariantAnimation(parent) { }

        void setCurrentHeight(int _height) { currentHeight_ = _height; }
        int getCurrentHeight() const noexcept { return currentHeight_; }

        void setTargetHeight(int _height) { targetHeight_ = _height; }
        int getTargetHeight() const noexcept { return targetHeight_; }

        void setEffect(QGraphicsOpacityEffect* _effect) { targetEffect_ = _effect; }
        QGraphicsOpacityEffect* getEffect() const noexcept { return targetEffect_; }

    private:
        int currentHeight_ = 0;
        int targetHeight_ = 0;
        QGraphicsOpacityEffect* targetEffect_ = nullptr;
    };

    enum class InputFeature
    {
        None = 0,
        AttachFile = 1 << 0,
        Ptt = 1 << 1,
        Suggests = 1 << 2,
        LocalState = 1 << 3,
    };
    Q_DECLARE_FLAGS(InputFeatures, InputFeature)
    Q_DECLARE_OPERATORS_FOR_FLAGS(InputFeatures)

    constexpr InputFeatures defaultInputFeatures() noexcept
    {
        return InputFeature::AttachFile | InputFeature::Ptt | InputFeature::Suggests;
    }

    bool isApplePageUp(int _key, Qt::KeyboardModifiers _modifiers) noexcept;
    bool isApplePageDown(int _key, Qt::KeyboardModifiers _modifiers) noexcept;
    bool isApplePageEnd(int _key, Qt::KeyboardModifiers _modifiers) noexcept;
    bool isCtrlEnd(int _key, Qt::KeyboardModifiers _modifiers) noexcept;

    bool isApplePageUp(QKeyEvent* _e) noexcept;
    bool isApplePageDown(QKeyEvent* _e) noexcept;
    bool isApplePageEnd(QKeyEvent* _e) noexcept;
    bool isCtrlEnd(QKeyEvent* _e) noexcept;


    class InputWidget;
    const std::vector<InputWidget*>& getInputWidgetInstances();
    void registerInputWidgetInstance(InputWidget* _input);
    void deregisterInputWidgetInstance(InputWidget* _input);

    template <typename Class, typename Func>
    QWidget* getWidgetImpl(QWidget* _parent, Func _func)
    {
        if (_parent)
        {
            const auto widgets = _parent->findChildren<Class*>();
            for (auto w : widgets)
            {
                if (_func(w))
                    return w;
            }
        }
        return nullptr;
    }

    template <typename Class, typename Method>
    QWidget* getWidgetIf(QWidget* _parent, Method _method)
    {
        return getWidgetImpl<Class>(_parent, [m = std::move(_method)](auto w) { return ((*w).*m)(); });
    }

    template <typename Class, typename Method>
    QWidget* getWidgetIfNot(QWidget* _parent, Method _method)
    {
        return getWidgetImpl<Class>(_parent, [m = std::move(_method)](auto w) { return !((*w).*m)(); });
    }
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