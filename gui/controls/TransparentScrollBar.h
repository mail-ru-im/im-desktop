#pragma once

namespace Utils
{
    class OpacityEffect;
}

namespace Ui
{
    enum class FadeOutPolicy
    {
        Auto,
        Manual
    };

    //////////////////////////////////////////////////////////////////////////
    // TransparentAnimation
    //////////////////////////////////////////////////////////////////////////
    class TransparentAnimation : public QObject
    {
        Q_OBJECT
    public:
        TransparentAnimation(float _minOpacity, float _maxOpacity, QWidget* _host, FadeOutPolicy _fadeOutPolicy = FadeOutPolicy::Auto);
        virtual ~TransparentAnimation();

        qreal currentOpacity() const;
        void setOpacity(const double _opacity);

    Q_SIGNALS:
        void fadeOutStarted();

    public Q_SLOTS:
        void fadeIn();
        void fadeOut();

    private:
        Utils::OpacityEffect* opacityEffect_;
        QPropertyAnimation* fadeAnimation_;
        float                        minOpacity_;
        float                        maxOpacity_;
        QWidget* host_;
        QTimer* timer_;
    };

    enum class ScrollBarType
    {
        vertical = 0,
        horizontal = 1
    };

    enum class ScrollBarPolicy
    {
        AutoHide,
        AlwaysVisible
    };

    //////////////////////////////////////////////////////////////////////////
    // TransparentScrollButton
    //////////////////////////////////////////////////////////////////////////
    class TransparentScrollButton : public QWidget
    {
        Q_OBJECT

    Q_SIGNALS:
        void fadeOutStarted();

    public Q_SLOTS:

        void fadeIn();
        void fadeOut();

    public:
        TransparentScrollButton(QWidget* parent, FadeOutPolicy _fadeOutPolicy);
        virtual ~TransparentScrollButton();

        virtual void hoverOn() = 0;
        virtual void hoverOff() = 0;
        QPoint dragPoint() const { return dragPoint_; }

    Q_SIGNALS:
        void moved(QPoint);

    protected:
        void mouseMoveEvent(QMouseEvent*) override;
        void mousePressEvent(QMouseEvent*) override;
        void paintEvent(QPaintEvent* event) override;

        QPropertyAnimation* maxSizeAnimation_;
        QPropertyAnimation* minSizeAnimation_;

        void setHovered(const bool _hovered);

    private:
        TransparentAnimation* transparentAnimation_;
        QPoint dragPoint_;

        bool isHovered_;
    };





    //////////////////////////////////////////////////////////////////////////
    // TransparentScrollButtonV
    //////////////////////////////////////////////////////////////////////////
    class TransparentScrollButtonV : public TransparentScrollButton
    {
        Q_OBJECT

    public:
        int getMinHeight() const;
        int getMinWidth() const;
        int getMaxWidth() const;

        TransparentScrollButtonV(QWidget* _parent, FadeOutPolicy _fadeOutPolicy);

        void hoverOn() override;
        void hoverOff() override;

    private:
        int minScrollButtonWidth_;
        int maxScrollButtonWidth_;
        int minScrollButtonHeight_;
    };




    //////////////////////////////////////////////////////////////////////////
    // TransparentScrollButtonH
    //////////////////////////////////////////////////////////////////////////
    class TransparentScrollButtonH : public TransparentScrollButton
    {
        Q_OBJECT

    public:
        TransparentScrollButtonH(QWidget* _parent, FadeOutPolicy _fadeOutPolicy);

        int getMinWidth() const;
        int getMinHeight() const;
        int getMaxHeight() const;

        void hoverOn() override;
        void hoverOff() override;

    private:

        int minScrollButtonHeight_;
        int maxScrollButtonHeight_;
        int minScrollButtonWidth_;
    };



    class AbstractWidgetWithScrollBar;


    //////////////////////////////////////////////////////////////////////////
    // TransparentScrollBar
    //////////////////////////////////////////////////////////////////////////
    class TransparentScrollBar : public QWidget
    {
        Q_OBJECT

    public:
        TransparentScrollBar(ScrollBarPolicy _scrollBarPolicy = ScrollBarPolicy::AutoHide);
        virtual ~TransparentScrollBar();

        void fadeIn();
        void fadeOut();
        void setScrollArea(QAbstractScrollArea* _view);
        void setParentWidget(QWidget* _view);
        void setGetContentSizeFunc(std::function<QSize()> _getContentSize);

        QPointer<TransparentScrollButton> getScrollButton() const;
        QScrollBar* getDefaultScrollBar() const;

        void init();

    public Q_SLOTS:

        void updatePos();

    protected:
        bool eventFilter(QObject* obj, QEvent* event) override;
        void paintEvent(QPaintEvent* event) override;
        void resizeEvent(QResizeEvent* event) override;
        void mousePressEvent(QMouseEvent* event) override;

