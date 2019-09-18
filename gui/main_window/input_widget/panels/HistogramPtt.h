#pragma once

#include "controls/ClickWidget.h"

namespace Ui
{
    class PlayerHistogram;
    class RecorderHistogram;
    class PlayButton;

    namespace TextRendering
    {
        class TextUnit;
    }

    class HistogramContainer : public QWidget
    {
        Q_OBJECT

    public:
        explicit HistogramContainer(QWidget* parent);

        PlayerHistogram* playerHistogram() noexcept { return playerHistogram_; }
        RecorderHistogram* recorderHistogram() noexcept { return recorderHistogram_; }

    private:
        PlayerHistogram* playerHistogram_ = nullptr;
        RecorderHistogram* recorderHistogram_ = nullptr;
    };


    class HistogramPtt : public ClickableWidget
    {
        Q_OBJECT

    Q_SIGNALS:
        void underMouseChanged(bool, QPrivateSignal) const;
        void stopButtonClicked(QPrivateSignal) const;
        void playButtonClicked(QPrivateSignal) const;
        void pauseButtonClicked(QPrivateSignal) const;

        void clickOnSample(int sample, double resampleCoeff, QPrivateSignal) const;

    public:
        explicit HistogramPtt(QWidget* _parent, std::function<std::chrono::milliseconds(int, double)> _durationCalc);
        ~HistogramPtt();

        void start();
        void stop();

        void switchToInit();

        void setDuration(std::chrono::milliseconds _duration);
        void enableCircleHover(bool _val);

        void setPlayingButton();
        void setPausedButton();
        void setStoppedButton();

        void setCurrentSample(int _current);

        void updateHover(bool _hovered = false);

        void onSpectrum(const QVector<double>& _v);

        QWidget* getButtonWidget() const;

    protected:
        void paintEvent(QPaintEvent* _e) override;
        void leaveEvent(QEvent* _e) override;
        void enterEvent(QEvent* _e) override;
        void mouseMoveEvent(QMouseEvent* _e) override;

    private:
        // use homebrew tooltip engine
        void onTooltipTimer() override {}
        bool canShowTooltip() override { return false; };
        void showToolTip() override;
        void hideToolTip();
        //
        void updatePressed();
        void onClicked();
        void onPlayButtonClicked();

    private:
        const std::function<std::chrono::milliseconds(int, double)> durationCalc_;
        std::chrono::milliseconds duration_ = std::chrono::milliseconds::zero();
        std::unique_ptr<TextRendering::TextUnit> durationText_;
        PlayButton* playButton_ = nullptr;
        HistogramContainer* histograms_;

        bool underMouse_ = false;
        bool pressed_ = false;
        bool enableTooltip_ = false;

        QTimer tooltipTimer_;
    };
}