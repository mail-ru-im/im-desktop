#pragma once

#include "controls/SimpleListWidget.h"

namespace Ui
{
    namespace TextRendering
    {
        class TextUnit;
    }

    class TabItem : public SimpleListItem
    {
        Q_OBJECT;

    public:
        explicit TabItem(const QString& _iconPath, const QString& _iconActivePath, QWidget* _parent = nullptr);
        ~TabItem();

        void setSelected(bool _value) override;
        bool isSelected() const override;
        void setBadgeText(const QString& _text);
        void setBadgeIcon(const QString& _icon);
        void setName(const QString& _name);

    protected:
        void paintEvent(QPaintEvent* _event) override;

    private:
        void updateTextColor();
        void setBadgeFont(const QFont& _font);

    private:
        bool isSelected_;
        QString name_;
        QString badgeText_;
        QString badgeIcon_;
        QPixmap badgePixmap_;

        const QString iconPath_;
        const QString iconActivePath_;

        const QPixmap activeIconPixmap_;
        const QPixmap hoveredIconPixmap_;
        const QPixmap normalIconPixmap_;

        std::unique_ptr<TextRendering::TextUnit> nameTextUnit_;
        std::unique_ptr<TextRendering::TextUnit> badgeTextUnit_;
    };

    class TabBar : public SimpleListWidget
    {
        Q_OBJECT;

    public:
        explicit TabBar(QWidget* _parent = nullptr);
        ~TabBar();

        void setBadgeText(int _index, const QString& _text);
        void setBadgeIcon(int _index, const QString& _icon);

        static int getDefaultHeight();

    protected:
        void paintEvent(QPaintEvent* _e) override;
    };
}