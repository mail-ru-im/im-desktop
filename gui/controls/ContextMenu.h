#pragma once
#include "../utils/utils.h"

namespace Ui
{
    class MenuStyle: public QProxyStyle
    {
        Q_OBJECT

    public:
        virtual int pixelMetric(PixelMetric _metric, const QStyleOption* _option = 0, const QWidget* _widget = 0 ) const;

    };

   class ContextMenu : public QMenu
   {
       Q_OBJECT

   public:
       explicit ContextMenu(QWidget* parent);

       static void applyStyle(QMenu* menu, bool withPadding, int fonSize, int height);

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

       bool hasAction(const QString& _command);

       void removeAction(const QString& _command);

       void invertRight(bool _invert);
       void setIndent(int _indent);

       void popup(const QPoint& _pos, QAction* _at = nullptr);
       void clear();

   protected:
       virtual void showEvent(QShowEvent* _e) override;
       virtual void hideEvent(QHideEvent* _e) override;
       virtual void focusOutEvent(QFocusEvent *_e) override;

   private:
       QIcon makeIcon(const QString& _iconPath) const;
       QAction* addActionWithIcon(const QIcon& _icon, const QString& _name, const QVariant& _data);

   private:
       bool InvertRight_;
       int Indent_;
       QPoint Pos_;
   };
}
