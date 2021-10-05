#pragma once

#include "../InputWidgetUtils.h"

namespace ptt
{
    enum class State2;
    struct StatInfo;
}

namespace Ui
{
    class InputPanelPttImpl;
    class SubmitButton;

    class InputPanelPtt : public QWidget, public StyledInputElement
    {
        Q_OBJECT

    Q_SIGNALS:
        void pttReady(const QString& _file, std::chrono::seconds, const ptt::StatInfo&, QPrivateSignal) const;
        void stateChanged(ptt::State2, QPrivateSignal) const;
        void pttRemoved(QPrivateSignal) const;

    public:
        InputPanelPtt(const QString& _contact, QWidget* _parent, SubmitButton* _submit);
        ~InputPanelPtt();

        void record();
        void stop();
        void pause();
        void send(ptt::StatInfo&&);
        bool removeIfUnderMouse();
        void remove();
        bool canLock() const;
        bool tryPlay();
        bool tryPause();

        void pressedMouseMove();

        void enableCircleHover(bool _val);
        void setUnderLongPress(bool _val);

        void setUnderQuote(const bool _underQuote);
        void setFocusOnDelete();

    protected:
        void updateStyleImpl(const InputStyleMode _mode) override;

    private:
        InputPanelPttImpl* panel_;
        SubmitButton* buttonSubmit_;
        std::shared_ptr<QThread> pttThread_;
    };
}
