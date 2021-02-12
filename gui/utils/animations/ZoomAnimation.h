#pragma once

namespace Utils
{

class ZoomAnimation : public QObject
{
    Q_OBJECT
public:
    ZoomAnimation(QWidget* _viewer, double _zoomFactor, std::string_view _propertyName);
    ~ZoomAnimation();

public Q_SLOTS:
    void start();
    void stop();

private Q_SLOTS:
    void animate();

private:
    std::unique_ptr<class ZoomAnimationPrivate> d;
};

} // end namespace Utils
