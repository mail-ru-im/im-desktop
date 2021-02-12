#pragma once
#include "stdafx.h"

namespace Utils
{

class CursorEventFilter : public QObject
{
    Q_OBJECT
    Q_PROPERTY(int delay READ delay WRITE setDelay)
    Q_PROPERTY(bool enabled READ isEnabled WRITE setEnabled)

public:
    explicit CursorEventFilter(QObject* _parent = nullptr);
    ~CursorEventFilter();

    void setWidget(QWidget* _widget);
    QWidget* widget() const;

    void setEnabled(bool _on = true);
    bool isEnabled() const;

    void setDelay(int _msec);
    int delay() const;

    inline void setDelay(std::chrono::milliseconds _msecs)
    {
        setDelay(int(_msecs.count()));
    }

    inline std::chrono::milliseconds delayAs() const
    {
        return std::chrono::milliseconds(this->delay());
    }

public Q_SLOTS:
    void showCursor();
    void hideCursor();

Q_SIGNALS:
    void cursorChanged(bool _visible);

protected:
    bool eventFilter(QObject* _watched, QEvent* _event) override;

private:
    std::unique_ptr<class CursorEventFilterPrivate> d;
};

} // end namespace GaleryUtils
