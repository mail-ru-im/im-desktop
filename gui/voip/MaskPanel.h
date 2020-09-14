#pragma once
#include "CommonUI.h"
#include "Masks.h"

namespace Ui
{
    // Draw round icon for mask.
    class MaskWidget : public QPushButton
    {
        Q_OBJECT

        // Used for animation.
        Q_PROPERTY(int xPos    READ x WRITE setXPos)
        Q_PROPERTY(int yCenter READ yCenter WRITE setYCenter)
        Q_PROPERTY(int yPos    READ y WRITE setYPos)
        Q_PROPERTY(int xCenter READ xCenter WRITE setXCenter)

    public:
        MaskWidget(voip_masks::Mask* _mask);

        const QString& maskPath() const;

        void setPixmap(const QPixmap& _image);
        const QPixmap& pixmap() const;
        void setChecked(bool _check);
        bool isEmptyMask() const;
        bool isLoaded() const;
        const QString& name() const;

        void setXPos(int _x);
        void setYPos(int _y);

        void setYCenter(int _y);
        void setXCenter(int _x);

        int yCenter() const;
        int xCenter() const;

        void updateSize();

        void load();

    Q_SIGNALS:
        void loaded();

    public Q_SLOTS:
        void setMaskEngineReady(bool _ready);

    protected Q_SLOTS:
        void updatePreview();
        void updateJsonPath();
        void updateLoadingProgress(unsigned);

    protected:
        void changeEvent(QEvent* _event) override;
        void moveEvent(QMoveEvent* _event) override;
        void paintEvent(QPaintEvent*) override;

        QPixmap image_;
        QString maskPath_;
        QString name_;
        QPointer<voip_masks::Mask> mask_;
        float loadingProgress_ = 0.0f;
        bool maskEngineReady_ = false;
        bool applyWhenEnabled_ = false;
        QPixmap closeIcon_;

        QPropertyAnimation* resizeAnimationMin_;
        QPropertyAnimation* resizeAnimationMax_;

        bool loading_ = false;
    };


    struct UIEffects;

    class MaskPanel : public BaseBottomVideoPanel
    {
    Q_OBJECT

    Q_SIGNALS:
        void onMouseEnter();
        void onMouseLeave();
        void onkeyEscPressed();
        void makePreviewPrimary();
        void makeInterlocutorPrimary();
        void onShowMaskList();
        void getCallStatus(bool& _isAccepted);
        void onHideMaskPanel();

    public:
        MaskPanel(QWidget* _parent, QWidget* _container, int _topOffset, int _bottomOffset);
        ~MaskPanel();

        void setMaskList(const voip_masks::MaskList& _maskList);

        void fadeIn(unsigned int _duration) override;
        void fadeOut(unsigned int _duration) override;

        void setTopOffset(int _topOffset);
        void chooseFirstMask();
        bool isOpened();

        MaskWidget* getSelectedWidget();
        MaskWidget* getFirstMask();

    public Q_SLOTS:
        void selectMask(MaskWidget* _mask);
        void callDestroyed();
        void scrollToWidget(const QString& _maskName);

    protected Q_SLOTS:
        void showMaskList();
        MaskWidget* appendItem(voip_masks::Mask* _mask);
        void changedMask();
        void scrollListUp();
        void scrollListDown();
        void updateUpDownButtons();
        void setVideoStatus(bool);
        void onVoipCallCreated(const voip_manager::ContactEx& _contactEx);
        void onVoipCallIncomingAccepted(const std::string& _callId);
        void onMaskLoaded();

    protected:
        void changeEvent(QEvent* _e) override;
        void enterEvent(QEvent* _e) override;
        void leaveEvent(QEvent* _e) override;
        void keyReleaseEvent(QKeyEvent* _e) override;
        void resizeEvent(QResizeEvent*) override;
        void wheelEvent(QWheelEvent* _event) override;
        void showEvent(QShowEvent*) override;
        void hideEvent(QHideEvent*) override;

        QWidget* createMaskListWidget();
        void clearMaskList();

        // Send statistic to core.
        void sendOpenStatistic();
        // Send mask select statistic to core.
        void sendSelectMaskStatistic(MaskWidget* _mask);

        // Enumirate all masks. First mask is empty mask.
        void enumerateMask(std::function<void(MaskWidget* mask)> _func);

        void updateMaskList();

        void updatePosition(const QWidget& _parent) override;

    private:

        QWidget* container_;
        QWidget* rootWidget_;

        QScrollArea* maskList_;
        QWidget *scrollWidget_;
        QHBoxLayout* layoutMaskListWidget_;

        QHBoxLayout* maskLayout_;

        // Offsets for panel layout.
        int topOffset_;
        int bottomOffset_;

        // Scroll mask buttons.
        QPushButton* upButton_;
        QPushButton* downButton_;

        QWidget* maskListWidget_;

        // Current layout direction.
        QBoxLayout::Direction direction_;

        // Is local video on of off.
        bool hasLocalVideo_;

        // Background widget to able process mouse events on transparent parts of panel.
        PanelBackground* backgroundWidget_;

        std::unique_ptr<UIEffects> rootEffect_;

        // Last fade state.
        bool isFadedVisible_;

        QPointer<MaskWidget> current_;
    };
}
