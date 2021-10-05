#pragma once
#include "stdafx.h"

namespace Utils
{

class SlideController : public QObject
{
    Q_OBJECT
    Q_DISABLE_COPY(SlideController)
    Q_PROPERTY(QEasingCurve easingCurve READ easingCurve WRITE setEasingCurve)
    Q_PROPERTY(SlideEffects effects READ effects WRITE setEffects)
    Q_PROPERTY(SlideDirection slideDirection READ slideDirection WRITE setSlideDirection)
    Q_PROPERTY(QColor fillColor READ fillColor WRITE setFillColor)
    Q_PROPERTY(int duration READ duration WRITE setDuration)
    Q_PROPERTY(Fading fading READ fading WRITE setFading)
    Q_PROPERTY(bool faded READ isFaded)
    Q_PROPERTY(bool inverse READ isInverse WRITE setInverse)
    Q_PROPERTY(bool enabled READ isEnabled WRITE setEnabled)

public:
    enum class CachingPolicy
    {
        CacheNone,
        CacheCurrent,
        CacheAll
    };
    Q_ENUM(CachingPolicy)

    enum class SlideDirection
    {
        SlideUp,
        SlideDown,
        SlideLeft,
        SlideRight,
        NoSliding
    };
    Q_ENUM(SlideDirection)

    enum SlideEffect
    {
        NoEffect = 0x00,
        ZoomEffect = 0x01,
        RotateEffect = 0x02,
        ShearEffect = 0x04,
        SwapEffect = 0x08,
        RollEffect = 0x10
    };
    Q_DECLARE_FLAGS(SlideEffects, SlideEffect)

    enum class Fading
    {
        NoFade,
        FadeIn,
        FadeOut,
        FadeInOut
    };
    Q_ENUM(Fading)

    explicit SlideController(QWidget* _parent = nullptr);
    ~SlideController();

    void setCachingPolicy(CachingPolicy _policy);
    CachingPolicy cachingPolicy() const;

    void setWidget(QStackedWidget* _widget);
    QWidget* widget() const;

    QPixmap activePixmap() const;

    void setEasingCurve(const QEasingCurve& _easing);
    QEasingCurve easingCurve() const;

    SlideEffects effects() const;
    SlideDirection slideDirection() const;
    QColor fillColor() const;

    int duration() const;

    inline void setDuration(std::chrono::milliseconds _msecs)
    {
        setDuration(int(_msecs.count()));
    }

    inline std::chrono::milliseconds durationAs() const
    {
        return std::chrono::milliseconds(this->duration());
    }

    bool isFaded() const;
    bool isInverse() const;
    bool isEnabled() const;

    Fading fading() const;

public Q_SLOTS:
    void setDuration(int _ms);
    void setEffects(SlideEffects _flags);
    void setSlideDirection(SlideDirection _dir);
    void setFillColor(QColor _color);
    void setFading(Fading _fading);
    void setInverse(bool _on = true);
    void setEnabled(bool _on = true);
    void updateCache();

Q_SIGNALS:
    void currentIndexChanged(int, QPrivateSignal);

protected:
    bool eventFilter(QObject* _object, QEvent* _event) override;

    QWidget* currentWidget() const;
    QWidget* widget(int _id) const;
    int currentIndex() const;
    int count() const;

private Q_SLOTS:
    void onCurrentChange(int _index);
    void onAnimationFinished();
    void render(double _value);

private:
    std::unique_ptr<class SlideControllerPrivate> d;
};

Q_DECLARE_OPERATORS_FOR_FLAGS(SlideController::SlideEffects)

} // end namespace SlideController
