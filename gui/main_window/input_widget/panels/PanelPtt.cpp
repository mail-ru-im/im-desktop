#include "stdafx.h"

#include "PanelPtt.h"
#include "PanelPttImpl.h"

#include "utils/utils.h"
#include "media/ptt/AudioUtils.h"

namespace
{
    static std::weak_ptr<QThread> recorderThread_;

    void threadDeleter(QThread* _thread)
    {
        _thread->quit();
        _thread->wait();
        delete _thread;
    }
}

namespace Ui
{
    InputPanelPtt::InputPanelPtt(const QString& _contact, QWidget* _parent, SubmitButton* _submit)
        : QWidget(_parent)
        , buttonSubmit_(_submit)
        , pttThread_(recorderThread_.lock())
    {
        if (!pttThread_)
        {
            pttThread_ = std::shared_ptr<QThread>(new QThread(), threadDeleter);
            pttThread_->setObjectName(qsl("RecorderThread"));
            pttThread_->start();

            recorderThread_ = pttThread_;
        }

        setFixedHeight(getDefaultInputHeight());

        panel_ = new InputPanelPttImpl(this, _contact, pttThread_.get(), buttonSubmit_);
        connect(panel_, &InputPanelPttImpl::pttReady, this, [this](const QString& _file, std::chrono::seconds _duration, const ptt::StatInfo& _statInfo)
        {
            Q_EMIT pttReady(_file, _duration, _statInfo, QPrivateSignal());
        });
        connect(panel_, &InputPanelPttImpl::stateChanged, this, [this](ptt::State2 _state)
        {
            Q_EMIT stateChanged(_state, QPrivateSignal());
        });
        connect(panel_, &InputPanelPttImpl::pttRemoved, this, [this]()
        {
            Q_EMIT pttRemoved(QPrivateSignal());
        });

        auto layout = Utils::emptyHLayout(this);
        layout->addWidget(panel_);
    }

    InputPanelPtt::~InputPanelPtt()
    {
        stop();
    }

    void InputPanelPtt::record()
    {
        panel_->record();
    }

    void InputPanelPtt::stop()
    {
        panel_->stop();
    }

    void InputPanelPtt::pause()
    {
        panel_->pauseRecord();
    }

    void InputPanelPtt::send(ptt::StatInfo&& _statInfo)
    {
        panel_->getData(std::move(_statInfo));
    }

    bool InputPanelPtt::removeIfUnderMouse()
    {
        return panel_->removeIfUnderMouse();
    }

    void InputPanelPtt::remove()
    {
        panel_->removeRecord();
    }

    bool InputPanelPtt::canLock() const
    {
        return panel_->canLock();
    }

    bool InputPanelPtt::tryPlay()
    {
        return panel_->tryPlay();
    }

    bool InputPanelPtt::tryPause()
    {
        return panel_->tryPausePlay();
    }

    void InputPanelPtt::pressedMouseMove()
    {
        panel_->pressedMouseMove();
    }

    void InputPanelPtt::enableCircleHover(bool _val)
    {
        panel_->enableCircleHover(_val);
    }

    void InputPanelPtt::setUnderLongPress(bool _val)
    {
        panel_->setUnderLongPress(_val);
    }

    void InputPanelPtt::updateStyleImpl(const InputStyleMode _mode)
    {
        panel_->updateStyle(_mode);
    }

    void InputPanelPtt::setUnderQuote(const bool _underQuote)
    {
        const auto h = getDefaultInputHeight() - (_underQuote ? getVerMargin() : 0);
        setFixedHeight(h);

        panel_->setUnderQuote(_underQuote);
    }

    void InputPanelPtt::setFocusOnDelete()
    {
        panel_->setFocusOnDelete();
    }
}
