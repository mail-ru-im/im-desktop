#pragma once

#include "../utils/utils.h"
#include "DialogButton.h"
#include "SimpleListWidget.h"

namespace Styling
{
    class StyledWidget;
}

namespace Ui
{
    namespace TextRendering
    {
        class TextUnit;
    }

    enum class ButtonsStateFlag
    {
        Normal = 0,
        InitiallyInactive = 1 << 1,
        RejectionForbidden = 1 << 2,
        AcceptingForbidden = 1 << 3,
    };
    Q_DECLARE_FLAGS(ButtonsStateFlags, ButtonsStateFlag)
    Q_DECLARE_OPERATORS_FOR_FLAGS(Ui::ButtonsStateFlags)

    class qt_gui_settings;
    class SemitransparentWindowAnimated;

    class GeneralDialog : public QDialog
    {
        Q_OBJECT

    Q_SIGNALS:
        void leftButtonClicked();
        void rightButtonClicked();
        void shown(QWidget*);
        void hidden(QWidget*);
        void moved(QWidget*);
        void resized(QWidget*);

    public:
        using KeysContainer = std::unordered_set<Qt::Key>;

    private Q_SLOTS:
        void leftButtonClick();
        void rightButtonClick();

    public Q_SLOTS:
        void setButtonActive(bool _active);
        void rejectDialog(const Utils::CloseWindowInfo& _info);
        void acceptDialog();
        void done(int r) override;

    public:
        struct Options
        {
            Options()
            {
                ignoredInfos_.emplace_back(Utils::CloseWindowInfo::Initiator::MainWindow, Utils::CloseWindowInfo::Reason::MW_Resizing);
            }

            std::vector<Utils::CloseWindowInfo> ignoredInfos_;
            int preferredWidth_ = 0;
            bool rejectOnMWResize_ = false;
            bool ignoreKeyPressEvents_ = false;
            bool fixedSize_ = true;
            bool rejectable_ = true;
            bool withSemiwindow_ = true;
            bool threadBadge_ = false;

            bool isIgnored(const Utils::CloseWindowInfo& _info) const;
        };

    public:
        GeneralDialog(QWidget* _mainWidget, QWidget* _parent, const Options& _options = {});
        ~GeneralDialog();

        static GeneralDialog* activeInstance();

        bool execute();
        QWidget* getMainHost();

        DialogButton* addAcceptButton(const QString& _buttonText, const bool _isEnabled);
        DialogButton* addCancelButton(const QString& _buttonText, const bool _setActive = false);
        QPair<DialogButton* /* ok (right) button */, DialogButton* /* cancel (left) button */>
            addButtonsPair(const QString& _buttonTextLeft, const QString& _buttonTextRight, ButtonsStateFlags _flags = ButtonsStateFlag::Normal, QWidget* _area = nullptr);
        void setButtonsAreaMargins(const QMargins& _margins);

        DialogButton* getAcceptButton() const noexcept { return nextButton_; };
        DialogButton* takeAcceptButton();
        void setAcceptButtonText(const QString& _text);

        void addLabel(const QString& _text, Qt::Alignment _alignment = Qt::AlignTop | Qt::AlignLeft, int maxLinesNumber = -1);
        void addText(const QString& _messageText, int _upperMarginPx);
        void addText(const QString& _messageText, int _upperMarginPx, const QFont& _font, const QColor& _color);
        void addError(const QString& _messageText);

        void setShadow(bool b) { shadow_ = b; }

        void setLeftButtonDisableOnClicked(bool v) { leftButtonDisableOnClicked_ = v; }
        void setRightButtonDisableOnClicked(bool v) { rightButtonDisableOnClicked_ = v; }

        void updateSize();

        void setIgnoreClicksInSemiWindow(bool _ignore);
        void setIgnoredKeys(const KeysContainer& _keys);

        static bool isActive();
        static int maximumHeight() noexcept;
        static int verticalMargin() noexcept;
        static int shadowWidth() noexcept;

        int getHeaderHeight() const;

        SemitransparentWindowAnimated* getSemiWindow() const { return semiWindow_; }

        void setTransparentBackground(bool _enable);

        QSize sizeHint() const override;
        void setFocusPolicyButtons(Qt::FocusPolicy _policy);

    protected:
        void showEvent(QShowEvent*) override;
        void hideEvent(QHideEvent*) override;
        void moveEvent(QMoveEvent*) override;
        void resizeEvent(QResizeEvent*) override;
        void mousePressEvent(QMouseEvent* _e) override;
        void keyPressEvent(QKeyEvent* _e) override;
        bool eventFilter(QObject* _obj, QEvent* _event) override;

    private:
        QHBoxLayout* getBottomLayout();
        void rejectDialogInternal();

        Styling::StyledWidget* mainHost_;
        QWidget* shadowHost_;
        QWidget* mainWidget_;
        int addWidth_;
        DialogButton* nextButton_;
        SemitransparentWindowAnimated* semiWindow_;
        QWidget* bottomWidget_;
        QWidget* textHost_;
        QWidget* errorHost_;
        QWidget* headerLabelHost_;
        QWidget* areaWidget_;

        bool shadow_;

        bool leftButtonDisableOnClicked_;
        bool rightButtonDisableOnClicked_;
        KeysContainer ignoredKeys_;
        Options options_;

        static std::stack<GeneralDialog*> instances_;
        static bool inExec_;
    };


    class OptionWidget : public SimpleListItem
    {
        Q_OBJECT

    public:
        OptionWidget(QWidget* _parent, const QString& _icon, const QString& _caption);
        ~OptionWidget();

    protected:
        void paintEvent(QPaintEvent* _event) override;
        void resizeEvent(QResizeEvent* _event) override;

    private:
        Utils::StyledPixmap icon_;
        TextRendering::TextUnitPtr caption_;
    };

    class MultipleOptionsWidget : public QWidget
    {
        Q_OBJECT

    public:
        using optionType = std::pair<QString, QString>;
        using optionsVector = std::vector<optionType>;

        MultipleOptionsWidget(QWidget* _parent, const optionsVector& _options);
        int selectedIndex() const noexcept { return selectedIndex_; }

    private:
        SimpleListWidget* listWidget_;
        int selectedIndex_ = -1;
    };
}
