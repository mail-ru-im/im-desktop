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
       enum class Color
       {
           Default,
           Dark
       };
       explicit ContextMenu(QWidget* parent);
       ContextMenu(QWidget* parent, Color _color, int _iconSize = 20);
       ContextMenu(QWidget* parent, int _iconSize);

       static void applyStyle(QMenu* menu, bool withPadding, int fonSize, int height, Color color, const QSize& _iconSize = QSize(20, 20));
       static void applyStyle(QMenu* menu, bool withPadding, int fonSize, int height, const QSize& _iconSize = QSize(20, 20));

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

       QAction* addActionWithIcon(const QString& _iconPath, const QString& _name, const QVariant& _data);

       bool modifyAction(QStringView _command, const QString& _iconPath, const QString& _name, const QVariant& _data, bool _enable);

       bool hasAction(QStringView _command) const;

       void removeAction(QStringView _command);

       void invertRight(bool _invert);
       void setIndent(int _indent);

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
       bool InvertRight_ = false;
       int Indent_ = 0;
       QPoint Pos_;
       Color color_ = Color::Default;
       bool showAsync_ = false;
       QSize iconSize_;
   };
}
