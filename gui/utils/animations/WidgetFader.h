#pragma once

namespace Utils
{

class WidgetFader :
    public QGraphicsOpacityEffect
{
    Q_OBJECT
    Q_PROPERTY(int duration READ duration WRITE setDuration)
    Q_PROPERTY(bool enabled READ isEnabled WRITE setEnabled)

public:
    explicit WidgetFader(QWidget* _widget, double _lower = 0.0, double _upper = 1.0);

    ~WidgetFader();

    void setEventDirection(QEvent::Type _type, QPropertyAnimation::Direction _dir);
    QPropertyAnimation::Direction eventDirection(QEvent::Type _type) const;

    inline void setDuration(std::chrono::milliseconds _msecs)
    {
        setDuration(int(_msecs.count()));
    }

    inline std::chrono::milliseconds durationAs() const
    {
        return std::chrono::milliseconds(this->duration());
    }

    void setDuration(int _msecs);
    int duration() const;

    void setEnabled(bool _on);
    bool isEnabled() const;

    static void setEffectEnabled(QWidget* _root, bool _on);

protected:
    bool eventFilter(QObject* _object, QEvent* _event) override;

private:
    std::unique_ptr<class WidgetFaderPrivate> d;
};

} // end namespace Utils