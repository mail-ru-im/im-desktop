#pragma once

#include <unordered_set>
#include <qnamespace.h>
#include <QSize>
#include "../utils/utils.h"
#include "DialogButton.h"

namespace Ui
{
    namespace TextRendering
    {
        class TextUnit;
    }

    class qt_gui_settings;
    class SemitransparentWindowAnimated;

    class GeneralDialog : public QDialog
    {
        Q_OBJECT

    Q_SIGNALS:
        void leftButtonClicked();
        void rightButtonClicked();
        void shown(QWidget *);
        void hidden(QWidget *);
        void moved(QWidget *);
        void resized(QWidget *);

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
            Options(const QSize& _prefSize = QSize(0, 0))
                : preferredSize_(_prefSize)
            {
                ignoreRejectDlgPairs_ = {
                    { Utils::CloseWindowInfo::Initiator::MainWindow, Utils::CloseWindowInfo::Reason::MW_Resizing }
                };
            }

            QSize preferredSize_;
            std::vector<std::pair<Utils::CloseWindowInfo::Initiator, Utils::CloseWindowInfo::Reason>> ignoreRejectDlgPairs_;
            bool rejectOnMWResize_ = false;
        };

    public:
        GeneralDialog(QWidget* _mainWidget, QWidget* _parent, bool _ignoreKeyPressEvents = false, bool _fixed_size = true, bool _rejectable = true, bool _withSemiwindow = true,
                      const Options& _options = Options());
        ~GeneralDialog();

        bool showInCenter();
        QWidget* getMainHost();

        DialogButton* addAcceptButton(const QString& _buttonText, const bool _isEnabled);
        DialogButton* addCancelButton(const QString& _buttonText, const bool _setActive = false);
        QPair<DialogButton* /* ok (right) button */, DialogButton* /* cancel (left) button */>
        addButtonsPair(const QString& _buttonTextLeft, const QString& _buttonTextRight, bool _isActive, bool _rejectable = true, bool _acceptable = true, QWidget* _area = nullptr);
        void setButtonsAreaMargins(const QMargins& _margins);

        DialogButton* getAcceptButton() const noexcept { return nextButton_; };
        DialogButton* takeAcceptButton();
        void setAcceptButtonText(const QString& _text);

        void addLabel(const QString& _text, Qt::Alignment _alignment = Qt::AlignTop | Qt::AlignLeft, int maxLinesNumber = -1);
        void addText(const QString& _messageText, int _upperMarginPx);
        void addText(const QString& _messageText, int _upperMarginPx, const QFont& _font, const QColor& _color);
        void addError(const QString& _messageText);

        inline void setShadow(bool b) { shadow_ = b; }

        inline void setLeftButtonDisableOnClicked(bool v) { leftButtonDisableOnClicked_ = v; }
        inline void setRightButtonDisableOnClicked(bool v) { rightButtonDisableOnClicked_ = v; }

        void updateSize();

        void setIgnoreClicksInSemiWindow(bool _ignore);
        void setIgnoredKeys(const KeysContainer& _keys);

        static bool isActive();

        int getHeaderHeight() const;

        SemitransparentWindowAnimated* getSemiWindow() const { return semiWindow_; }

    protected:
        void showEvent(QShowEvent *) override;
        void hideEvent(QHideEvent *) override;
        void moveEvent(QMoveEvent *) override;
        void resizeEvent(QResizeEvent *) override;
        void mousePressEvent(QMouseEvent* _e) override;
        void keyPressEvent(QKeyEvent* _e) override;
        bool eventFilter(QObject* _obj, QEvent* _event) override;

    private:
        QHBoxLayout* getBottomLayout();
        void rejectDialogInternal();

        QWidget* mainHost_;
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

        bool ignoreKeyPressEvents_;
        bool shadow_;

        bool leftButtonDisableOnClicked_;
        bool rightButtonDisableOnClicked_;
        bool rejectable_;
        bool withSemiwindow_;
        KeysContainer ignoredKeys_;
        Options options_;

        static bool inExec_;
    };

    class TwoOptionsWidget : public QWidget
    {
        Q_OBJECT

    public:
        TwoOptionsWidget(QWidget* _parent, const QString& _firstOptionIcon, const QString& _firstOption, const QString& _secondOptionIcon, const QString& _secondOption);

        bool isFirstSelected() const;
        bool isSecondSelected() const;

    protected:
        void paintEvent(QPaintEvent* _event) override;
        void resizeEvent(QResizeEvent* _event) override;
        void mouseMoveEvent(QMouseEvent* _event) override;
        void mousePressEvent(QMouseEvent* _event) override;
        void mouseReleaseEvent(QMouseEvent* _event) override;
        void leaveEvent(QEvent* _event) override;

    private:
        TextRendering::TextUnitPtr firstOption_;
        TextRendering::TextUnitPtr secondOption_;
        QPixmap firstOptionIcon_;
        QPixmap secondOptionIcon_;
        bool firstHovered_;
        bool firstSelected_;
        bool secondHovered_;
        bool secondSelected_;
        QPoint pos_;
    };
}
