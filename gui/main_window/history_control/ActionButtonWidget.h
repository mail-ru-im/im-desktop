#pragma once

#include "../../namespaces.h"

#include "../../animation/animation.h"

namespace Themes
{
    using ThemePixmapSptr = std::shared_ptr<class ThemePixmap>;

    enum class PixmapResourceId;
}

UI_NS_BEGIN

class ActionButtonWidgetLayout;

namespace ActionButtonResource
{
    struct ResourceSet
    {
        static const ResourceSet Play_;
        static const ResourceSet DownloadMediaFile_;
        static const ResourceSet DownloadVideo_;

        typedef Themes::PixmapResourceId ResId;

        ResourceSet(
            const ResId startButtonImage,
            const ResId startButtonImageActive,
            const ResId startButtonImageHover,
            const ResId stopButtonImage,
            const ResId stopButtonImageActive,
            const ResId stopButtonImageHover);

        ResId StartButtonImage_;
        ResId StartButtonImageActive_;
        ResId StartButtonImageHover_;
        ResId StopButtonImage_;
        ResId StopButtonImageActive_;
        ResId StopButtonImageHover_;
    };
}

class ActionButtonWidget final : public QWidget
{
    Q_OBJECT

Q_SIGNALS:
    void startClickedSignal(QPoint globalClickCoords);

    void stopClickedSignal(QPoint globalClickCoords);

    void dragSignal();

    void internallyChangedSignal();

public:

    static QSize getMaxIconSize();

    ActionButtonWidget(const ActionButtonResource::ResourceSet &resourceIds, QWidget *parent = nullptr);

    ~ActionButtonWidget();

    QPoint getCenterBias() const;

    QSize getIconSize() const;

    QPoint getLogicalCenter() const;

    const QString& getProgressText() const;

    const QFont& getProgressTextFont() const;

    const ActionButtonResource::ResourceSet& getResourceSet() const;

    void setProgress(const double progress);

    void setProgressPen(const QColor color, const double a, const double width);

    void setProgressText(const QString &progressText);

    void setDefaultProgressText(const QString &progressText);

    void setResourceSet(const ActionButtonResource::ResourceSet &resourceIds);

    virtual QSize sizeHint() const override;

    void startAnimation(const int32_t _delay = 0);

    void stopAnimation();

    void onVisibilityChanged(const bool isVisible);

    bool isWaitingForDelay();

protected:
    void enterEvent(QEvent *event) override;

    void hideEvent(QHideEvent*) override;

    void showEvent(QShowEvent *) override;

    void leaveEvent(QEvent *event) override;

    void mouseMoveEvent(QMouseEvent *event) override;

    void mousePressEvent(QMouseEvent *event) override;

    void mouseReleaseEvent(QMouseEvent *event) override;

    void paintEvent(QPaintEvent *event) override;

private:
    Q_PROPERTY(int ProgressBarBaseAngle READ getProgressBarBaseAngle WRITE setProgressBarBaseAngle)

    void createAnimationStartTimer();

    void drawIcon(QPainter &p);

    void drawProgress(QPainter &p);

    void drawProgressText(QPainter &p);

    int getProgressBarBaseAngle() const;

    void setProgressBarBaseAngle(const int32_t _val);

    void initResources();

    void resetLayoutGeometry();

    bool selectCurrentIcon();

    void startProgressBarAnimation();

    void stopProgressBarAnimation();

    void pauseAnimation();
    void resumeAnimation();

    QTimer *AnimationStartTimer_;

    Themes::ThemePixmapSptr CurrentIcon_;

    QPropertyAnimation *ProgressBaseAngleAnimation_;

    QPen ProgressPen_;

    Themes::ThemePixmapSptr StartButtonImage_;

    Themes::ThemePixmapSptr StartButtonImageHover_;

    Themes::ThemePixmapSptr StartButtonImageActive_;

    Themes::ThemePixmapSptr StopButtonImage_;

    Themes::ThemePixmapSptr StopButtonImageHover_;

    Themes::ThemePixmapSptr StopButtonImageActive_;

    ActionButtonResource::ResourceSet ResourceIds_;

    double Progress_;

    int32_t ProgressBarAngle_;

    QString ProgressText_;

    QString DefaultProgressText_;

    QFont ProgressTextFont_;

    bool ResourcesInitialized_;

    bool IsHovered_;

    bool IsPressed_;

    bool IsAnimating_;

    ActionButtonWidgetLayout *Layout_;

    anim::Animation animation_;

private Q_SLOTS:
    void onAnimationStartTimeout();

};

UI_NS_END
