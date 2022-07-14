#pragma once

class SvgIcon : public QQuickPaintedItem
{
    Q_OBJECT
    Q_PROPERTY(QString iconPath READ iconPath WRITE setIconPath NOTIFY iconPathChanged)
    Q_PROPERTY(QColor color READ color WRITE setColor NOTIFY colorChanged)

public:
    SvgIcon(QQuickItem* _parent = nullptr);
    void paint(QPainter* _painter) override;

public Q_SLOTS:
    QString iconPath() const;
    void setIconPath(const QString& _path);
    QColor color() const;
    void setColor(const QColor& _color);

Q_SIGNALS:
    void iconPathChanged();
    void colorChanged();

private Q_SLOTS:
    void updatePixmap();

private:
    QString iconPath_;
    QColor color_;
    QPixmap pixmap_;
};