        virtual double calcButtonHeight() = 0;
        virtual double calcButtonWidth() = 0;
        virtual double calcScrollBarRatio() = 0;
        virtual void updatePosition() = 0;
        virtual QPointer<TransparentScrollButton> createScrollButton(QWidget* _parent, FadeOutPolicy _fadeOutPolicy) = 0;
        virtual void setDefaultScrollBar(QAbstractScrollArea* _view) = 0;
        virtual void onResize(QResizeEvent* _e) = 0;
        virtual void moveToGlobalPos(QPoint _moveTo) = 0;

        QPointer<QWidget> view_;
        std::function<QSize()> getContentSize_;
        QScrollBar* scrollBar_;

    private Q_SLOTS:
        void onScrollBtnMoved(QPoint);

    private:
        TransparentAnimation* transparentAnimation_;
        QPointer<TransparentScrollButton> scrollButton_;
        ScrollBarPolicy scrollBarPolicy_;
    };







    //////////////////////////////////////////////////////////////////////////
    // TransparentScrollBarV
    //////////////////////////////////////////////////////////////////////////
    class TransparentScrollBarV : public TransparentScrollBar
    {
        Q_OBJECT

    public:
        using TransparentScrollBar::TransparentScrollBar;

    protected:
        double calcButtonHeight() override;
        double calcButtonWidth() override;
        double calcScrollBarRatio() override;
        void updatePosition() override;
        void setDefaultScrollBar(QAbstractScrollArea* _view) override;
        QPointer<TransparentScrollButton> createScrollButton(QWidget* _parent, FadeOutPolicy _fadeOutPolicy) override;
        void onResize(QResizeEvent* _e) override;
        void moveToGlobalPos(QPoint _moveTo) override;
    };







    //////////////////////////////////////////////////////////////////////////
    // TransparentScrollBarH
    //////////////////////////////////////////////////////////////////////////
    class TransparentScrollBarH : public TransparentScrollBar
    {
        Q_OBJECT

    public:
        using TransparentScrollBar::TransparentScrollBar;

    protected:
        double calcButtonHeight() override;
        double calcButtonWidth() override;
        double calcScrollBarRatio() override;
        void updatePosition() override;
        void setDefaultScrollBar(QAbstractScrollArea* _view) override;
        QPointer<TransparentScrollButton> createScrollButton(QWidget* _parent, FadeOutPolicy _fadeOutPolicy) override;
        void onResize(QResizeEvent* _e) override;
        void moveToGlobalPos(QPoint _moveTo) override;
    };







    //////////////////////////////////////////////////////////////////////////
    // AbstractWidgetWithScrollBar
    //////////////////////////////////////////////////////////////////////////
    class AbstractWidgetWithScrollBar
    {
    public:
        AbstractWidgetWithScrollBar();
        virtual ~AbstractWidgetWithScrollBar() {};

        virtual QSize contentSize() const = 0;

        virtual TransparentScrollBarV* getScrollBarV() const;
        virtual void setScrollBarV(TransparentScrollBarV* _scrollBar);

        virtual TransparentScrollBarH* getScrollBarH() const;
        virtual void setScrollBarH(TransparentScrollBarH* _scrollBar);
        void fadeIn();

    protected:
        virtual void mouseMoveEvent(QMouseEvent* event);
        virtual void wheelEvent(QWheelEvent* event);
        virtual void updateGeometries();

    private:
        TransparentScrollBarV* scrollBarV_;
        TransparentScrollBarH* scrollBarH_;
    };


    //////////////////////////////////////////////////////////////////////////
    // GestureHandler
    //////////////////////////////////////////////////////////////////////////

    class ListViewGestureHandler : public QObject
    {
        Q_OBJECT

    Q_SIGNALS:
        void tapAndHoldGesture(const QModelIndex& _index, QPrivateSignal);
        void tapGesture(const QModelIndex& _index, QPrivateSignal);
        void rightClick(const QModelIndex& _index, QPrivateSignal);

    public:
        explicit ListViewGestureHandler(QObject* _parent);
        virtual void gestureEvent(QGestureEvent* _e);
        virtual void setView(QListView* _view);
        virtual void mouseEvent(QMouseEvent* _e);

    private:
        QModelIndex startGestureIndex_;
        QListView* view_ = nullptr;
        bool wasTapAndHold_ = false;
    };


    //////////////////////////////////////////////////////////////////////////
    // FocusableListView
    //////////////////////////////////////////////////////////////////////////
    class FocusableListView : public QListView
    {
        Q_OBJECT

    Q_SIGNALS:
        void mousePosChanged(const QPoint& _pos, const QModelIndex& _index);
        void wheeled();

    public:
        explicit FocusableListView(QWidget *_parent = nullptr);

        void setSelectByMouseHover(const bool _enabled);
        bool getSelectByMouseHover() const;

        void updateSelectionUnderCursor();
        void setListViewGestureHandler(ListViewGestureHandler* _handler);
        ListViewGestureHandler* getListViewGestureHandler() const;

