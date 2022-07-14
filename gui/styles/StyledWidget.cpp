#include "stdafx.h"
#include "ThemesContainer.h"
#include "ThemeParameters.h"
#include "StyledWidget.h"

#include "../utils/utils.h"

namespace
{
    auto defaultColorKey() { return Styling::ThemeColorKey { Styling::StyleVariable::BASE_GLOBALWHITE }; }
} // namespace

namespace Styling
{
    StyledWidget::StyledWidget(QWidget* _parent)
        : QWidget(_parent)
    {
        connect(&getThemesContainer(), &ThemesContainer::globalThemeChanged, this, &StyledWidget::markNeedUpdateBackground);
    }

    void StyledWidget::setDefaultBackground() { setBackgroundKey(defaultColorKey()); }

    void StyledWidget::setBackgroundKey(const ThemeColorKey& _key)
    {
        im_assert(_key.isValid());
        setAutoFillBackground(true);
        backgroundKey_ = _key;
        updateBackground();
    }

    bool StyledWidget::event(QEvent* _event)
    {
        if ((_event->type() == QEvent::UpdateLater) && needUpdateBackground_)
        {
            updateBackground();
            needUpdateBackground_ = false;
        }
        return QWidget::event(_event);
    }

    void StyledWidget::updateBackground()
    {
        if (backgroundKey_.isValid())
            Utils::updateBgColor(this, getColor(backgroundKey_));
    }

    void StyledWidget::markNeedUpdateBackground()
    {
        if (isVisible())
            updateBackground();
        else
            needUpdateBackground_ = true;
    }
} // namespace Styling
