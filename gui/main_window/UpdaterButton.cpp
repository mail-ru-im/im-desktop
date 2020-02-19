#include "stdafx.h"

#include "UpdaterButton.h"

#include "../utils/utils.h"
#include "../controls/TextUnit.h"
#include "../fonts.h"
#include "styles/ThemeParameters.h"

#include "../common.shared/config/config.h"

#include "MainWindow.h"
#include "../core_dispatcher.h"
#include "../utils/InterConnector.h"
#include "../utils/application.h"

namespace Ui
{
    UpdaterButton::UpdaterButton(QWidget* _parent)
        : ClickableWidget(_parent)
    {
        setFixedHeight(Utils::scale_value(44));

        const auto name = config::get().string(config::values::product_name_full);
        textUnit_ = TextRendering::MakeTextUnit(QT_TRANSLATE_NOOP("UpdaterButton", "Update %1").arg(QString::fromUtf8(name.data(), name.size()).toUpper()), {}, TextRendering::LinksVisible::DONT_SHOW_LINKS);
        textUnit_->init(Fonts::appFontScaled(15), Styling::getParameters().getColor(Styling::StyleVariable::TEXT_SOLID_PERMANENT), QColor(), QColor(), QColor(), TextRendering::HorAligment::CENTER);

        QObject::connect(this, &ClickableWidget::clicked, this, []()
        {
            GetDispatcher()->post_stats_to_core(core::stats::stats_event_names::settingsscr_restart_action);
#ifdef __linux__
            Utils::restartApplication();
#elif _WIN32
            qApp->closeAllWindows();
            qApp->quit();
            Utils::Application::updating();
#endif
        });
    }

    UpdaterButton::~UpdaterButton() = default;

    void UpdaterButton::paintEvent(QPaintEvent* _event)
    {
        QPainter p(this);
        p.setRenderHint(QPainter::Antialiasing);
        p.setRenderHint(QPainter::SmoothPixmapTransform);

        const auto color = Styling::getParameters().getColor(Styling::StyleVariable::PRIMARY_SELECTED);
        const auto r = rect();
        p.fillRect(r, color);

        const auto textHeight = textUnit_->getHeight(r.width());

        textUnit_->setOffsets(0, r.height() / 2);
        textUnit_->draw(p, TextRendering::VerPosition::MIDDLE);
    }
}