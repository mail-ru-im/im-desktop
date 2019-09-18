#pragma once

#include "../InputWidgetUtils.h"

#include "media/ptt/AudioPlayer.h"

namespace ptt
{
    class AudioRecorder2;
    struct Buffer;
    struct StatInfo;
    enum class State2;
    enum class Error2;
}

namespace Ui
{
    class HistogramPtt;
    class ButtonWithCircleHover;
    class SubmitButton;

    class InputPanelPttImpl : public QWidget, public StyledInputElement
    {
        Q_OBJECT

    Q_SIGNALS:
        void pttReady(const QString& _file, std::chrono::seconds, const ptt::StatInfo&, QPrivateSignal) const;
        void stateChanged(ptt::State2, QPrivateSignal) const;
        void pttRemoved(QPrivateSignal) const;
        void histogramUnderMouseChanged(bool, QPrivateSignal) const;

    public:
        InputPanelPttImpl(QWidget* _parent, const QString& _contact, QThread* pttThread, SubmitButton* _submit);
        ~InputPanelPttImpl();

        void record();
        void stop();
        void pauseRecord();

        void play();
        void pausePlay();

        void getData(ptt::StatInfo&&);

        bool removeIfUnderMouse();
        void removeRecord();
        void pressedMouseMove();

        void enableCircleHover(bool _val);

        bool canLock() const;

        void setUnderQuote(const bool _underQuote);
        void setFocusOnDelete();

        void setButtonsTabOrder();

    protected:
        void updateStyleImpl(const InputStyleMode _mode) override {}
        void showEvent(QShowEvent* _event) override;
        void hideEvent(QHideEvent* _event) override;

    private:
        void onError(ptt::Error2);
        void onStateChanged(ptt::State2 _state);
        void onPcmDataReady(const ptt::Buffer& _buffer);
        void onSampleClicked(int _sample, double _resampleCoeff);

        void onDeleteClicked();

        void onPlayerStateChanged(ptt::AudioPlayer::State);

    private:
        QPointer<ptt::AudioRecorder2> recorder_;
        QPointer<ptt::AudioPlayer> player_;
        HistogramPtt* histogram_ = nullptr;
        ButtonWithCircleHover* deleteButton_ = nullptr;
        bool playButtonWasClicked_ = false;
        bool recordScheduled_ = false;

        SubmitButton* buttonSubmit_ = nullptr;
    };
}