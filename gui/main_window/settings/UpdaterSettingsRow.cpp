#include "stdafx.h"

#include "UpdaterSettingsRow.h"

#include "../MainWindow.h"
#include "../../core_dispatcher.h"

#include "../../controls/TextUnit.h"
#include "../../controls/SimpleListWidget.h"

#include "../../controls/DialogButton.h"
#include "../../utils/log/log.h"
#include "../../utils/utils.h"
#include "../../utils/InterConnector.h"
#include "../../fonts.h"
#include "../../../gui/utils/application.h"
#include "styles/ThemeParameters.h"

namespace Ui
{
    class UpdatesWidget_p
    {
    public:
        TextRendering::TextUnitPtr headerUnit_;
        TextRendering::TextUnitPtr textUnit_;

        static auto size()
        {
            return Utils::scale_value(QSize(240, 143));
        }

        static auto commonMargin()
        {
            return Utils::scale_value(16);
        }

        static  auto textTopMargin()
        {
            return Utils::scale_value(8);
        }
    };

    class UpdatesWidget : public QWidget
    {
    public:
        UpdatesWidget(QWidget* _parent = nullptr)
            : QWidget(_parent)
            , d(std::make_unique<UpdatesWidget_p>())
        {
            setFixedSize(d->size());

            const auto textColor = Styling::getParameters().getColor(Styling::StyleVariable::TEXT_PRIMARY);

            d->headerUnit_ = TextRendering::MakeTextUnit(QT_TRANSLATE_NOOP("burger_menu", "Update is available"));
            d->headerUnit_->init(Fonts::appFontScaled(14, Fonts::FontWeight::SemiBold), textColor);
            d->headerUnit_->evaluateDesiredSize();
            d->headerUnit_->setOffsets(d->commonMargin(), d->commonMargin());

            const auto headerHeight = d->headerUnit_->getHeight(d->size().width() - 2 * d->commonMargin());

            d->textUnit_ = TextRendering::MakeTextUnit(QT_TRANSLATE_NOOP("burger_menu", "Restart application, \nso update could take effect"));

            d->textUnit_->init(Fonts::appFontScaled(12, Fonts::FontWeight::Normal), textColor);
            d->textUnit_->evaluateDesiredSize();
            d->textUnit_->setOffsets(d->commonMargin(), d->commonMargin() + headerHeight + d->textTopMargin());
            d->textUnit_->getHeight(d->size().width() - 2 * d->commonMargin());

            auto button = new DialogButton(this, QT_TRANSLATE_NOOP("burger_menu", "RESTART"), DialogButtonRole::RESTART);
            button->move(d->commonMargin(), Utils::scale_value(95));
            button->adjustSize();

            connect(button, &DialogButton::clicked, []()
            {
                GetDispatcher()->post_stats_to_core(core::stats::stats_event_names::settingsscr_restart_action);
#ifdef __linux__
                Utils::restartApplication();
#elif _WIN32
                auto mainWindow = Utils::InterConnector::instance().getMainWindow();
                if (mainWindow)
                    ShowWindow(mainWindow->getParentWindow(), SW_HIDE);

                qApp->closeAllWindows();
                qApp->quit();
                Utils::Application::updating();
#endif
            });
        }

    protected:
        void paintEvent(QPaintEvent* _event) override
        {
            QPainter p(this);

            d->headerUnit_->draw(p);
            d->textUnit_->draw(p);

            const auto r = Utils::scale_value(3);
            auto pointX = d->commonMargin() + d->headerUnit_->desiredWidth() + 2 * r;
            Utils::drawUpdatePoint(p, QPoint(pointX, d->commonMargin() + 2 * r), r, 0);
        }

    private:
        std::unique_ptr<UpdatesWidget_p> d;
    };


    UpdaterSettingsRow::UpdaterSettingsRow(QWidget* _parent)
        : SimpleListItem(_parent)
        , isCompactMode_(false)
        , isUpdateReady_(false)
    {
        setHoverStateCursor(Qt::ArrowCursor);
        updatesWidget_ = new UpdatesWidget();

        auto layout = Utils::emptyHLayout(this);
        layout->setAlignment(Qt::AlignLeft);
        layout->addWidget(updatesWidget_);
    }

    UpdaterSettingsRow::~UpdaterSettingsRow()
    {
    }

    void UpdaterSettingsRow::setSelected(bool)
    {
    }

    bool UpdaterSettingsRow::isSelected() const
    {
        return false;
    }

    void UpdaterSettingsRow::setCompactMode(bool _value)
    {
        if (isCompactMode_ != _value)
        {
            isCompactMode_ = _value;
            if (!isCompactMode_ && isUpdateReady_)
                show();
            else
                hide();
        }
    }

    bool UpdaterSettingsRow::isCompactMode() const
    {
        return isCompactMode_;
    }

    void UpdaterSettingsRow::setUpdateReady(bool _value)
    {
        if (isUpdateReady_ != _value)
        {
            isUpdateReady_ = _value;
            if (!isCompactMode())
                show();
        }
    }
}
