#include "stdafx.h"

#include "UpdaterButton.h"

#include "../utils/utils.h"
#include "../controls/TextUnit.h"
#include "../controls/GeneralDialog.h"
#include "../fonts.h"
#include "styles/ThemeParameters.h"

#include "../common.shared/config/config.h"

#include "MainWindow.h"
#include "../core_dispatcher.h"
#include "../utils/InterConnector.h"
#include "../utils/application.h"
#include "../main_window/history_control/HistoryControlPage.h"
#include "../main_window/MainPage.h"
#include "../previewer/toast.h"

namespace Ui
{
    UpdaterButton::UpdaterButton(QWidget* _parent)
        : ClickableWidget(_parent)
    {
        setFixedHeight(Utils::scale_value(44));

        const auto name = config::get().string(config::values::product_name_full);
        textUnit_ = TextRendering::MakeTextUnit(QT_TRANSLATE_NOOP("UpdaterButton", "Update %1").arg(QString::fromUtf8(name.data(), name.size()).toUpper()), {}, TextRendering::LinksVisible::DONT_SHOW_LINKS);
        textUnit_->init(Fonts::appFontScaled(15), Styling::getParameters().getColor(Styling::StyleVariable::TEXT_SOLID_PERMANENT), QColor(), QColor(), QColor(), TextRendering::HorAligment::CENTER);

        connect(this, &ClickableWidget::clicked, this, &UpdaterButton::updateAction);
        connect(&Utils::InterConnector::instance(), &Utils::InterConnector::updateWhenUserInactive, this, [this]()
        {
            if (isInactiveUpdateAllowed())
                updateAction();
        });
        Testing::setAccessibleName(this, qsl("AS UpdaterButton"));
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
        Q_UNUSED(textHeight);

        textUnit_->setOffsets(0, r.height() / 2);
        textUnit_->draw(p, TextRendering::VerPosition::MIDDLE);
    }

    void UpdaterButton::updateAction()
    {
        GetDispatcher()->post_stats_to_core(core::stats::stats_event_names::settingsscr_restart_action);
#ifdef __APPLE__
        Utils::InterConnector::instance().getMainWindow()->installUpdates();
#else
#ifndef _WIN32
        Utils::restartApplication();
#else
        const auto mainWindow = Utils::InterConnector::instance().getMainWindow();
        const auto mainWindowMode = (mainWindow && mainWindow->isMinimized()) ? Utils::Application::MainWindowMode::Minimized : Utils::Application::MainWindowMode::Normal;

        Utils::Application::updating(mainWindowMode);
#endif
#endif
    }

    bool UpdaterButton::isInactiveUpdateAllowed() const
    {
        // update allowed when:
        // - empty input widget
        // - user doesn't reply
        // - no active PTT

        for (auto page : Utils::InterConnector::instance().getVisibleHistoryPages())
        {
            if (page && page->isInputWidgetActive())
                return false;
        }

        // - no multi selection
        if (Utils::InterConnector::instance().isMultiselect())
            return false;

        // - no active dialog
        if (const auto mainWindow = Utils::InterConnector::instance().getMainWindow())
        {
            const auto generalDialogs = mainWindow->findChildren<GeneralDialog*>();
            const auto fileDialogs = mainWindow->findChildren<QFileDialog*>();
            if (!generalDialogs.isEmpty() || !fileDialogs.isEmpty())
                return false;
        }

        // - no active voip call
        if (const auto mainPage = Utils::InterConnector::instance().getMessengerPage(); mainPage && mainPage->isVideoWindowActive())
            return false;

        return true;
    }
}
