#include "stdafx.h"

#include "PanelPtt.h"
#include "PanelPttImpl.h"

#include "media/ptt/AudioUtils.h"

#include <boost/range/adaptor/map.hpp>
#include <boost/range/adaptor/reversed.hpp>
#include <boost/range.hpp>
#include <boost/range/algorithm.hpp>

namespace Ui
{
    InputPanelPtt::InputPanelPtt(QWidget* _parent, SubmitButton* _submit)
        : QStackedWidget(_parent)
        , pttThread_(new QThread)
        , buttonSubmit_(_submit)
    {
        setFixedHeight(getDefaultInputHeight());
        pttThread_->start();
    }

    InputPanelPtt::~InputPanelPtt()
    {
        stopAllRecorders();
        deleteAllRecorders();
        pttThread_->quit();
        pttThread_->deleteLater();
    }

    void InputPanelPtt::open(const QString& _contact, AutoStart _mode)
    {
        auto& panel = panels_[_contact];
        if (!panel)
        {
            panel = new InputPanelPttImpl(this, _contact, pttThread_, buttonSubmit_);
            QObject::connect(panel, &InputPanelPttImpl::pttReady, this, [this, _contact](const QString& _file, std::chrono::seconds _duration, const ptt::StatInfo& _stat)
            {
                emit pttReady(_contact, _file, _duration, _stat, QPrivateSignal());
            });
            QObject::connect(panel, &InputPanelPttImpl::stateChanged, this, [this, _contact](ptt::State2 _state)
            {
                emit stateChanged(_contact, _state, QPrivateSignal());
            });
            QObject::connect(panel, &InputPanelPttImpl::pttRemoved, this, [this, _contact]()
            {
                emit pttRemoved(_contact, QPrivateSignal());
            });
            addWidget(panel);
        }
        setCurrentWidget(panel);

        if (_mode == AutoStart::Yes)
            panel->record();
    }

    void InputPanelPtt::close(const QString& _contact)
    {
        if (const auto it = panels_.find(_contact); it != panels_.end())
        {
            auto& panel = it->second;
            panel->stop();
            removeWidget(panel);
            panel->deleteLater();
            panels_.erase(it);
        }
    }

    void InputPanelPtt::record(const QString& _contact)
    {
        if (const auto it = panels_.find(_contact); it != panels_.end())
            it->second->record();
    }

    void InputPanelPtt::stop(const QString& _contact)
    {
        if (const auto it = panels_.find(_contact); it != panels_.end())
            it->second->stop();
    }

    void InputPanelPtt::stopAll()
    {
        for (auto& panel : boost::adaptors::values(panels_))
            panel->stop();
    }

    void InputPanelPtt::pause(const QString& _contact)
    {
        if (const auto it = panels_.find(_contact); it != panels_.end())
            it->second->pauseRecord();
    }

    void InputPanelPtt::send(const QString& _contact, ptt::StatInfo&& _statInfo)
    {
        if (const auto it = panels_.find(_contact); it != panels_.end())
            it->second->getData(std::move(_statInfo));
    }

    bool InputPanelPtt::removeIfUnderMouse(const QString& _contact)
    {
        if (const auto it = panels_.find(_contact); it != panels_.end())
            return it->second->removeIfUnderMouse();
        return false;
    }

    void InputPanelPtt::remove(const QString& _contact)
    {
        if (const auto it = panels_.find(_contact); it != panels_.end())
            it->second->removeRecord();
    }

    bool InputPanelPtt::canLock(const QString& _contact) const
    {
        if (const auto it = panels_.find(_contact); it != panels_.end())
            return it->second->canLock();
        return false;
    }

    void InputPanelPtt::pressedMouseMove(const QString& _contact)
    {
        if (const auto it = panels_.find(_contact); it != panels_.end())
            it->second->pressedMouseMove();
    }

    void InputPanelPtt::enableCircleHover(const QString& _contact, bool _val)
    {
        if (const auto it = panels_.find(_contact); it != panels_.end())
            it->second->enableCircleHover(_val);
    }

    void InputPanelPtt::updateStyleImpl(const InputStyleMode _mode)
    {
        for (auto& panel : boost::adaptors::values(panels_))
            panel->updateStyle(_mode);
    }

    void InputPanelPtt::setUnderQuote(const bool _underQuote)
    {
        const auto h = getDefaultInputHeight() - (_underQuote ? getVerMargin() : 0);
        setFixedHeight(h);

        for (auto& panel : boost::adaptors::values(panels_))
            panel->setUnderQuote(_underQuote);
    }

    void InputPanelPtt::setFocusOnDelete()
    {
        if (auto panel = qobject_cast<InputPanelPttImpl*>(currentWidget()))
            panel->setFocusOnDelete();
    }

    void InputPanelPtt::stopAllRecorders()
    {
        for (auto& panel : boost::adaptors::values(panels_))
            panel->stop();
    }

    void InputPanelPtt::deleteAllRecorders()
    {
        for (auto& panel : boost::adaptors::values(panels_))
            panel->deleteLater();
    }
}