    protected:
        void enterEvent(QEvent* _e) override;
        void leaveEvent(QEvent* _e) override;
        void mouseMoveEvent(QMouseEvent* _e) override;
        void wheelEvent(QWheelEvent* _e) override;
        void resizeEvent(QResizeEvent*) override;
        bool eventFilter(QObject* _obj, QEvent* _event) override;

        QItemSelectionModel::SelectionFlags selectionCommand(const QModelIndex &index, const QEvent *event = 0) const override;

        bool selectByMouseHoverEnabled_;
        QTimer* rescheduleLayoutTimer_;
        ListViewGestureHandler* getstureHandler_;

    private Q_SLOTS:
        void scrollAnimationValueChanged();

    private:
        void updateSelectionUnderCursor(const QPoint& _pos);
        void selectItem(const QModelIndex& _index);
        void updateCursor(const QModelIndex& _index);
        static void updateSmoothScrollAvailability();

        QVariantAnimation* scrollAnimation_;

        static bool smoothAnimationEnabled_;
    };







    //////////////////////////////////////////////////////////////////////////
    // ListViewWithTrScrollBar
    //////////////////////////////////////////////////////////////////////////
    class ListViewWithTrScrollBar : public FocusableListView, public AbstractWidgetWithScrollBar
    {
        Q_OBJECT

    public:
        explicit ListViewWithTrScrollBar(QWidget *parent = nullptr);
        virtual ~ListViewWithTrScrollBar();

        QSize contentSize() const override;
        void setScrollBarV(TransparentScrollBarV* _scrollBar) override;

    protected:
        void mouseMoveEvent(QMouseEvent *event) override;
        void wheelEvent(QWheelEvent *event) override;
        void updateGeometries() override;
    };







    //////////////////////////////////////////////////////////////////////////
    // MouseMoveEventFilterForTrScrollBar
    //////////////////////////////////////////////////////////////////////////
    class MouseMoveEventFilterForTrScrollBar : public QObject
    {
        Q_OBJECT

    public:
        MouseMoveEventFilterForTrScrollBar(TransparentScrollBar* _scrollbar);

    protected:
        bool eventFilter(QObject* _obj, QEvent* _event);

    private:
        TransparentScrollBar* scrollbar_;
    };








    //////////////////////////////////////////////////////////////////////////
    // ScrollAreaWithTrScrollBar
    //////////////////////////////////////////////////////////////////////////
    class ScrollAreaWithTrScrollBar : public QScrollArea, public AbstractWidgetWithScrollBar
    {
        Q_OBJECT

    Q_SIGNALS:
        void resized();

    public:
        explicit ScrollAreaWithTrScrollBar(QWidget *parent = nullptr);
        virtual ~ScrollAreaWithTrScrollBar();

        QSize contentSize() const override;
        void setScrollBarV(TransparentScrollBarV* _scrollBar) override;
        void setScrollBarH(TransparentScrollBarH* _scrollBar) override;
        void setWidget(QWidget* widget);

        void setMaxContentWidth(int _width);
        int getMaxContentWidth() const noexcept { return maxWidth_; }

    protected:
        void mouseMoveEvent(QMouseEvent *event) override;
        void wheelEvent(QWheelEvent *event) override;
        void resizeEvent(QResizeEvent *event) override;
        void updateGeometry();
        void updateMargins();

    private:
        bool eventFilter(QObject *obj, QEvent *event) override;

    private:
        int maxWidth_ = -1;
    };



    void smoothScroll(QScrollBar* _scrollBar, QVariantAnimation* _animation, int _delta);



    //////////////////////////////////////////////////////////////////////////
    // TextEditExWithTrScrollBar
    //////////////////////////////////////////////////////////////////////////
    class TextEditExWithTrScrollBar : public QTextBrowser, public AbstractWidgetWithScrollBar
    {
        Q_OBJECT

    public:
        explicit TextEditExWithTrScrollBar(QWidget *parent = nullptr);
        virtual ~TextEditExWithTrScrollBar();

        QSize contentSize() const override;
        void setScrollBarV(TransparentScrollBarV* _scrollBar) override;

    protected:
        void mouseMoveEvent(QMouseEvent *event) override;
        void wheelEvent(QWheelEvent *event) override;
        void updateGeometry();
    };

    ScrollAreaWithTrScrollBar* CreateScrollAreaAndSetTrScrollBarV(QWidget* parent, ScrollBarPolicy _scrollBarPolicy = ScrollBarPolicy::AutoHide);
    ScrollAreaWithTrScrollBar* CreateScrollAreaAndSetTrScrollBarH(QWidget* parent, ScrollBarPolicy _scrollBarPolicy = ScrollBarPolicy::AutoHide);
    ListViewWithTrScrollBar* CreateFocusableViewAndSetTrScrollBar(QWidget* parent);
    QTextBrowser* CreateTextEditExWithTrScrollBar(QWidget* parent);
}
