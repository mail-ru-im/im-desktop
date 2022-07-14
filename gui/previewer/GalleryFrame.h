#pragma once

#include "../previewer/Drawable.h"
#include "../controls/TextUnit.h"

namespace Ui
{
    class TextWidget;
}

namespace Previewer
{
    class GalleryFrame_p;

    enum ControlType : short;

    class CustomMenu;

    class CaptionArea : public QScrollArea
    {
        Q_OBJECT
    Q_SIGNALS:
        void needHeight(int);

    public:
        CaptionArea(QWidget* _parent);

        void setCaption(const QString& _caption, int _totalWidth);
        void setExpanded(bool _expanded);

        bool isExpanded() const;

    protected:
        virtual void paintEvent(QPaintEvent*);
        virtual void mouseMoveEvent(QMouseEvent*);
        virtual void enterEvent(QEvent*);
        virtual void leaveEvent(QEvent*);
        virtual void mousePressEvent(QMouseEvent*);
        virtual void mouseReleaseEvent(QMouseEvent*);

    private:
        void updateSize();

    private:
        Ui::TextWidget* textWidget_;
        std::unique_ptr<Ui::TextRendering::TextUnit> textPreview_;
        std::unique_ptr<Ui::TextRendering::TextUnit> more_;
        bool expanded_;
        QPoint clicked_;
        int totalWidth_;
        QVariantAnimation* anim_;
    };

    class GalleryFrame : public QWidget
    {
        Q_OBJECT

    Q_SIGNALS:
        void needHeight(int);
    public:

        enum Action
        {
            GoTo = 0x1,
            Copy = 0x2,
            Forward = 0x4,
            SaveAs = 0x8,
            DeleteForMe = 0x10,
            DeleteForAll = 0x20,
            SaveToFavorites = 0x40
        };

        Q_DECLARE_FLAGS(Actions, Action)

        GalleryFrame(QWidget* _parent);
        ~GalleryFrame();

        void setFixedSize(const QSize& _size);

        void setNextEnabled(bool _enable);
        void setPrevEnabled(bool _enable);
        void setZoomInEnabled(bool _enable);
        void setZoomOutEnabled(bool _enable);
        void setSaveEnabled(bool _enable);
        void setMenuEnabled(bool _enable);
        void setGotoEnabled(bool _enable);

        static QString actionIconPath(Action action);
        static QString actionText(Action action);

        void setMenuActions(Actions _actions);
        Actions menuActions() const;

        void setZoomVisible(bool _visible);
        void setNavigationButtonsVisible(bool _visible);

        void setCounterText(const QString& _text);
        void setAuthorText(const QString& _text);
        void setDateText(const QString& _text);
        void setCaption(const QString& _caption);

        void setAuthorAndDateVisible(bool _visible);

        void collapseCaption();
        bool isCaptionExpanded() const;

        void onNext();
        void onPrev();

    Q_SIGNALS:
        void prev();
        void next();
        void zoomIn();
        void zoomOut();
        void close();
        void save();
        void saveAs();
        void copy();
        void forward();
        void goToMessage();
        void deleteForMe();
        void deleteForAll();
        void openContact();
        void saveToFavorites();

    protected:
        void paintEvent(QPaintEvent* _event) override;
        void mouseMoveEvent(QMouseEvent* _event) override;
        void mousePressEvent(QMouseEvent* _event) override;
        void mouseReleaseEvent(QMouseEvent* _event) override;
        bool event(QEvent *_event) override;
        void leaveEvent(QEvent* _event) override;
        void hideEvent(QHideEvent* _event) override;

    private:
        template <typename ... Args>
        QAction* makeAction(Action action, CustomMenu* parent, Args && ...args);
        template <typename ... Args>
        void addAction(Action action, CustomMenu* parent, Args && ...args);
        void onClick(ControlType _type);
        void setLabelText(ControlType _type, const QString& _text);

        CustomMenu* createMenu();
        void showMenuAt(const QPoint& _pos);
        void closeMenu();

        std::unique_ptr<GalleryFrame_p> d;

        friend class AccessibleGalleryFrame;
    };

    class AccessibleGalleryFrame : public QAccessibleWidget
    {
    public:
        AccessibleGalleryFrame(GalleryFrame* widget);
        int childCount() const override;
        QAccessibleInterface* child(int index) const override;
        int indexOfChild(const QAccessibleInterface* child) const override;
        QString text(QAccessible::Text t) const override { return QAccessible::Text::Name == t ? qsl("AS Preview AccessibleGalleryFrame") : QString(); }

    protected:
        std::vector<QAccessibleInterface*> children_;
    };

    class CustomMenu_p;

    class CustomMenu : public QMenu
    {
        Q_OBJECT
    public:
        enum Feature
        {
            NoFeatures = 0x0,
            ScreenAware = 0x1,
            TargetAware = 0x2,
            DefaultPopup = 0x4
        };
        Q_DECLARE_FLAGS(Features, Feature)

        explicit CustomMenu(QWidget* _parent = nullptr);
        ~CustomMenu();

        void setFeatures(Features _features);
        Features features() const;

        void setArrowSize(const QSize& _size);
        QSize arrowSize() const;

        void setCheckMark(const QPixmap& _icon);

        void addAction(QAction* _action, const QPixmap& _icon);

        // Deprecated: it's left to keep old source code working
        // Please do NOT use this method to show the menu
        // prefer new exec() overloaded methods
        Q_DECL_DEPRECATED void showAtPos(const QPoint& _pos);

        void exec(QWidget* _target, Qt::Alignment _align, const QMargins& _margins = {});
        void exec(QWidget* _target, const QRect& _rect, Qt::Alignment _align);

        void clear();

        QSize sizeHint() const override;

    protected:
        void paintEvent(QPaintEvent* _event) override;
        void mouseMoveEvent(QMouseEvent* _event) override;
        void mousePressEvent(QMouseEvent* _event) override;
        void mouseReleaseEvent(QMouseEvent* _event) override;

    private:
        void updateHeight();
        void popupMenu(const QPoint& _globalPos, QScreen* _screen, const QRect& _targetRect, Qt::Alignment _align);

        std::unique_ptr<CustomMenu_p> d;
    };

    Q_DECLARE_OPERATORS_FOR_FLAGS(CustomMenu::Features)
}
