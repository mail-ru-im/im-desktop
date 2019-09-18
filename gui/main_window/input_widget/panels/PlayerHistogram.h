#pragma once

#include "controls/ClickWidget.h"

namespace Ui
{
    class PlayerHistogram : public ClickableWidget
    {
        Q_OBJECT

    public:
        struct ScaledCoeff
        {
            int idx = 0;
            double coeff = 1.0;
        };

    public:
        explicit PlayerHistogram(QWidget* _parent);
        ~PlayerHistogram();

        void start();
        void stop();

        bool isActive() const;

        void clear();

        bool hasSamples() const;

        void addSpectrum(const QVector<double>& _v);
        void setCurrentSample(int _current);
        void setVolume(double _v);

        static int getSampleWidth();

        std::optional<ScaledCoeff> sampleUnderCursor() const;

    protected:
        void paintEvent(QPaintEvent*) override;
        void resizeEvent(QResizeEvent* _e) override;

    private:
        void resample(int _width);

    private:
        bool isActive_ = false;
        std::vector<double> ampl_;
        std::vector<double> resampledAmpl_;

        int pos_ = 0;
        int currentSample_ = -1;
        double volume_ = 1.0;
    };
}