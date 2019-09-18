#include "stdafx.h"
#include "SemitransparentWindowAnimated.h"
#include "../utils/InterConnector.h"
#include "../main_window/MainWindow.h"

namespace
{
    const int min_step = 0;
    const int max_step = 100;
    const char* semiWindowsCountProperty          = "SemiwindowsCount";
    const char* semiWindowsTouchSwallowedProperty = "SemiwindowsTouchSwallowed";
}

namespace Ui
{
    SemitransparentWindowAnimated::SemitransparentWindowAnimated(QWidget* _parent, int _duration, bool _forceIgnoreClicks)
        : QWidget(_parent)
        , Step_(min_step)
        , main_(false)
        , isMainWindow_(false)
        , forceIgnoreClicks_(_forceIgnoreClicks)
    {
        Animation_ = new QPropertyAnimation(this, QByteArrayLiteral("step"), this);
        Animation_->setDuration(_duration);

        updateSize();

        connect(Animation_, &QPropertyAnimation::finished, this, &SemitransparentWindowAnimated::finished);
    }

    SemitransparentWindowAnimated::~SemitransparentWindowAnimated()
    {
    }

    void SemitransparentWindowAnimated::Show()
    {
        main_ = true; // allow many SemitransparentWindowAnimated
        incSemiwindowsCount();

        Animation_->stop();
        Animation_->setCurrentTime(0);
        setStep(min_step);
        show();
        Animation_->setStartValue(min_step);
        Animation_->setEndValue(max_step);
        Animation_->start();
    }

    void SemitransparentWindowAnimated::Hide()
    {
        Animation_->stop();
        Animation_->setCurrentTime(0);
        setStep(max_step);
        Animation_->setStartValue(max_step);
        Animation_->setEndValue(min_step);
        Animation_->start();
    }

    void SemitransparentWindowAnimated::forceHide()
    {
        hide();
        decSemiwindowsCount();
    }

    bool SemitransparentWindowAnimated::isSemiWindowVisible() const
    {
        return getSemiwindowsCount() != 0;
    }

    void SemitransparentWindowAnimated::finished()
    {
        if (Animation_->endValue() == min_step)
        {
            setStep(min_step);
            forceHide();
        }
    }

    void SemitransparentWindowAnimated::paintEvent(QPaintEvent* _e)
    {
        if (main_)
        {
            QPainter p(this);
            QColor windowOpacity(Qt::black);
            windowOpacity.setAlphaF(0.52 * (Step_ / double(max_step)));
            p.fillRect(rect(), windowOpacity);
        }
    }

    void SemitransparentWindowAnimated::mousePressEvent(QMouseEvent *e)
    {
        QWidget::mousePressEvent(e);
        if (clicksAreIgnored())
            return;

        if (!isSemiWindowsTouchSwallowed())
        {
            setSemiwindowsTouchSwallowed(true);
            emit Utils::InterConnector::instance().closeAnySemitransparentWindow(closeInfo_ ? *closeInfo_ : Utils::CloseWindowInfo());
        }
    }

    bool SemitransparentWindowAnimated::isMainWindow() const
    {
        return isMainWindow_;
    }

    void SemitransparentWindowAnimated::updateSize()
    {
        if (parentWidget())
        {
            const auto rect = parentWidget()->rect();
            auto width = rect.width();
            auto height = rect.height();

            isMainWindow_ = (qobject_cast<MainWindow*>(parentWidget()) != nullptr);

            const auto titleHeight = isMainWindow_ ? Utils::InterConnector::instance().getMainWindow()->getTitleHeight() : 0;
            setFixedSize(width, height - titleHeight);
            move(0, titleHeight);
        }
    }

    bool SemitransparentWindowAnimated::clicksAreIgnored() const
    {
        return forceIgnoreClicks_;
    }

    void SemitransparentWindowAnimated::setForceIgnoreClicks(bool _forceIgnoreClicks)
    {
        forceIgnoreClicks_ = _forceIgnoreClicks;
    }

    void SemitransparentWindowAnimated::setCloseWindowInfo(const Utils::CloseWindowInfo& _info)
    {
        closeInfo_ = std::make_unique<Utils::CloseWindowInfo>(_info);
    }

    void SemitransparentWindowAnimated::decSemiwindowsCount()
    {
        if (window())
        {
            auto variantCount = window()->property(semiWindowsCountProperty);
            if (variantCount.isValid())
            {
                int count = variantCount.toInt() - 1;
                if (count < 0)
                {
                    count = 0;
                }

                if (count <= 1)
                    setSemiwindowsTouchSwallowed(false);

                window()->setProperty(semiWindowsCountProperty, count);
            }
        }
    }

    void SemitransparentWindowAnimated::incSemiwindowsCount()
    {
        if (window())
        {
            int count = 0;
            auto variantCount = window()->property(semiWindowsCountProperty);
            if (variantCount.isValid())
            {
                count = variantCount.toInt();
            }
            count++;

            window()->setProperty(semiWindowsCountProperty, count);
        }
    }

    int  SemitransparentWindowAnimated::getSemiwindowsCount() const
    {
        int res = 0;
        if (window())
        {
            auto variantCount = window()->property(semiWindowsCountProperty);
            if (variantCount.isValid())
            {
                res = variantCount.toInt();
            }
        }
        return res;
    }

    bool SemitransparentWindowAnimated::isSemiWindowsTouchSwallowed() const
    {
        bool res = false;
        if (window())
        {
            auto variantCount = window()->property(semiWindowsTouchSwallowedProperty);
            if (variantCount.isValid())
            {
                res = variantCount.toBool();
            }
        }
        return res;
    }

    void SemitransparentWindowAnimated::setSemiwindowsTouchSwallowed(bool _val)
    {
        if (window())
        {
            window()->setProperty(semiWindowsTouchSwallowedProperty, _val);
        }
    }

    void SemitransparentWindowAnimated::hideEvent(QHideEvent * event)
    {
        decSemiwindowsCount();
    }
}
