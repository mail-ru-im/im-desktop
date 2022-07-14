#include "stdafx.h"
#include "StyleSheetContainer.h"
#include "StyleSheetGenerator.h"
#include "ThemesContainer.h"

namespace
{
    bool styleSheetNeedPrepare(const QString& _styleSheet) { return Utils::prepareStyle(_styleSheet) != _styleSheet; }
} // namespace

namespace Styling
{
    StyleSheetContainer::StyleSheetContainer(QWidget* _styleTarget)
        : QObject(_styleTarget)
        , styleTarget_ { _styleTarget }
    {
        connect(&getThemesContainer(), &ThemesContainer::globalThemeChanged, this, &StyleSheetContainer::markNeedUpdateStyle);
        styleTarget_->installEventFilter(this);
    }

    StyleSheetContainer::~StyleSheetContainer() = default;

    void StyleSheetContainer::setGenerator(std::unique_ptr<BaseStyleSheetGenerator> _styleSheetGenerator)
    {
        styleSheetGenerator_ = std::move(_styleSheetGenerator);
        im_assert(!styleSheetGenerator_ || (!styleSheetNeedPrepare(styleSheetGenerator_->generate()) ^ (StyleSheetPolicy::ApplyFontsAndScale == policy_)));
        updateStyle();
    }

    void StyleSheetContainer::usePolicy(StyleSheetPolicy _policy) { policy_ = _policy; }

    bool StyleSheetContainer::eventFilter(QObject* _object, QEvent* _event)
    {
        const auto eventType = _event->type();
        if ((_object == styleTarget_) && needUpdateStyle_ && (eventType == QEvent::UpdateLater || eventType == QEvent::Show))
        {
            updateStyle();
            needUpdateStyle_ = false;
        }
        return false;
    }

    void StyleSheetContainer::updateStyle()
    {
        if (styleSheetGenerator_)
        {
            if (StyleSheetPolicy::ApplyFontsAndScale == policy_)
                Utils::ApplyStyle(styleTarget_, styleSheetGenerator_->generate());
            else
                styleTarget_->setStyleSheet(styleSheetGenerator_->generate());
        }
    }

    void StyleSheetContainer::markNeedUpdateStyle() { needUpdateStyle_ = true; }

    void setStyleSheet(QWidget* _styleTarget, std::unique_ptr<BaseStyleSheetGenerator> _styleSheetGenerator, StyleSheetPolicy _policy)
    {
        if (const bool alreadyContains = _styleTarget->findChild<StyleSheetContainer*>(QString {}, Qt::FindChildOption::FindDirectChildrenOnly) != nullptr)
        {
            im_assert(!"Already contains stylesheet container");
            return;
        }

        auto container = new StyleSheetContainer(_styleTarget);
        container->usePolicy(_policy);
        container->setGenerator(std::move(_styleSheetGenerator));
    }
} // namespace Styling
