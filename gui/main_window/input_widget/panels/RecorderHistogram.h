#pragma once

#include "controls/ClickWidget.h"

namespace Ui
{
    class RecorderHistogram : public ClickableWidget
    {
        Q_OBJECT

        Q_PROPERTY(double animationValue READ animationValue WRITE setAnimationValue)

    Q_SIGNALS:
        void clickOnSample(int sample, int blockSize, QPrivateSignal) const;

    public:
        explicit RecorderHistogram(QWidget* _parent);
        ~RecorderHistogram();

        void start();
        void stop();

        void clear();

        bool isActive() const;

        bool hasSamples() const;

        void addSpectrum(const QVector<double>& _v);
        void setCurrentSample(int _current);

        void setVolume(double _v);

    protected:
        void paintEvent(QPaintEvent*) override;

    private:
        double animationValue() const noexcept;
        void setAnimationValue(double _v);
        void onClicked();
        void animationLoopChanged();

    private:
        bool isActive_ = false;
        QPropertyAnimation* animation_;
        std::vector<double> pending_;
        std::vector<double> ampl_;
        std::vector<QPainterPath> samplePaths_;

        int pos_ = 0;
        double animationValue_ = 0.0;
        int currentSample_ = -1;
        double volume_ = 1.0;
    };
}
