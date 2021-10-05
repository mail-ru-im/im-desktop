#pragma once
#include "../utils/utils.h"

namespace Ui
{
    class MenuStyle: public QProxyStyle
    {
        Q_OBJECT

    public:
        MenuStyle(const int _iconSize);

        int pixelMetric(PixelMetric _metric, const QStyleOption* _option, const QWidget* _widget) const override;

    private:
        int iconSize_;
    };

    class ClickableTextWidget;

    class TextAction : public QWidgetAction
    {
        Q_OBJECT

    public:
        TextAction(const QString& text, QObject* parent);
        void setHovered(bool _v);

    private:
        ClickableTextWidget* textWidget_ = nullptr;
    };

   class ContextMenu : public QMenu
   {
       Q_OBJECT

   public:
       static QSize defaultIconSize() { return QSize(20, 20); }

   public:
       enum class Color
       {
           Default,
           Dark
       };
       explicit ContextMenu(QWidget* parent);
       ContextMenu(QWidget* parent, Color _color, int _iconSize = 20);
       ContextMenu(QWidget* parent, int _iconSize);

       static void applyStyle(QMenu* menu, bool withPadding, int fonSize, int height, Color color, const QSize& _iconSize = defaultIconSize());
       static void applyStyle(QMenu* menu, bool withPadding, int fonSize, int height, const QSize& _iconSize = defaultIconSize());

       static void updatePosition(QMenu* _menu, QPoint _position, bool _forceShowAtLeft = false);

       template<class Obj, typename Func1>
       QAction* addActionWithIcon(const QIcon& _icon, const QString& _name, const Obj* _receiver, Func1 _member)
       {
            return addAction(_icon, _name, _receiver, std::move(_member));
       }

       template<class Obj, typename Func1>
       QAction* addActionWithIcon(const QString& _iconPath, const QString& _name, const Obj* _receiver, Func1 _member)
       {
           return addAction(makeIcon(_iconPath), _name, _receiver, std::move(_member));
       }

       template<typename Func1>
       QAction* addActionWithIconAndFunc(const QString& _iconPath, const QString& _name, Func1 _func)
       {
           return addAction(makeIcon(_iconPath), _name, std::move(_func));
       }

       QAction* addActionWithIcon(const QString& _iconPath, const QString& _name, const QVariant& _data);

       bool modifyAction(QStringView _command, const QString& _iconPath, const QString& _name, const QVariant& _data, bool _enable);

       bool hasAction(QStringView _command) const;

       void removeAction(QStringView _command);

       void showAtLeft(bool _invert);

       void setPosition(const QPoint& _pos);
       void selectBestPositionAroundRectIfNeeded(const QRect& _rect);
       void popup(const QPoint& _pos, QAction* _at = nullptr);
       void clear();

       void setWheelCallback(std::function<void(QWheelEvent*)> _callback);

       void setShowAsync(const bool _byTimeout);
       bool isShowAsync() const;

   Q_SIGNALS:
       void hidden(QPrivateSignal);

   protected:
       void showEvent(QShowEvent* _e) override;
       void hideEvent(QHideEvent* _event) override;
       void focusOutEvent(QFocusEvent *_e) override;
       void wheelEvent(QWheelEvent* _event) override;
       void mouseReleaseEvent(QMouseEvent* _event) override;

   private:
       std::function<void(QWheelEvent*)> onWheel_;
       QIcon makeIcon(const QString& _iconPath) const;
       QAction* addActionWithIcon(const QIcon& _icon, const QString& _name, const QVariant& _data);

       QAction* findAction(QStringView _command) const;

   private:
       bool needShowAtLeft_ = false;
       std::optional<QPoint> pos_;
       Color color_ = Color::Default;
       bool showAsync_ = false;
       QSize iconSize_;
   };
}
