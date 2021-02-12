#pragma once
#include "stdafx.h"

namespace Utils
{

class WidgetResizer : public QObject
{
    Q_OBJECT
    Q_PROPERTY(int duration READ duration WRITE setDuration)
    Q_PROPERTY(bool enabled READ isEnabled WRITE setEnabled)
public:
    explicit WidgetResizer(QWidget* _widget, Qt::Alignment _align, int _minimum, int _maximum);
    ~WidgetResizer();

    void setEnabled(bool _on = true);
    bool isEnabled() const;

    void setDuration(int _msecs);
    int duration() const;

    inline void setDuration(std::chrono::milliseconds _msecs)
    {
        setDuration(int(_msecs.count()));
    }

    inline std::chrono::milliseconds durationAs() const
    {
        return std::chrono::milliseconds(this->duration());
    }

public Q_SLOTS:
    void slideIn();
    void slideOut();

private:
    std::unique_ptr<class WidgetResizerPrivate> d;
};

} // end namespace Utils
