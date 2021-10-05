#pragma once

#include "controls/SimpleListWidget.h"
#include "../controls/TextUnit.h"

namespace Ui
{
    enum class AppPageType;

    class BaseBarItem : public SimpleListItem
    {
        Q_OBJECT

    public:
        explicit BaseBarItem(const QString& _iconPath, const QString& _iconActivePath, const QColor& _activeColor, const QColor& _hoveredColor, const QColor& _normalColor, QWidget* _parent = nullptr);
        ~BaseBarItem();

        void setSelected(bool _value) override;
        bool isSelected() const override;
        void setBadgeText(const QString& _text);
        void setBadgeIcon(const QString& _icon);

        virtual void setName(const QString& _name);

    protected:

        const QPixmap& activeIconPixmap() const;
        const QPixmap& hoveredIconPixmap() const;
        const QPixmap& normalIconPixmap() const;

    private:
        void setBadgeFont(const QFont& _font);

    protected:
        QString badgeText_;
        QPixmap badgePixmap_;
        std::unique_ptr<TextRendering::TextUnit> badgeTextUnit_;
        QString name_;

    private:
        QString badgeIcon_;

        const QString iconPath_;
        const QString iconActivePath_;

        const QPixmap activeIconPixmap_;
        const QPixmap hoveredIconPixmap_;
        const QPixmap normalIconPixmap_;

        bool isSelected_;
    };

    class AppBarItem : public BaseBarItem
    {
        Q_OBJECT

    public:
        explicit AppBarItem(AppPageType _type, const QString& _iconPath, const QString& _iconActivePath, QWidget* _parent = nullptr);
        void setName(const QString& _name) override;

        AppPageType getType() const noexcept { return type_; }

    protected:
        void paintEvent(QPaintEvent* _event) override;
        QSize sizeHint() const override;

    private Q_SLOTS:
        void onTooltipTimer();
        void onHoverChanged(bool _hovered);

    private:
        void showTooltip();
        void hideTooltip();

    private:
        QTimer* tooltipTimer_;
        AppPageType type_;
        bool tooltipVisible_;
    };

    class CalendarItem : public AppBarItem
    {
        Q_OBJECT
    public:
        explicit CalendarItem(AppPageType _type, const QString& _iconPath, const QString& _iconActivePath, QWidget* _parent = nullptr);
        explicit CalendarItem(const QString& _iconPath, const QString& _iconActivePath, QWidget* _parent = nullptr);

    protected:
        void paintEvent(QPaintEvent* _event) override;
    };

    class TabItem : public BaseBarItem
    {
        Q_OBJECT
    public:
        explicit TabItem(const QString& _iconPath, const QString& _iconActivePath, QWidget* _parent = nullptr);
        void setSelected(bool _value) override;
        void setName(const QString& _name) override;

    protected:
        void paintEvent(QPaintEvent* _event) override;

    private:
        void updateTextColor();

    private:
        std::unique_ptr<TextRendering::TextUnit> nameTextUnit_;
    };


    class BaseTabBar : public SimpleListWidget
    {
    public:
        explicit BaseTabBar(Qt::Orientation _orientation, QWidget* _parent = nullptr);

        void setBadgeText(int _index, const QString& _text);
        void setBadgeIcon(int _index, const QString& _icon);
    };

    class AppsBar : public BaseTabBar
    {
        Q_OBJECT
    public:
        explicit AppsBar(QWidget* _parent = nullptr);

    };

    class TabBar : public BaseTabBar
    {
        Q_OBJECT
    public:
        explicit TabBar(QWidget* _parent = nullptr);
        ~TabBar();

    protected:
        void paintEvent(QPaintEvent* _e) override;
    };
}
