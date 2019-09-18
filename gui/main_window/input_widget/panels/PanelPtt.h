#pragma once

#include "utils/utils.h"
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

    enum class AutoStart
    {
        No,
        Yes
    };

    class InputPanelPtt : public QStackedWidget, public StyledInputElement
    {
        Q_OBJECT

    Q_SIGNALS:
        void pttReady(const QString& _contact, const QString& _file, std::chrono::seconds, const ptt::StatInfo&, QPrivateSignal) const;
        void stateChanged(const QString& _contact, ptt::State2, QPrivateSignal) const;
        void pttRemoved(const QString& _contact, QPrivateSignal) const;

    public:
        InputPanelPtt(QWidget* _parent, SubmitButton* _submit);
        ~InputPanelPtt();

        void open(const QString& _contact, AutoStart _mode = AutoStart::No);
        void close(const QString& _contact);
        void record(const QString& _contact);
        void stop(const QString& _contact);
        void stopAll();
        void pause(const QString& _contact);
        void send(const QString& _contact, ptt::StatInfo&&);
        bool removeIfUnderMouse(const QString& _contact);
        void remove(const QString& _contact);
        bool canLock(const QString& _contact) const;

        void pressedMouseMove(const QString& _contact);

        void enableCircleHover(const QString&, bool _val);

        void setUnderQuote(const bool _underQuote);
        void setFocusOnDelete();

    protected:
        void updateStyleImpl(const InputStyleMode _mode) override;

    private:
        void stopAllRecorders();
        void deleteAllRecorders();

    private:
        std::unordered_map<QString, QPointer<InputPanelPttImpl>, Utils::QStringHasher> panels_;
        QThread* pttThread_;
        SubmitButton* buttonSubmit_;
    };
}
